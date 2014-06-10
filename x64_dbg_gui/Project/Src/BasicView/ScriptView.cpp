#include "ScriptView.h"

ScriptView::ScriptView(StdTable *parent) : StdTable(parent)
{
    mEnableSyntaxHighlighting=false;
    enableMultiSelection(false);

    int charwidth=QFontMetrics(this->font()).width(QChar(' '));

    addColumnAt(8+charwidth*4, "Line", false);
    addColumnAt(8+charwidth*60, "Text", false);
    addColumnAt(8+charwidth*40, "Info", false);

    setIp(0); //no IP

    connect(Bridge::getBridge(), SIGNAL(scriptAdd(int,const char**)), this, SLOT(add(int,const char**)));
    connect(Bridge::getBridge(), SIGNAL(scriptClear()), this, SLOT(clear()));
    connect(Bridge::getBridge(), SIGNAL(scriptSetIp(int)), this, SLOT(setIp(int)));
    connect(Bridge::getBridge(), SIGNAL(scriptError(int,QString)), this, SLOT(error(int,QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptSetTitle(QString)), this, SLOT(setTitle(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptSetInfoLine(int,QString)), this, SLOT(setInfoLine(int,QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptMessage(QString)), this, SLOT(message(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptQuestion(QString)), this, SLOT(question(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptEnableHighlighting(bool)), this, SLOT(enableHighlighting(bool)));

    setupContextMenu();
}

QString ScriptView::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    bool wIsSelected=isSelected(rowBase, rowOffset);
    // Highlight if selected
    if(wIsSelected)
        painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#C0C0C0"))); //ScriptViewSelectionColor
    QString returnString;
    int line=rowBase+rowOffset+1;
    SCRIPTLINETYPE linetype=DbgScriptGetLineType(line);
    switch(col)
    {
    case 0: //line number
    {
        returnString=returnString.sprintf("%.4d", line);
        painter->save();
        if(line==mIpLine) //IP
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#000000"))); //ScriptViewIpColor
            if(DbgScriptBpGet(line)) //breakpoint
                painter->setPen(QPen(QColor("#FF0000"))); //red address (ScriptViewMainBpColor)
            else
                painter->setPen(QPen(QColor("#FFFFFF"))); //white address (ScriptViewIpTextColor)
        }
        else if(DbgScriptBpGet(line)) //breakpoint
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#FF0000"))); //ScriptViewMainBpColor
            painter->setPen(QPen(QColor("#000000"))); //black address //ScripViewMainBpTextColor
        }
        else
        {
            if(linetype==linecommand || linetype==linebranch)
                painter->setPen(QPen(QColor("#000000"))); //black address (ScriptViewMainTextColor)
            else
                painter->setPen(QPen(QColor("#808080"))); //grey address (ScriptViewOtherTextColor)
        }
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, returnString);
        painter->restore();
        returnString="";
    }
    break;

    case 1: //command
    {
        if(mEnableSyntaxHighlighting)
        {
            //initialize
            painter->save();
            int charwidth=QFontMetrics(this->font()).width(QChar(' '));
            int xadd=charwidth; //for testing
            QList<CustomRichText_t> richText;
            CustomRichText_t newRichText;
            QString command=getCellContent(rowBase+rowOffset, col);

            //handle comments
            int comment_idx=command.indexOf("//"); //find the index of the space
            QString comment="";
            if(comment_idx!=-1 && command.at(0)!=QChar('/')) //there is a comment
            {
                comment=command.right(command.length()-comment_idx);
                if(command.at(comment_idx-1)==QChar(' '))
                    command.truncate(comment_idx-1);
                else
                    command.truncate(comment_idx);
            }

            //setup the richText list
            switch(linetype)
            {
            case linecommand:
            {
                if(isScriptCommand(command, "ret"))
                {
                    newRichText.flags=FlagBackground;
                    newRichText.textBackground=QColor("#00FFFF");
                    newRichText.text="ret";
                    richText.push_back(newRichText);
                    QString remainder=command.right(command.length()-3);
                    if(remainder.length())
                    {
                        newRichText.flags=FlagNone;
                        newRichText.text=remainder;
                        richText.push_back(newRichText);
                    }
                }
                else
                {
                    newRichText.flags=FlagNone;
                    newRichText.text=command;
                    richText.push_back(newRichText);
                }
            }
            break;

            case linebranch:
            {
                SCRIPTBRANCH branchinfo;
                DbgScriptGetBranchInfo(line, &branchinfo);
                //jumps
                int i=command.indexOf(" "); //find the index of the space
                switch(branchinfo.type)
                {
                case scriptjmp: //unconditional jumps
                    newRichText.flags=FlagBackground;
                    newRichText.textBackground=QColor("#FFFF00");
                    break;

                case scriptjnejnz: //conditional jumps
                case scriptjejz:
                case scriptjbjl:
                case scriptjajg:
                case scriptjbejle:
                case scriptjaejge:
                    newRichText.flags=FlagAll;
                    newRichText.textBackground=QColor("#FFFF00");
                    newRichText.textColor=QColor("#FF0000");
                    break;

                case scriptcall: //calls
                    newRichText.flags=FlagBackground;
                    newRichText.textBackground=QColor("#00FFFF");
                    break;

                default:
                    newRichText.flags=FlagNone;
                    break;
                }
                newRichText.text=command.left(i);
                richText.push_back(newRichText);
                //space
                newRichText.flags=FlagNone;
                newRichText.text=" ";
                richText.push_back(newRichText);
                //label
                QString label=branchinfo.branchlabel;
                newRichText.flags=FlagBackground;
                newRichText.textBackground=QColor("#FFFF00");
                newRichText.text=label;
                richText.push_back(newRichText);
                //remainder
                QString remainder=command.right(command.length()-command.indexOf(label)-label.length());
                if(remainder.length())
                {
                    newRichText.flags=FlagNone;
                    newRichText.text=remainder;
                    richText.push_back(newRichText);
                }
            }
            break;

            case linelabel:
            {
                newRichText.flags=FlagColor;
                newRichText.textColor=QColor("#808080");
                newRichText.text=command;
                richText.push_back(newRichText);
                painter->drawLine(QPoint(x+xadd+2, y+h-2), QPoint(x+w-4, y+h-2));
            }
            break;

            case linecomment:
            {
                newRichText.flags=FlagColor;
                newRichText.textColor=QColor("#808080");
                newRichText.text=command;
                richText.push_back(newRichText);
            }
            break;

            case lineempty:
            {
            }
            break;
            }

            //append the comment (when present)
            if(comment.length())
            {
                CustomRichText_t newRichText;
                newRichText.flags=FlagNone;
                newRichText.text=" ";
                richText.push_back(newRichText); //space
                newRichText.flags=FlagColor;
                newRichText.textColor=QColor("#808080");
                newRichText.text=comment;
                richText.push_back(newRichText); //comment
            }

            //paint the rich text
            RichTextPainter::paintRichText(painter, x+1, y, w, h, xadd, &richText, charwidth);
            painter->restore();
            returnString="";
        }
        else //no syntax highlighting
            returnString=getCellContent(rowBase+rowOffset, col);
    }
    break;

    case 2: //info
    {
        returnString=getCellContent(rowBase+rowOffset, col);
    }
    break;
    }
    return returnString;
}

void ScriptView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu* wMenu = new QMenu(this);
    wMenu->addMenu(mLoadMenu);
    if(getRowCount())
    {
        wMenu->addAction(mScriptUnload);
        wMenu->addSeparator();
        wMenu->addAction(mScriptBpToggle);
        wMenu->addAction(mScriptRunCursor);
        wMenu->addAction(mScriptStep);
        wMenu->addAction(mScriptRun);
        wMenu->addAction(mScriptAbort);
        wMenu->addAction(mScriptNewIp);
    }
    wMenu->addSeparator();
    wMenu->addAction(mScriptCmdExec);
    wMenu->exec(event->globalPos());
}

void ScriptView::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    if(!getRowCount())
        return;
    newIp();
}

void ScriptView::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    if(key == Qt::Key_Up || key == Qt::Key_Down)
    {
        int_t botRVA = getTableOffset();
        int_t topRVA = botRVA + getNbrOfLineToPrint() - 1;
        if(key == Qt::Key_Up)
            selectPrevious();
        else
            selectNext();
        if(getInitialSelection() < botRVA)
        {
            setTableOffset(getInitialSelection());
        }
        else if(getInitialSelection() >= topRVA)
        {
            setTableOffset(getInitialSelection() - getNbrOfLineToPrint() + 2);
        }
        repaint();
    }
    else if(key == Qt::Key_Return || key == Qt::Key_Enter)
    {
        int line=getInitialSelection()+1;
        SCRIPTBRANCH branchinfo;
        memset(&branchinfo, 0, sizeof(SCRIPTBRANCH));
        if(DbgScriptGetBranchInfo(line, &branchinfo))
            setSelection(branchinfo.dest);
    }
    else
    {
        AbstractTableView::keyPressEvent(event);
    }
}

void ScriptView::setupContextMenu()
{
    mLoadMenu = new QMenu("Load Script", this);

    mScriptLoad = new QAction("Open...", this);
    mScriptLoad->setShortcutContext(Qt::WidgetShortcut);
    mScriptLoad->setShortcut(QKeySequence("ctrl+o"));
    this->addAction(mScriptLoad);
    connect(mScriptLoad, SIGNAL(triggered()), this, SLOT(openFile()));
    mLoadMenu->addAction(mScriptLoad);

    mScriptUnload = new QAction("Unload Script", this);
    mScriptUnload->setShortcutContext(Qt::WidgetShortcut);
    mScriptUnload->setShortcut(QKeySequence("ctrl+u"));
    this->addAction(mScriptUnload);
    connect(mScriptUnload, SIGNAL(triggered()), this, SLOT(unload()));

    mScriptRun = new QAction("Run", this);
    mScriptRun->setShortcutContext(Qt::WidgetShortcut);
    mScriptRun->setShortcut(QKeySequence("space"));
    this->addAction(mScriptRun);
    connect(mScriptRun, SIGNAL(triggered()), this, SLOT(run()));

    mScriptBpToggle = new QAction("Toggle BP", this);
    mScriptBpToggle->setShortcutContext(Qt::WidgetShortcut);
    mScriptBpToggle->setShortcut(QKeySequence("F2"));
    this->addAction(mScriptBpToggle);
    connect(mScriptBpToggle, SIGNAL(triggered()), this, SLOT(bpToggle()));

    mScriptRunCursor = new QAction("Run until selection", this);
    mScriptRunCursor->setShortcutContext(Qt::WidgetShortcut);
    mScriptRunCursor->setShortcut(QKeySequence("F4"));
    this->addAction(mScriptRunCursor);
    connect(mScriptRunCursor, SIGNAL(triggered()), this, SLOT(runCursor()));

    mScriptStep = new QAction("Step", this);
    mScriptStep->setShortcutContext(Qt::WidgetShortcut);
    mScriptStep->setShortcut(QKeySequence("tab"));
    this->addAction(mScriptStep);
    connect(mScriptStep, SIGNAL(triggered()), this, SLOT(step()));

    mScriptAbort = new QAction("Abort", this);
    mScriptAbort->setShortcutContext(Qt::WidgetShortcut);
    mScriptAbort->setShortcut(QKeySequence("esc"));
    this->addAction(mScriptAbort);
    connect(mScriptAbort, SIGNAL(triggered()), this, SLOT(abort()));

    mScriptCmdExec = new QAction("Execute Command...", this);
    mScriptCmdExec->setShortcutContext(Qt::WidgetShortcut);
    mScriptCmdExec->setShortcut(QKeySequence("x"));
    this->addAction(mScriptCmdExec);
    connect(mScriptCmdExec, SIGNAL(triggered()), this, SLOT(cmdExec()));

    mScriptNewIp = new QAction("Continue here...", this);
    mScriptNewIp->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mScriptNewIp);
    connect(mScriptNewIp, SIGNAL(triggered()), this, SLOT(newIp()));
}

bool ScriptView::isScriptCommand(QString text, QString cmd)
{
    int len=text.length();
    int cmdlen=cmd.length();
    if(cmdlen>len)
        return false;
    else if(cmdlen==len)
        return (text.compare(cmd, Qt::CaseInsensitive)==0);
    else if(text.at(cmdlen)==' ')
        return (text.left(cmdlen).compare(cmd, Qt::CaseInsensitive)==0);
    return false;
}

//slots
void ScriptView::add(int count, const char** lines)
{
    setRowCount(count);
    for(int i=0; i<count; i++)
        setCellContent(i, 1, QString(lines[i]));
    BridgeFree(lines);
    reloadData(); //repaint
    Bridge::getBridge()->BridgeSetResult(1);
}

void ScriptView::clear()
{
    setRowCount(0);
    mIpLine=0;
    reloadData(); //repaint
}

void ScriptView::setIp(int line)
{
    int offset=line-1;
    if(!isValidIndex(offset, 0))
    {
        mIpLine=0;
        return;
    }
    mIpLine=line;
    int rangefrom=getTableOffset();
    int rangeto=rangefrom+getViewableRowsCount()-1;
    if(offset<rangefrom) //ip lays before the current view
        setTableOffset(offset);
    else if(offset>(rangeto-1)) //ip lays after the current view
        setTableOffset(offset-getViewableRowsCount()+2);
    setSingleSelection(offset);
    reloadData(); //repaint
}

void ScriptView::setSelection(int line)
{
    int offset=line-1;
    if(!isValidIndex(offset, 0))
        return;
    int rangefrom=getTableOffset();
    int rangeto=rangefrom+getViewableRowsCount()-1;
    if(offset<rangefrom) //ip lays before the current view
        setTableOffset(offset);
    else if(offset>(rangeto-1)) //ip lays after the current view
        setTableOffset(offset-getViewableRowsCount()+2);
    setSingleSelection(offset);
    reloadData(); //repaint
}

void ScriptView::error(int line, QString message)
{
    QString title;
    if(isValidIndex(line-1, 0))
        title=title.sprintf("Error on line %.4d!", line);
    else
        title="Script Error!";
    QMessageBox msg(QMessageBox::Critical, title, message);
    msg.setWindowIcon(QIcon(":/icons/images/script-error.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void ScriptView::setTitle(QString title)
{
    setWindowTitle(title);
}

void ScriptView::setInfoLine(int line, QString info)
{
    setCellContent(line-1, 2, info);
    reloadData(); //repaint
}

void ScriptView::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select script"), 0, tr("Script files (*.txt *.scr);;All files (*.*)"));
    if(!filename.length())
        return;
    filename=QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)
    DbgScriptUnload();
    DbgScriptLoad(filename.toUtf8().constData());
}

void ScriptView::unload()
{
    DbgScriptUnload();
}

void ScriptView::run()
{
    if(!getRowCount())
        return;
    DbgScriptRun(0);
}

void ScriptView::bpToggle()
{
    if(!getRowCount())
        return;
    int selected=getInitialSelection()+1;
    if(!DbgScriptBpToggle(selected))
        error(selected, "Error setting script breakpoint!");
    reloadData();
}

void ScriptView::runCursor()
{
    if(!getRowCount())
        return;
    int selected=getInitialSelection()+1;
    DbgScriptRun(selected);
}

void ScriptView::step()
{
    if(!getRowCount())
        return;
    DbgScriptStep();
}

void ScriptView::abort()
{
    if(!getRowCount())
        return;
    DbgScriptAbort();
}

void ScriptView::cmdExec()
{
    LineEditDialog mLineEdit(this);
    mLineEdit.setWindowTitle("Execute Script Command...");
    if(mLineEdit.exec()!=QDialog::Accepted)
        return;
    if(!DbgScriptCmdExec(mLineEdit.editText.toUtf8().constData()))
        error(0, "Error executing command!");
}

void ScriptView::message(QString message)
{
    QMessageBox msg(QMessageBox::Information, "Information", message);
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void ScriptView::newIp()
{
    if(!getRowCount())
        return;
    int selected=getInitialSelection()+1;
    if(isValidIndex(selected-1, 0))
        DbgScriptSetIp(selected);
}

void ScriptView::question(QString message)
{
    QMessageBox msg(QMessageBox::Question, "Question", message, QMessageBox::Yes|QMessageBox::No);
    msg.setWindowIcon(QIcon(":/icons/images/question.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
    if(msg.exec()==QMessageBox::Yes)
        Bridge::getBridge()->BridgeSetResult(1);
    else
        Bridge::getBridge()->BridgeSetResult(0);
}

void ScriptView::enableHighlighting(bool enable)
{
    mEnableSyntaxHighlighting=enable;
}
