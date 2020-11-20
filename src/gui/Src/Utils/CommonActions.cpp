#include "CommonActions.h"
#include "MenuBuilder.h"
#include <QAction>
#include <QMessageBox>
#include <QFile>
#include "StringUtil.h"
#include "MiscUtil.h"
#include "Breakpoints.h"
#include "LineEditDialog.h"
#include "WordEditDialog.h"

CommonActions::CommonActions(QWidget* parent, ActionHelperFuncs funcs, GetSelectionFunc getSelection)
    : QObject(parent), ActionHelperProxy(funcs), mGetSelection(getSelection)
{
}

void CommonActions::build(MenuBuilder* builder, int actions)
{
    // Condition Lambda
    auto wIsDebugging = [this](QMenu*)
    {
        return mGetSelection() != 0 && DbgIsDebugging();
    };
    auto wIsValidReadPtrCallback = [this](QMenu*)
    {
        duint ptr = 0;
        DbgMemRead(mGetSelection(), (unsigned char*)&ptr, sizeof(duint));
        return DbgMemIsValidReadPtr(ptr);
    };

    // Menu action
    if(actions & ActionDisasm)
    {
        builder->addAction(makeShortcutAction(DIcon(ArchValue("processor32.png", "processor64.png")), tr("Follow in Disassembler"), std::bind(&CommonActions::followDisassemblySlot, this), "ActionFollowDisasm"), wIsDebugging);
    }
    if(actions & ActionDisasmData)
    {
        builder->addAction(makeCommandAction(DIcon("processor32.png"), ArchValue(tr("&Follow DWORD in Disassembler"), tr("&Follow QWORD in Disassembler")), "disasm [$]", "ActionFollowDwordQwordDisasm"), wIsValidReadPtrCallback);
    }
    if(actions & ActionDump)
    {
        builder->addAction(makeCommandAction(DIcon("dump.png"), tr("Follow in Dump"), "dump $"));
    }
    if(actions & ActionDumpData)
    {
        builder->addAction(makeCommandAction(DIcon("dump.png"), ArchValue(tr("&Follow DWORD in Current Dump"), tr("&Follow QWORD in Current Dump")), "dump [$]", "ActionFollowDwordQwordDump"), wIsValidReadPtrCallback);
    }
    if(actions & ActionStackDump)
    {
        builder->addAction(makeCommandAction(DIcon("stack.png"), tr("Follow in Stack"), "sdump $", "ActionFollowStack"), [this](QMenu*)
        {
            auto start = mGetSelection();
            return (DbgMemIsValidReadPtr(start) && DbgMemFindBaseAddr(start, 0) == DbgMemFindBaseAddr(DbgValFromString("csp"), 0));
        });
    }
    if(actions & ActionMemoryMap)
    {
        builder->addAction(makeCommandAction(DIcon("memmap_find_address_page.png"), tr("Follow in Memory Map"), "memmapdump $", "ActionFollowMemMap"), wIsDebugging);
    }
    if(actions & ActionBreakpoint)
    {
        QAction* toggleBreakpointAction = makeShortcutAction(DIcon("breakpoint_toggle.png"), tr("Toggle"), std::bind(&CommonActions::toggleInt3BPActionSlot, this), "ActionToggleBreakpoint");
        QAction* editSoftwareBreakpointAction = makeShortcutAction(DIcon("breakpoint_edit_alt.png"), tr("Edit"), std::bind(&CommonActions::editSoftBpActionSlot, this), "ActionEditBreakpoint");
        QAction* setHwBreakpointAction = makeShortcutAction(DIcon("breakpoint_execute.png"), tr("Set Hardware on Execution"), std::bind(&CommonActions::toggleHwBpActionSlot, this), "ActionSetHwBpE");
        QAction* removeHwBreakpointAction = makeShortcutAction(DIcon("breakpoint_remove.png"), tr("Remove Hardware"), std::bind(&CommonActions::toggleHwBpActionSlot, this), "ActionRemoveHwBp");

        QMenu* replaceSlotMenu = makeMenu(DIcon("breakpoint_execute.png"), tr("Set Hardware on Execution"));
        // Replacement slot menu are only used when the breakpoints are full, so using "Unknown" as the placeholder. Might want to change this in case we display the menu when there are still free slots.
        QAction* replaceSlot0Action = makeMenuAction(replaceSlotMenu, DIcon("breakpoint_execute_slot1.png"), tr("Replace Slot 0 (Unknown)"), std::bind(&CommonActions::setHwBpOnSlot0ActionSlot, this));
        QAction* replaceSlot1Action = makeMenuAction(replaceSlotMenu, DIcon("breakpoint_execute_slot2.png"), tr("Replace Slot 1 (Unknown)"), std::bind(&CommonActions::setHwBpOnSlot1ActionSlot, this));
        QAction* replaceSlot2Action = makeMenuAction(replaceSlotMenu, DIcon("breakpoint_execute_slot3.png"), tr("Replace Slot 2 (Unknown)"), std::bind(&CommonActions::setHwBpOnSlot2ActionSlot, this));
        QAction* replaceSlot3Action = makeMenuAction(replaceSlotMenu, DIcon("breakpoint_execute_slot4.png"), tr("Replace Slot 3 (Unknown)"), std::bind(&CommonActions::setHwBpOnSlot3ActionSlot, this));

        builder->addMenu(makeMenu(DIcon("breakpoint.png"), tr("Breakpoint")), [ = ](QMenu * menu)
        {
            auto selection = mGetSelection();
            if(selection == 0)
                return false;
            BPXTYPE bpType = DbgGetBpxTypeAt(selection);
            if((bpType & bp_normal) == bp_normal || (bpType & bp_hardware) == bp_hardware)
                editSoftwareBreakpointAction->setText(tr("Edit"));
            else
                editSoftwareBreakpointAction->setText(tr("Set Conditional Breakpoint"));
            menu->addAction(editSoftwareBreakpointAction);

            menu->addAction(toggleBreakpointAction);

            if((bpType & bp_hardware) == bp_hardware)
            {
                menu->addAction(removeHwBreakpointAction);
            }
            else
            {
                BPMAP bpList;
                DbgGetBpList(bp_hardware, &bpList);

                //get enabled hwbp count
                int enabledCount = bpList.count;
                for(int i = 0; i < bpList.count; i++)
                    if(!bpList.bp[i].enabled)
                        enabledCount--;

                if(enabledCount < 4)
                {
                    menu->addAction(setHwBreakpointAction);
                }
                else
                {
                    for(int i = 0; i < bpList.count; i++)
                    {
                        if(bpList.bp[i].enabled)
                        {
                            switch(bpList.bp[i].slot)
                            {
                            case 0:
                                replaceSlot0Action->setText(tr("Replace Slot %1 (0x%2)").arg(1).arg(ToPtrString(bpList.bp[i].addr)));
                                break;
                            case 1:
                                replaceSlot1Action->setText(tr("Replace Slot %1 (0x%2)").arg(2).arg(ToPtrString(bpList.bp[i].addr)));
                                break;
                            case 2:
                                replaceSlot2Action->setText(tr("Replace Slot %1 (0x%2)").arg(3).arg(ToPtrString(bpList.bp[i].addr)));
                                break;
                            case 3:
                                replaceSlot3Action->setText(tr("Replace Slot %1 (0x%2)").arg(4).arg(ToPtrString(bpList.bp[i].addr)));
                                break;
                            default:
                                break;
                            }
                        }
                    }
                    menu->addMenu(replaceSlotMenu);
                }
                if(bpList.count)
                    BridgeFree(bpList.bp);
            }
            return true;
        });
    }
    if(actions & ActionLabel)
    {
        builder->addAction(makeShortcutAction(DIcon("label.png"), tr("Label Current Address"), std::bind(&CommonActions::setLabelSlot, this), "ActionSetLabel"), wIsDebugging);
    }
    if(actions & ActionComment)
    {
        builder->addAction(makeShortcutAction(DIcon("comment.png"), tr("Comment"), std::bind(&CommonActions::setCommentSlot, this), "ActionSetComment"), wIsDebugging);
    }
    if(actions & ActionBookmark)
    {
        builder->addAction(makeShortcutAction(DIcon("bookmark_toggle.png"), tr("Toggle Bookmark"), std::bind(&CommonActions::setBookmarkSlot, this), "ActionToggleBookmark"), wIsDebugging);
    }
    if(actions & ActionNewOrigin)
    {
        builder->addAction(makeShortcutAction(DIcon("neworigin.png"), tr("Set New Origin Here"), std::bind(&CommonActions::setNewOriginHereActionSlot, this), "ActionSetNewOriginHere"));
    }
    if(actions & ActionNewThread)
    {
        builder->addAction(makeShortcutAction(DIcon("createthread.png"), tr("Create New Thread Here"), std::bind(&CommonActions::createThreadSlot, this), "ActionCreateNewThreadHere"));
    }
    if(actions & ActionWatch)
    {
        builder->addAction(makeCommandAction(DIcon("animal-dog.png"), ArchValue(tr("&Watch DWORD"), tr("&Watch QWORD")), "AddWatch \"[$]\", \"uint\"", "ActionWatchDwordQword"));
    }
}

QAction* CommonActions::makeCommandAction(const QIcon & icon, const QString & text, const char* cmd, const char* shortcut)
{
    return makeShortcutAction(icon, text, [cmd, this]()
    {
        DbgCmdExec(QString(cmd).replace("$", ToPtrString(mGetSelection())));
    }, shortcut);
}

QAction* CommonActions::makeCommandAction(const QIcon & icon, const QString & text, const char* cmd)
{
    return makeAction(icon, text, [cmd, this]()
    {
        DbgCmdExec(QString(cmd).replace("$", ToPtrString(mGetSelection())));
    });
}

QWidget* CommonActions::widgetparent()
{
    return dynamic_cast<QWidget*>(parent());
}

// Actions slots
void CommonActions::followDisassemblySlot()
{
    duint cip = mGetSelection();
    if(DbgMemIsValidReadPtr(cip))
        DbgCmdExec(QString("dis ").append(ToPtrString(cip)));
    else
        GuiAddStatusBarMessage(tr("Cannot follow %1. Address is invalid.\n").arg(ToPtrString(cip)).toUtf8().constData());
}

void CommonActions::setLabelSlot()
{
    duint wVA = mGetSelection();
    LineEditDialog mLineEdit(widgetparent());
    mLineEdit.setTextMaxLength(MAX_LABEL_SIZE - 2);
    QString addr_text = ToPtrString(wVA);
    char label_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetLabelAt((duint)wVA, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle(tr("Add label at ") + addr_text);
restart:
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    QByteArray utf8data = mLineEdit.editText.toUtf8();
    if(!utf8data.isEmpty() && DbgIsValidExpression(utf8data.constData()) && DbgValFromString(utf8data.constData()) != wVA)
    {
        QMessageBox msg(QMessageBox::Warning, tr("The label may be in use"),
                        tr("The label \"%1\" may be an existing label or a valid expression. Using such label might have undesired effects. Do you still want to continue?").arg(mLineEdit.editText),
                        QMessageBox::Yes | QMessageBox::No, widgetparent());
        msg.setWindowIcon(DIcon("compile-warning.png"));
        msg.setParent(widgetparent(), Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::No)
            goto restart;
    }
    if(!DbgSetLabelAt(wVA, utf8data.constData()))
        SimpleErrorBox(widgetparent(), tr("Error!"), tr("DbgSetLabelAt failed!"));

    GuiUpdateAllViews();
}

void CommonActions::setCommentSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = mGetSelection();
    LineEditDialog mLineEdit(widgetparent());
    mLineEdit.setTextMaxLength(MAX_COMMENT_SIZE - 2);
    QString addr_text = ToPtrString(wVA);
    char comment_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetCommentAt((duint)wVA, comment_text))
    {
        if(comment_text[0] == '\1') //automatic comment
            mLineEdit.setText(QString(comment_text + 1));
        else
            mLineEdit.setText(QString(comment_text));
    }
    mLineEdit.setWindowTitle(tr("Add comment at ") + addr_text);
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    QString comment = mLineEdit.editText.replace('\r', "").replace('\n', "");
    if(!DbgSetCommentAt(wVA, comment.toUtf8().constData()))
        SimpleErrorBox(widgetparent(), tr("Error!"), tr("DbgSetCommentAt failed!"));

    static bool easter = isEaster();
    if(easter && comment.toLower() == "oep")
    {
        QFile file(":/icons/images/egg.wav");
        if(file.open(QIODevice::ReadOnly))
        {
            QByteArray egg = file.readAll();
            PlaySoundA(egg.data(), 0, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
        }
    }

    GuiUpdateAllViews();
}

void CommonActions::setBookmarkSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = mGetSelection();
    bool result;
    if(DbgGetBookmarkAt(wVA))
        result = DbgSetBookmarkAt(wVA, false);
    else
        result = DbgSetBookmarkAt(wVA, true);
    if(!result)
        SimpleErrorBox(widgetparent(), tr("Error!"), tr("DbgSetBookmarkAt failed!"));
    GuiUpdateAllViews();
}

bool CommonActions::WarningBoxNotExecutable(const QString & text, duint wVA)
{
    if(DbgFunctions()->IsDepEnabled() && !DbgFunctions()->MemIsCodePage(wVA, false))
    {
        QMessageBox msgyn(QMessageBox::Warning, tr("Current address is not executable"), text, QMessageBox::Yes | QMessageBox::No, widgetparent());
        msgyn.setWindowIcon(DIcon("compile-warning.png"));
        msgyn.setParent(widgetparent(), Qt::Dialog);
        msgyn.setWindowFlags(msgyn.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msgyn.exec() == QMessageBox::No)
            return false;
    }
    return true;
}

void CommonActions::toggleInt3BPActionSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = mGetSelection();
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_normal) == bp_normal)
        wCmd = "bc " + ToPtrString(wVA);
    else
    {
        if(!WarningBoxNotExecutable(tr("Setting software breakpoint here may result in crash. Do you really want to continue?"), wVA))
            return;
        wCmd = "bp " + ToPtrString(wVA);
    }

    DbgCmdExec(wCmd);
    //emit Disassembly::repainted();
}

void CommonActions::editSoftBpActionSlot()
{
    auto selection = mGetSelection();
    if(selection == 0)
        return;
    BPXTYPE bpType = DbgGetBpxTypeAt(selection);
    if((bpType & bp_hardware) == bp_hardware)
        Breakpoints::editBP(bp_hardware, ToHexString(selection), widgetparent());
    else if((bpType & bp_normal) == bp_normal)
        Breakpoints::editBP(bp_normal, ToHexString(selection), widgetparent());
    else
    {
        DbgCmdExecDirect(QString("bp %1").arg(ToHexString(selection))); //Blocking call
        if(!Breakpoints::editBP(bp_normal, ToHexString(selection), widgetparent()))
            Breakpoints::removeBP(bp_normal, selection);
    }
}

void CommonActions::toggleHwBpActionSlot()
{
    duint wVA = mGetSelection();
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_hardware) == bp_hardware)
    {
        wCmd = "bphwc " + ToPtrString(wVA);
    }
    else
    {
        wCmd = "bphws " + ToPtrString(wVA);
    }

    DbgCmdExec(wCmd);
}


void CommonActions::setHwBpOnSlot0ActionSlot()
{
    setHwBpAt(mGetSelection(), 0);
}

void CommonActions::setHwBpOnSlot1ActionSlot()
{
    setHwBpAt(mGetSelection(), 1);
}

void CommonActions::setHwBpOnSlot2ActionSlot()
{
    setHwBpAt(mGetSelection(), 2);
}

void CommonActions::setHwBpOnSlot3ActionSlot()
{
    setHwBpAt(mGetSelection(), 3);
}

void CommonActions::setHwBpAt(duint va, int slot)
{
    int wI = 0;
    int wSlotIndex = -1;
    BPMAP wBPList;
    QString wCmd = "";

    DbgGetBpList(bp_hardware, &wBPList);

    // Find index of slot slot in the list
    for(wI = 0; wI < wBPList.count; wI++)
    {
        if(wBPList.bp[wI].slot == (unsigned short)slot)
        {
            wSlotIndex = wI;
            break;
        }
    }

    if(wSlotIndex < 0) // Slot not used
    {
        wCmd = "bphws " + ToPtrString(va);
        DbgCmdExec(wCmd);
    }
    else // Slot used
    {
        wCmd = "bphwc " + ToPtrString((duint)(wBPList.bp[wSlotIndex].addr));
        DbgCmdExec(wCmd);

        Sleep(200);

        wCmd = "bphws " + ToPtrString(va);
        DbgCmdExec(wCmd);
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);
}

void CommonActions::setNewOriginHereActionSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = mGetSelection();
    if(!WarningBoxNotExecutable(tr("Setting new origin here may result in crash. Do you really want to continue?"), wVA))
        return;
    QString wCmd = "cip=" + ToPtrString(wVA);
    DbgCmdExec(wCmd);
}

void CommonActions::createThreadSlot()
{
    duint wVA = mGetSelection();
    if(!WarningBoxNotExecutable(tr("Creating new thread here may result in crash. Do you really want to continue?"), wVA))
        return;
    WordEditDialog argWindow(widgetparent());
    argWindow.setup(tr("Argument for the new thread"), 0, sizeof(duint));
    if(argWindow.exec() != QDialog::Accepted)
        return;
    DbgCmdExec(QString("createthread %1, %2").arg(ToPtrString(wVA)).arg(ToPtrString(argWindow.getVal())));
}
