#include "ScriptView.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QClipboard>
#include <QMimeData>
#include "Configuration.h"
#include "Bridge.h"
#include "RichTextPainter.h"
#include "LineEditDialog.h"
#include "MRUList.h"

ScriptView::ScriptView(StdTable* parent) : StdTable(parent)
{
    mEnableSyntaxHighlighting = false;
    enableMultiSelection(false);
    enableColumnSorting(false);
    setDrawDebugOnly(false);
    setDisassemblyPopupEnabled(false);

    int charwidth = getCharWidth();

    addColumnAt(8 + charwidth * 4, tr("Line"), false);
    addColumnAt(8 + charwidth * 60, tr("Text"), false);
    addColumnAt(8 + charwidth * 40, tr("Info"), false);
    loadColumnFromConfig("Script");

    setIp(0); //no IP

    setupContextMenu();

    //message box
    msg = new QMessageBox(this);
    msg->setWindowFlags(msg->windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg->setModal(false);
    connect(msg, SIGNAL(finished(int)), this, SLOT(messageResult(int)));

    // recent script
    mMRUList = new MRUList(this, "Recent Scripts");
    connect(mMRUList, SIGNAL(openFile(QString)), this, SLOT(openRecentFile(QString)));
    mMRUList->load();

    // command line edit dialog
    mCmdLineEdit = new LineEditDialog(this);
    mCmdLineEdit->setWindowTitle(tr("Execute Script Command..."));

    // Slots
    connect(Bridge::getBridge(), SIGNAL(scriptAdd(int, const char**)), this, SLOT(add(int, const char**)));
    connect(Bridge::getBridge(), SIGNAL(scriptClear()), this, SLOT(clear()));
    connect(Bridge::getBridge(), SIGNAL(scriptSetIp(int)), this, SLOT(setIp(int)));
    connect(Bridge::getBridge(), SIGNAL(scriptError(int, QString)), this, SLOT(error(int, QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptSetTitle(QString)), this, SLOT(setTitle(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptSetInfoLine(int, QString)), this, SLOT(setInfoLine(int, QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptMessage(QString)), this, SLOT(message(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptQuestion(QString)), this, SLOT(question(QString)));
    connect(Bridge::getBridge(), SIGNAL(scriptEnableHighlighting(bool)), this, SLOT(enableHighlighting(bool)));
    connect(Bridge::getBridge(), SIGNAL(shutdown()), this, SLOT(shutdownSlot()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));

    Initialize();
}

void ScriptView::updateColors()
{
    StdTable::updateColors();

    mSelectionColor = ConfigColor("DisassemblySelectionColor");
    mBackgroundColor = ConfigColor("DisassemblyBackgroundColor");
}

QString ScriptView::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    bool wIsSelected = isSelected(rowBase, rowOffset);
    // Highlight if selected
    if(wIsSelected)
        painter->fillRect(QRect(x, y, w, h), QBrush(mSelectionColor)); //ScriptViewSelectionColor
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
            painter->setPen(QPen(ConfigColor("DisassemblyBreakpointColor"))); //black address //ScripViewMainBpTextColor
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
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, returnString);
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
            RichTextPainter::List richText;
            RichTextPainter::CustomRichText_t newRichText;
            newRichText.underline = false;
            QString command = getCellContent(rowBase + rowOffset, col);

            //handle comments
            int comment_idx = command.indexOf("\1"); //find the index of the comment
            QString comment = "";
            if(comment_idx != -1 && command.at(0) != QChar('/')) //there is a comment
            {
                comment = command.right(command.length() - comment_idx - 1);
                if(command.at(comment_idx - 1) == QChar(' '))
                    command.truncate(comment_idx - 1);
                else
                    command.truncate(comment_idx);
            }

            QString mnemonic, argument;

            //setup the richText list
            switch(linetype)
            {
            case linecommand:
            {
                if(isScriptCommand(command, "ret", mnemonic, argument))
                {
                    newRichText.flags = RichTextPainter::FlagAll;
                    newRichText.textColor = ConfigColor("InstructionRetColor");
                    newRichText.textBackground = ConfigColor("InstructionRetBackgroundColor");
                    newRichText.text = mnemonic;
                    richText.push_back(newRichText);
                    if(argument.length())
                    {
                        newRichText.flags = RichTextPainter::FlagAll;
                        newRichText.textColor = ConfigColor("InstructionUncategorizedColor");
                        newRichText.textBackground = ConfigColor("InstructionUncategorizedBackgroundColor");
                        newRichText.text = argument;
                        richText.push_back(newRichText);
                    }
                }
                else if(isScriptCommand(command, "invalid", mnemonic, argument) || isScriptCommand(command, "error", mnemonic, argument))
                {
                    newRichText.flags = RichTextPainter::FlagAll;
                    newRichText.textColor = ConfigColor("InstructionUnusualColor");
                    newRichText.textBackground = ConfigColor("InstructionUnusualBackgroundColor");
                    newRichText.text = mnemonic;
                    richText.push_back(newRichText);
                    if(argument.length())
                    {
                        newRichText.flags = RichTextPainter::FlagAll;
                        newRichText.textColor = ConfigColor("InstructionUncategorizedColor");
                        newRichText.textBackground = ConfigColor("InstructionUncategorizedBackgroundColor");
                        newRichText.text = argument;
                        richText.push_back(newRichText);
                    }
                }
                else if(isScriptCommand(command, "nop", mnemonic, argument))
                {
                    newRichText.flags = RichTextPainter::FlagAll;
                    newRichText.textColor = ConfigColor("InstructionNopColor");
                    newRichText.textBackground = ConfigColor("InstructionNopBackgroundColor");
                    newRichText.text = mnemonic;
                    richText.push_back(newRichText);
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
                newRichText.underline = false;
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
            RichTextPainter::paintRichText(painter, x + 1, y, w, h, xadd, richText, mFontMetrics);
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
    QMenu wMenu(this);
    mMenu->build(&wMenu);
    wMenu.exec(mapToGlobal(pos));
}

void ScriptView::mouseDoubleClickEvent(QMouseEvent* event)
{
    AbstractTableView::mouseDoubleClickEvent(event);
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
        dsint botRVA = getTableOffset();
        dsint topRVA = botRVA + getNbrOfLineToPrint() - 1;
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
        reloadData();
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
    mMenu = new MenuBuilder(this);
    MenuBuilder* loadMenu = new MenuBuilder(this);
    loadMenu->addAction(makeShortcutAction(DIcon("folder-horizontal-open"), tr("&Open..."), SLOT(openFile()), "ActionLoadScript"));
    loadMenu->addAction(makeShortcutAction(DIcon("binary_paste"), tr("&Paste"), SLOT(paste()), "ActionBinaryPaste"), [](QMenu*)
    {
        return QApplication::clipboard()->mimeData()->hasText();
    });
    loadMenu->addSeparator();
    loadMenu->addBuilder(new MenuBuilder(this, [this](QMenu * menu)
    {
        mMRUList->appendMenu(menu);
        return true;
    }));
    mMenu->addMenu(makeMenu(DIcon("load-script"), tr("Load Script")), loadMenu);
    auto isempty = [this](QMenu*)
    {
        return getRowCount() != 0;
    };
    auto isemptyclipboard = [this](QMenu*)
    {
        return getRowCount() != 0 && !filename.isEmpty();
    };
    mMenu->addAction(makeShortcutAction(DIcon("arrow-restart"), tr("Re&load Script"), SLOT(reload()), "ActionReloadScript"), isemptyclipboard);
    mMenu->addAction(makeShortcutAction(DIcon("control-exit"), tr("&Unload Script"), SLOT(unload()), "ActionUnloadScript"), isempty);
    mMenu->addAction(makeShortcutAction(DIcon("edit-script"), tr("&Edit Script"), SLOT(edit()), "ActionEditScript"), isemptyclipboard);
    mMenu->addSeparator();
    mMenu->addAction(makeShortcutAction(DIcon("breakpoint_toggle"), tr("Toggle &BP"), SLOT(bpToggle()), "ActionToggleBreakpointScript"), isempty);
    mMenu->addAction(makeShortcutAction(DIcon("arrow-run-cursor"), tr("Ru&n until selection"), SLOT(runCursor()), "ActionRunToCursorScript"), isempty);
    mMenu->addAction(makeShortcutAction(DIcon("arrow-step-into"), tr("&Step"), SLOT(step()), "ActionStepScript"), isempty);
    mMenu->addAction(makeShortcutAction(DIcon("arrow-run"), tr("&Run"), SLOT(run()), "ActionRunScript"), isempty);
    mMenu->addAction(makeShortcutAction(DIcon("control-stop"), tr("&Abort"), SLOT(abort()), "ActionAbortScript"), isempty);
    mMenu->addAction(makeAction(DIcon("neworigin"), tr("&Continue here..."), SLOT(newIp())), isempty);
    mMenu->addSeparator();
    MenuBuilder* copyMenu = new MenuBuilder(this);
    setupCopyMenu(copyMenu);
    mMenu->addMenu(makeMenu(DIcon("copy"), tr("Copy")), copyMenu);
    mMenu->addAction(makeShortcutAction(DIcon("terminal-command"), tr("E&xecute Command..."), SLOT(cmdExec()), "ActionExecuteCommandScript"));
}

bool ScriptView::isScriptCommand(QString text, QString cmd, QString & mnemonic, QString & argument)
{
    mnemonic = cmd;
    argument.clear();
    int len = text.length();
    int cmdlen = cmd.length();
    if(cmdlen > len)
        return false;
    else if(cmdlen == len)
        return (text.compare(cmd, Qt::CaseInsensitive) == 0);
    else if(text.at(cmdlen) == ' ')
    {
        argument = text.mid(cmdlen);
        return (text.left(cmdlen).compare(cmd, Qt::CaseInsensitive) == 0);
    }
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
    Bridge::getBridge()->setResult(BridgeResult::ScriptAdd, 1);
}

void ScriptView::clear()
{
    setRowCount(0);
    mIpLine = 0;
    reloadData(); //repaint
}

void ScriptView::setIp(int line)
{
    mIpLine = scrollSelect(line - 1) ? line : 0;
    reloadData(); //repaint
}

void ScriptView::setSelection(int line)
{
    scrollSelect(line - 1);
    reloadData(); //repaint
}

void ScriptView::error(int line, QString message)
{
    QString title;
    if(isValidIndex(line - 1, 0))
        title = tr("Error on line") + title.sprintf(" %.4d!", line);
    else
        title = tr("Script Error!");
    msg->setIcon(QMessageBox::Critical);
    msg->setWindowTitle(title);
    msg->setText(message);
    msg->setStandardButtons(QMessageBox::Ok);
    msg->setWindowIcon(DIcon("script-error"));
    msg->show();
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

void ScriptView::openRecentFile(QString file)
{
    filename = file;
    DbgScriptUnload();
    DbgScriptLoad(filename.toUtf8().constData());
    mMRUList->addEntry(filename);
    mMRUList->save();
}

void ScriptView::openFile()
{
    filename = QFileDialog::getOpenFileName(this, tr("Select script"), 0, tr("Script files (*.txt *.scr);;All files (*.*)"));
    if(!filename.length())
        return;
    filename = QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)
    openRecentFile(filename);
}

void ScriptView::paste()
{
    filename.clear();
    DbgScriptUnload();
    DbgScriptLoad("x64dbg://localhost/clipboard");
}

void ScriptView::reload()
{
    if(!filename.isEmpty())
        openRecentFile(filename);
}

void ScriptView::unload()
{
    filename.clear();
    DbgScriptUnload();
}

void ScriptView::edit()
{
    if(!filename.isEmpty())
        if(!QDesktopServices::openUrl(QUrl(QDir::fromNativeSeparators(filename))))
            SimpleWarningBox(this, tr("Error!"), tr("File open failed! Please open the file yourself..."));
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
        error(selected, tr("Error setting script breakpoint!"));
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
    if(mCmdLineEdit->exec() != QDialog::Accepted)
        return;
    if(!DbgScriptCmdExec(mCmdLineEdit->editText.toUtf8().constData()))
        error(0, tr("Error executing command!"));
}

void ScriptView::message(QString message)
{
    msg->setIcon(QMessageBox::Information);
    msg->setWindowTitle(tr("Message"));
    msg->setText(message);
    msg->setStandardButtons(QMessageBox::Ok);
    msg->setWindowIcon(DIcon("information"));
    msg->show();
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
    msg->setIcon(QMessageBox::Question);
    msg->setWindowTitle(tr("Question"));
    msg->setText(message);
    msg->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg->setWindowIcon(DIcon("question"));
    msg->show();
}

void ScriptView::enableHighlighting(bool enable)
{
    mEnableSyntaxHighlighting = enable;
}

void ScriptView::messageResult(int result)
{
    Bridge::getBridge()->setResult(BridgeResult::ScriptMessage, result == QMessageBox::Yes);
}

void ScriptView::shutdownSlot()
{
    msg->close();
    unload();
}
