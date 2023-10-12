#include "CPUStack.h"
#include "CPUDump.h"
#include <QClipboard>
#include "Configuration.h"
#include "Bridge.h"
#include "CommonActions.h"
#include "HexEditDialog.h"
#include "WordEditDialog.h"
#include "CPUMultiDump.h"
#include "GotoDialog.h"

CPUStack::CPUStack(CPUMultiDump* multiDump, QWidget* parent)
    : HexDump(multiDump->getArchitecture(), parent)
{
    setWindowTitle("Stack");
    setShowHeader(false);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;
    mMultiDump = multiDump;

    mForceColumn = 1;

    colDesc.isData = true; //void*
    colDesc.itemCount = 1;
    colDesc.separator = 0;
#ifdef _WIN64
    colDesc.data.itemSize = Qword;
    colDesc.data.qwordMode = HexQword;
#else
    colDesc.data.itemSize = Dword;
    colDesc.data.dwordMode = HexDword;
#endif
    appendDescriptor(10 + charwidth * 2 * sizeof(duint), "void*", false, colDesc);

    colDesc.isData = false; //comments
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(2000, tr("Comments"), false, colDesc);

    setupContextMenu();

    mGoto = 0;

    // Slots
    connect(Bridge::getBridge(), SIGNAL(stackDumpAt(duint, duint)), this, SLOT(stackDumpAt(duint, duint)));
    connect(Bridge::getBridge(), SIGNAL(selectionStackGet(SELECTIONDATA*)), this, SLOT(selectionGet(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionStackSet(const SELECTIONDATA*)), this, SLOT(selectionSet(const SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(focusStack()), this, SLOT(setFocus()));
    connect(Bridge::getBridge(), SIGNAL(updateDump()), this, SLOT(updateSlot()));

    connect(this, SIGNAL(selectionUpdated()), this, SLOT(selectionUpdatedSlot()));

    Initialize();
}

void CPUStack::updateColors()
{
    HexDump::updateColors();

    mBackgroundColor = ConfigColor("StackBackgroundColor");
    mTextColor = ConfigColor("StackTextColor");
    mSelectionColor = ConfigColor("StackSelectionColor");
    mStackReturnToColor = ConfigColor("StackReturnToColor");
    mStackSEHChainColor = ConfigColor("StackSEHChainColor");
    mUserStackFrameColor = ConfigColor("StackFrameColor");
    mSystemStackFrameColor = ConfigColor("StackFrameSystemColor");
}

void CPUStack::updateFonts()
{
    setFont(ConfigFont("Stack"));
    invalidateCachedFont();
}

void CPUStack::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this, [](QMenu*)
    {
        return DbgIsDebugging();
    });
    mCommonActions = new CommonActions(this, getActionHelperFuncs(), [this]()
    {
        return rvaToVa(getSelectionStart());
    });

    //Realign
    mMenuBuilder->addAction(makeAction(DIcon("align-stack-pointer"), tr("Align Stack Pointer"), SLOT(realignSlot())), [this](QMenu*)
    {
        return (mCsp & (sizeof(duint) - 1)) != 0;
    });

    // Modify
    mMenuBuilder->addAction(makeAction(DIcon("modify"), tr("Modify"), SLOT(modifySlot())));

    auto binaryMenu = new MenuBuilder(this);

    //Binary->Edit
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_edit"), tr("&Edit"), SLOT(binaryEditSlot()), "ActionBinaryEdit"));

    //Binary->Fill
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_fill"), tr("&Fill..."), SLOT(binaryFillSlot()), "ActionBinaryFill"));

    //Binary->Separator
    binaryMenu->addSeparator();

    //Binary->Copy
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_copy"), tr("&Copy"), SLOT(binaryCopySlot()), "ActionBinaryCopy"));

    //Binary->Paste
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_paste"), tr("&Paste"), SLOT(binaryPasteSlot()), "ActionBinaryPaste"));

    //Binary->Paste (Ignore Size)
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_paste_ignoresize"), tr("Paste (&Ignore Size)"), SLOT(binaryPasteIgnoreSizeSlot()), "ActionBinaryPasteIgnoreSize"));

    mMenuBuilder->addMenu(makeMenu(DIcon("binary"), tr("B&inary")), binaryMenu);

    auto copyMenu = new MenuBuilder(this);
    copyMenu->addAction(mCopySelection);
    copyMenu->addAction(mCopyAddress);
    copyMenu->addAction(mCopyRva, [this](QMenu*)
    {
        return DbgFunctions()->ModBaseFromAddr(rvaToVa(getInitialSelection())) != 0;
    });

    //Copy->DWORD/QWORD
    QString ptrName = ArchValue(tr("&DWORD"), tr("&QWORD"));
    copyMenu->addAction(makeAction(ptrName, SLOT(copyPtrColumnSlot())));

    //Copy->Comments
    copyMenu->addAction(makeAction(tr("&Comments"), SLOT(copyCommentsColumnSlot())));

    mMenuBuilder->addMenu(makeMenu(DIcon("copy"), tr("&Copy")), copyMenu);

    //Breakpoint (hardware access) menu
    auto hardwareAccessMenu = makeMenu(DIcon("breakpoint_access"), tr("Hardware, Access"));
    hardwareAccessMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_byte"), tr("&Byte"), "bphws $, r, 1"));
    hardwareAccessMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_word"), tr("&Word"), "bphws $, r, 2"));
    hardwareAccessMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_dword"), tr("&Dword"), "bphws $, r, 4"));
#ifdef _WIN64
    hardwareAccessMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_qword"), tr("&Qword"), "bphws $, r, 8"));
#endif //_WIN64

    //Breakpoint (hardware write) menu
    auto hardwareWriteMenu = makeMenu(DIcon("breakpoint_write"), tr("Hardware, Write"));
    hardwareWriteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_byte"), tr("&Byte"), "bphws $, w, 1"));
    hardwareWriteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_word"), tr("&Word"), "bphws $, w, 2"));
    hardwareWriteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_dword"), tr("&Dword"), "bphws $, w, 4"));
#ifdef _WIN64
    hardwareWriteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_qword"), tr("&Qword"), "bphws $, r, 8"));
#endif //_WIN64

    //Breakpoint (remove hardware)
    auto hardwareRemove = mCommonActions->makeCommandAction(DIcon("breakpoint_remove"), tr("Remove &Hardware"), "bphwc $");

    //Breakpoint (memory access) menu
    auto memoryAccessMenu = makeMenu(DIcon("breakpoint_memory_access"), tr("Memory, Access"));
    memoryAccessMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_memory_singleshoot"), tr("&Singleshoot"), "bpm $, 0, a"));
    memoryAccessMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_memory_restore_on_hit"), tr("&Restore on hit"), "bpm $, 1, a"));

    //Breakpoint (memory write) menu
    auto memoryWriteMenu = makeMenu(DIcon("breakpoint_memory_write"), tr("Memory, Write"));
    memoryWriteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_memory_singleshoot"), tr("&Singleshoot"), "bpm $, 0, w"));
    memoryWriteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_memory_restore_on_hit"), tr("&Restore on hit"), "bpm $, 1, w"));

    //Breakpoint (remove memory) menu
    auto memoryRemove = mCommonActions->makeCommandAction(DIcon("breakpoint_remove"), tr("Remove &Memory"), "bpmc $");

    //Breakpoint menu
    auto breakpointMenu = new MenuBuilder(this);

    //Breakpoint menu
    breakpointMenu->addBuilder(new MenuBuilder(this, [ = ](QMenu * menu)
    {
        duint selectedAddr = rvaToVa(getInitialSelection());
        if(DbgGetBpxTypeAt(selectedAddr) & bp_hardware) //hardware breakpoint set
        {
            menu->addAction(hardwareRemove);
        }
        else //memory breakpoint not set
        {
            menu->addMenu(hardwareAccessMenu);
            menu->addMenu(hardwareWriteMenu);
        }

        menu->addSeparator();

        if(DbgGetBpxTypeAt(selectedAddr) & bp_memory) //memory breakpoint set
        {
            menu->addAction(memoryRemove);
        }
        else //memory breakpoint not set
        {
            menu->addMenu(memoryAccessMenu);
            menu->addMenu(memoryWriteMenu);
        }
        return true;
    }));
    mMenuBuilder->addMenu(makeMenu(DIcon("breakpoint"), tr("Brea&kpoint")), breakpointMenu);

    // Restore Selection
    mMenuBuilder->addAction(makeShortcutAction(DIcon("eraser"), tr("&Restore selection"), SLOT(undoSelectionSlot()), "ActionUndoSelection"), [this](QMenu*)
    {
        dsint start = rvaToVa(getSelectionStart());
        dsint end = rvaToVa(getSelectionEnd());
        return DbgFunctions()->PatchInRange(start, end);
    });

    //Find Pattern
    mMenuBuilder->addAction(makeShortcutAction(DIcon("search-for"), tr("&Find Pattern..."), SLOT(findPattern()), "ActionFindPattern"));

    //Follow CSP
    mMenuBuilder->addAction(makeShortcutAction(DIcon("neworigin"), ArchValue(tr("Follow E&SP"), tr("Follow R&SP")), SLOT(gotoCspSlot()), "ActionGotoOrigin"));
    mMenuBuilder->addAction(makeShortcutAction(DIcon("cbp"), ArchValue(tr("Follow E&BP"), tr("Follow R&BP")), SLOT(gotoCbpSlot()), "ActionGotoCBP"), [](QMenu*)
    {
        return DbgMemIsValidReadPtr(DbgValFromString("cbp"));
    });

    auto gotoMenu = new MenuBuilder(this);

    //Go to Expression
    gotoMenu->addAction(makeShortcutAction(DIcon("geolocation-goto"), tr("Go to &Expression"), SLOT(gotoExpressionSlot()), "ActionGotoExpression"));

    //Go to Base of Stack Frame
    gotoMenu->addAction(makeShortcutAction(DIcon("neworigin"), tr("Go to Base of Stack Frame"), SLOT(gotoFrameBaseSlot()), "ActionGotoBaseOfStackFrame"));

    //Go to Previous Frame
    gotoMenu->addAction(makeShortcutAction(DIcon("previous"), tr("Go to Previous Stack Frame"), SLOT(gotoPreviousFrameSlot()), "ActionGotoPrevStackFrame"));

    //Go to Next Frame
    gotoMenu->addAction(makeShortcutAction(DIcon("next"), tr("Go to Next Stack Frame"), SLOT(gotoNextFrameSlot()), "ActionGotoNextStackFrame"));

    //Go to Previous
    gotoMenu->addAction(makeShortcutAction(DIcon("previous"), tr("Go to Previous"), SLOT(gotoPreviousSlot()), "ActionGotoPrevious"), [this](QMenu*)
    {
        return mHistory.historyHasPrev();
    });

    //Go to Next
    gotoMenu->addAction(makeShortcutAction(DIcon("next"), tr("Go to Next"), SLOT(gotoNextSlot()), "ActionGotoNext"), [this](QMenu*)
    {
        return mHistory.historyHasNext();
    });

    mMenuBuilder->addMenu(makeMenu(DIcon("goto"), tr("&Go to")), gotoMenu);

    //Freeze the stack
    mMenuBuilder->addAction(mFreezeStack = makeShortcutAction(DIcon("freeze"), tr("Freeze the stack"), SLOT(freezeStackSlot()), "ActionFreezeStack"));
    mFreezeStack->setCheckable(true);

    //Follow in Memory Map
    mCommonActions->build(mMenuBuilder, CommonActions::ActionMemoryMap | CommonActions::ActionDump | CommonActions::ActionDumpData);

    //Follow in Stack
    auto followStackName = ArchValue(tr("Follow DWORD in &Stack"), tr("Follow QWORD in &Stack"));
    mFollowStack = makeAction(DIcon("stack"), followStackName, SLOT(followStackSlot()));
    mFollowStack->setShortcutContext(Qt::WidgetShortcut);
    mFollowStack->setShortcut(QKeySequence("enter"));
    mMenuBuilder->addAction(mFollowStack, [this](QMenu*)
    {
        duint ptr;
        if(!DbgMemRead(rvaToVa(getInitialSelection()), (unsigned char*)&ptr, sizeof(ptr)))
            return false;
        duint stackBegin = mMemPage->getBase();
        duint stackEnd = stackBegin + mMemPage->getSize();
        return ptr >= stackBegin && ptr < stackEnd;
    });

    //Follow in Disassembler
    auto disasmIcon = DIcon(ArchValue("processor32", "processor64"));
    mFollowDisasm = makeAction(disasmIcon, ArchValue(tr("&Follow DWORD in Disassembler"), tr("&Follow QWORD in Disassembler")), SLOT(followDisasmSlot()));
    mFollowDisasm->setShortcutContext(Qt::WidgetShortcut);
    mFollowDisasm->setShortcut(QKeySequence("enter"));
    mMenuBuilder->addAction(mFollowDisasm, [this](QMenu*)
    {
        duint ptr;
        return DbgMemRead(rvaToVa(getInitialSelection()), (unsigned char*)&ptr, sizeof(ptr)) && DbgMemIsValidReadPtr(ptr);
    });

    //Follow PTR in Dump
    auto followDumpName = ArchValue(tr("Follow DWORD in &Dump"), tr("Follow QWORD in &Dump"));

    mCommonActions->build(mMenuBuilder, CommonActions::ActionDumpN | CommonActions::ActionWatch);
    mMenuBuilder->addAction(makeAction("Edit columns...", SLOT(editColumnDialog())));

    mPluginMenu = new QMenu(this);
    Bridge::getBridge()->emitMenuAddToList(this, mPluginMenu, GUI_STACK_MENU);

    mMenuBuilder->addSeparator();
    mMenuBuilder->addBuilder(new MenuBuilder(this, [this](QMenu * menu)
    {
        DbgMenuPrepare(GUI_STACK_MENU);
        menu->addActions(mPluginMenu->actions());
        return true;
    }));

    mMenuBuilder->loadFromConfig();
}

void CPUStack::updateFreezeStackAction()
{
    if(bStackFrozen)
        mFreezeStack->setText(tr("Unfreeze the stack"));
    else
        mFreezeStack->setText(tr("Freeze the stack"));
    mFreezeStack->setChecked(bStackFrozen);
}

void CPUStack::getColumnRichText(duint col, duint rva, RichTextPainter::List & richText)
{
    // Compute VA
    duint va = rvaToVa(rva);

    bool activeStack = (va >= mCsp); //inactive stack

    STACK_COMMENT comment;
    RichTextPainter::CustomRichText_t curData;
    curData.underline = false;
    curData.flags = RichTextPainter::FlagColor;
    curData.textColor = mTextColor;

    if(col && mDescriptor.at(col - 1).isData == true) //paint stack data
    {
        HexDump::getColumnRichText(col, rva, richText);
        if(!activeStack)
        {
            QColor inactiveColor = ConfigColor("StackInactiveTextColor");
            for(int i = 0; i < int(richText.size()); i++)
            {
                richText[i].flags = RichTextPainter::FlagColor;
                richText[i].textColor = inactiveColor;
            }
        }
    }
    else if(col && DbgStackCommentGet(va, &comment)) //paint stack comments
    {
        if(activeStack)
        {
            if(*comment.color)
            {
                if(comment.color[0] == '!')
                {
                    if(strcmp(comment.color, "!sehclr") == 0)
                        curData.textColor = mStackSEHChainColor;
                    else if(strcmp(comment.color, "!rtnclr") == 0)
                        curData.textColor = mStackReturnToColor;
                    else
                        curData.textColor = mTextColor;
                }
                else
                    curData.textColor = QColor(QString(comment.color));
            }
            else
                curData.textColor = mTextColor;
        }
        else
            curData.textColor = ConfigColor("StackInactiveTextColor");
        curData.text = comment.comment;
        richText.push_back(curData);
    }
    else
        HexDump::getColumnRichText(col, rva, richText);
}

QString CPUStack::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    // Compute RVA
    auto bytePerRowCount = getBytePerRowCount();
    dsint rva = row * bytePerRowCount - mByteOffset;
    duint va = rvaToVa(rva);

    bool rowSelected = isSelected(rva);
    if(rowSelected) //highlight if selected
        painter->fillRect(QRect(x, y, w, h), QBrush(mSelectionColor));

    if(col == 0) // paint stack address
    {
        QColor background;
        char labelText[MAX_LABEL_SIZE] = "";
        if(DbgGetLabelAt(va, SEG_DEFAULT, labelText)) //label
        {
            if(va == mCsp) //CSP
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
            if(va == mCsp) //CSP
            {
                background = ConfigColor("StackCspBackgroundColor");
                painter->setPen(QPen(ConfigColor("StackCspColor")));
            }
            else if(rowSelected) //selected normal address
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
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, makeAddrText(va));
        return QString();
    }
    else if(col == 1) // paint stack data
    {
        if(mCallstack.size())
        {
            int stackFrameBitfield = 0; // 0:none, 1:top of stack frame, 2:bottom of stack frame, 4:middle of stack frame
            int party = 0;
            if(va >= mCallstack[0].addr)
            {
                for(size_t i = 0; i < mCallstack.size() - 1; i++)
                {
                    if(va >= mCallstack[i].addr && va < mCallstack[i + 1].addr)
                    {
                        stackFrameBitfield |= (mCallstack[i].addr == va) ? 1 : 0;
                        stackFrameBitfield |= (mCallstack[i + 1].addr == va + sizeof(duint)) ? 2 : 0;
                        if(stackFrameBitfield == 0)
                            stackFrameBitfield = 4;
                        party = mCallstack[i].party;
                        break;
                    }
                }
                // draw stack frame
                if(stackFrameBitfield == 0)
                    return HexDump::paintContent(painter, row, 1, x, y, w, h);
                else
                {
                    int height = getRowHeight();
                    int halfHeight = height / 2;
                    int width = 5;
                    int offset = 2;
                    auto result = HexDump::paintContent(painter, row, 1, x + (width - 2), y, w - (width - 2), h);
                    if(party == mod_user)
                        painter->setPen(QPen(mUserStackFrameColor, 2));
                    else
                        painter->setPen(QPen(mSystemStackFrameColor, 2));
                    if((stackFrameBitfield & 1) != 0)
                    {
                        painter->drawLine(x + width, y + halfHeight / 2, x + offset, y + halfHeight / 2);
                        painter->drawLine(x + offset, y + halfHeight / 2, x + offset, y + halfHeight);
                    }
                    else
                        painter->drawLine(x + offset, y, x + offset, y + halfHeight);
                    if((stackFrameBitfield & 2) != 0)
                    {
                        painter->drawLine(x + width, y + height / 4 * 3, x + offset, y + height / 4 * 3);
                        painter->drawLine(x + offset, y + height / 4 * 3, x + offset, y + halfHeight);
                    }
                    else
                        painter->drawLine(x + offset, y + height, x + offset, y + halfHeight);
                    return result;
                }
            }
            else
                return HexDump::paintContent(painter, row, 1, x, y, w, h);
        }
        else
            return HexDump::paintContent(painter, row, 1, x, y, w, h);
    }
    else
        return HexDump::paintContent(painter, row, col, x, y, w, h);
}

void CPUStack::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this); //create context menu
    mMenuBuilder->build(&menu);
    menu.exec(event->globalPos());
}

void CPUStack::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton || !DbgIsDebugging())
        return;
    switch(getColumnIndexFromX(event->x()))
    {
    case 0: //address
    {
        //very ugly way to calculate the base of the current row (no clue why it works)
        auto deltaRowBase = getInitialSelection() % getBytePerRowCount() + mByteOffset;
        if(deltaRowBase >= getBytePerRowCount())
            deltaRowBase -= getBytePerRowCount();
        dsint mSelectedVa = rvaToVa(getInitialSelection() - deltaRowBase);
        if(mRvaDisplayEnabled && mSelectedVa == mRvaDisplayBase)
        {
            mRvaDisplayEnabled = false;
        }
        else
        {
            mRvaDisplayEnabled = true;
            mRvaDisplayBase = mSelectedVa;
            mRvaDisplayPageBase = mMemPage->getBase();
        }
        reloadData();
    }
    break;

    case 1: // value
    {
        modifySlot();
    }
    break;

    default:
    {
        duint va = rvaToVa(getInitialSelection());
        STACK_COMMENT comment;
        if(DbgStackCommentGet(va, &comment) && strcmp(comment.color, "!rtnclr") == 0)
            followDisasmSlot();
    }
    break;
    }
}

void CPUStack::wheelEvent(QWheelEvent* event)
{
    if(event->modifiers() == Qt::NoModifier)
        AbstractTableView::wheelEvent(event);
    else if(event->modifiers() == Qt::ControlModifier) // Zoom
        Config()->zoomFont("Stack", event);
}

void CPUStack::stackDumpAt(duint addr, duint csp)
{
    if(DbgMemIsValidReadPtr(addr))
        mHistory.addVaToHistory(addr);
    mCsp = csp;

    // Get the callstack
    DBGCALLSTACK callstack;
    memset(&callstack, 0, sizeof(DBGCALLSTACK));
    DbgFunctions()->GetCallStack(&callstack);
    mCallstack.resize(callstack.total);
    if(mCallstack.size())
    {
        // callstack data highest >> lowest
        std::qsort(callstack.entries, callstack.total, sizeof(DBGCALLSTACKENTRY), [](const void* a, const void* b)
        {
            auto p = (const DBGCALLSTACKENTRY*)a;
            auto q = (const DBGCALLSTACKENTRY*)b;
            if(p->addr < q->addr)
                return -1;
            else
                return 1;
        });
        for(size_t i = 0; i < mCallstack.size(); i++)
        {
            mCallstack[i].addr = callstack.entries[i].addr;
            mCallstack[i].party = DbgFunctions()->ModGetParty(callstack.entries[i].to);
        }
        BridgeFree(callstack.entries);
    }
    bool isInvisible;
    isInvisible = (addr < mMemPage->va(getTableOffsetRva())) || (addr >= mMemPage->va(getTableOffsetRva() + getViewableRowsCount() * getBytePerRowCount()));

    printDumpAt(addr, true, true, isInvisible || addr == csp);
}

void CPUStack::updateSlot()
{
    if(!DbgIsDebugging())
        return;
    // Get the callstack
    DBGCALLSTACK callstack;
    memset(&callstack, 0, sizeof(DBGCALLSTACK));
    DbgFunctions()->GetCallStack(&callstack);
    mCallstack.resize(callstack.total);
    if(mCallstack.size())
    {
        // callstack data highest >> lowest
        std::qsort(callstack.entries, callstack.total, sizeof(DBGCALLSTACKENTRY), [](const void* a, const void* b)
        {
            auto p = (const DBGCALLSTACKENTRY*)a;
            auto q = (const DBGCALLSTACKENTRY*)b;
            if(p->addr < q->addr)
                return -1;
            else
                return 1;
        });
        for(size_t i = 0; i < mCallstack.size(); i++)
        {
            mCallstack[i].addr = callstack.entries[i].addr;
            mCallstack[i].party = DbgFunctions()->ModGetParty(callstack.entries[i].to);
        }
        BridgeFree(callstack.entries);
    }
}

void CPUStack::disasmSelectionChanged(duint parVA)
{
    // When the selected instruction is changed, select the argument that is in the stack.
    DISASM_INSTR instr;
    if(!DbgIsDebugging() || !DbgMemIsValidReadPtr(parVA))
        return;
    DbgDisasmAt(parVA, &instr);

    duint underlineStart = 0;
    duint underlineEnd = 0;

    for(int i = 0; i < instr.argcount; i++)
    {
        const DISASM_ARG & arg = instr.arg[i];
        if(arg.type == arg_memory)
        {
            if(arg.value >= mMemPage->getBase() && arg.value < mMemPage->getBase() + mMemPage->getSize())
            {
                if(Config()->getBool("Gui", "AutoFollowInStack"))
                {
                    //TODO: When the stack is unaligned?
                    stackDumpAt(arg.value & (~(sizeof(void*) - 1)), mCsp);
                }
                else
                {
                    BASIC_INSTRUCTION_INFO info;
                    DbgDisasmFastAt(parVA, &info);
                    underlineStart = arg.value;
                    underlineEnd = mUnderlineRangeStartVa + info.memory.size - 1;
                }
                break;
            }
        }
    }

    if(mUnderlineRangeStartVa != underlineStart || mUnderlineRangeEndVa != underlineEnd)
    {
        mUnderlineRangeStartVa = underlineStart;
        mUnderlineRangeEndVa = underlineEnd;
        reloadData();
    }
}

void CPUStack::gotoCspSlot()
{
    DbgCmdExec("sdump csp");
}

void CPUStack::gotoCbpSlot()
{
    DbgCmdExec("sdump cbp");
}

int CPUStack::getCurrentFrame(const std::vector<CPUStack::CPUCallStack> & mCallstack, duint va)
{
    if(mCallstack.size())
        for(size_t i = 0; i < mCallstack.size() - 1; i++)
            if(va >= mCallstack[i].addr && va < mCallstack[i + 1].addr)
                return int(i);
    return -1;
}

void CPUStack::gotoFrameBaseSlot()
{
    int frame = getCurrentFrame(mCallstack, rvaToVa(getInitialSelection()));
    if(frame != -1)
        DbgCmdExec(QString("sdump \"%1\"").arg(ToPtrString(mCallstack[frame].addr)));
}

void CPUStack::gotoNextFrameSlot()
{
    int frame = getCurrentFrame(mCallstack, rvaToVa(getInitialSelection()));
    if(frame != -1 && frame + 1 < int(mCallstack.size()))
        DbgCmdExec(QString("sdump \"%1\"").arg(ToPtrString(mCallstack[frame + 1].addr)));
}

void CPUStack::gotoPreviousFrameSlot()
{
    int frame = getCurrentFrame(mCallstack, rvaToVa(getInitialSelection()));
    if(frame > 0)
        DbgCmdExec(QString("sdump \"%1\"").arg(ToPtrString(mCallstack[frame - 1].addr)));
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
    mGoto->setInitialExpression(ToPtrString(rvaToVa(getInitialSelection())));
    if(mGoto->exec() == QDialog::Accepted)
    {
        duint value = DbgValFromString(mGoto->expressionText.toUtf8().constData());
        DbgCmdExec(QString().sprintf("sdump %p", value));
    }
}

void CPUStack::selectionGet(SELECTIONDATA* selection)
{
    selection->start = rvaToVa(getSelectionStart());
    selection->end = rvaToVa(getSelectionEnd());
    Bridge::getBridge()->setResult(BridgeResult::SelectionGet, 1);
}

void CPUStack::selectionSet(const SELECTIONDATA* selection)
{
    dsint selMin = mMemPage->getBase();
    dsint selMax = selMin + mMemPage->getSize();
    dsint start = selection->start;
    dsint end = selection->end;
    if(start < selMin || start >= selMax || end < selMin || end >= selMax) //selection out of range
    {
        Bridge::getBridge()->setResult(BridgeResult::SelectionSet, 0);
        return;
    }
    setSingleSelection(start - selMin);
    expandSelectionUpTo(end - selMin);
    reloadData();
    Bridge::getBridge()->setResult(BridgeResult::SelectionSet, 1);
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
                disconnect(SIGNAL(enterPressedSignal()));
                connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followStackSlot()));
                mFollowDisasm->setShortcut(QKeySequence(""));
                mFollowStack->setShortcut(QKeySequence("enter"));
            }
            else
            {
                disconnect(SIGNAL(enterPressedSignal()));
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
            QString addrText = ToPtrString(selectedData);
            DbgCmdExec(QString("disasm " + addrText));
        }
}

void CPUStack::followStackSlot()
{
    duint selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText = ToPtrString(selectedData);
            DbgCmdExec(QString("sdump " + addrText));
        }
}

void CPUStack::binaryEditSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.showKeepSize(true);
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
    hexEdit.setWindowTitle(tr("Fill data at %1").arg(ToPtrString(rvaToVa(selStart))));
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

void CPUStack::findPattern()
{
    HexEditDialog hexEdit(this);
    hexEdit.isDataCopiable(false);
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle(tr("Find Pattern..."));
    if(hexEdit.exec() != QDialog::Accepted)
        return;

    dsint addr = rvaToVa(getSelectionStart());
    addr = DbgMemFindBaseAddr(addr, 0);

    QString addrText = ToPtrString(addr);
    DbgCmdExec(QString("findall " + addrText + ", " + hexEdit.mHexEdit->pattern() + ", &data&"));
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
    WordEditDialog editDialog(this);
    dsint value = 0;
    mMemPage->read(&value, addr, sizeof(dsint));
    editDialog.setup(tr("Modify"), value, sizeof(dsint));
    if(editDialog.exec() != QDialog::Accepted)
        return;
    value = editDialog.getVal();
    mMemPage->write(&value, addr, sizeof(dsint));
    GuiUpdateAllViews();
}

void CPUStack::realignSlot()
{
#ifdef _WIN64
    mCsp &= ~0x7;
#else //x86
    mCsp &= ~0x3;
#endif //_WIN64
    DbgValToString("csp", mCsp);
    GuiUpdateAllViews();
}

void CPUStack::freezeStackSlot()
{
    if(bStackFrozen)
        DbgCmdExec(QString("setfreezestack 0"));
    else
        DbgCmdExec(QString("setfreezestack 1"));

    bStackFrozen = !bStackFrozen;

    updateFreezeStackAction();
    if(!bStackFrozen)
        gotoCspSlot();
}

void CPUStack::dbgStateChangedSlot(DBGSTATE state)
{
    if(state == initialized)
    {
        bStackFrozen = false;
        updateFreezeStackAction();
    }
}

void CPUStack::copyPtrColumnSlot()
{
    const duint wordSize = sizeof(duint);
    dsint selStart = getSelectionStart();
    dsint selLen = getSelectionEnd() - selStart + 1;
    duint wordCount = selLen / wordSize;

    duint* data = new duint[wordCount];
    mMemPage->read((byte_t*)data, selStart, wordCount * wordSize);

    QString clipboard;
    for(duint i = 0; i < wordCount; i++)
    {
        if(i > 0)
            clipboard += "\r\n";
        clipboard += ToPtrString(data[i]);
    }
    delete [] data;

    Bridge::CopyToClipboard(clipboard);
}

void CPUStack::copyCommentsColumnSlot()
{
    int commentsColumn = 2;
    const duint wordSize = sizeof(duint);
    dsint selStart = getSelectionStart();
    dsint selLen = getSelectionEnd() - selStart + 1;

    QString clipboard;
    for(dsint i = 0; i < selLen; i += wordSize)
    {
        RichTextPainter::List richText;
        getColumnRichText(commentsColumn, selStart + i, richText);
        QString colText;
        for(auto & r : richText)
            colText += r.text;

        if(i > 0)
            clipboard += "\r\n";
        clipboard += colText;
    }

    Bridge::CopyToClipboard(clipboard);
}
