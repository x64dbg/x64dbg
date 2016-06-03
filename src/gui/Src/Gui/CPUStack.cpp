#include "CPUStack.h"
#include "CPUDump.h"
#include <QClipboard>
#include "Configuration.h"
#include "Bridge.h"
#include "HexEditDialog.h"
#include "WordEditDialog.h"
#include "CPUMultiDump.h"

CPUStack::CPUStack(CPUMultiDump* multiDump, QWidget* parent) : HexDump(parent)
{
    setShowHeader(false);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;
    bStackFrozen = false;
    mMultiDump = multiDump;

    mForceColumn = 1;

    wColDesc.isData = true; //void*
    wColDesc.itemCount = 1;
    wColDesc.separator = 0;
#ifdef _WIN64
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = HexQword;
#else
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = HexDword;
#endif
    appendDescriptor(8 + charwidth * 2 * sizeof(duint), "void*", false, wColDesc);

    wColDesc.isData = false; //comments
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(2000, "Comments", false, wColDesc);

    setupContextMenu();

    mGoto = 0;

    // Slots
    connect(Bridge::getBridge(), SIGNAL(stackDumpAt(duint, duint)), this, SLOT(stackDumpAt(duint, duint)));
    connect(Bridge::getBridge(), SIGNAL(selectionStackGet(SELECTIONDATA*)), this, SLOT(selectionGet(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionStackSet(const SELECTIONDATA*)), this, SLOT(selectionSet(const SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(focusStack()), this, SLOT(setFocus()));

    Initialize();
}

void CPUStack::updateColors()
{
    HexDump::updateColors();

    backgroundColor = ConfigColor("StackBackgroundColor");
    textColor = ConfigColor("StackTextColor");
    selectionColor = ConfigColor("StackSelectionColor");
}

void CPUStack::updateFonts()
{
    setFont(ConfigFont("Stack"));
}

void CPUStack::setupContextMenu()
{
    //Binary menu
    mBinaryMenu = new QMenu(tr("B&inary"), this);
    mBinaryMenu->setIcon(QIcon(":/icons/images/binary.png"));

    //Binary->Edit
    mBinaryEditAction = new QAction(tr("&Edit"), this);
    mBinaryEditAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryEditAction);
    connect(mBinaryEditAction, SIGNAL(triggered()), this, SLOT(binaryEditSlot()));
    mBinaryMenu->addAction(mBinaryEditAction);

    //Binary->Fill
    mBinaryFillAction = new QAction(tr("&Fill..."), this);
    mBinaryFillAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryFillAction);
    connect(mBinaryFillAction, SIGNAL(triggered()), this, SLOT(binaryFillSlot()));
    mBinaryMenu->addAction(mBinaryFillAction);

    //Binary->Separator
    mBinaryMenu->addSeparator();

    //Binary->Copy
    mBinaryCopyAction = new QAction(tr("&Copy"), this);
    mBinaryCopyAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryCopyAction);
    connect(mBinaryCopyAction, SIGNAL(triggered()), this, SLOT(binaryCopySlot()));
    mBinaryMenu->addAction(mBinaryCopyAction);

    //Binary->Paste
    mBinaryPasteAction = new QAction(tr("&Paste"), this);
    mBinaryPasteAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryPasteAction);
    connect(mBinaryPasteAction, SIGNAL(triggered()), this, SLOT(binaryPasteSlot()));
    mBinaryMenu->addAction(mBinaryPasteAction);

    //Binary->Paste (Ignore Size)
    mBinaryPasteIgnoreSizeAction = new QAction(tr("Paste (&Ignore Size)"), this);
    mBinaryPasteIgnoreSizeAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryPasteIgnoreSizeAction);
    connect(mBinaryPasteIgnoreSizeAction, SIGNAL(triggered()), this, SLOT(binaryPasteIgnoreSizeSlot()));
    mBinaryMenu->addAction(mBinaryPasteIgnoreSizeAction);

    //Breakpoint menu
    mBreakpointMenu = new QMenu(tr("Brea&kpoint"), this);
    mBreakpointMenu->setIcon(QIcon(":/icons/images/breakpoint.png"));

    //Breakpoint (hardware access) menu
    mBreakpointHardwareAccessMenu = new QMenu(tr("Hardware, Access"), this);
    mBreakpointHardwareAccess1 = new QAction(tr("&Byte"), this);
    connect(mBreakpointHardwareAccess1, SIGNAL(triggered()), this, SLOT(hardwareAccess1Slot()));
    mBreakpointHardwareAccessMenu->addAction(mBreakpointHardwareAccess1);

    mBreakpointHardwareAccess2 = new QAction(tr("&Word"), this);
    connect(mBreakpointHardwareAccess2, SIGNAL(triggered()), this, SLOT(hardwareAccess2Slot()));
    mBreakpointHardwareAccessMenu->addAction(mBreakpointHardwareAccess2);

    mBreakpointHardwareAccess4 = new QAction(tr("&Dword"), this);
    connect(mBreakpointHardwareAccess4, SIGNAL(triggered()), this, SLOT(hardwareAccess4Slot()));
    mBreakpointHardwareAccessMenu->addAction(mBreakpointHardwareAccess4);

#ifdef _WIN64
    mBreakpointHardwareAccess8 = new QAction(tr("&Qword"), this);
    connect(mBreakpointHardwareAccess8, SIGNAL(triggered()), this, SLOT(hardwareAccess8Slot()));
    mBreakpointHardwareAccessMenu->addAction(mBreakpointHardwareAccess8);
#endif //_WIN64
    mBreakpointMenu->addMenu(mBreakpointHardwareAccessMenu);

    //Breakpoint (hardware write) menu
    mBreakpointHardwareWriteMenu = new QMenu(tr("Hardware, Write"), this);
    mBreakpointHardwareWrite1 = new QAction(tr("&Byte"), this);
    connect(mBreakpointHardwareWrite1, SIGNAL(triggered()), this, SLOT(hardwareWrite1Slot()));
    mBreakpointHardwareWriteMenu->addAction(mBreakpointHardwareWrite1);

    mBreakpointHardwareWrite2 = new QAction(tr("&Word"), this);
    connect(mBreakpointHardwareWrite2, SIGNAL(triggered()), this, SLOT(hardwareWrite2Slot()));
    mBreakpointHardwareWriteMenu->addAction(mBreakpointHardwareWrite2);

    mBreakpointHardwareWrite4 = new QAction(tr("&Dword"), this);
    connect(mBreakpointHardwareWrite4, SIGNAL(triggered()), this, SLOT(hardwareWrite4Slot()));
    mBreakpointHardwareWriteMenu->addAction(mBreakpointHardwareWrite4);

#ifdef _WIN64
    mBreakpointHardwareWrite8 = new QAction(tr("&Qword"), this);
    connect(mBreakpointHardwareWrite8, SIGNAL(triggered()), this, SLOT(hardwareWrite8Slot()));
    mBreakpointHardwareWriteMenu->addAction(mBreakpointHardwareWrite8);
#endif //_WIN64
    mBreakpointMenu->addMenu(mBreakpointHardwareWriteMenu);
    mBreakpointHardwareRemove = new QAction(tr("Remove &Hardware"), this);
    connect(mBreakpointHardwareRemove, SIGNAL(triggered()), this, SLOT(hardwareRemoveSlot()));
    mBreakpointMenu->addAction(mBreakpointHardwareRemove);
    mBreakpointMenu->addSeparator();

    //Breakpoint memory menu
    mBreakpointMemoryAccessMenu = new QMenu(tr("Memory, Access"), this);
    mBreakpointMemoryWriteMenu = new QMenu(tr("Memory, Write"), this);

    mBreakpointMemoryAccessSingleshoot = new QAction(tr("&Singleshoot"), this);
    connect(mBreakpointMemoryAccessSingleshoot, SIGNAL(triggered()), this, SLOT(memoryAccessSingleshootSlot()));
    mBreakpointMemoryAccessMenu->addAction(mBreakpointMemoryAccessSingleshoot);

    mBreakpointMemoryAccessRestore = new QAction(tr("&Restore on hit"), this);
    connect(mBreakpointMemoryAccessRestore, SIGNAL(triggered()), this, SLOT(memoryAccessRestoreSlot()));
    mBreakpointMemoryAccessMenu->addAction(mBreakpointMemoryAccessRestore);

    mBreakpointMemoryWriteSingleShoot = new QAction(tr("&Singleshoot"), this);
    connect(mBreakpointMemoryWriteSingleShoot, SIGNAL(triggered()), this, SLOT(memoryWriteSingleshootSlot()));
    mBreakpointMemoryWriteMenu->addAction(mBreakpointMemoryWriteSingleShoot);

    mBreakpointMemoryWriteRestore = new QAction(tr("&Restore on hit"), this);
    connect(mBreakpointMemoryWriteRestore, SIGNAL(triggered()), this, SLOT(memoryWriteRestoreSlot()));
    mBreakpointMemoryWriteMenu->addAction(mBreakpointMemoryWriteRestore);
    mBreakpointMenu->addMenu(mBreakpointMemoryAccessMenu);
    mBreakpointMemoryRemove = new QAction(tr("Remove &Memory"), this);
    connect(mBreakpointMemoryRemove, SIGNAL(triggered()), this, SLOT(memoryRemoveSlot()));
    mBreakpointMenu->addAction(mBreakpointMemoryRemove);
    mBreakpointMenu->addMenu(mBreakpointMemoryWriteMenu);

    // Restore Selection
    mUndoSelection = new QAction(QIcon(":/icons/images/eraser.png"), tr("&Restore selection"), this);
    mUndoSelection->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mUndoSelection);
    connect(mUndoSelection, SIGNAL(triggered()), this, SLOT(undoSelectionSlot()));

    // Modify
    mModifyAction = new QAction(QIcon(":/icons/images/modify.png"), tr("Modify"), this);
    connect(mModifyAction, SIGNAL(triggered()), this, SLOT(modifySlot()));

    auto cspIcon = QIcon(":/icons/images/neworigin.png");
    auto cbpIcon = QIcon(":/icons/images/cbp.png");
#ifdef _WIN64
    mGotoSp = new QAction(cspIcon, tr("Follow R&SP"), this);
    mGotoBp = new QAction(cbpIcon, tr("Follow R&BP"), this);
#else
    mGotoSp = new QAction(cspIcon, tr("Follow E&SP"), this);
    mGotoBp = new QAction(cbpIcon, tr("Follow E&BP"), this);
#endif //_WIN64
    mGotoSp->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoSp);
    connect(mGotoSp, SIGNAL(triggered()), this, SLOT(gotoSpSlot()));
    connect(mGotoBp, SIGNAL(triggered()), this, SLOT(gotoBpSlot()));

    mFreezeStack = new QAction(QIcon(":/icons/images/freeze.png"), tr("Freeze the stack"), this);
    this->addAction(mFreezeStack);
    connect(mFreezeStack, SIGNAL(triggered()), this, SLOT(freezeStackSlot()));

    //Find Pattern
    mFindPatternAction = new QAction(QIcon(":/icons/images/search-for.png"), tr("&Find Pattern..."), this);
    mFindPatternAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mFindPatternAction);
    connect(mFindPatternAction, SIGNAL(triggered()), this, SLOT(findPattern()));

    //Go to Expression
    mGotoExpression = new QAction(QIcon(":/icons/images/geolocation-goto.png"), tr("Go to &Expression"), this);
    mGotoExpression->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoExpression);
    connect(mGotoExpression, SIGNAL(triggered()), this, SLOT(gotoExpressionSlot()));

    //Go to Previous
    mGotoPrevious = new QAction(QIcon(":/icons/images/previous.png"), tr("Go to Previous"), this);
    mGotoPrevious->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoPrevious);
    connect(mGotoPrevious, SIGNAL(triggered(bool)), this, SLOT(gotoPreviousSlot()));

    //Go to Next
    mGotoNext = new QAction(QIcon(":/icons/images/next.png"), tr("Go to Next"), this);
    mGotoNext->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoNext);
    connect(mGotoNext, SIGNAL(triggered(bool)), this, SLOT(gotoNextSlot()));

    //Follow in Disassembler
    auto disasmIcon = QIcon(QString(":/icons/images/") + ArchValue("processor32.png", "processor64.png"));
    mFollowDisasm = new QAction(disasmIcon, tr("&Follow in Disassembler"), this);
    mFollowDisasm->setShortcutContext(Qt::WidgetShortcut);
    mFollowDisasm->setShortcut(QKeySequence("enter"));
    this->addAction(mFollowDisasm);
    connect(mFollowDisasm, SIGNAL(triggered()), this, SLOT(followDisasmSlot()));
    connect(this, SIGNAL(selectionUpdated()), this, SLOT(selectionUpdatedSlot()));

    //Follow in Dump
    auto followDumpName = ArchValue(tr("Follow DWORD in &Dump"), tr("Follow QWORD in &Dump"));
    mFollowDump = new QAction(QIcon(":/icons/images/dump.png"), followDumpName, this);
    connect(mFollowDump, SIGNAL(triggered()), this, SLOT(followDumpSlot()));

    auto followDumpMenuName = ArchValue(tr("&Follow DWORD in Dump"), tr("&Follow QWORD in Dump"));
    mFollowInDumpMenu = new QMenu(followDumpMenuName, this);

    int maxDumps = mMultiDump->getMaxCPUTabs();
    for(int i = 0; i < maxDumps; i++)
    {
        QAction* action = new QAction(tr("Dump %1").arg(i + 1), this);
        connect(action, SIGNAL(triggered()), this, SLOT(followinDumpNSlot()));
        mFollowInDumpMenu->addAction(action);
        mFollowInDumpActions.push_back(action);
    }

    auto followStackName = ArchValue(tr("Follow DWORD in &Stack"), tr("Follow QWORD in &Stack"));
    mFollowStack = new QAction(QIcon(":/icons/images/stack.png"), followStackName, this);
    connect(mFollowStack, SIGNAL(triggered()), this, SLOT(followStackSlot()));

    mPluginMenu = new QMenu(this);
    mPluginMenu->setIcon(QIcon(":/icons/images/plugin.png"));
    Bridge::getBridge()->emitMenuAddToList(this, mPluginMenu, GUI_STACK_MENU);

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void CPUStack::updateFreezeStackAction()
{
    QFont font = mFreezeStack->font();

    if(bStackFrozen)
    {
        font.setBold(true);
        mFreezeStack->setFont(font);
        mFreezeStack->setText(tr("Unfreeze the stack"));
    }
    else
    {
        font.setBold(false);
        mFreezeStack->setFont(font);
        mFreezeStack->setText(tr("Freeze the stack"));
    }
}

void CPUStack::refreshShortcutsSlot()
{
    mBinaryEditAction->setShortcut(ConfigShortcut("ActionBinaryEdit"));
    mBinaryFillAction->setShortcut(ConfigShortcut("ActionBinaryFill"));
    mBinaryCopyAction->setShortcut(ConfigShortcut("ActionBinaryCopy"));
    mBinaryPasteAction->setShortcut(ConfigShortcut("ActionBinaryPaste"));
    mBinaryPasteIgnoreSizeAction->setShortcut(ConfigShortcut("ActionBinaryPasteIgnoreSize"));
    mUndoSelection->setShortcut(ConfigShortcut("ActionUndoSelection"));
    mGotoSp->setShortcut(ConfigShortcut("ActionGotoOrigin"));
    mFindPatternAction->setShortcut(ConfigShortcut("ActionFindPattern"));
    mGotoExpression->setShortcut(ConfigShortcut("ActionGotoExpression"));
    mGotoPrevious->setShortcut(ConfigShortcut("ActionGotoPrevious"));
    mGotoNext->setShortcut(ConfigShortcut("ActionGotoNext"));
}

void CPUStack::getColumnRichText(int col, dsint rva, RichTextPainter::List & richText)
{
    // Compute RVA
    dsint wRva = rva;
    duint wVa = rvaToVa(wRva);

    bool wActiveStack = true;
    if(wVa < mCsp) //inactive stack
        wActiveStack = false;

    STACK_COMMENT comment;
    RichTextPainter::CustomRichText_t curData;
    curData.highlight = false;
    curData.flags = RichTextPainter::FlagColor;
    curData.textColor = textColor;

    if(col && mDescriptor.at(col - 1).isData == true) //paint stack data
    {
        HexDump::getColumnRichText(col, rva, richText);
        if(!wActiveStack)
        {
            QColor inactiveColor = ConfigColor("StackInactiveTextColor");
            for(int i = 0; i < int(richText.size()); i++)
            {
                richText[i].flags = RichTextPainter::FlagColor;
                richText[i].textColor = inactiveColor;
            }
        }
    }
    else if(col && DbgStackCommentGet(rvaToVa(wRva), &comment)) //paint stack comments
    {
        if(wActiveStack)
        {
            if(*comment.color)
                curData.textColor = QColor(QString(comment.color));
            else
                curData.textColor = textColor;
        }
        else
            curData.textColor = ConfigColor("StackInactiveTextColor");
        curData.text = comment.comment;
        richText.push_back(curData);
    }
    else
        HexDump::getColumnRichText(col, rva, richText);
}

QString CPUStack::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    // Compute RVA
    int wBytePerRowCount = getBytePerRowCount();
    dsint wRva = (rowBase + rowOffset) * wBytePerRowCount - mByteOffset;
    duint wVa = rvaToVa(wRva);

    // This sets the first visible row to be selected when stack is frozen, so that we can scroll the stack without it being reset to first selection
    if(bStackFrozen && rowOffset == 0)
        setSingleSelection(wRva);

    bool wIsSelected = isSelected(wRva);
    if(wIsSelected) //highlight if selected
        painter->fillRect(QRect(x, y, w, h), QBrush(selectionColor));

    if(col == 0) // paint stack address
    {
        QColor background;
        if(DbgGetLabelAt(wVa, SEG_DEFAULT, nullptr)) //label
        {
            if(wVa == mCsp) //CSP
            {
                background = ConfigColor("StackCspBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackCspColor")));
            }
            else //no CSP
            {
                background = ConfigColor("StackLabelBackgroundColor");
                painter->setPen(ConfigColor("StackLabelColor"));
            }
        }
        else //no label
        {
            if(wVa == mCsp) //CSP
            {
                background = ConfigColor("StackCspBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackCspColor")));
            }
            else if(wIsSelected) //selected normal address
            {
                background = ConfigColor("StackSelectedAddressBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackSelectedAddressColor"))); //black address (DisassemblySelectedAddressColor)
            }
            else //normal address
            {
                background = ConfigColor("StackAddressBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackAddressColor")));
            }
        }
        if(background.alpha())
            painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background when defined
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, makeAddrText(wVa));
        return "";
    }
    return HexDump::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);;
}

void CPUStack::contextMenuEvent(QContextMenuEvent* event)
{
    dsint selectedAddr = rvaToVa(getInitialSelection());

    if(!DbgIsDebugging())
        return;

    QMenu wMenu(this); //create context menu
    wMenu.addAction(mModifyAction);
    wMenu.addMenu(mBinaryMenu);
    QMenu wCopyMenu("&Copy", this);
    wCopyMenu.setIcon(QIcon(":/icons/images/copy.png"));
    wCopyMenu.addAction(mCopySelection);
    wCopyMenu.addAction(mCopyAddress);
    if(DbgFunctions()->ModBaseFromAddr(selectedAddr))
        wCopyMenu.addAction(mCopyRva);
    wMenu.addMenu(&wCopyMenu);
    wMenu.addMenu(mBreakpointMenu);
    dsint start = rvaToVa(getSelectionStart());
    dsint end = rvaToVa(getSelectionEnd());
    if(DbgFunctions()->PatchInRange(start, end)) //nothing patched in selected range
        wMenu.addAction(mUndoSelection);
    wMenu.addAction(mFindPatternAction);
    wMenu.addAction(mGotoSp);
    wMenu.addAction(mGotoBp);
    wMenu.addAction(mFreezeStack);
    wMenu.addAction(mGotoExpression);
    if(historyHasPrev())
        wMenu.addAction(mGotoPrevious);
    if(historyHasNext())
        wMenu.addAction(mGotoNext);

    duint selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            duint stackBegin = mMemPage->getBase();
            duint stackEnd = stackBegin + mMemPage->getSize();
            if(selectedData >= stackBegin && selectedData < stackEnd)
                wMenu.addAction(mFollowStack);
            wMenu.addAction(mFollowDisasm);
            wMenu.addAction(mFollowDump);
            wMenu.addMenu(mFollowInDumpMenu);
        }

    wMenu.addSeparator();
    wMenu.addActions(mPluginMenu->actions());


    if(DbgGetBpxTypeAt(selectedAddr) & bp_hardware) //hardware breakpoint set
    {
        mBreakpointHardwareAccessMenu->menuAction()->setVisible(false);
        mBreakpointHardwareWriteMenu->menuAction()->setVisible(false);
        mBreakpointHardwareRemove->setVisible(true);
    }
    else //hardware breakpoint not set
    {
        mBreakpointHardwareAccessMenu->menuAction()->setVisible(true);
        mBreakpointHardwareWriteMenu->menuAction()->setVisible(true);
        mBreakpointHardwareRemove->setVisible(false);
    }
    if(DbgGetBpxTypeAt(selectedAddr) & bp_memory) //memory breakpoint set
    {
        mBreakpointMemoryAccessMenu->menuAction()->setVisible(false);
        mBreakpointMemoryWriteMenu->menuAction()->setVisible(false);
        mBreakpointMemoryRemove->setVisible(true);
    }
    else //memory breakpoint not set
    {
        mBreakpointMemoryAccessMenu->menuAction()->setVisible(true);
        mBreakpointMemoryWriteMenu->menuAction()->setVisible(true);
        mBreakpointMemoryRemove->setVisible(false);
    }

    wMenu.exec(event->globalPos());
}

void CPUStack::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton)
        return;
    switch(getColumnIndexFromX(event->x()))
    {
    case 0: //address
    {
        //very ugly way to calculate the base of the current row (no clue why it works)
        dsint deltaRowBase = getInitialSelection() % getBytePerRowCount() + mByteOffset;
        if(deltaRowBase >= getBytePerRowCount())
            deltaRowBase -= getBytePerRowCount();
        dsint mSelectedVa = rvaToVa(getInitialSelection() - deltaRowBase);
        if(mRvaDisplayEnabled && mSelectedVa == mRvaDisplayBase)
            mRvaDisplayEnabled = false;
        else
        {
            mRvaDisplayEnabled = true;
            mRvaDisplayBase = mSelectedVa;
            mRvaDisplayPageBase = mMemPage->getBase();
        }
        reloadData();
    }
    break;

    default:
    {
        modifySlot();
    }
    break;
    }
}

void CPUStack::stackDumpAt(duint addr, duint csp)
{
    setFocus();
    addVaToHistory(addr);
    mCsp = csp;
    printDumpAt(addr);
}

void CPUStack::gotoSpSlot()
{
    if(!DbgIsDebugging())
        return;
    DbgCmdExec("sdump csp");
}

void CPUStack::gotoBpSlot()
{
#ifdef _WIN64
    DbgCmdExec("sdump rbp");
#else
    DbgCmdExec("sdump ebp");
#endif //_WIN64
}

void CPUStack::gotoExpressionSlot()
{
    if(!DbgIsDebugging())
        return;
    duint size = 0;
    duint base = DbgMemFindBaseAddr(mCsp, &size);
    if(!mGoto)
        mGoto = new GotoDialog(this);
    mGoto->validRangeStart = base;
    mGoto->validRangeEnd = base + size;
    mGoto->setWindowTitle(tr("Enter expression to follow in Stack..."));
    if(mGoto->exec() == QDialog::Accepted)
    {
        QString cmd;
        DbgCmdExec(cmd.sprintf("sdump \"%s\"", mGoto->expressionText.toUtf8().constData()).toUtf8().constData());
    }
}

void CPUStack::gotoPreviousSlot()
{
    historyPrev();
}

void CPUStack::gotoNextSlot()
{
    historyNext();
}

void CPUStack::selectionGet(SELECTIONDATA* selection)
{
    selection->start = rvaToVa(getSelectionStart());
    selection->end = rvaToVa(getSelectionEnd());
    Bridge::getBridge()->setResult(1);
}

void CPUStack::selectionSet(const SELECTIONDATA* selection)
{
    dsint selMin = mMemPage->getBase();
    dsint selMax = selMin + mMemPage->getSize();
    dsint start = selection->start;
    dsint end = selection->end;
    if(start < selMin || start >= selMax || end < selMin || end >= selMax) //selection out of range
    {
        Bridge::getBridge()->setResult(0);
        return;
    }
    setSingleSelection(start - selMin);
    expandSelectionUpTo(end - selMin);
    reloadData();
    Bridge::getBridge()->setResult(1);
}
void CPUStack::selectionUpdatedSlot()
{
    duint selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            duint stackBegin = mMemPage->getBase();
            duint stackEnd = stackBegin + mMemPage->getSize();
            if(selectedData >= stackBegin && selectedData < stackEnd) //data is a pointer to stack address
            {
                this->disconnect(SIGNAL(enterPressedSignal()));
                connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followStackSlot()));
                mFollowDisasm->setShortcut(QKeySequence(""));
                mFollowStack->setShortcut(QKeySequence("enter"));
            }
            else
            {
                this->disconnect(SIGNAL(enterPressedSignal()));
                connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followDisasmSlot()));
                mFollowStack->setShortcut(QKeySequence(""));
                mFollowDisasm->setShortcut(QKeySequence("enter"));
            }
        }
}

void CPUStack::followDisasmSlot()
{
    duint selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText = QString("%1").arg(selectedData, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
            DbgCmdExec(QString("disasm " + addrText).toUtf8().constData());
        }
}

void CPUStack::followDumpSlot()
{
    duint selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText = QString("%1").arg(selectedData, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
            DbgCmdExec(QString("dump " + addrText).toUtf8().constData());
        }
}

void CPUStack::followinDumpNSlot()
{
    duint selectedData = rvaToVa(getInitialSelection());

    if(DbgMemIsValidReadPtr(selectedData))
    {
        for(int i = 0; i < mFollowInDumpActions.length(); i++)
        {
            if(mFollowInDumpActions[i] == sender())
            {
                QString addrText = QString("%1").arg(ToPtrString(selectedData));
                DbgCmdExec(QString("dump [%1], %2").arg(addrText.toUtf8().constData()).arg(i).toUtf8().constData());
            }
        }
    }
}

void CPUStack::followStackSlot()
{
    duint selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText = QString("%1").arg(selectedData, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
            DbgCmdExec(QString("sdump " + addrText).toUtf8().constData());
        }
}

void CPUStack::binaryEditSlot()
{
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.setWindowTitle(tr("Edit data at %1").arg(ToPtrString(rvaToVa(selStart))));
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    dsint dataSize = hexEdit.mHexEdit->data().size();
    dsint newSize = selSize > dataSize ? selSize : dataSize;
    data = new byte_t[newSize];
    mMemPage->read(data, selStart, newSize);
    QByteArray patched = hexEdit.mHexEdit->applyMaskedData(QByteArray((const char*)data, newSize));
    mMemPage->write(patched.constData(), selStart, patched.size());
    GuiUpdateAllViews();
}

void CPUStack::binaryFillSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.mHexEdit->setOverwriteMode(false);
    dsint selStart = getSelectionStart();
    hexEdit.setWindowTitle(tr("Fill data at %1").arg(rvaToVa(selStart), sizeof(dsint) * 2, 16, QChar('0')).toUpper());
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    QString pattern = hexEdit.mHexEdit->pattern();
    dsint selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.mHexEdit->fill(0, QString(pattern));
    QByteArray patched(hexEdit.mHexEdit->data());
    mMemPage->write(patched, selStart, patched.size());
    GuiUpdateAllViews();
}

void CPUStack::binaryCopySlot()
{
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    Bridge::CopyToClipboard(hexEdit.mHexEdit->pattern(true));
}

void CPUStack::binaryPasteSlot()
{
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
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

void CPUStack::binaryPasteIgnoreSizeSlot()
{
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    QClipboard* clipboard = QApplication::clipboard();
    hexEdit.mHexEdit->setData(clipboard->text());

    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    QByteArray patched = hexEdit.mHexEdit->applyMaskedData(QByteArray((const char*)data, selSize));
    delete [] data;
    mMemPage->write(patched.constData(), selStart, patched.size());
    GuiUpdateAllViews();
}

// Copied from "CPUDump.cpp".
void CPUStack::hardwareAccess1Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", r, 1").toUtf8().constData());
}

void CPUStack::hardwareAccess2Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", r, 2").toUtf8().constData());
}

void CPUStack::hardwareAccess4Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", r, 4").toUtf8().constData());
}

void CPUStack::hardwareAccess8Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", r, 8").toUtf8().constData());
}

void CPUStack::hardwareWrite1Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", w, 1").toUtf8().constData());
}

void CPUStack::hardwareWrite2Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", w, 2").toUtf8().constData());
}

void CPUStack::hardwareWrite4Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", w, 4").toUtf8().constData());
}

void CPUStack::hardwareWrite8Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", w, 8").toUtf8().constData());
}

void CPUStack::hardwareRemoveSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphwc " + addr_text).toUtf8().constData());
}

void CPUStack::memoryAccessSingleshootSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm " + addr_text + ", 0, r").toUtf8().constData());
}

void CPUStack::memoryAccessRestoreSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm " + addr_text + ", 1, r").toUtf8().constData());
}

void CPUStack::memoryWriteSingleshootSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm " + addr_text + ", 0, w").toUtf8().constData());
}

void CPUStack::memoryWriteRestoreSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm " + addr_text + ", 1, w").toUtf8().constData());
}

void CPUStack::memoryRemoveSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpmc " + addr_text).toUtf8().constData());
}

void CPUStack::findPattern()
{
    HexEditDialog hexEdit(this);
    hexEdit.showEntireBlock(true);
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle("Find Pattern...");
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    dsint addr = rvaToVa(getSelectionStart());
    if(hexEdit.entireBlock())
        addr = DbgMemFindBaseAddr(addr, 0);
    QString addrText = QString("%1").arg(addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("findall " + addrText + ", " + hexEdit.mHexEdit->pattern() + ", &data&").toUtf8().constData());
    emit displayReferencesWidget();
}

void CPUStack::undoSelectionSlot()
{
    dsint start = rvaToVa(getSelectionStart());
    dsint end = rvaToVa(getSelectionEnd());
    if(!DbgFunctions()->PatchInRange(start, end)) //nothing patched in selected range
        return;
    DbgFunctions()->PatchRestoreRange(start, end);
    reloadData();
}

void CPUStack::modifySlot()
{
    dsint addr = getInitialSelection();
    WordEditDialog wEditDialog(this);
    dsint value = 0;
    mMemPage->read(&value, addr, sizeof(dsint));
    wEditDialog.setup(tr("Modify"), value, sizeof(dsint));
    if(wEditDialog.exec() != QDialog::Accepted)
        return;
    value = wEditDialog.getVal();
    mMemPage->write(&value, addr, sizeof(dsint));
    GuiUpdateAllViews();
}

void CPUStack::freezeStackSlot()
{
    if(bStackFrozen)
        DbgCmdExec(QString("setfreezestack 0").toUtf8().constData());
    else
        DbgCmdExec(QString("setfreezestack 1").toUtf8().constData());

    bStackFrozen = !bStackFrozen;

    updateFreezeStackAction();
}

void CPUStack::dbgStateChangedSlot(DBGSTATE state)
{
    if(state == initialized)
        bStackFrozen = false;

    updateFreezeStackAction();
}
