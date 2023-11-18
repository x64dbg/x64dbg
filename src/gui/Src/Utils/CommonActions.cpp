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
    auto isDebugging = [this](QMenu*)
    {
        return mGetSelection() != 0 && DbgIsDebugging();
    };
    auto isValidReadPtr = [this](QMenu*)
    {
        duint ptr = 0;
        DbgMemRead(mGetSelection(), (unsigned char*)&ptr, sizeof(duint));
        return DbgMemIsValidReadPtr(ptr);
    };

    // Menu action
    if(actions & ActionDisasm)
    {
        builder->addAction(makeShortcutDescAction(DIcon(ArchValue("processor32", "processor64")), tr("Follow in Disassembler"), tr("Show this address in disassembler. Equivalent command \"d address\"."), std::bind(&CommonActions::followDisassemblySlot, this), "ActionFollowDisasm"), isDebugging);
    }
    if(actions & ActionDisasmData)
    {
        builder->addAction(makeCommandAction(DIcon("processor32"), ArchValue(tr("&Follow DWORD in Disassembler"), tr("&Follow QWORD in Disassembler")), "disasm [$]", "ActionFollowDwordQwordDisasm"), isValidReadPtr);
    }
    if(actions & ActionDump)
    {
        builder->addAction(makeCommandDescAction(DIcon("dump"), tr("Follow in Dump"), tr("Show the address in dump. Equivalent command \"dump address\"."), "dump $"));
    }
    if(actions & ActionDumpData)
    {
        builder->addAction(makeCommandAction(DIcon("dump"), ArchValue(tr("&Follow DWORD in Current Dump"), tr("&Follow QWORD in Current Dump")), "dump [$]", "ActionFollowDwordQwordDump"), isValidReadPtr);
    }
    if(actions & ActionDumpN)
    {
        //Follow in Dump N menu
        MenuBuilder* followDumpNMenu = new MenuBuilder(this, [this](QMenu*)
        {
            duint ptr;
            return DbgMemRead(mGetSelection(), (unsigned char*)&ptr, sizeof(ptr)) && DbgMemIsValidReadPtr(ptr);
        });
        const int maxDumps = 5; // TODO: get this value from CPUMultiDump
        for(int i = 0; i < maxDumps; i++)
            // TODO: Get dump tab names
            followDumpNMenu->addAction(makeAction(tr("Dump %1").arg(i + 1), [i, this]
        {
            duint selectedData = mGetSelection();
            if(DbgMemIsValidReadPtr(selectedData))
                DbgCmdExec(QString("dump [%1], %2").arg(ToPtrString(selectedData)).arg(i + 1));
        }));
        builder->addMenu(makeMenu(DIcon("dump"), ArchValue(tr("Follow DWORD in Dump"), tr("Follow QWORD in Dump"))), followDumpNMenu);
    }
    if(actions & ActionStackDump)
    {
        builder->addAction(makeCommandDescAction(DIcon("stack"), tr("Follow in Stack"), tr("Show this address in stack view. Equivalent command \"sdump address\"."), "sdump $", "ActionFollowStack"), [this](QMenu*)
        {
            auto start = mGetSelection();
            return (DbgMemIsValidReadPtr(start) && DbgMemFindBaseAddr(start, 0) == DbgMemFindBaseAddr(DbgValFromString("csp"), 0));
        });
    }
    if(actions & ActionMemoryMap)
    {
        builder->addAction(makeCommandDescAction(DIcon("memmap_find_address_page"), tr("Follow in Memory Map"), tr("Show this address in memory map view. Equivalent command \"memmapdump address\"."), "memmapdump $", "ActionFollowMemMap"), isDebugging);
    }
    if(actions & ActionGraph)
    {
        builder->addAction(makeShortcutDescAction(DIcon("graph"), tr("Graph"), tr("Show the control flow graph of this function in CPU view. Equivalent command \"graph address\"."), std::bind(&CommonActions::graphSlot, this), "ActionGraph"));
    }
    if(actions & ActionBreakpoint)
    {
        struct ActionHolder
        {
            QAction* toggleBreakpointAction;
            QAction* editSoftwareBreakpointAction;
            QAction* setHwBreakpointAction;
            QAction* removeHwBreakpointAction;
            QMenu* replaceSlotMenu;
            QAction* replaceSlotAction[4];
        } hodl;

        hodl.toggleBreakpointAction = makeShortcutAction(DIcon("breakpoint_toggle"), tr("Toggle"), std::bind(&CommonActions::toggleInt3BPActionSlot, this), "ActionToggleBreakpoint");
        hodl.editSoftwareBreakpointAction = makeShortcutAction(DIcon("breakpoint_edit_alt"), tr("Edit"), std::bind(&CommonActions::editSoftBpActionSlot, this), "ActionEditBreakpoint");
        hodl.setHwBreakpointAction = makeShortcutAction(DIcon("breakpoint_execute"), tr("Set Hardware on Execution"), std::bind(&CommonActions::toggleHwBpActionSlot, this), "ActionSetHwBpE");
        hodl.removeHwBreakpointAction = makeShortcutAction(DIcon("breakpoint_remove"), tr("Remove Hardware"), std::bind(&CommonActions::toggleHwBpActionSlot, this), "ActionRemoveHwBp");

        hodl.replaceSlotMenu = makeMenu(DIcon("breakpoint_execute"), tr("Set Hardware on Execution"));
        // Replacement slot menu are only used when the breakpoints are full, so using "Unknown" as the placeholder. Might want to change this in case we display the menu when there are still free slots.
        hodl.replaceSlotAction[0] = makeMenuAction(hodl.replaceSlotMenu, DIcon("breakpoint_execute_slot1"), tr("Replace Slot %1 (Unknown)").arg(1), std::bind(&CommonActions::setHwBpOnSlot0ActionSlot, this));
        hodl.replaceSlotAction[1] = makeMenuAction(hodl.replaceSlotMenu, DIcon("breakpoint_execute_slot2"), tr("Replace Slot %1 (Unknown)").arg(2), std::bind(&CommonActions::setHwBpOnSlot1ActionSlot, this));
        hodl.replaceSlotAction[2] = makeMenuAction(hodl.replaceSlotMenu, DIcon("breakpoint_execute_slot3"), tr("Replace Slot %1 (Unknown)").arg(3), std::bind(&CommonActions::setHwBpOnSlot2ActionSlot, this));
        hodl.replaceSlotAction[3] = makeMenuAction(hodl.replaceSlotMenu, DIcon("breakpoint_execute_slot4"), tr("Replace Slot %1 (Unknown)").arg(4), std::bind(&CommonActions::setHwBpOnSlot3ActionSlot, this));

        builder->addMenu(makeMenu(DIcon("breakpoint"), tr("Breakpoint")), [this, hodl](QMenu * menu)
        {
            auto selection = mGetSelection();
            if(selection == 0)
                return false;
            BPXTYPE bpType = DbgGetBpxTypeAt(selection);
            if((bpType & bp_normal) == bp_normal || (bpType & bp_hardware) == bp_hardware)
                hodl.editSoftwareBreakpointAction->setText(tr("Edit"));
            else
                hodl.editSoftwareBreakpointAction->setText(tr("Set Conditional Breakpoint"));
            menu->addAction(hodl.editSoftwareBreakpointAction);

            menu->addAction(hodl.toggleBreakpointAction);

            if((bpType & bp_hardware) == bp_hardware)
            {
                menu->addAction(hodl.removeHwBreakpointAction);
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
                    menu->addAction(hodl.setHwBreakpointAction);
                }
                else
                {
                    for(int i = 0; i < bpList.count; i++)
                    {
                        if(bpList.bp[i].enabled && bpList.bp[i].slot < 4)
                        {
                            hodl.replaceSlotAction[bpList.bp[i].slot]->setText(tr("Replace Slot %1 (0x%2)").arg(bpList.bp[i].slot + 1).arg(ToPtrString(bpList.bp[i].addr)));
                        }
                    }
                    menu->addMenu(hodl.replaceSlotMenu);
                }
                if(bpList.count)
                    BridgeFree(bpList.bp);
            }
            return true;
        });
    }
    if(actions & ActionLabel)
    {
        builder->addAction(makeShortcutAction(DIcon("label"), tr("Label Current Address"), std::bind(&CommonActions::setLabelSlot, this), "ActionSetLabel"), isDebugging);
    }
    if(actions & ActionComment)
    {
        builder->addAction(makeShortcutAction(DIcon("comment"), tr("Comment"), std::bind(&CommonActions::setCommentSlot, this), "ActionSetComment"), isDebugging);
    }
    if(actions & ActionBookmark)
    {
        builder->addAction(makeShortcutDescAction(DIcon("bookmark_toggle"), tr("Toggle Bookmark"), tr("Set a bookmark here, or remove bookmark. Equivalent command \"bookmarkset address\"/\"bookmarkdel address\"."), std::bind(&CommonActions::setBookmarkSlot, this), "ActionToggleBookmark"), isDebugging);
    }
    if(actions & ActionNewOrigin)
    {
        builder->addAction(makeShortcutDescAction(DIcon("neworigin"), tr("Set %1 Here").arg(ArchValue("EIP", "RIP")), tr("Set the next executed instruction to this address. Equivalent command \"mov cip, address\"."), std::bind(&CommonActions::setNewOriginHereActionSlot, this), "ActionSetNewOriginHere"));
    }
    if(actions & ActionNewThread)
    {
        builder->addAction(makeShortcutDescAction(DIcon("createthread"), tr("Create New Thread Here"), tr("Create a new thread at this address. Equivalent command \"createthread address, argument\"."), std::bind(&CommonActions::createThreadSlot, this), "ActionCreateNewThreadHere"));
    }
    if(actions & ActionWatch)
    {
        builder->addAction(makeCommandDescAction(DIcon("animal-dog"), ArchValue(tr("&Watch DWORD"), tr("&Watch QWORD")), tr("Add the address in the watch view. Equivalent command \"AddWatch [address], \"uint\"\"."), "AddWatch \"[$]\", \"uint\"", "ActionWatchDwordQword"));
    }
}

QAction* CommonActions::makeCommandAction(const QIcon & icon, const QString & text, const char* cmd, const char* shortcut)
{
    // sender() doesn't work in slots
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

static QAction* makeDescActionHelper(QAction* action, const QString & description)
{
    action->setStatusTip(description);
    return action;
}

QAction* CommonActions::makeCommandDescAction(const QIcon & icon, const QString & text, const QString & description, const char* cmd)
{
    return makeDescActionHelper(makeCommandAction(icon, text, cmd), description);
}

QAction* CommonActions::makeCommandDescAction(const QIcon & icon, const QString & text, const QString & description, const char* cmd, const char* shortcut)
{
    return makeDescActionHelper(makeCommandAction(icon, text, cmd, shortcut), description);
}

QWidget* CommonActions::widgetparent() const
{
    return dynamic_cast<QWidget*>(parent());
}

// Actions slots
// Follow in disassembly
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
    duint va = mGetSelection();
    LineEditDialog mLineEdit(widgetparent());
    mLineEdit.setTextMaxLength(MAX_LABEL_SIZE - 2);
    QString addrText = ToPtrString(va);
    char label_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetLabelAt((duint)va, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle(tr("Add label at ") + addrText);
restart:
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    QByteArray utf8data = mLineEdit.editText.toUtf8();
    if(!utf8data.isEmpty() && DbgIsValidExpression(utf8data.constData()) && DbgValFromString(utf8data.constData()) != va)
    {
        QMessageBox msg(QMessageBox::Warning, tr("The label may be in use"),
                        tr("The label \"%1\" may be an existing label or a valid expression. Using such label might have undesired effects. Do you still want to continue?").arg(mLineEdit.editText),
                        QMessageBox::Yes | QMessageBox::No, widgetparent());
        msg.setWindowIcon(DIcon("compile-warning"));
        msg.setParent(widgetparent(), Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::No)
            goto restart;
    }
    if(!DbgSetLabelAt(va, utf8data.constData()))
        SimpleErrorBox(widgetparent(), tr("Error!"), tr("DbgSetLabelAt failed!"));

    GuiUpdateAllViews();
}

void CommonActions::setCommentSlot()
{
    if(!DbgIsDebugging())
        return;
    duint va = mGetSelection();
    LineEditDialog mLineEdit(widgetparent());
    mLineEdit.setTextMaxLength(MAX_COMMENT_SIZE - 2);
    QString addrText = ToPtrString(va);
    char comment_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetCommentAt((duint)va, comment_text))
    {
        if(comment_text[0] == '\1') //automatic comment
            mLineEdit.setText(QString(comment_text + 1));
        else
            mLineEdit.setText(QString(comment_text));
    }
    mLineEdit.setWindowTitle(tr("Add comment at ") + addrText);
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    QString comment = mLineEdit.editText.replace('\r', "").replace('\n', "");
    if(!DbgSetCommentAt(va, comment.toUtf8().constData()))
        SimpleErrorBox(widgetparent(), tr("Error!"), tr("DbgSetCommentAt failed!"));

    static bool easter = isEaster();
    if(easter && comment.toLower() == "oep")
    {
        QFile file(":/Default/icons/egg.wav");
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
    duint va = mGetSelection();
    bool result;
    result = DbgSetBookmarkAt(va, !DbgGetBookmarkAt(va));
    if(!result)
        SimpleErrorBox(widgetparent(), tr("Error!"), tr("DbgSetBookmarkAt failed!"));
    GuiUpdateAllViews();
}

// Give a warning about the selected address is not executable
bool CommonActions::WarningBoxNotExecutable(const QString & text, duint va) const
{
    if(DbgFunctions()->IsDepEnabled() && !DbgFunctions()->MemIsCodePage(va, false))
    {
        QMessageBox msgyn(QMessageBox::Warning, tr("Address %1 is not executable").arg(ToPtrString(va)), text, QMessageBox::Yes | QMessageBox::No, widgetparent());
        msgyn.setWindowIcon(DIcon("compile-warning"));
        msgyn.setParent(widgetparent(), Qt::Dialog);
        msgyn.setDefaultButton(QMessageBox::No);
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
    duint va = mGetSelection();
    BPXTYPE bpType = DbgGetBpxTypeAt(va);
    QString cmd;

    if((bpType & bp_normal) == bp_normal)
        cmd = "bc " + ToPtrString(va);
    else
    {
        if(!WarningBoxNotExecutable(tr("Setting software breakpoint here may result in crash. Do you really want to continue?"), va))
            return;
        cmd = "bp " + ToPtrString(va);
    }

    DbgCmdExec(cmd);
    //emit Disassembly::repainted();
}

// Display the edit breakpoint dialog
void CommonActions::editSoftBpActionSlot()
{
    auto selection = mGetSelection();
    if(selection == 0)
    {
        return;
    }

    BPXTYPE bpType = DbgGetBpxTypeAt(selection);
    if((bpType & bp_hardware) == bp_hardware)
    {
        Breakpoints::editBP(bp_hardware, ToHexString(selection), widgetparent());
    }
    else if((bpType & bp_normal) == bp_normal)
    {
        Breakpoints::editBP(bp_normal, ToHexString(selection), widgetparent());
    }
    else
    {
        auto createCommand = QString("bp %1").arg(ToHexString(selection));
        Breakpoints::editBP(bp_normal, ToHexString(selection), widgetparent(), createCommand);
    }
}

void CommonActions::toggleHwBpActionSlot()
{
    duint va = mGetSelection();
    BPXTYPE bpType = DbgGetBpxTypeAt(va);
    QString cmd;

    if((bpType & bp_hardware) == bp_hardware)
    {
        cmd = "bphwc " + ToPtrString(va);
    }
    else
    {
        cmd = "bphws " + ToPtrString(va);
    }

    DbgCmdExec(cmd);
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
    int slotIndex = -1;
    BPMAP bpList = {};
    DbgGetBpList(bp_hardware, &bpList);

    // Find index of slot slot in the list
    for(int i = 0; i < bpList.count; i++)
    {
        if(bpList.bp[i].slot == (unsigned short)slot)
        {
            slotIndex = i;
            break;
        }
    }

    QString cmd;
    if(slotIndex < 0) // Slot not used
    {
        cmd = "bphws " + ToPtrString(va);
        DbgCmdExec(cmd);
    }
    else // Slot used
    {
        cmd = "bphwc " + ToPtrString((duint)(bpList.bp[slotIndex].addr));
        DbgCmdExec(cmd);

        Sleep(200);

        cmd = "bphws " + ToPtrString(va);
        DbgCmdExec(cmd);
    }
    if(bpList.count)
        BridgeFree(bpList.bp);
}

void CommonActions::graphSlot()
{
    if(DbgCmdExecDirect(QString("graph %1").arg(ToPtrString(mGetSelection()))))
        GuiFocusView(GUI_GRAPH);
}

void CommonActions::setNewOriginHereActionSlot()
{
    if(!DbgIsDebugging())
        return;
    duint va = mGetSelection();
    if(!WarningBoxNotExecutable(tr("Setting new origin here may result in crash. Do you really want to continue?"), va))
        return;
    QString cmd = "cip=" + ToPtrString(va);
    DbgCmdExec(cmd);
}

void CommonActions::createThreadSlot()
{
    duint va = mGetSelection();
    if(!WarningBoxNotExecutable(tr("Creating new thread here may result in crash. Do you really want to continue?"), va))
        return;
    WordEditDialog argWindow(widgetparent());
    argWindow.setup(tr("Argument for the new thread"), 0, sizeof(duint));
    if(argWindow.exec() != QDialog::Accepted)
        return;
    DbgCmdExec(QString("createthread %1, %2").arg(ToPtrString(va)).arg(ToPtrString(argWindow.getVal())));
}
