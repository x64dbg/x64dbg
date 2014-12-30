#include "CPUDisassembly.h"
#include <QMessageBox>
#include <QClipboard>
#include "Configuration.h"
#include "Bridge.h"
#include "LineEditDialog.h"
#include "WordEditDialog.h"
#include "HexEditDialog.h"

CPUDisassembly::CPUDisassembly(QWidget* parent) : Disassembly(parent)
{
    // Create the action list for the right click context menu
    setupRightClickContextMenu();

    connect(Bridge::getBridge(), SIGNAL(disassembleAt(int_t, int_t)), this, SLOT(disassembleAt(int_t, int_t)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(selectionDisasmGet(SELECTIONDATA*)), this, SLOT(selectionGet(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionDisasmSet(const SELECTIONDATA*)), this, SLOT(selectionSet(const SELECTIONDATA*)));

    mGoto = 0;
}

void CPUDisassembly::mousePressEvent(QMouseEvent* event)
{
    if(event->buttons() == Qt::MiddleButton) //copy address to clipboard
    {
        if(!DbgIsDebugging())
            return;
        MessageBeep(MB_OK);
        copyAddress();
    }
    else
    {
        Disassembly::mousePressEvent(event);
        if(mHighlightingMode) //disable highlighting mode after clicked
        {
            mHighlightingMode = false;
            reloadData();
        }
    }
}

void CPUDisassembly::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton)
        return;
    switch(getColumnIndexFromX(event->x()))
    {
    case 0: //address
    {
        int_t mSelectedVa = rvaToVa(getInitialSelection());
        if(mRvaDisplayEnabled && mSelectedVa == mRvaDisplayBase)
            mRvaDisplayEnabled = false;
        else
        {
            mRvaDisplayEnabled = true;
            mRvaDisplayBase = mSelectedVa;
            mRvaDisplayPageBase = getBase();
        }
        reloadData();
    }
    break;

    case 1: //opcodes
    {
        toggleInt3BPAction(); //toggle INT3 breakpoint
    }
    break;

    case 2: //disassembly
    {
        assembleAt();
    }
    break;

    case 3: //comments
    {
        setComment();
    }
    break;

    default:
        Disassembly::mouseDoubleClickEvent(event);
        break;
    }
}

void CPUDisassembly::addFollowMenuItem(QString name, int_t value)
{
    foreach(QAction * action, mFollowMenu->actions()) //check for duplicate action
    if(action->text() == name)
        return;
    QAction* newAction = new QAction(name, this);
    newAction->setFont(QFont("Courier New", 8));
    mFollowMenu->addAction(newAction);
    newAction->setObjectName(QString("DUMP|") + QString("%1").arg(value, sizeof(int_t) * 2, 16, QChar('0')).toUpper());
    connect(newAction, SIGNAL(triggered()), this, SLOT(followActionSlot()));
}

void CPUDisassembly::setupFollowMenu(int_t wVA)
{
    //remove previous actions
    QList<QAction*> list = mFollowMenu->actions();
    for(int i = 0; i < list.length(); i++)
        mFollowMenu->removeAction(list.at(i));

    //most basic follow action
    addFollowMenuItem("&Selection", wVA);

    //add follow actions
    DISASM_INSTR instr;
    DbgDisasmAt(wVA, &instr);

    for(int i = 0; i < instr.argcount; i++)
    {
        const DISASM_ARG arg = instr.arg[i];
        if(arg.type == arg_memory)
        {
            if(DbgMemIsValidReadPtr(arg.value))
                addFollowMenuItem("&Address: " + QString(arg.mnemonic).toUpper().trimmed(), arg.value);
            if(arg.value != arg.constant)
            {
                QString constant = QString("%1").arg(arg.constant, 1, 16, QChar('0')).toUpper();
                if(DbgMemIsValidReadPtr(arg.constant))
                    addFollowMenuItem("&Constant: " + constant, arg.constant);
            }
            if(DbgMemIsValidReadPtr(arg.memvalue))
                addFollowMenuItem("&Value: [" + QString(arg.mnemonic) + "]", arg.memvalue);
        }
        else
        {
            if(DbgMemIsValidReadPtr(arg.value))
                addFollowMenuItem(QString(arg.mnemonic).toUpper().trimmed(), arg.value);
        }
    }
}


/************************************************************************************
                            Mouse Management
************************************************************************************/
/**
 * @brief       This method has been reimplemented. It manages the richt click context menu.
 *
 * @param[in]   event       Context menu event
 *
 * @return      Nothing.
 */
void CPUDisassembly::contextMenuEvent(QContextMenuEvent* event)
{
    if(getSize() != 0)
    {
        int wI;
        QMenu* wMenu = new QMenu(this);
        uint_t wVA = rvaToVa(getInitialSelection());
        BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);

        // Build Menu
        wMenu->addMenu(mBinaryMenu);
        wMenu->addMenu(mCopyMenu);
        int_t start = rvaToVa(getSelectionStart());
        int_t end = rvaToVa(getSelectionEnd());
        if(DbgFunctions()->PatchInRange(start, end)) //nothing patched in selected range
            wMenu->addAction(mUndoSelection);

        // BP Menu
        mBPMenu->clear();
        // Soft BP
        mBPMenu->addAction(mToggleInt3BpAction);
        // Hardware BP
        if((wBpType & bp_hardware) == bp_hardware)
        {
            mBPMenu->addAction(mClearHwBpAction);
        }
        else
        {
            BPMAP wBPList;
            DbgGetBpList(bp_hardware, &wBPList);

            //get enabled hwbp count
            int enabledCount = wBPList.count;
            for(int i = 0; i < wBPList.count; i++)
                if(!wBPList.bp[i].enabled)
                    enabledCount--;

            if(enabledCount < 4)
            {
                mBPMenu->addAction(mSetHwBpAction);
            }
            else
            {
                REGDUMP wRegDump;
                DbgGetRegDump(&wRegDump);

                for(wI = 0; wI < 4; wI++)
                {
                    switch(wBPList.bp[wI].slot)
                    {
                    case 0:
                        msetHwBPOnSlot0Action->setText("Replace Slot 0 (0x" + QString("%1").arg(wBPList.bp[wI].addr, 8, 16, QChar('0')).toUpper() + ")");
                        break;
                    case 1:
                        msetHwBPOnSlot1Action->setText("Replace Slot 1 (0x" + QString("%1").arg(wBPList.bp[wI].addr, 8, 16, QChar('0')).toUpper() + ")");
                        break;
                    case 2:
                        msetHwBPOnSlot2Action->setText("Replace Slot 2 (0x" + QString("%1").arg(wBPList.bp[wI].addr, 8, 16, QChar('0')).toUpper() + ")");
                        break;
                    case 3:
                        msetHwBPOnSlot3Action->setText("Replace Slot 3 (0x" + QString("%1").arg(wBPList.bp[wI].addr, 8, 16, QChar('0')).toUpper() + ")");
                        break;
                    default:
                        break;
                    }
                }

                mHwSlotSelectMenu->addAction(msetHwBPOnSlot0Action);
                mHwSlotSelectMenu->addAction(msetHwBPOnSlot1Action);
                mHwSlotSelectMenu->addAction(msetHwBPOnSlot2Action);
                mHwSlotSelectMenu->addAction(msetHwBPOnSlot3Action);
                mBPMenu->addMenu(mHwSlotSelectMenu);
            }
            if(wBPList.count)
                BridgeFree(wBPList.bp);
        }
        wMenu->addMenu(mBPMenu);
        wMenu->addMenu(mFollowMenu);
        setupFollowMenu(wVA);
        wMenu->addAction(mEnableHighlightingMode);
        wMenu->addSeparator();


        wMenu->addAction(mSetLabel);
        wMenu->addAction(mSetComment);
        wMenu->addAction(mSetBookmark);

        uint_t selection_start = rvaToVa(getSelectionStart());
        uint_t selection_end = rvaToVa(getSelectionEnd());
        if(!DbgFunctionOverlaps(selection_start, selection_end))
        {
            mToggleFunction->setText("Add function");
            wMenu->addAction(mToggleFunction);
        }
        else if(DbgFunctionOverlaps(selection_start, selection_end))
        {
            mToggleFunction->setText("Delete function");
            wMenu->addAction(mToggleFunction);
        }

        wMenu->addAction(mAssemble);

        wMenu->addAction(mPatchesAction);

        wMenu->addSeparator();

        // New origin
        wMenu->addAction(mSetNewOriginHere);

        // Goto Menu
        mGotoMenu->addAction(mGotoOrigin);
        if(historyHasPrevious())
            mGotoMenu->addAction(mGotoPrevious);
        if(historyHasNext())
            mGotoMenu->addAction(mGotoNext);
        mGotoMenu->addAction(mGotoExpression);
        char modname[MAX_MODULE_SIZE] = "";
        if(DbgGetModuleAt(wVA, modname))
            mGotoMenu->addAction(mGotoFileOffset);
        wMenu->addMenu(mGotoMenu);
        wMenu->addSeparator();

        wMenu->addMenu(mSearchMenu);

        mReferencesMenu->addAction(mReferenceSelectedAddress);
        wMenu->addMenu(mReferencesMenu);

        wMenu->exec(event->globalPos());
    }
}


/************************************************************************************
                         Context Menu Management
************************************************************************************/
void CPUDisassembly::setupRightClickContextMenu()
{
    //Binary
    mBinaryMenu = new QMenu("&Binary", this);

    //Binary->Edit
    mBinaryEditAction = new QAction("&Edit", this);
    mBinaryEditAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryEditAction);
    mBinaryMenu->addAction(mBinaryEditAction);
    connect(mBinaryEditAction, SIGNAL(triggered()), this, SLOT(binaryEditSlot()));

    //Binary->Fill
    mBinaryFillAction = new QAction("&Fill...", this);
    mBinaryFillAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryFillAction);
    connect(mBinaryFillAction, SIGNAL(triggered()), this, SLOT(binaryFillSlot()));
    mBinaryMenu->addAction(mBinaryFillAction);

    //Binary->Fill with NOPs
    mBinaryFillNopsAction = new QAction("Fill with &NOPs", this);
    mBinaryFillNopsAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryFillNopsAction);
    connect(mBinaryFillNopsAction, SIGNAL(triggered()), this, SLOT(binaryFillNopsSlot()));
    mBinaryMenu->addAction(mBinaryFillNopsAction);

    //Binary->Separator
    mBinaryMenu->addSeparator();

    //Binary->Copy
    mBinaryCopyAction = new QAction("&Copy", this);
    mBinaryCopyAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryCopyAction);
    connect(mBinaryCopyAction, SIGNAL(triggered()), this, SLOT(binaryCopySlot()));
    mBinaryMenu->addAction(mBinaryCopyAction);

    //Binary->Paste
    mBinaryPasteAction = new QAction("&Paste", this);
    mBinaryPasteAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryPasteAction);
    connect(mBinaryPasteAction, SIGNAL(triggered()), this, SLOT(binaryPasteSlot()));
    mBinaryMenu->addAction(mBinaryPasteAction);

    //Binary->Paste (Ignore Size)
    mBinaryPasteIgnoreSizeAction = new QAction("Paste (&Ignore Size)", this);
    mBinaryPasteIgnoreSizeAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryPasteIgnoreSizeAction);
    connect(mBinaryPasteIgnoreSizeAction, SIGNAL(triggered()), this, SLOT(binaryPasteIgnoreSizeSlot()));
    mBinaryMenu->addAction(mBinaryPasteIgnoreSizeAction);

    // Restore Selection
    mUndoSelection = new QAction("&Restore selection", this);
    mUndoSelection->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mUndoSelection);
    connect(mUndoSelection, SIGNAL(triggered()), this, SLOT(undoSelectionSlot()));

    // Labels
    mSetLabel = new QAction("Label", this);
    mSetLabel->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mSetLabel);
    connect(mSetLabel, SIGNAL(triggered()), this, SLOT(setLabel()));

    // Comments
    mSetComment = new QAction("Comment", this);
    mSetComment->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mSetComment);
    connect(mSetComment, SIGNAL(triggered()), this, SLOT(setComment()));

    // Bookmarks
    mSetBookmark = new QAction("Bookmark", this);
    mSetBookmark->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mSetBookmark);
    connect(mSetBookmark, SIGNAL(triggered()), this, SLOT(setBookmark()));

    // Functions
    mToggleFunction = new QAction("Function", this);
    mToggleFunction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mToggleFunction);
    connect(mToggleFunction, SIGNAL(triggered()), this, SLOT(toggleFunction()));

    // Assemble
    mAssemble = new QAction("Assemble", this);
    mAssemble->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mAssemble);
    connect(mAssemble, SIGNAL(triggered()), this, SLOT(assembleAt()));

    //---------------------- Breakpoints -----------------------------
    // Menu
    mBPMenu = new QMenu("Breakpoint", this);

    // Standard breakpoint (option set using SetBPXOption)
    mToggleInt3BpAction = new QAction("Toggle", this);
    mToggleInt3BpAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mToggleInt3BpAction);
    connect(mToggleInt3BpAction, SIGNAL(triggered()), this, SLOT(toggleInt3BPAction()));

    // HW BP
    mHwSlotSelectMenu = new QMenu("Set Hardware on Execution", this);

    mSetHwBpAction = new QAction("Set Hardware on Execution", this);
    connect(mSetHwBpAction, SIGNAL(triggered()), this, SLOT(toggleHwBpActionSlot()));

    mClearHwBpAction = new QAction("Remove Hardware", this);
    connect(mClearHwBpAction, SIGNAL(triggered()), this, SLOT(toggleHwBpActionSlot()));

    msetHwBPOnSlot0Action = new QAction("Set Hardware on Execution on Slot 0 (Free)", this);
    connect(msetHwBPOnSlot0Action, SIGNAL(triggered()), this, SLOT(setHwBpOnSlot0ActionSlot()));

    msetHwBPOnSlot1Action = new QAction("Set Hardware on Execution on Slot 1 (Free)", this);
    connect(msetHwBPOnSlot1Action, SIGNAL(triggered()), this, SLOT(setHwBpOnSlot1ActionSlot()));

    msetHwBPOnSlot2Action = new QAction("Set Hardware on Execution on Slot 2 (Free)", this);
    connect(msetHwBPOnSlot2Action, SIGNAL(triggered()), this, SLOT(setHwBpOnSlot2ActionSlot()));

    msetHwBPOnSlot3Action = new QAction("Set Hardware on Execution on Slot 3 (Free)", this);
    connect(msetHwBPOnSlot3Action, SIGNAL(triggered()), this, SLOT(setHwBpOnSlot3ActionSlot()));

    mPatchesAction = new QAction(QIcon(":/icons/images/patch.png"), "Patches", this);
    mPatchesAction->setShortcutContext(Qt::WidgetShortcut);
    connect(mPatchesAction, SIGNAL(triggered()), this, SLOT(showPatchesSlot()));

    //--------------------------------------------------------------------

    //---------------------- New origin here -----------------------------
    mSetNewOriginHere = new QAction("Set New Origin Here", this);
    mSetNewOriginHere->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mSetNewOriginHere);
    connect(mSetNewOriginHere, SIGNAL(triggered()), this, SLOT(setNewOriginHereActionSlot()));


    //---------------------- Go to -----------------------------------
    // Menu
    mGotoMenu = new QMenu("Go to", this);

    // Origin action
    mGotoOrigin = new QAction("Origin", this);
    mGotoOrigin->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoOrigin);
    connect(mGotoOrigin, SIGNAL(triggered()), this, SLOT(gotoOrigin()));

    // Previous action
    mGotoPrevious = new QAction("Previous", this);
    mGotoPrevious->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoPrevious);
    connect(mGotoPrevious, SIGNAL(triggered()), this, SLOT(gotoPrevious()));

    // Next action
    mGotoNext = new QAction("Next", this);
    mGotoNext->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoNext);
    connect(mGotoNext, SIGNAL(triggered()), this, SLOT(gotoNext()));

    // Address action
    mGotoExpression = new QAction("Expression", this);
    mGotoExpression->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoExpression);
    connect(mGotoExpression, SIGNAL(triggered()), this, SLOT(gotoExpression()));

    // File offset action
    mGotoFileOffset = new QAction("File Offset", this);
    connect(mGotoFileOffset, SIGNAL(triggered()), this, SLOT(gotoFileOffset()));

    //-------------------- Follow in Dump ----------------------------
    // Menu
    mFollowMenu = new QMenu("&Follow in Dump", this);

    //-------------------- Copy -------------------------------------
    mCopyMenu = new QMenu("&Copy", this);

    mCopySelection = new QAction("&Selection", this);
    mCopySelection->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mCopySelection);
    connect(mCopySelection, SIGNAL(triggered()), this, SLOT(copySelection()));

    mCopySelectionNoBytes = new QAction("Selection (&No Bytes)", this);
    connect(mCopySelectionNoBytes, SIGNAL(triggered()), this, SLOT(copySelectionNoBytes()));

    mCopyAddress = new QAction("&Address", this);
    connect(mCopyAddress, SIGNAL(triggered()), this, SLOT(copyAddress()));

    mCopyDisassembly = new QAction("Disassembly", this);
    connect(mCopyDisassembly, SIGNAL(triggered()), this, SLOT(copyDisassembly()));

    mCopyMenu->addAction(mCopySelection);
    mCopyMenu->addAction(mCopySelectionNoBytes);
    mCopyMenu->addAction(mCopyAddress);
    mCopyMenu->addAction(mCopyDisassembly);


    //-------------------- Find references to -----------------------
    // Menu
    mReferencesMenu = new QMenu("Find &references to", this);

    // Selected address
    mReferenceSelectedAddress = new QAction("&Selected address", this);
    mReferenceSelectedAddress->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mReferenceSelectedAddress);
    connect(mReferenceSelectedAddress, SIGNAL(triggered()), this, SLOT(findReferences()));

    //---------------------- Search for -----------------------------
    // Menu
    mSearchMenu = new QMenu("&Search for", this);

    // Command
    mSearchCommand = new QAction("C&ommand", this);
    mSearchCommand->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mSearchCommand);
    connect(mSearchCommand, SIGNAL(triggered()), this, SLOT(findCommand()));
    mSearchMenu->addAction(mSearchCommand);

    // Constant
    mSearchConstant = new QAction("&Constant", this);
    connect(mSearchConstant, SIGNAL(triggered()), this, SLOT(findConstant()));
    mSearchMenu->addAction(mSearchConstant);

    // String References
    mSearchStrings = new QAction("&String references", this);
    connect(mSearchStrings, SIGNAL(triggered()), this, SLOT(findStrings()));
    mSearchMenu->addAction(mSearchStrings);

    // Intermodular Calls
    mSearchCalls = new QAction("&Intermodular calls", this);
    connect(mSearchCalls, SIGNAL(triggered()), this, SLOT(findCalls()));
    mSearchMenu->addAction(mSearchCalls);

    // Pattern
    mSearchPattern = new QAction("&Pattern", this);
    mSearchPattern->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mSearchPattern);
    connect(mSearchPattern, SIGNAL(triggered()), this, SLOT(findPattern()));
    mSearchMenu->addAction(mSearchPattern);

    // Highlighting mode
    mEnableHighlightingMode = new QAction("&Highlighting mode", this);
    mEnableHighlightingMode->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mEnableHighlightingMode);
    connect(mEnableHighlightingMode, SIGNAL(triggered()), this, SLOT(enableHighlightingMode()));

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void CPUDisassembly::refreshShortcutsSlot()
{
    mBinaryEditAction->setShortcut(ConfigShortcut("ActionBinaryEdit"));
    mBinaryFillAction->setShortcut(ConfigShortcut("ActionBinaryFill"));
    mBinaryFillNopsAction->setShortcut(ConfigShortcut("ActionBinaryFillNops"));
    mBinaryCopyAction->setShortcut(ConfigShortcut("ActionBinaryCopy"));
    mBinaryPasteAction->setShortcut(ConfigShortcut("ActionBinaryPaste"));
    mBinaryPasteIgnoreSizeAction->setShortcut(ConfigShortcut("ActionBinaryPasteIgnoreSize"));
    mUndoSelection->setShortcut(ConfigShortcut("ActionUndoSelection"));
    mSetLabel->setShortcut(ConfigShortcut("ActionSetLabel"));
    mSetComment->setShortcut(ConfigShortcut("ActionSetComment"));
    mSetBookmark->setShortcut(ConfigShortcut("ActionToggleBookmark"));
    mToggleFunction->setShortcut(ConfigShortcut("ActionToggleFunction"));
    mAssemble->setShortcut(ConfigShortcut("ActionAssemble"));
    mToggleInt3BpAction->setShortcut(ConfigShortcut("ActionToggleBreakpoint"));
    mPatchesAction->setShortcut(ConfigShortcut("ViewPatches"));
    mSetNewOriginHere->setShortcut(ConfigShortcut("ActionSetNewOriginHere"));
    mGotoOrigin->setShortcut(ConfigShortcut("ActionGotoOrigin"));
    mGotoPrevious->setShortcut(ConfigShortcut("ActionGotoPrevious"));
    mGotoNext->setShortcut(ConfigShortcut("ActionGotoNext"));
    mGotoExpression->setShortcut(ConfigShortcut("ActionGotoExpression"));
    mReferenceSelectedAddress->setShortcut(ConfigShortcut("ActionFindReferencesToSelectedAddress"));
    mSearchPattern->setShortcut(ConfigShortcut("ActionFindPattern"));
    mEnableHighlightingMode->setShortcut(ConfigShortcut("ActionHighlightingMode"));
    mCopySelection->setShortcut(ConfigShortcut("ActionCopy"));
    mSearchCommand->setShortcut(ConfigShortcut("ActionFind"));
}

void CPUDisassembly::gotoOrigin()
{
    if(!DbgIsDebugging())
        return;
    DbgCmdExec("disasm cip");
}


void CPUDisassembly::toggleInt3BPAction()
{
    if(!DbgIsDebugging())
        return;
    uint_t wVA = rvaToVa(getInitialSelection());
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_normal) == bp_normal)
    {
        wCmd = "bc " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    else
    {
        wCmd = "bp " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }

    DbgCmdExec(wCmd.toUtf8().constData());
    //emit Disassembly::repainted();
}


void CPUDisassembly::toggleHwBpActionSlot()
{
    uint_t wVA = rvaToVa(getInitialSelection());
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_hardware) == bp_hardware)
    {
        wCmd = "bphwc " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    else
    {
        wCmd = "bphws " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }

    DbgCmdExec(wCmd.toUtf8().constData());
}


void CPUDisassembly::setHwBpOnSlot0ActionSlot()
{
    setHwBpAt(rvaToVa(getInitialSelection()), 0);
}

void CPUDisassembly::setHwBpOnSlot1ActionSlot()
{
    setHwBpAt(rvaToVa(getInitialSelection()), 1);
}

void CPUDisassembly::setHwBpOnSlot2ActionSlot()
{
    setHwBpAt(rvaToVa(getInitialSelection()), 2);
}

void CPUDisassembly::setHwBpOnSlot3ActionSlot()
{
    setHwBpAt(rvaToVa(getInitialSelection()), 3);
}

void CPUDisassembly::setHwBpAt(uint_t va, int slot)
{
    BPXTYPE wBpType = DbgGetBpxTypeAt(va);

    if((wBpType & bp_hardware) == bp_hardware)
    {
        mBPMenu->addAction(mClearHwBpAction);
    }


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
        wCmd = "bphws " + QString("%1").arg(va, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(wCmd.toUtf8().constData());
    }
    else // Slot used
    {
        wCmd = "bphwc " + QString("%1").arg((uint_t)(wBPList.bp[wSlotIndex].addr), sizeof(uint_t) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(wCmd.toUtf8().constData());

        Sleep(200);

        wCmd = "bphws " + QString("%1").arg(va, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(wCmd.toUtf8().constData());
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);
}

void CPUDisassembly::setNewOriginHereActionSlot()
{
    if(!DbgIsDebugging())
        return;
    uint_t wVA = rvaToVa(getInitialSelection());
    QString wCmd = "cip=" + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(wCmd.toUtf8().constData());
}

void CPUDisassembly::setLabel()
{
    if(!DbgIsDebugging())
        return;
    uint_t wVA = rvaToVa(getInitialSelection());
    LineEditDialog mLineEdit(this);
    QString addr_text = QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    char label_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetLabelAt((duint)wVA, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle("Add label at " + addr_text);
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    if(!DbgSetLabelAt(wVA, mLineEdit.editText.toUtf8().constData()))
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "DbgSetLabelAt failed!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
    }
    GuiUpdateAllViews();
}

void CPUDisassembly::setComment()
{
    if(!DbgIsDebugging())
        return;
    uint_t wVA = rvaToVa(getInitialSelection());
    LineEditDialog mLineEdit(this);
    QString addr_text = QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    char comment_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetCommentAt((duint)wVA, comment_text))
    {
        if(comment_text[0] == '\1') //automatic comment
            mLineEdit.setText(QString(comment_text + 1));
        else
            mLineEdit.setText(QString(comment_text));
    }
    mLineEdit.setWindowTitle("Add comment at " + addr_text);
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    if(!DbgSetCommentAt(wVA, mLineEdit.editText.toUtf8().constData()))
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "DbgSetCommentAt failed!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
    }
    GuiUpdateAllViews();
}

void CPUDisassembly::setBookmark()
{
    if(!DbgIsDebugging())
        return;
    uint_t wVA = rvaToVa(getInitialSelection());
    bool result;
    if(DbgGetBookmarkAt(wVA))
        result = DbgSetBookmarkAt(wVA, false);
    else
        result = DbgSetBookmarkAt(wVA, true);
    if(!result)
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "DbgSetBookmarkAt failed!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
    }
    GuiUpdateAllViews();
}

void CPUDisassembly::toggleFunction()
{
    if(!DbgIsDebugging())
        return;
    uint_t start = rvaToVa(getSelectionStart());
    uint_t end = rvaToVa(getSelectionEnd());
    uint_t function_start = 0;
    uint_t function_end = 0;
    if(!DbgFunctionOverlaps(start, end))
    {
        QString start_text = QString("%1").arg(start, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        QString end_text = QString("%1").arg(end, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        char labeltext[MAX_LABEL_SIZE] = "";
        QString label_text = "";
        if(DbgGetLabelAt(start, SEG_DEFAULT, labeltext))
            label_text = " (" + QString(labeltext) + ")";

        QMessageBox msg(QMessageBox::Question, "Add the function?", start_text + "-" + end_text + label_text, QMessageBox::Yes | QMessageBox::No);
        msg.setWindowIcon(QIcon(":/icons/images/compile.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() != QMessageBox::Yes)
            return;
        QString cmd = "functionadd " + start_text + "," + end_text;
        DbgCmdExec(cmd.toUtf8().constData());
    }
    else
    {
        for(uint_t i = start; i <= end; i++)
        {
            if(DbgFunctionGet(i, &function_start, &function_end))
                break;
        }
        QString start_text = QString("%1").arg(function_start, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        QString end_text = QString("%1").arg(function_end, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        char labeltext[MAX_LABEL_SIZE] = "";
        QString label_text = "";
        if(DbgGetLabelAt(function_start, SEG_DEFAULT, labeltext))
            label_text = " (" + QString(labeltext) + ")";

        QMessageBox msg(QMessageBox::Warning, "Deleting function:", start_text + "-" + end_text + label_text, QMessageBox::Ok | QMessageBox::Cancel);
        msg.setDefaultButton(QMessageBox::Cancel);
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() != QMessageBox::Ok)
            return;
        QString cmd = "functiondel " + start_text;
        DbgCmdExec(cmd.toUtf8().constData());
    }
}

void CPUDisassembly::assembleAt()
{
    if(!DbgIsDebugging())
        return;

    do
    {
        int_t wRVA = getInitialSelection();
        uint_t wVA = rvaToVa(wRVA);
        QString addr_text = QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();

        QByteArray wBuffer;

        int_t wMaxByteCountToRead = 16 * 2;

        //TODO: fix size problems
        int_t size = getSize();
        if(!size)
            size = wRVA;

        // Bounding
        wMaxByteCountToRead = wMaxByteCountToRead > (size - wRVA) ? (size - wRVA) : wMaxByteCountToRead;

        wBuffer.resize(wMaxByteCountToRead);

        mMemPage->read(reinterpret_cast<byte_t*>(wBuffer.data()), wRVA, wMaxByteCountToRead);

        QBeaEngine disasm;
        Instruction_t instr = disasm.DisassembleAt(reinterpret_cast<byte_t*>(wBuffer.data()), wMaxByteCountToRead, 0, 0, wVA);

        QString actual_inst = instr.instStr;

        bool assembly_error;
        do
        {
            assembly_error = false;

            LineEditDialog mLineEdit(this);
            mLineEdit.setText(actual_inst);
            mLineEdit.setWindowTitle("Assemble at " + addr_text);
            mLineEdit.setCheckBoxText("&Fill with NOP's");
            mLineEdit.enableCheckBox(true);
            mLineEdit.setCheckBox(ConfigBool("Disassembler", "FillNOPs"));
            if(mLineEdit.exec() != QDialog::Accepted)
                return;

            //if the instruction its unkown or is the old instruction or empty (easy way to skip from GUI) skipping
            if(mLineEdit.editText == QString("???") || mLineEdit.editText.toLower() == instr.instStr.toLower() || mLineEdit.editText == QString(""))
                break;

            Config()->setBool("Disassembler", "FillNOPs", mLineEdit.bChecked);

            char error[MAX_ERROR_SIZE] = "";
            if(!DbgFunctions()->AssembleAtEx(wVA, mLineEdit.editText.toUtf8().constData(), error, mLineEdit.bChecked))
            {
                QMessageBox msg(QMessageBox::Critical, "Error!", "Failed to assemble instruction \"" + mLineEdit.editText + "\" (" + error + ")");
                msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
                msg.setParent(this, Qt::Dialog);
                msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
                msg.exec();
                actual_inst = mLineEdit.editText;
                assembly_error = true;
            }
        }
        while(assembly_error);

        //select next instruction after assembling
        setSingleSelection(wRVA);

        int_t botRVA = getTableOffset();
        int_t topRVA = getInstructionRVA(getTableOffset(), getNbrOfLineToPrint() - 1);

        int_t wInstrSize = getInstructionRVA(wRVA, 1) - wRVA - 1;

        expandSelectionUpTo(wRVA + wInstrSize);
        selectNext(false);

        if(getSelectionStart() < botRVA)
            setTableOffset(getSelectionStart());
        else if(getSelectionEnd() >= topRVA)
            setTableOffset(getInstructionRVA(getSelectionEnd(), -getNbrOfLineToPrint() + 2));

        //refresh view
        GuiUpdateAllViews();
    }
    while(1);
}

void CPUDisassembly::gotoExpression()
{
    if(!DbgIsDebugging())
        return;
    if(!mGoto)
        mGoto = new GotoDialog(this);
    if(mGoto->exec() == QDialog::Accepted)
    {
        DbgCmdExec(QString().sprintf("disasm \"%s\"", mGoto->expressionText.toUtf8().constData()).toUtf8().constData());
    }
}

void CPUDisassembly::gotoFileOffset()
{
    if(!DbgIsDebugging())
        return;
    char modname[MAX_MODULE_SIZE] = "";
    if(!DbgFunctions()->ModNameFromAddr(rvaToVa(getInitialSelection()), modname, true))
    {
        QMessageBox::critical(this, "Error!", "Not inside a module...");
        return;
    }
    GotoDialog mGotoDialog(this);
    mGotoDialog.fileOffset = true;
    mGotoDialog.modName = QString(modname);
    mGotoDialog.setWindowTitle("Goto File Offset in " + QString(modname));
    if(mGotoDialog.exec() != QDialog::Accepted)
        return;
    uint_t value = DbgValFromString(mGotoDialog.expressionText.toUtf8().constData());
    value = DbgFunctions()->FileOffsetToVa(modname, value);
    DbgCmdExec(QString().sprintf("disasm \"%p\"", value).toUtf8().constData());
}

void CPUDisassembly::followActionSlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action && action->objectName().startsWith("DUMP|"))
        DbgCmdExec(QString().sprintf("dump \"%s\"", action->objectName().mid(5).toUtf8().constData()).toUtf8().constData());
}

void CPUDisassembly::gotoPrevious()
{
    historyPrevious();
}

void CPUDisassembly::gotoNext()
{
    historyNext();
}

void CPUDisassembly::findReferences()
{
    QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("findref " + addrText + ", " + addrText).toUtf8().constData());
    emit displayReferencesWidget();
}

void CPUDisassembly::findConstant()
{
    WordEditDialog wordEdit(this);
    wordEdit.setup("Enter Constant", 0, sizeof(int_t));
    if(wordEdit.exec() != QDialog::Accepted) //cancel pressed
        return;
    QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    QString constText = QString("%1").arg(wordEdit.getVal(), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("findref " + constText + ", " + addrText).toUtf8().constData());
    emit displayReferencesWidget();
}

void CPUDisassembly::findStrings()
{
    QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("strref " + addrText).toUtf8().constData());
    emit displayReferencesWidget();
}

void CPUDisassembly::findCalls()
{
    QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("modcallfind " + addrText).toUtf8().constData());
    emit displayReferencesWidget();
}

void CPUDisassembly::findPattern()
{
    HexEditDialog hexEdit(this);
    hexEdit.showEntireBlock(true);
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle("Find Pattern...");
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    int_t addr = rvaToVa(getSelectionStart());
    if(hexEdit.entireBlock())
        addr = DbgMemFindBaseAddr(addr, 0);
    QString addrText = QString("%1").arg(addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("findall " + addrText + ", " + hexEdit.mHexEdit->pattern()).toUtf8().constData());
    emit displayReferencesWidget();
}

void CPUDisassembly::selectionGet(SELECTIONDATA* selection)
{
    selection->start = rvaToVa(getSelectionStart());
    selection->end = rvaToVa(getSelectionEnd());
    Bridge::getBridge()->BridgeSetResult(1);
}

void CPUDisassembly::selectionSet(const SELECTIONDATA* selection)
{
    int_t selMin = getBase();
    int_t selMax = selMin + getSize();
    int_t start = selection->start;
    int_t end = selection->end;
    if(start < selMin || start >= selMax || end < selMin || end >= selMax) //selection out of range
    {
        Bridge::getBridge()->BridgeSetResult(0);
        return;
    }
    setSingleSelection(start - selMin);
    expandSelectionUpTo(end - selMin);
    reloadData();
    Bridge::getBridge()->BridgeSetResult(1);
}

void CPUDisassembly::enableHighlightingMode()
{
    if(mHighlightingMode)
        mHighlightingMode = false;
    else
        mHighlightingMode = true;
    reloadData();
}

void CPUDisassembly::binaryEditSlot()
{
    HexEditDialog hexEdit(this);
    int_t selStart = getSelectionStart();
    int_t selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.setWindowTitle("Edit code at " + QString("%1").arg(rvaToVa(selStart), sizeof(int_t) * 2, 16, QChar('0')).toUpper());
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    int_t dataSize = hexEdit.mHexEdit->data().size();
    int_t newSize = selSize > dataSize ? selSize : dataSize;
    data = new byte_t[newSize];
    mMemPage->read(data, selStart, newSize);
    QByteArray patched = hexEdit.mHexEdit->applyMaskedData(QByteArray((const char*)data, newSize));
    mMemPage->write(patched.constData(), selStart, patched.size());
    GuiUpdateAllViews();
}

void CPUDisassembly::binaryFillSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.mHexEdit->setOverwriteMode(false);
    int_t selStart = getSelectionStart();
    hexEdit.setWindowTitle("Fill code at " + QString("%1").arg(rvaToVa(selStart), sizeof(int_t) * 2, 16, QChar('0')).toUpper());
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    QString pattern = hexEdit.mHexEdit->pattern();
    int_t selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.mHexEdit->fill(0, QString(pattern));
    QByteArray patched(hexEdit.mHexEdit->data());
    mMemPage->write(patched, selStart, patched.size());
    GuiUpdateAllViews();
}

void CPUDisassembly::binaryFillNopsSlot()
{
    HexEditDialog hexEdit(this);
    int_t selStart = getSelectionStart();
    int_t selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.mHexEdit->fill(0, QString("90"));
    QByteArray patched(hexEdit.mHexEdit->data());
    mMemPage->write(patched, selStart, patched.size());
    GuiUpdateAllViews();
}

void CPUDisassembly::binaryCopySlot()
{
    HexEditDialog hexEdit(this);
    int_t selStart = getSelectionStart();
    int_t selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    Bridge::CopyToClipboard(hexEdit.mHexEdit->pattern(true));
}

void CPUDisassembly::binaryPasteSlot()
{
    HexEditDialog hexEdit(this);
    int_t selStart = getSelectionStart();
    int_t selSize = getSelectionEnd() - selStart + 1;
    QClipboard* clipboard = QApplication::clipboard();
    hexEdit.mHexEdit->setData(clipboard->text());

    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    QByteArray patched = hexEdit.mHexEdit->applyMaskedData(QByteArray((const char*)data, selSize));
    if(patched.size() < selSize)
        selSize = patched.size();
    mMemPage->write(patched.constData(), selStart, selSize);
    GuiUpdateAllViews();
}

void CPUDisassembly::undoSelectionSlot()
{
    int_t start = rvaToVa(getSelectionStart());
    int_t end = rvaToVa(getSelectionEnd());
    if(!DbgFunctions()->PatchInRange(start, end)) //nothing patched in selected range
        return;
    DbgFunctions()->PatchRestoreRange(start, end);
    reloadData();
}

void CPUDisassembly::binaryPasteIgnoreSizeSlot()
{
    HexEditDialog hexEdit(this);
    int_t selStart = getSelectionStart();
    int_t selSize = getSelectionEnd() - selStart + 1;
    QClipboard* clipboard = QApplication::clipboard();
    hexEdit.mHexEdit->setData(clipboard->text());

    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    QByteArray patched = hexEdit.mHexEdit->applyMaskedData(QByteArray((const char*)data, selSize));
    delete [] data;
    mMemPage->write(patched.constData(), selStart, patched.size());
    GuiUpdateAllViews();
}

void CPUDisassembly::showPatchesSlot()
{
    emit showPatches();
}

void CPUDisassembly::copySelection(bool copyBytes)
{
    QList<Instruction_t> instBuffer;
    prepareDataRange(getSelectionStart(), getSelectionEnd(), &instBuffer);
    QString clipboard = "";
    const int addressLen = getColumnWidth(0) / getCharWidth() - 1;
    const int bytesLen = getColumnWidth(1) / getCharWidth() - 1;
    const int disassemblyLen = getColumnWidth(2) / getCharWidth() - 1;
    for(int i = 0; i < instBuffer.size(); i++)
    {
        if(i)
            clipboard += "\r\n";
        int_t cur_addr = rvaToVa(instBuffer.at(i).rva);
        QString address = getAddrText(cur_addr, 0);
        QString bytes;
        for(int j = 0; j < instBuffer.at(i).dump.size(); j++)
        {
            if(j)
                bytes += " ";
            bytes += QString("%1").arg((unsigned char)(instBuffer.at(i).dump.at(j)), 2, 16, QChar('0')).toUpper();
        }
        QString disassembly;
        const BeaTokenizer::BeaInstructionToken* token = &instBuffer.at(i).tokens;
        for(int j = 0; j < token->tokens.size(); j++)
            disassembly += token->tokens.at(j).text;
        char comment[MAX_COMMENT_SIZE] = "";
        QString fullComment;
        if(DbgGetCommentAt(cur_addr, comment))
        {
            if(comment[0] == '\1') //automatic comment
                fullComment = " ;" + QString(comment + 1);
            else
                fullComment = " ;" + QString(comment);
        }
        clipboard += address.leftJustified(addressLen, QChar(' '), true);
        if(copyBytes)
            clipboard += " | " + bytes.leftJustified(bytesLen, QChar(' '), true);
        clipboard += " | " + disassembly.leftJustified(disassemblyLen, QChar(' '), true) + " |" + fullComment;
    }
    Bridge::CopyToClipboard(clipboard);
}

void CPUDisassembly::copySelection()
{
    copySelection(true);
}

void CPUDisassembly::copySelectionNoBytes()
{
    copySelection(false);
}

void CPUDisassembly::copyAddress()
{
    QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    Bridge::CopyToClipboard(addrText);
}

void CPUDisassembly::copyDisassembly()
{
    QList<Instruction_t> instBuffer;
    prepareDataRange(getSelectionStart(), getSelectionEnd(), &instBuffer);
    QString clipboard = "";
    for(int i = 0; i < instBuffer.size(); i++)
    {
        if(i)
            clipboard += "\r\n";
        const BeaTokenizer::BeaInstructionToken* token = &instBuffer.at(i).tokens;
        for(int j = 0; j < token->tokens.size(); j++)
            clipboard += token->tokens.at(j).text;
    }
    Bridge::CopyToClipboard(clipboard);
}

void CPUDisassembly::findCommand()
{
    if(!DbgIsDebugging())
        return;

    LineEditDialog mLineEdit(this);
    mLineEdit.enableCheckBox(true);
    mLineEdit.setCheckBoxText("Entire &Block");
    mLineEdit.setCheckBox(ConfigBool("Disassembler", "FindCommandEntireBlock"));
    mLineEdit.setWindowTitle("Find Command");
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    Config()->setBool("Disassembler", "FindCommandEntireBlock", mLineEdit.bChecked);

    char error[MAX_ERROR_SIZE] = "";
    unsigned char dest[16];
    int asmsize = 0;
    uint_t va = rvaToVa(getInitialSelection());

    if(!DbgFunctions()->Assemble(va + mMemPage->getSize() / 2, dest, &asmsize, mLineEdit.editText.toUtf8().constData(), error))
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "Failed to assemble instruction \"" + mLineEdit.editText + "\" (" + error + ")");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }

    QString addr_text = QString("%1").arg(va, sizeof(int_t) * 2, 16, QChar('0')).toUpper();

    if(!mLineEdit.bChecked)
    {
        int_t size = mMemPage->getSize();
        DbgCmdExec(QString("findasm \"%1\", %2, .%3").arg(mLineEdit.editText).arg(addr_text).arg(size).toUtf8().constData());
    }
    else
        DbgCmdExec(QString("findasm \"%1\", %2").arg(mLineEdit.editText).arg(addr_text).toUtf8().constData());

    emit displayReferencesWidget();
}
