#include "TraceStack.h"
#include "TraceDump.h"
#include "TraceFileReader.h"
#include "TraceFileDump.h"
#include "TraceBrowser.h"
#include "CPUDump.h"
#include <QClipboard>
#include "Configuration.h"
#include "Bridge.h"
#include "CommonActions.h"
#include "HexEditDialog.h"
#include "WordEditDialog.h"
#include "CPUMultiDump.h"
#include "GotoDialog.h"

TraceStack::TraceStack(Architecture* architecture, TraceBrowser* disas, TraceFileDumpMemoryPage* memoryPage, QWidget* parent)
    : mMemoryPage(memoryPage), mDisas(disas), HexDump(architecture, parent, memoryPage)
{
    setWindowTitle("Stack");
    setDrawDebugOnly(false);
    setShowHeader(false);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

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
    //connect(this, SIGNAL(selectionUpdated()), this, SLOT(selectionUpdatedSlot()));

    Initialize();
}

void TraceStack::updateColors()
{
    HexDump::updateColors();

    mBackgroundColor = ConfigColor("StackBackgroundColor");
    mTextColor = ConfigColor("StackTextColor");
    mSelectionColor = ConfigColor("StackSelectionColor");
    //mStackReturnToColor = ConfigColor("StackReturnToColor");
    //mStackSEHChainColor = ConfigColor("StackSEHChainColor");
    //mUserStackFrameColor = ConfigColor("StackFrameColor");
    //mSystemStackFrameColor = ConfigColor("StackFrameSystemColor");
}

void TraceStack::updateFonts()
{
    setFont(ConfigFont("Stack"));
    invalidateCachedFont();
}

void TraceStack::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this, [this](QMenu*)
    {
        return mMemoryPage->isAvailable();
    });
    mCommonActions = new CommonActions(this, getActionHelperFuncs(), [this]()
    {
        return rvaToVa(getSelectionStart());
    });

    auto binaryMenu = new MenuBuilder(this);

    //Binary->Copy
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_copy"), tr("&Copy"), SLOT(binaryCopySlot()), "ActionBinaryCopy"));

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

    //TODO: Find Pattern

    //Follow CSP
    mMenuBuilder->addAction(makeShortcutAction(DIcon("neworigin"), ArchValue(tr("Follow E&SP"), tr("Follow R&SP")), SLOT(gotoCspSlot()), "ActionGotoOrigin"));
    mMenuBuilder->addAction(makeShortcutAction(DIcon("cbp"), ArchValue(tr("Follow E&BP"), tr("Follow R&BP")), SLOT(gotoCbpSlot()), "ActionGotoCBP"));

    auto gotoMenu = new MenuBuilder(this);

    //Go to Expression
    gotoMenu->addAction(makeShortcutAction(DIcon("geolocation-goto"), tr("Go to &Expression"), SLOT(gotoExpressionSlot()), "ActionGotoExpression"));

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

    mMenuBuilder->addAction(makeShortcutAction(DIcon("xrefs"), tr("xrefs..."), SLOT(gotoXrefSlot()), "ActionXrefs"));

    //Freeze the stack
    //mMenuBuilder->addAction(mFreezeStack = makeShortcutAction(DIcon("freeze"), tr("Freeze the stack"), SLOT(freezeStackSlot()), "ActionFreezeStack"));
    //mFreezeStack->setCheckable(true);

    //Follow in Memory Map
    //mCommonActions->build(mMenuBuilder, CommonActions::ActionMemoryMap | CommonActions::ActionDump | CommonActions::ActionDumpData);

    //Follow in Stack
    //auto followStackName = ArchValue(tr("Follow DWORD in &Stack"), tr("Follow QWORD in &Stack"));
    //mFollowStack = makeAction(DIcon("stack"), followStackName, SLOT(followStackSlot()));
    //mFollowStack->setShortcutContext(Qt::WidgetShortcut);
    //mFollowStack->setShortcut(QKeySequence("enter"));
    //mMenuBuilder->addAction(mFollowStack, [this](QMenu*)
    //{
    //    duint ptr;
    //    if(!DbgMemRead(rvaToVa(getInitialSelection()), (unsigned char*)&ptr, sizeof(ptr)))
    //        return false;
    //    duint stackBegin = mMemPage->getBase();
    //    duint stackEnd = stackBegin + mMemPage->getSize();
    //    return ptr >= stackBegin && ptr < stackEnd;
    //});

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

    mMenuBuilder->addAction(makeAction("Edit columns...", SLOT(editColumnDialog())));

    mMenuBuilder->loadFromConfig();
    disconnect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChanged(DBGSTATE)));
    updateShortcuts();
}

//void TraceStack::updateFreezeStackAction()
//{
//    if(bStackFrozen)
//        mFreezeStack->setText(tr("Unfreeze the stack"));
//    else
//        mFreezeStack->setText(tr("Freeze the stack"));
//    mFreezeStack->setChecked(bStackFrozen);
//}

void TraceStack::getColumnRichText(duint col, duint rva, RichTextPainter::List & richText)
{
    // Compute VA
    duint va = rvaToVa(rva);

    bool activeStack = (va >= mCsp); //inactive stack

    //STACK_COMMENT comment;
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
    else if(col) //TODO: paint stack comments
    {
        /*if(activeStack)
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
            else*/
        curData.textColor = mTextColor;
        /*}
        else
            curData.textColor = ConfigColor("StackInactiveTextColor");*/
        curData.text = "";
        richText.push_back(curData);
    }
    else
        HexDump::getColumnRichText(col, rva, richText);
}

void TraceStack::reloadData()
{
    mCsp = mDisas->getTraceFile()->Registers(mDisas->getInitialSelection()).regcontext.csp;
    HexDump::reloadData();
}

QString TraceStack::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
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
    else // TODO: Print call stack
        return HexDump::paintContent(painter, row, col, x, y, w, h);
}

void TraceStack::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this); //create context menu
    mMenuBuilder->build(&menu);
    menu.exec(event->globalPos());
}

void TraceStack::mouseDoubleClickEvent(QMouseEvent* event)
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
        //modifySlot();
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

void TraceStack::wheelEvent(QWheelEvent* event)
{
    if(event->modifiers() == Qt::NoModifier)
        AbstractTableView::wheelEvent(event);
    else if(event->modifiers() == Qt::ControlModifier) // Zoom
        Config()->zoomFont("Stack", event);
}

void TraceStack::stackDumpAt(duint addr, duint csp)
{
    if(DbgMemIsValidReadPtr(addr))
        mHistory.addVaToHistory(addr);
    mCsp = csp;

    // Get the callstack
    //DBGCALLSTACK callstack;
    //memset(&callstack, 0, sizeof(DBGCALLSTACK));
    //DbgFunctions()->GetCallStack(&callstack);
    //mCallstack.resize(callstack.total);
    //if(mCallstack.size())
    //{
    // callstack data highest >> lowest
    //std::qsort(callstack.entries, callstack.total, sizeof(DBGCALLSTACKENTRY), [](const void* a, const void* b)
    //{
    //auto p = (const DBGCALLSTACKENTRY*)a;
    //auto q = (const DBGCALLSTACKENTRY*)b;
    //if(p->addr < q->addr)
    //return -1;
    //else
    //return 1;
    //});
    //for(size_t i = 0; i < mCallstack.size(); i++)
    //{
    //mCallstack[i].addr = callstack.entries[i].addr;
    //mCallstack[i].party = DbgFunctions()->ModGetParty(callstack.entries[i].to);
    //}
    //BridgeFree(callstack.entries);
    //}
    bool isInvisible;
    isInvisible = (addr < mMemPage->va(getTableOffsetRva())) || (addr >= mMemPage->va(getTableOffsetRva() + getViewableRowsCount() * getBytePerRowCount()));

    printDumpAt(addr, true, true, isInvisible || addr == csp);
}

void TraceStack::updateSlot()
{
    if(!DbgIsDebugging())
        return;
    // Get the callstack
    DBGCALLSTACK callstack;
    memset(&callstack, 0, sizeof(DBGCALLSTACK));
    DbgFunctions()->GetCallStack(&callstack);
    //mCallstack.resize(callstack.total);
    //if(mCallstack.size())
    //{
    // callstack data highest >> lowest
    //std::qsort(callstack.entries, callstack.total, sizeof(DBGCALLSTACKENTRY), [](const void* a, const void* b)
    //{
    //auto p = (const DBGCALLSTACKENTRY*)a;
    //auto q = (const DBGCALLSTACKENTRY*)b;
    //if(p->addr < q->addr)
    //return -1;
    //else
    //return 1;
    //});
    //for(size_t i = 0; i < mCallstack.size(); i++)
    //{
    //mCallstack[i].addr = callstack.entries[i].addr;
    //mCallstack[i].party = DbgFunctions()->ModGetParty(callstack.entries[i].to);
    //}
    //BridgeFree(callstack.entries);
    //}
}

void TraceStack::printDumpAt(duint parVA, bool select, bool repaint, bool updateTableOffset)
{
    // Modified from Hexdump, removed memory page information
    // TODO: get memory range from trace instead
    const duint wSize = 0x1000;  // TODO: Using 4KB pages currently
    auto wBase = mMemoryPage->getBase();
    dsint wRVA = parVA - wBase; //calculate rva
    if(wRVA < 0 || wRVA >= wSize)
    {
        wBase = parVA & ~(wSize - 1);
        mMemoryPage->setAttributes(wBase, wSize);
        wRVA = parVA - wBase; //calculate rva
    }
    int wBytePerRowCount = getBytePerRowCount(); //get the number of bytes per row
    dsint wRowCount;

    // Byte offset used to be aligned on the given RVA
    mByteOffset = (int)((dsint)wRVA % (dsint)wBytePerRowCount);
    mByteOffset = mByteOffset > 0 ? wBytePerRowCount - mByteOffset : 0;

    // Compute row count
    wRowCount = wSize / wBytePerRowCount;
    wRowCount += mByteOffset > 0 ? 1 : 0;

    //if(mRvaDisplayEnabled && mMemPage->getBase() != mRvaDisplayPageBase)
    //    mRvaDisplayEnabled = false;

    setRowCount(wRowCount); //set the number of rows

    if(updateTableOffset)
    {
        setTableOffset(-1); //make sure the requested address is always first
        setTableOffset((wRVA + mByteOffset) / wBytePerRowCount); //change the displayed offset
    }

    if(select)
    {
        setSingleSelection(wRVA);
        dsint wEndingAddress = wRVA + getSizeOf(mDescriptor.at(0).data.itemSize) - 1;
        expandSelectionUpTo(wEndingAddress);
    }

    if(repaint)
        reloadData();
}

void TraceStack::disasmSelectionChanged(duint parVA)
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

// TODO
void TraceStack::gotoCspSlot()
{
    if(getInitialSelection() != mCsp)
        stackDumpAt(mCsp, mCsp);
}

void TraceStack::gotoCbpSlot()
{
    stackDumpAt(mDisas->getTraceFile()->Registers(mDisas->getInitialSelection()).regcontext.cbp, mCsp);
}

void TraceStack::gotoExpressionSlot()
{
    if(!mMemoryPage->isAvailable())
        return;
    if(!mGoto)
        mGoto = new GotoDialog(this, false, true, true);
    //TODO: Address validation doesn't work for now
    mGoto->setWindowTitle(tr("Enter expression to follow in Stack..."));
    mGoto->setInitialExpression(ToPtrString(rvaToVa(getInitialSelection())));
    if(mGoto->exec() == QDialog::Accepted)
    {
        duint value = DbgValFromString(mGoto->expressionText.toUtf8().constData());
        stackDumpAt(value, mCsp);
    }
}

//void TraceStack::selectionUpdatedSlot()
//{
//    duint selectedData;
//    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
//        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
//        {
//            duint stackBegin = mMemPage->getBase();
//            duint stackEnd = stackBegin + mMemPage->getSize();
//            if(selectedData >= stackBegin && selectedData < stackEnd) //data is a pointer to stack address
//            {
//                disconnect(SIGNAL(enterPressedSignal()));
//                connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followStackSlot()));
//                mFollowDisasm->setShortcut(QKeySequence(""));
//                mFollowStack->setShortcut(QKeySequence("enter"));
//            }
//            else
//            {
//                disconnect(SIGNAL(enterPressedSignal()));
//                connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followDisasmSlot()));
//                mFollowStack->setShortcut(QKeySequence(""));
//                mFollowDisasm->setShortcut(QKeySequence("enter"));
//            }
//        }
//}

void TraceStack::followDisasmSlot()
{
    duint selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
        if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        {
            QString addrText = ToPtrString(selectedData);
            DbgCmdExec(QString("disasm " + addrText));
        }
}

void TraceStack::followStackSlot()
{
    duint selectedData;
    if(mMemPage->read((byte_t*)&selectedData, getInitialSelection(), sizeof(duint)))
        //if(DbgMemIsValidReadPtr(selectedData)) //data is a pointer
        //{
        stackDumpAt(selectedData, mCsp);
    //}
}

void TraceStack::gotoXrefSlot()
{
    emit xrefSignal(rvaToVa(getInitialSelection()));
}

void TraceStack::binaryCopySlot()
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

void TraceStack::copyPtrColumnSlot()
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

void TraceStack::copyCommentsColumnSlot()
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
