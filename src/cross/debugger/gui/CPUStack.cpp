#include "CPUStack.h"

#include <vector>

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>

#include <Disassembler/QZydis.h>
#include <Gui/WordEditDialog.h>
#include <Memory/MemoryPage.h>
#include "Configuration.h"
#include "StringUtil.h"

CPUStack::CPUStack(Architecture* architecture, DbgAdapter* adapter, QWidget* parent)
    : HexDump(architecture, parent)
    , mDisasm(std::make_unique<QZydis>(MAX_MODULE_SIZE, architecture))
    , mAdapter(adapter)
{
    setWindowTitle("Stack");
    setShowHeader(false);

    CPUStack::updateColors();

    setupColumns();
    setupContextMenu();

    connect(mAdapter, &DbgAdapter::registersUpdated,
            this, &CPUStack::onRegistersUpdated,
            Qt::QueuedConnection);

    connect(mAdapter, &DbgAdapter::processCreated,
            this, &CPUStack::onProcessStarted,
            Qt::QueuedConnection);

    connect(mAdapter, &DbgAdapter::sessionEnded,
            this, &CPUStack::onSessionEnded,
            Qt::QueuedConnection);
}

void CPUStack::setupColumns()
{
    const int charwidth = getCharWidth();

    mForceColumn = 1;

    ColumnDescriptor colDesc{};

    colDesc.isData = true;
    colDesc.itemCount = 1;
    colDesc.separator = 0;
    if constexpr(sizeof(duint) == 8)
    {
        colDesc.data.itemSize = Qword;
        colDesc.data.qwordMode = HexQword;
    }
    else
    {
        colDesc.data.itemSize = Dword;
        colDesc.data.dwordMode = HexDword;
    }
    appendDescriptor(24 + charwidth * 2 * sizeof(duint), "void*", false, colDesc);

    colDesc.isData = false;
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    appendDescriptor(2000, tr("Comments"), false, colDesc);
}

void CPUStack::setupContextMenu()
{
    mContextMenu = new QMenu(this);

    mRealignAction = mContextMenu->addAction(tr("Align Stack Pointer"), this, &CPUStack::realignSlot);
    mModifyAction = mContextMenu->addAction(tr("Modify"), this, &CPUStack::modifySlot);

    mContextMenu->addSeparator();

    mGotoCspAction = mContextMenu->addAction(tr("Go to RSP"), this, &CPUStack::gotoCspSlot);
    mGotoCbpAction = mContextMenu->addAction(tr("Go to RBP"), this, &CPUStack::gotoCbpSlot);

    mContextMenu->addSeparator();

    mFollowDisasmAction = mContextMenu->addAction(tr("Follow in Disassembly"), this, &CPUStack::followDisasmSlot);

    QMenu* copyMenu = mContextMenu->addMenu(tr("&Copy"));
    copyMenu->addAction(tr("&Selection"), this, &HexDump::copySelectionSlot);
    copyMenu->addAction(tr("&Address"), this, &HexDump::copyAddressSlot);
    const QString ptrName = sizeof(duint) == 8 ? tr("&QWORD") : tr("&DWORD");
    copyMenu->addAction(ptrName, this, &CPUStack::copyPtrColumnSlot);
    copyMenu->addAction(tr("&Comments"), this, &CPUStack::copyCommentsColumnSlot);

    mContextMenu->addSeparator();

    mFreezeAction = mContextMenu->addAction(tr("Freeze Stack"), this, &CPUStack::freezeStackSlot);
    mFreezeAction->setCheckable(true);
}

void CPUStack::updateColors()
{
    HexDump::updateColors();

    mBackgroundColor = ConfigColor("StackBackgroundColor");
    mTextColor = ConfigColor("StackTextColor");
    mSelectionColor = ConfigColor("StackSelectionColor");
    mStackReturnToColor = ConfigColor("StackReturnToColor");
    mStackCspColor = ConfigColor("StackCspColor");
    mStackCspBackgroundColor = ConfigColor("StackCspBackgroundColor");
    mStackAddressColor = ConfigColor("StackAddressColor");
    mStackAddressBackgroundColor = ConfigColor("StackAddressBackgroundColor");
    mStackSelectedAddressColor = ConfigColor("StackSelectedAddressColor");
    mStackSelectedAddressBackgroundColor = ConfigColor("StackSelectedAddressBackgroundColor");
    mStackInactiveTextColor = ConfigColor("StackInactiveTextColor");
}

void CPUStack::refreshActionState() const
{
    const bool debugging = mCsp != 0;

    mRealignAction->setEnabled(debugging && (mCsp & (sizeof(duint) - 1)) != 0);
    mModifyAction->setEnabled(debugging);
    mGotoCspAction->setEnabled(debugging);
    mGotoCbpAction->setEnabled(debugging && mCbp != 0);
    mFollowDisasmAction->setEnabled(debugging);
    mFreezeAction->setEnabled(debugging);
    mFreezeAction->setChecked(mStackFrozen);
}

void CPUStack::stackDumpAt(const duint addr, const duint csp)
{
    mCsp = csp;

    const duint viewStartVa = mMemPage->va(getTableOffsetRva());
    const duint viewEndVa = mMemPage->va(getTableOffsetRva() + getViewableRowsCount() * getBytePerRowCount());
    const bool isInvisible = (addr < viewStartVa) || (addr >= viewEndVa);

    // Window the page around addr so scrolling a few rows doesn't refetch.
    constexpr duint kWindowHalf = 0x1000;
    const duint windowBase = addr > kWindowHalf ? addr - kWindowHalf : 0;
    mMemPage->setAttributes(windowBase, kWindowHalf * 2);

    printDumpAt(addr, true, true, isInvisible || addr == csp);
}

void CPUStack::onRegistersUpdated(const REGDUMP & regs)
{
    mCbp = regs.regcontext.cbp;

    if(mStackFrozen && regs.regcontext.csp != 0)
    {
        // Keep mCsp live so "Go to RSP" still targets the live value.
        mCsp = regs.regcontext.csp;
        return;
    }

    stackDumpAt(regs.regcontext.csp, regs.regcontext.csp);
}

void CPUStack::onProcessStarted()
{
    mStackFrozen = false;
    if(mFreezeAction)
        mFreezeAction->setChecked(false);
    if(mCsp != 0)
        stackDumpAt(mCsp, mCsp);
}

void CPUStack::onSessionEnded()
{
    mCsp = 0;
    mCbp = 0;
    mStackFrozen = false;
    if(mFreezeAction)
        mFreezeAction->setChecked(false);
    reloadData();
}

void CPUStack::gotoCspSlot()
{
    if(mCsp != 0)
        stackDumpAt(mCsp, mCsp);
}

void CPUStack::gotoCbpSlot()
{
    if(mCbp != 0)
        stackDumpAt(mCbp, mCsp);
}

void CPUStack::followDisasmSlot()
{
    const duint selection = getInitialSelection();
    const duint alignedRva = selection - (selection % sizeof(duint));

    duint ptr = 0;
    if(!mMemPage->read(&ptr, alignedRva, sizeof(duint)))
        return;

    if(!mAdapter->isCodePtr(ptr))
        return;

    emit followDisasmRequested(ptr);
}

void CPUStack::freezeStackSlot()
{
    mStackFrozen = !mStackFrozen;
    mFreezeAction->setChecked(mStackFrozen);
}

void CPUStack::realignSlot() const
{
    const duint aligned = mCsp & ~static_cast<duint>(sizeof(duint) - 1);
    mAdapter->writeRegister("csp", aligned);
}

void CPUStack::modifySlot()
{
    const duint selection = getInitialSelection();
    const duint alignedRva = selection - (selection % sizeof(duint));

    duint currentValue = 0;
    if(!mMemPage->read(&currentValue, alignedRva, sizeof(duint)))
        return;

    WordEditDialog editDialog(this);
    editDialog.setup(tr("Modify"), currentValue, sizeof(duint));
    if(editDialog.exec() != QDialog::Accepted)
        return;

    const duint newValue = editDialog.getVal();
    mMemPage->write(&newValue, alignedRva, sizeof(duint));
    GuiUpdateAllViews();
}

void CPUStack::copyPtrColumnSlot() const
{
    constexpr duint wordSize = sizeof(duint);
    const dsint selStart = getSelectionStart();
    const dsint selLen = getSelectionEnd() - selStart + 1;
    const duint wordCount = selLen / wordSize;
    if(wordCount == 0)
        return;

    std::vector<duint> data(wordCount);
    mMemPage->read(data.data(), selStart, wordCount * wordSize);

    QString clipboard;
    for(duint i = 0; i < wordCount; i++)
    {
        if(i > 0)
            clipboard += "\r\n";
        clipboard += ToPtrString(data[i]);
    }
    Bridge::CopyToClipboard(clipboard);
}

void CPUStack::copyCommentsColumnSlot() const
{
    constexpr duint wordSize = sizeof(duint);
    const dsint selStart = getSelectionStart();
    const dsint selLen = getSelectionEnd() - selStart + 1;

    QString clipboard;
    for(dsint i = 0; i < selLen; i += wordSize)
    {
        constexpr duint commentsColumn = 2;
        RichTextPainter::List richText;
        getColumnRichText(commentsColumn, selStart + i, richText);
        QString colText;
        for(const auto & r : richText)
            colText += r.text;

        if(i > 0)
            clipboard += "\r\n";
        clipboard += colText;
    }
    Bridge::CopyToClipboard(clipboard);
}

void CPUStack::contextMenuEvent(QContextMenuEvent* event)
{
    refreshActionState();
    mContextMenu->popup(event->globalPos());
}

void CPUStack::mouseDoubleClickEvent(QMouseEvent* event)
{
    HexDump::mouseDoubleClickEvent(event);

    if(event->button() != Qt::LeftButton || !DbgIsDebugging())
        return;

    switch(getColumnIndexFromX(event->pos().x()))
    {
    case 0: // address column - no action (?? - cross has no RVA display toggle)
        break;
    case 1: // void* - edit the slot
        modifySlot();
        break;
    default: // comment - follow into disassembly only if this is a return-to slot
    {
        const duint selection = getInitialSelection();
        const duint alignedRva = selection - (selection % sizeof(duint));
        QString commentText;
        bool isReturnTo = false;
        if(resolveSlotComment(alignedRva, commentText, isReturnTo) && isReturnTo)
            followDisasmSlot();
        break;
    }
    }
}

QString CPUStack::paintContent(QPainter* painter, const duint row, const duint column, const int x, const int y,
                               const int w, const int h)
{
    const auto bytePerRowCount = getBytePerRowCount();
    const dsint rva = static_cast<dsint>(row) * static_cast<dsint>(bytePerRowCount) - mByteOffset;
    const duint va = rvaToVa(static_cast<duint>(rva));
    const bool rowSelected = isSelected(rva);

    if(rowSelected)
        painter->fillRect(QRect(x, y, w, h), QBrush(mSelectionColor));

    if(column == 0)
    {
        QColor background;
        QColor textColor;
        if(va == mCsp && mCsp != 0)
        {
            background = mStackCspBackgroundColor;
            textColor = mStackCspColor;
        }
        else if(rowSelected)
        {
            background = mStackSelectedAddressBackgroundColor;
            textColor = mStackSelectedAddressColor;
        }
        else
        {
            background = mStackAddressBackgroundColor;
            textColor = mStackAddressColor;
        }
        if(background.alpha())
            painter->fillRect(QRect(x, y, w, h), QBrush(background));
        painter->setPen(QPen(textColor));
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, makeAddrText(va));
        return {};
    }

    return HexDump::paintContent(painter, row, column, x, y, w, h);
}

void CPUStack::getColumnRichText(const duint col, const duint rva, RichTextPainter::List & richText) const
{
    // Mirrors src/gui/Src/Gui/CPUStack.cpp::getColumnRichText.
    const duint va = rvaToVa(rva);
    const bool activeStack = (va >= mCsp);

    RichTextPainter::CustomRichText_t curData;
    curData.underline = false;
    curData.flags = RichTextPainter::FlagColor;
    curData.textColor = mTextColor;

    if(col && mDescriptor.at(col - 1).isData)
    {
        HexDump::getColumnRichText(col, rva, richText);
        if(!activeStack)
        {
            for(auto & rt : richText)
            {
                rt.flags = RichTextPainter::FlagColor;
                rt.textColor = mStackInactiveTextColor;
            }
        }
        return;
    }

    QString commentText;
    bool isReturnTo = false;
    if(col && resolveSlotComment(rva, commentText, isReturnTo))
    {
        if(activeStack)
            curData.textColor = isReturnTo ? mStackReturnToColor : mTextColor;
        else
            curData.textColor = mStackInactiveTextColor;
        curData.text = commentText;
        richText.push_back(curData);
        return;
    }

    HexDump::getColumnRichText(col, rva, richText);
}

bool CPUStack::resolveSlotComment(const duint rva, QString & out, bool & isReturnTo) const
{
    isReturnTo = false;

    duint ptr = 0;
    if(!mMemPage->read(&ptr, rva, sizeof(duint)))
        return false;

    char modName[MAX_MODULE_SIZE] = {};
    if(!DbgFunctions()->ModNameFromAddr(ptr, modName, false))
        return false;

    duint fromAddr;
    isReturnTo = resolveReturnTo(ptr, fromAddr);

    const QString mod = QString::fromUtf8(modName);
    const QString addr = QString("%1").arg(ptr, sizeof(duint) * 2, 16, QLatin1Char('0')).toUpper();

    if(!isReturnTo)
    {
        out = QString("%1.%2").arg(mod, addr);
        return true;
    }

    QString fromPart = QStringLiteral("???");
    if(fromAddr != 0)
    {
        const QString fromAddrStr = QString("%1").arg(fromAddr, sizeof(duint) * 2, 16, QLatin1Char('0')).toUpper();
        fromPart = fromAddrStr;
        char fromModName[MAX_MODULE_SIZE] = {};
        if(DbgFunctions()->ModNameFromAddr(fromAddr, fromModName, false))
            fromPart = QString("%1.%2").arg(QString::fromUtf8(fromModName), fromAddrStr);
    }

    out = QString("return to %1.%2 from %3").arg(mod, addr, fromPart);
    return true;
}

bool CPUStack::resolveReturnTo(const duint returnAddress, duint & fromAddress) const
{
    fromAddress = 0;

    if(!DbgFunctions()->MemIsCodePage(returnAddress, false))
        return false;

    duint regionSize = 0;
    const duint regionBase = DbgMemFindBaseAddr(returnAddress, &regionSize);
    if(regionBase == 0)
        return false;

    constexpr size_t kLookback = 64;
    const duint maxBack = returnAddress - regionBase;
    const size_t lookback = maxBack < kLookback ? static_cast<size_t>(maxBack) : kLookback;
    if(lookback < 2)
        return false;

    constexpr size_t kExtra = 16;
    const duint readStart = returnAddress - lookback;
    size_t readSize = lookback + kExtra;
    const duint regionEnd = regionBase + regionSize;
    if(readStart + readSize > regionEnd)
        readSize = regionEnd - readStart;

    std::vector<uint8_t> buf(readSize);
    if(!DbgMemRead(readStart, buf.data(), readSize))
        return false;

    const duint prevOffset = mDisasm->DisassembleBack(buf.data(), readStart, readSize, lookback, 1);
    if(prevOffset >= lookback)
        return false;

    const duint prevVa = readStart + prevOffset;
    const Instruction_t instr = mDisasm->DisassembleAt(buf.data() + prevOffset, readSize - prevOffset,
                                readStart, prevOffset, false);
    if(instr.length <= 0)
        return false;
    if(prevVa + static_cast<duint>(instr.length) != returnAddress)
        return false;
    if(instr.branchType != Instruction_t::Call)
        return false;

    fromAddress = instr.branchDestination;
    return true;
}
