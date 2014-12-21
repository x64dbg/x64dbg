#include "ScriptView.h"
#include <QMessageBox>
#include <QFileDialog>
#include "Configuration.h"
#include "Bridge.h"
#include "RichTextPainter.h"
#include "LineEditDialog.h"

ScriptView::ScriptView(StdTable* parent) : StdTable(parent)
{
    mEnableSyntaxHighlighting = false;
    enableMultiSelection(false);

    int charwidth = getCharWidth();

    addColumnAt(8 + charwidth * 4, "Line", false);
    addColumnAt(8 + charwidth * 60, "Text", false);
    addColumnAt(8 + charwidth * 40, "Info", false);

    setIp(0); //no IP

    connect(Bridge::getBridge(), SIGNAL(scriptAdd(int, const char**)), this, SLOT(add(int, const char**)));
    connect(Bridge::getBridge(), SIGNAL(scriptClear()), this, SLOT(clear()));
    connect(Bridge::getBridge(), SIGNAL(scriptSetIp(int)), this, SLOT(setIp(int)));
    connect(Bridge::getBridge(), SIGNAL(scriptError(int, QString)), this, SLOT(error(int, QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptSetTitle(QString)), this, SLOT(setTitle(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptSetInfoLine(int, QString)), this, SLOT(setInfoLine(int, QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptMessage(QString)), this, SLOT(message(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptQuestion(QString)), this, SLOT(question(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptEnableHighlighting(bool)), this, SLOT(enableHighlighting(bool)));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));

    setupContextMenu();

    selectionColor = ConfigColor("DisassemblySelectionColor");
    backgroundColor = ConfigColor("DisassemblyBackgroundColor");
}

void ScriptView::colorsUpdated()
{
    StdTable::colorsUpdated();
    selectionColor = ConfigColor("DisassemblySelectionColor");
    backgroundColor = ConfigColor("DisassemblyBackgroundColor");
}

QString ScriptView::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    bool wIsSelected = isSelected(rowBase, rowOffset);
    // Highlight if selected
    if(wIsSelected)
        painter->fillRect(QRect(x, y, w, h), QBrush(selectionColor)); //ScriptViewSelectionColor
    QString returnString;
    int line = rowBase + rowOffset + 1;
    SCRIPTLINETYPE linetype = DbgScriptGetLineType(line);
    switch(col)
    {
    case 0: //line number
    {
        returnString = returnString.sprintf("%.4d", line);
        if(line == mIpLine) //IP
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyCipBackgroundColor")));
            if(DbgScriptBpGet(line)) //breakpoint
            {
                QColor bpColor = ConfigColor("DisassemblyBreakpointBackgroundColor");
                if(!bpColor.alpha()) //we don't want transparent text
                    bpColor = ConfigColor("DisassemblyBreakpointColor");
                painter->setPen(QPen(bpColor));
            }
            else
                painter->setPen(QPen(ConfigColor("DisassemblyCipColor"))); //white address (ScriptViewIpTextColor)
        }
        else if(DbgScriptBpGet(line)) //breakpoint
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("DisassemblyBreakpointBackgroundColor")));
            painter->setPen(QPen(ConfigColor("DisassemblyBreakpointBackgroundColor"))); //black address //ScripViewMainBpTextColor
        }
        else
        {
            QColor background;
            if(linetype == linecommand || linetype == linebranch)
            {
                background = ConfigColor("DisassemblySelectedAddressBackgroundColor");
                painter->setPen(QPen(ConfigColor("DisassemblySelectedAddressColor"))); //black address (DisassemblySelectedAddressColor)
            }
            else
            {
                background = ConfigColor("DisassemblyAddressBackgroundColor");
                painter->setPen(QPen(ConfigColor("DisassemblyAddressColor"))); //grey address
            }
            if(background.alpha())
                painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background
        }
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, returnString);
        returnString = "";
    }
    break;

    case 1: //command
    {
        if(mEnableSyntaxHighlighting)
        {
            //initialize
            int charwidth = getCharWidth();
            int xadd = charwidth; //for testing
            QList<RichTextPainter::CustomRichText_t> richText;
            RichTextPainter::CustomRichText_t newRichText;
            newRichText.highlight = false;
            QString command = getCellContent(rowBase + rowOffset, col);

            //handle comments
            int comment_idx = command.indexOf("//"); //find the index of the space
            QString comment = "";
            if(comment_idx != -1 && command.at(0) != QChar('/')) //there is a comment
            {
                comment = command.right(command.length() - comment_idx);
                if(command.at(comment_idx - 1) == QChar(' '))
                    command.truncate(comment_idx - 1);
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
                    newRichText.flags = RichTextPainter::FlagAll;
                    newRichText.textColor = ConfigColor("InstructionRetColor");
                    newRichText.textBackground = ConfigColor("InstructionRetBackgroundColor");
                    newRichText.text = "ret";
                    richText.push_back(newRichText);
                    QString remainder = command.right(command.length() - 3);
                    if(remainder.length())
                    {
                        newRichText.flags = RichTextPainter::FlagAll;
                        newRichText.textColor = ConfigColor("InstructionUncategorizedColor");
                        newRichText.textBackground = ConfigColor("InstructionUncategorizedBackgroundColor");
                        newRichText.text = remainder;
                        richText.push_back(newRichText);
                    }
                }
                else
                {
                    newRichText.flags = RichTextPainter::FlagAll;
                    newRichText.textColor = ConfigColor("InstructionUncategorizedColor");
                    newRichText.textBackground = ConfigColor("InstructionUncategorizedBackgroundColor");
                    newRichText.text = command;
                    richText.push_back(newRichText);
                }
            }
            break;

            case linebranch:
            {
                SCRIPTBRANCH branchinfo;
                DbgScriptGetBranchInfo(line, &branchinfo);
                //jumps
                int i = command.indexOf(" "); //find the index of the space
                switch(branchinfo.type)
                {
                case scriptjmp: //unconditional jumps
                    newRichText.flags = RichTextPainter::FlagAll;
                    newRichText.textColor = ConfigColor("InstructionUnconditionalJumpColor");
                    newRichText.textBackground = ConfigColor("InstructionUnconditionalJumpBackgroundColor");
                    break;

                case scriptjnejnz: //conditional jumps
                case scriptjejz:
                case scriptjbjl:
                case scriptjajg:
                case scriptjbejle:
                case scriptjaejge:
                    newRichText.flags = RichTextPainter::FlagAll;
                    newRichText.textColor = ConfigColor("InstructionConditionalJumpColor");
                    newRichText.textBackground = ConfigColor("InstructionConditionalJumpBackgroundColor");
                    break;

                case scriptcall: //calls
                    newRichText.flags = RichTextPainter::FlagAll;
                    newRichText.textColor = ConfigColor("InstructionCallColor");
                    newRichText.textBackground = ConfigColor("InstructionCallBackgroundColor");
                    break;

                default:
                    newRichText.flags = RichTextPainter::FlagAll;
                    newRichText.textColor = ConfigColor("InstructionUncategorizedColor");
                    newRichText.textBackground = ConfigColor("InstructionUncategorizedBackgroundColor");
                    break;
                }
                newRichText.text = command.left(i);
                richText.push_back(newRichText);
                //space
                newRichText.flags = RichTextPainter::FlagNone;
                newRichText.text = " ";
                richText.push_back(newRichText);
                //label
                QString label = branchinfo.branchlabel;
                newRichText.flags = RichTextPainter::FlagAll;
                newRichText.textColor = ConfigColor("InstructionAddressColor");
                newRichText.textBackground = ConfigColor("InstructionAddressBackgroundColor");
                newRichText.text = label;
                richText.push_back(newRichText);
                //remainder
                QString remainder = command.right(command.length() - command.indexOf(label) - label.length());
                if(remainder.length())
                {
                    newRichText.textColor = ConfigColor("InstructionUncategorizedColor");
                    newRichText.textBackground = ConfigColor("InstructionUncategorizedBackgroundColor");
                    newRichText.text = remainder;
                    richText.push_back(newRichText);
                }
            }
            break;

            case linelabel:
            {
                newRichText.flags = RichTextPainter::FlagAll;
                newRichText.textColor = ConfigColor("DisassemblyAddressColor");
                newRichText.textBackground = ConfigColor("DisassemblyAddressBackgroundColor");
                newRichText.text = command;
                richText.push_back(newRichText);
                painter->drawLine(QPoint(x + xadd + 2, y + h - 2), QPoint(x + w - 4, y + h - 2));
            }
            break;

            case linecomment:
            {
                newRichText.flags = RichTextPainter::FlagAll;
                newRichText.textColor = ConfigColor("DisassemblyAddressColor");
                newRichText.textBackground = ConfigColor("DisassemblyAddressBackgroundColor");
                newRichText.text = command;
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
                RichTextPainter::CustomRichText_t newRichText;
                newRichText.highlight = false;
                newRichText.flags = RichTextPainter::FlagNone;
                newRichText.text = " ";
                richText.push_back(newRichText); //space
                newRichText.flags = RichTextPainter::FlagAll;
                newRichText.textColor = ConfigColor("DisassemblyAddressColor");
                newRichText.textBackground = ConfigColor("DisassemblyAddressBackgroundColor");
                newRichText.text = comment;
                richText.push_back(newRichText); //comment
            }

            //paint the rich text
            RichTextPainter::paintRichText(painter, x + 1, y, w, h, xadd, &richText, charwidth);
            returnString = "";
        }
        else //no syntax highlighting
            returnString = getCellContent(rowBase + rowOffset, col);
    }
    break;

    case 2: //info
    {
        returnString = getCellContent(rowBase + rowOffset, col);
    }
    break;
    }
    return returnString;
}

void ScriptView::contextMenuSlot(const QPoint & pos)
{
    QMenu* wMenu = new QMenu(this);
    wMenu->addMenu(mLoadMenu);
    if(getRowCount())
    {
        wMenu->addAction(mScriptReload);
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
    wMenu->exec(mapToGlobal(pos));
}

void ScriptView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton)
        return;
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
        int line = getInitialSelection() + 1;
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
    this->addAction(mScriptLoad);
    connect(mScriptLoad, SIGNAL(triggered()), this, SLOT(openFile()));
    mLoadMenu->addAction(mScriptLoad);

    mScriptReload = new QAction("Reload Script", this);
    mScriptReload->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mScriptReload);
    connect(mScriptReload, SIGNAL(triggered()), this, SLOT(reload()));

    mScriptUnload = new QAction("Unload Script", this);
    mScriptUnload->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mScriptUnload);
    connect(mScriptUnload, SIGNAL(triggered()), this, SLOT(unload()));

    mScriptRun = new QAction("Run", this);
    mScriptRun->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mScriptRun);
    connect(mScriptRun, SIGNAL(triggered()), this, SLOT(run()));

    mScriptBpToggle = new QAction("Toggle BP", this);
    mScriptBpToggle->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mScriptBpToggle);
    connect(mScriptBpToggle, SIGNAL(triggered()), this, SLOT(bpToggle()));

    mScriptRunCursor = new QAction("Run until selection", this);
    mScriptRunCursor->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mScriptRunCursor);
    connect(mScriptRunCursor, SIGNAL(triggered()), this, SLOT(runCursor()));

    mScriptStep = new QAction("Step", this);
    mScriptStep->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mScriptStep);
    connect(mScriptStep, SIGNAL(triggered()), this, SLOT(step()));

    mScriptAbort = new QAction("Abort", this);
    mScriptAbort->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mScriptAbort);
    connect(mScriptAbort, SIGNAL(triggered()), this, SLOT(abort()));

    mScriptCmdExec = new QAction("Execute Command...", this);
    mScriptCmdExec->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mScriptCmdExec);
    connect(mScriptCmdExec, SIGNAL(triggered()), this, SLOT(cmdExec()));

    mScriptNewIp = new QAction("Continue here...", this);
    mScriptNewIp->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mScriptNewIp);
    connect(mScriptNewIp, SIGNAL(triggered()), this, SLOT(newIp()));

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void ScriptView::refreshShortcutsSlot()
{
    mScriptLoad->setShortcut(ConfigShortcut("ActionLoadScript"));
    mScriptReload->setShortcut(ConfigShortcut("ActionReloadScript"));
    mScriptUnload->setShortcut(ConfigShortcut("ActionUnloadScript"));
    mScriptRun->setShortcut(ConfigShortcut("ActionRunScript"));
    mScriptBpToggle->setShortcut(ConfigShortcut("ActionToggleBreakpointScript"));
    mScriptRunCursor->setShortcut(ConfigShortcut("ActionRunToCursorScript"));
    mScriptStep->setShortcut(ConfigShortcut("ActionStepScript"));
    mScriptAbort->setShortcut(ConfigShortcut("ActionAbortScript"));
    mScriptCmdExec->setShortcut(ConfigShortcut("ActionExecuteCommandScript"));
}

bool ScriptView::isScriptCommand(QString text, QString cmd)
{
    int len = text.length();
    int cmdlen = cmd.length();
    if(cmdlen > len)
        return false;
    else if(cmdlen == len)
        return (text.compare(cmd, Qt::CaseInsensitive) == 0);
    else if(text.at(cmdlen) == ' ')
        return (text.left(cmdlen).compare(cmd, Qt::CaseInsensitive) == 0);
    return false;
}

//slots
void ScriptView::add(int count, const char** lines)
{
    setRowCount(count);
    for(int i = 0; i < count; i++)
        setCellContent(i, 1, QString(lines[i]));
    BridgeFree(lines);
    reloadData(); //repaint
    Bridge::getBridge()->BridgeSetResult(1);
}

void ScriptView::clear()
{
    setRowCount(0);
    mIpLine = 0;
    reloadData(); //repaint
}

void ScriptView::setIp(int line)
{
    int offset = line - 1;
    if(!isValidIndex(offset, 0))
    {
        mIpLine = 0;
        return;
    }
    mIpLine = line;
    int rangefrom = getTableOffset();
    int rangeto = rangefrom + getViewableRowsCount() - 1;
    if(offset < rangefrom) //ip lays before the current view
        setTableOffset(offset);
    else if(offset > (rangeto - 1)) //ip lays after the current view
        setTableOffset(offset - getViewableRowsCount() + 2);
    setSingleSelection(offset);
    reloadData(); //repaint
}

void ScriptView::setSelection(int line)
{
    int offset = line - 1;
    if(!isValidIndex(offset, 0))
        return;
    int rangefrom = getTableOffset();
    int rangeto = rangefrom + getViewableRowsCount() - 1;
    if(offset < rangefrom) //ip lays before the current view
        setTableOffset(offset);
    else if(offset > (rangeto - 1)) //ip lays after the current view
        setTableOffset(offset - getViewableRowsCount() + 2);
    setSingleSelection(offset);
    reloadData(); //repaint
}

void ScriptView::error(int line, QString message)
{
    QString title;
    if(isValidIndex(line - 1, 0))
        title = title.sprintf("Error on line %.4d!", line);
    else
        title = "Script Error!";
    QMessageBox msg(QMessageBox::Critical, title, message);
    msg.setWindowIcon(QIcon(":/icons/images/script-error.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void ScriptView::setTitle(QString title)
{
    setWindowTitle(title);
}

void ScriptView::setInfoLine(int line, QString info)
{
    setCellContent(line - 1, 2, info);
    reloadData(); //repaint
}

void ScriptView::openFile()
{
    filename = QFileDialog::getOpenFileName(this, tr("Select script"), 0, tr("Script files (*.txt *.scr);;All files (*.*)"));
    if(!filename.length())
        return;
    filename = QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)
    DbgScriptUnload();
    DbgScriptLoad(filename.toUtf8().constData());
}

void ScriptView::reload()
{
    if(!filename.length())
        return;
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
    int selected = getInitialSelection() + 1;
    if(!DbgScriptBpToggle(selected))
        error(selected, "Error setting script breakpoint!");
    reloadData();
}

void ScriptView::runCursor()
{
    if(!getRowCount())
        return;
    int selected = getInitialSelection() + 1;
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
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    if(!DbgScriptCmdExec(mLineEdit.editText.toUtf8().constData()))
        error(0, "Error executing command!");
}

void ScriptView::message(QString message)
{
    QMessageBox msg(QMessageBox::Information, "Information", message);
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void ScriptView::newIp()
{
    if(!getRowCount())
        return;
    int selected = getInitialSelection() + 1;
    if(isValidIndex(selected - 1, 0))
        DbgScriptSetIp(selected);
}

void ScriptView::question(QString message)
{
    QMessageBox msg(QMessageBox::Question, "Question", message, QMessageBox::Yes | QMessageBox::No);
    msg.setWindowIcon(QIcon(":/icons/images/question.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    if(msg.exec() == QMessageBox::Yes)
        Bridge::getBridge()->BridgeSetResult(1);
    else
        Bridge::getBridge()->BridgeSetResult(0);
}

void ScriptView::enableHighlighting(bool enable)
{
    mEnableSyntaxHighlighting = enable;
}
