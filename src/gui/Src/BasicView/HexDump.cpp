#include "HexDump.h"
#include <sstream>
#include "Configuration.h"
#include "Bridge.h"

HexDump::HexDump(QWidget* parent) : AbstractTableView(parent)
{
    SelectionData_t data;
    memset(&data, 0, sizeof(SelectionData_t));
    mSelection = data;

    mGuiState = HexDump::NoState;

    setRowCount(0);

    mMemPage = new MemoryPage(0, 0);
    mForceColumn = -1;

    clearDescriptors();

    backgroundColor = ConfigColor("HexDumpBackgroundColor");
    textColor = ConfigColor("HexDumpTextColor");
    selectionColor = ConfigColor("HexDumpSelectionColor");

    mRvaDisplayEnabled = false;

    // Slots
    connect(Bridge::getBridge(), SIGNAL(updateDump()), this, SLOT(reloadData()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChanged(DBGSTATE)));

    Initialize();
}

void HexDump::updateColors()
{
    AbstractTableView::updateColors();

    backgroundColor = ConfigColor("HexDumpBackgroundColor");
    textColor = ConfigColor("HexDumpTextColor");
    selectionColor = ConfigColor("HexDumpSelectionColor");
    reloadData();
}

void HexDump::updateFonts()
{
    setFont(ConfigFont("HexDump"));
}

void HexDump::printDumpAt(dsint parVA, bool select, bool repaint, bool updateTableOffset)
{
    dsint wBase = DbgMemFindBaseAddr(parVA, 0); //get memory base
    dsint wSize = DbgMemGetPageSize(wBase); //get page size
    if(!wBase || !wSize)
        return;
    dsint wRVA = parVA - wBase; //calculate rva
    int wBytePerRowCount = getBytePerRowCount(); //get the number of bytes per row
    dsint wRowCount;

    // Byte offset used to be aligned on the given RVA
    mByteOffset = (int)((dsint)wRVA % (dsint)wBytePerRowCount);
    mByteOffset = mByteOffset > 0 ? wBytePerRowCount - mByteOffset : 0;

    // Compute row count
    wRowCount = wSize / wBytePerRowCount;
    wRowCount += mByteOffset > 0 ? 1 : 0;

    if(mRvaDisplayEnabled && mMemPage->getBase() != mRvaDisplayPageBase)
        mRvaDisplayEnabled = false;

    setRowCount(wRowCount); //set the number of rows

    mMemPage->setAttributes(wBase, wSize);  // Set base and size (Useful when memory page changed)

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

void HexDump::printDumpAt(dsint parVA)
{
    printDumpAt(parVA, true);
}

duint HexDump::rvaToVa(dsint rva)
{
    return mMemPage->va(rva);
}

duint HexDump::getTableOffsetRva()
{
    return getTableOffset() * getBytePerRowCount() - mByteOffset;
}

void HexDump::mouseMoveEvent(QMouseEvent* event)
{
    bool wAccept = true;

    int x = event->x();
    int y = event->y();

    if(mGuiState == HexDump::MultiRowsSelectionState)
    {
        //qDebug() << "State = MultiRowsSelectionState";

        if((transY(y) >= 0) && y <= this->height())
        {
            for(int wI = 1; wI < getColumnCount(); wI++)    // Skip first column (Addresses)
            {
                int wColIndex = getColumnIndexFromX(x);

                if(mForceColumn != -1)
                {
                    wColIndex = mForceColumn;
                    x = getColumnPosition(mForceColumn) + 1;
                }

                if(wColIndex > 0) // No selection for first column (addresses)
                {
                    dsint wStartingAddress = getItemStartingAddress(x, y);
                    dsint dataSize = getSizeOf(mDescriptor.at(wColIndex - 1).data.itemSize) - 1;
                    dsint wEndingAddress = wStartingAddress + dataSize;

                    if(wEndingAddress < (dsint)mMemPage->getSize())
                    {
                        if(wStartingAddress < getInitialSelection())
                        {
                            expandSelectionUpTo(wStartingAddress);
                            mSelection.toIndex += dataSize;
                            emit selectionUpdated();
                        }
                        else
                            expandSelectionUpTo(wEndingAddress);

                        mGuiState = HexDump::MultiRowsSelectionState;

                        updateViewport();
                    }
                }
            }

            wAccept = true;
        }
    }

    if(wAccept == true)
        AbstractTableView::mouseMoveEvent(event);
}

void HexDump::mousePressEvent(QMouseEvent* event)
{
    if(event->buttons() == Qt::MiddleButton) //copy address to clipboard
    {
        if(!DbgIsDebugging())
            return;
        MessageBeep(MB_OK);
        QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        Bridge::CopyToClipboard(addrText);
        return;
    }
    //qDebug() << "HexDump::mousePressEvent";

    int x = event->x();
    int y = event->y();

    bool wAccept = false;

    if(((event->buttons() & Qt::LeftButton) != 0) && ((event->buttons() & Qt::RightButton) == 0))
    {
        if(getGuiState() == AbstractTableView::NoState)
        {
            if(y > getHeaderHeight() && y <= this->height())
            {
                int wColIndex = getColumnIndexFromX(x);

                if(mForceColumn != -1)
                {
                    wColIndex = mForceColumn;
                    x = getColumnPosition(mForceColumn) + 1;
                }

                if(wColIndex > 0 && mDescriptor.at(wColIndex - 1).isData == true) // No selection for first column (addresses) and no data columns
                {
                    dsint wStartingAddress = getItemStartingAddress(x, y);
                    dsint dataSize = getSizeOf(mDescriptor.at(wColIndex - 1).data.itemSize) - 1;
                    dsint wEndingAddress = wStartingAddress + dataSize;

                    if(wEndingAddress < (dsint)mMemPage->getSize())
                    {
                        bool bUpdateTo = false;
                        if(!(event->modifiers() & Qt::ShiftModifier))
                            setSingleSelection(wStartingAddress);
                        else if(getInitialSelection() > wEndingAddress)
                        {
                            wEndingAddress -= dataSize;
                            bUpdateTo = true;
                        }
                        expandSelectionUpTo(wEndingAddress);
                        if(bUpdateTo)
                        {
                            mSelection.toIndex += dataSize;
                            emit selectionUpdated();
                        }

                        mGuiState = HexDump::MultiRowsSelectionState;

                        updateViewport();
                    }
                }

                wAccept = true;
            }
        }
    }

    if(wAccept == false)
        AbstractTableView::mousePressEvent(event);
}

void HexDump::mouseReleaseEvent(QMouseEvent* event)
{
    bool wAccept = true;

    if((event->buttons() & Qt::LeftButton) == 0)
    {
        if(mGuiState == HexDump::MultiRowsSelectionState)
        {
            mGuiState = HexDump::NoState;

            updateViewport();

            wAccept = false;
        }
    }

    if(wAccept == true)
        AbstractTableView::mouseReleaseEvent(event);
}

QString HexDump::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    // Reset byte offset when base address is reached
    if(rowBase == 0 && mByteOffset != 0)
        printDumpAt(mMemPage->getBase(), false, false);

    // Compute RVA
    int wBytePerRowCount = getBytePerRowCount();
    dsint wRva = (rowBase + rowOffset) * wBytePerRowCount - mByteOffset;

    QString wStr = "";
    if(col == 0)    // Addresses
    {
        wStr += QString("%1").arg(rvaToVa(wRva), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }
    else if(mDescriptor.at(col - 1).isData == true) //paint data
    {
        printSelected(painter, rowBase, rowOffset, col, x, y, w, h);
        QList<RichTextPainter::CustomRichText_t> richText;
        getString(col - 1, wRva, &richText);
        RichTextPainter::paintRichText(painter, x, y, w, h, 4, &richText, getCharWidth());
    }

    return wStr;
}

void HexDump::printSelected(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    if((col > 0) && ((col - 1) < mDescriptor.size()))
    {
        ColumnDescriptor_t curDescriptor = mDescriptor.at(col - 1);
        int wBytePerRowCount = getBytePerRowCount();
        dsint wRva = (rowBase + rowOffset) * wBytePerRowCount - mByteOffset;
        int wItemPixWidth = getItemPixelWidth(curDescriptor);
        int wCharWidth = getCharWidth();
        if(wItemPixWidth == wCharWidth)
            x += 4;

        for(int i = 0; i < curDescriptor.itemCount; i++)
        {
            int wSelectionX = x + i * wItemPixWidth;
            if(isSelected(wRva + i * getSizeOf(curDescriptor.data.itemSize)) == true)
            {
                int wSelectionWidth = wItemPixWidth > w - (wSelectionX - x) ? w - (wSelectionX - x) : wItemPixWidth;
                wSelectionWidth = wSelectionWidth < 0 ? 0 : wSelectionWidth;
                painter->setPen(textColor);
                painter->fillRect(QRect(wSelectionX, y, wSelectionWidth, h), QBrush(selectionColor));
            }
            int separator = curDescriptor.separator;
            if(i && separator && !(i % separator))
            {
                painter->setPen(separatorColor);
                painter->drawLine(wSelectionX, y, wSelectionX, y + h);
            }
        }
    }
}

/************************************************************************************
                                Selection Management
************************************************************************************/
void HexDump::expandSelectionUpTo(dsint rva)
{
    if(rva < mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = rva;
        mSelection.toIndex = mSelection.firstSelectedIndex;
        emit selectionUpdated();
    }
    else if(rva > mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = mSelection.firstSelectedIndex;
        mSelection.toIndex = rva;
        emit selectionUpdated();
    }
    else if(rva == mSelection.firstSelectedIndex)
    {
        setSingleSelection(rva);
    }
}

void HexDump::setSingleSelection(dsint rva)
{
    mSelection.firstSelectedIndex = rva;
    mSelection.fromIndex = rva;
    mSelection.toIndex = rva;
    emit selectionUpdated();
}

dsint HexDump::getInitialSelection()
{
    return mSelection.firstSelectedIndex;
}

dsint HexDump::getSelectionStart()
{
    return mSelection.fromIndex;
}

dsint HexDump::getSelectionEnd()
{
    return mSelection.toIndex;
}

bool HexDump::isSelected(dsint rva)
{
    if(rva >= mSelection.fromIndex && rva <= mSelection.toIndex)
        return true;
    else
        return false;
}

void HexDump::getString(int col, dsint rva, QList<RichTextPainter::CustomRichText_t>* richText)
{
    int wI;
    QString wStr = "";

    int wByteCount = getSizeOf(mDescriptor.at(col).data.itemSize);
    int wBufferByteCount = mDescriptor.at(col).itemCount * wByteCount;

    wBufferByteCount = wBufferByteCount > (dsint)(mMemPage->getSize() - rva) ? mMemPage->getSize() - rva : wBufferByteCount;

    byte_t* wData = new byte_t[wBufferByteCount];
    //byte_t wData[mDescriptor.at(col).itemCount * wByteCount];

    mMemPage->read(wData, rva, wBufferByteCount);

    RichTextPainter::CustomRichText_t curData;
    curData.highlight = false;
    curData.flags = RichTextPainter::FlagColor;

    QColor highlightColor = ConfigColor("HexDumpModifiedBytesColor");

    for(wI = 0; wI < mDescriptor.at(col).itemCount && (rva + wI) < (dsint)mMemPage->getSize(); wI++)
    {
        int maxLen = getStringMaxLength(mDescriptor.at(col).data);
        QString append = " ";
        if(!maxLen)
            append = "";
        if((rva + wI + wByteCount - 1) < (dsint)mMemPage->getSize())
            wStr = toString(mDescriptor.at(col).data, (void*)(wData + wI * wByteCount)).rightJustified(maxLen, ' ') + append;
        else
            wStr = QString("?").rightJustified(maxLen, ' ') + append;
        curData.text = wStr;
        dsint start = rvaToVa(rva + wI * wByteCount);
        dsint end = start + wByteCount - 1;
        curData.textColor = DbgFunctions()->PatchInRange(start, end) ? highlightColor : textColor;
        richText->push_back(curData);
    }

    delete[] wData;
}

QString HexDump::toString(DataDescriptor_t desc, void* data) //convert data to string
{
    QString wStr = "";

    switch(desc.itemSize)
    {
    case Byte:
    {
        wStr = byteToString(*((byte_t*)data), desc.byteMode);
    }
    break;

    case Word:
    {
        wStr = wordToString(*((uint16*)data), desc.wordMode);
    }
    break;

    case Dword:
    {
        wStr = dwordToString(*((uint32*)data), desc.dwordMode);
    }
    break;

    case Qword:
    {
        wStr = qwordToString(*((uint64*)data), desc.qwordMode);
    }
    break;

    case Tword:
    {
        //TODO: sizeof(long double)=12, not 10
        wStr = twordToString(*((long double*)data), desc.twordMode);
    }
    break;

    default:
    {

    }
    break;
    }

    return wStr;
}

QString HexDump::byteToString(byte_t byte, ByteViewMode_e mode)
{
    QString wStr = "";

    switch(mode)
    {
    case HexByte:
    {
        wStr = QString("%1").arg((unsigned char)byte, 2, 16, QChar('0')).toUpper();
    }
    break;

    case AsciiByte:
    {
        QChar wChar = QChar::fromLatin1((char)byte);

        if(wChar.isPrint() == true)
            wStr = QString(wChar);
        else
            wStr = ".";
    }
    break;

    case SignedDecByte:
    {
        wStr = QString::number((int)((char)byte));
    }
    break;

    case UnsignedDecByte:
    {
        wStr = QString::number((unsigned int)byte);
    }
    break;

    default:
    {

    }
    break;
    }

    return wStr;
}

QString HexDump::wordToString(uint16 word, WordViewMode_e mode)
{
    QString wStr = "";

    switch(mode)
    {
    case HexWord:
    {
        wStr = QString("%1").arg((unsigned short)word, 4, 16, QChar('0')).toUpper();
    }
    break;

    case UnicodeWord:
    {
        QChar wChar = QChar::fromLatin1((char)word & 0xFF);
        if(wChar.isPrint() == true && (word >> 8) == 0)
            wStr = QString(wChar);
        else
            wStr = ".";
    }
    break;

    case SignedDecWord:
    {
        wStr = QString::number((int)((short)word));
    }
    break;

    case UnsignedDecWord:
    {
        wStr = QString::number((unsigned int)word);
    }
    break;

    default:
    {

    }
    break;
    }

    return wStr;
}

QString HexDump::dwordToString(uint32 dword, DwordViewMode_e mode)
{
    QString wStr = "";

    switch(mode)
    {
    case HexDword:
    {
        wStr = QString("%1").arg((unsigned int)dword, 8, 16, QChar('0')).toUpper();
    }
    break;

    case SignedDecDword:
    {
        wStr = QString::number((int)dword);
    }
    break;

    case UnsignedDecDword:
    {
        wStr = QString::number((unsigned int)dword);
    }
    break;

    case FloatDword:
    {
        float* wPtr = (float*)&dword;
        wStr = QString::number((double) * wPtr);
    }
    break;

    default:
    {

    }
    break;
    }

    return wStr;
}

QString HexDump::qwordToString(uint64 qword, QwordViewMode_e mode)
{
    QString wStr = "";

    switch(mode)
    {
    case HexQword:
    {
        wStr = QString("%1").arg((unsigned long long)qword, 16, 16, QChar('0')).toUpper();
    }
    break;

    case SignedDecQword:
    {
        wStr = QString::number((long long)qword);
    }
    break;

    case UnsignedDecQword:
    {
        wStr = QString::number((unsigned long long)qword);
    }
    break;

    case DoubleQword:
    {
        double* wPtr = (double*)&qword;
        wStr = QString::number((double) * wPtr);
    }
    break;

    default:
    {

    }
    break;
    }

    return wStr;
}

QString HexDump::twordToString(long double tword, TwordViewMode_e mode)
{
    QString wStr = "";

    switch(mode)
    {
    case FloatTword:
    {
        std::stringstream wlongDoubleStr;
        wlongDoubleStr <<  std::scientific << (long double)tword;

        wStr = QString::fromStdString(wlongDoubleStr.str());
    }
    break;

    default:
    {

    }
    break;
    }

    return wStr;
}

int HexDump::getSizeOf(DataSize_e size)
{
    int wSize = 0;

    switch(size)
    {
    case Byte:          // 1 Byte
    {
        wSize = 1;
    }
    break;

    case Word:          // 2 Bytes
    {
        wSize = 2;
    }
    break;

    case Dword:         // 4 Bytes
    {
        wSize = 4;
    }
    break;

    case Qword:         // 8 Bytes
    {
        wSize = 8;
    }
    break;

    case Tword:         // 10 Bytes
    {
        wSize = 10;
    }
    break;

    default:
    {
        wSize = 0;
    }
    }

    return wSize;
}

int HexDump::getStringMaxLength(DataDescriptor_t desc)
{
    int wLength = 0;

    switch(desc.itemSize)
    {
    case Byte:
    {
        wLength = byteStringMaxLength(desc.byteMode);
    }
    break;

    case Word:
    {
        wLength = wordStringMaxLength(desc.wordMode);
    }
    break;

    case Dword:
    {
        wLength = dwordStringMaxLength(desc.dwordMode);
    }
    break;

    case Qword:
    {
        wLength = qwordStringMaxLength(desc.qwordMode);
    }
    break;

    case Tword:
    {
        wLength = twordStringMaxLength(desc.twordMode);
    }
    break;

    default:
    {

    }
    break;
    }

    return wLength;
}

int HexDump::byteStringMaxLength(ByteViewMode_e mode)
{
    int wLength = 0;

    switch(mode)
    {
    case HexByte:
    {
        wLength = 2;
    }
    break;

    case AsciiByte:
    {
        wLength = 0;
    }
    break;

    case SignedDecByte:
    {
        wLength = 4;
    }
    break;

    case UnsignedDecByte:
    {
        wLength = 3;
    }
    break;

    default:
    {

    }
    break;
    }

    return wLength;
}

int HexDump::wordStringMaxLength(WordViewMode_e mode)
{
    int wLength = 0;

    switch(mode)
    {
    case HexWord:
    {
        wLength = 4;
    }
    break;

    case UnicodeWord:
    {
        wLength = 0;
    }
    break;

    case SignedDecWord:
    {
        wLength = 6;
    }
    break;

    case UnsignedDecWord:
    {
        wLength = 5;
    }
    break;

    default:
    {

    }
    break;
    }

    return wLength;
}

int HexDump::dwordStringMaxLength(DwordViewMode_e mode)
{
    int wLength = 0;

    switch(mode)
    {
    case HexDword:
    {
        wLength = 8;
    }
    break;

    case SignedDecDword:
    {
        wLength = 11;
    }
    break;

    case UnsignedDecDword:
    {
        wLength = 10;
    }
    break;

    case FloatDword:
    {
        wLength = 13;
    }
    break;

    default:
    {

    }
    break;
    }

    return wLength;
}

int HexDump::qwordStringMaxLength(QwordViewMode_e mode)
{
    int wLength = 0;

    switch(mode)
    {
    case HexQword:
    {
        wLength = 16;
    }
    break;

    case SignedDecQword:
    {
        wLength = 20;
    }
    break;

    case UnsignedDecQword:
    {
        wLength = 20;
    }
    break;

    case DoubleQword:
    {
        wLength = 23;
    }
    break;

    default:
    {

    }
    break;
    }

    return wLength;
}

int HexDump::twordStringMaxLength(TwordViewMode_e mode)
{
    int wLength = 0;

    switch(mode)
    {
    case FloatTword:
    {
        wLength = 29;
    }
    break;

    default:
    {

    }
    break;
    }

    return wLength;
}

int HexDump::getItemIndexFromX(int x)
{
    int wColIndex = getColumnIndexFromX(x);

    if(wColIndex > 0)
    {
        int wColStartingPos = getColumnPosition(wColIndex);
        int wRelativeX = x - wColStartingPos;

        int wItemPixWidth = getItemPixelWidth(mDescriptor.at(wColIndex - 1));
        int wCharWidth = getCharWidth();
        if(wItemPixWidth == wCharWidth)
            wRelativeX -= 4;

        int wItemIndex = wRelativeX / wItemPixWidth;

        wItemIndex = wItemIndex < 0 ? 0 : wItemIndex;
        wItemIndex = wItemIndex > (mDescriptor.at(wColIndex - 1).itemCount - 1) ? (mDescriptor.at(wColIndex - 1).itemCount - 1) : wItemIndex;

        return wItemIndex;
    }
    else
    {
        return 0;
    }
}

dsint HexDump::getItemStartingAddress(int x, int y)
{
    int wRowOffset = getIndexOffsetFromY(transY(y));
    int wItemIndex = getItemIndexFromX(x);
    int wColIndex = getColumnIndexFromX(x);
    dsint wStartingAddress = 0;

    if(wColIndex > 0)
    {
        wColIndex -= 1;
        wStartingAddress = (getTableOffset() + wRowOffset) * (mDescriptor.at(wColIndex).itemCount * getSizeOf(mDescriptor.at(wColIndex).data.itemSize)) + wItemIndex * getSizeOf(mDescriptor.at(wColIndex).data.itemSize) - mByteOffset;
    }

    return wStartingAddress;
}

int HexDump::getBytePerRowCount()
{
    return mDescriptor.at(0).itemCount * getSizeOf(mDescriptor.at(0).data.itemSize);
}

int HexDump::getItemPixelWidth(ColumnDescriptor_t desc)
{
    int wCharWidth = getCharWidth();
    int wItemPixWidth = getStringMaxLength(desc.data) * wCharWidth + wCharWidth;

    return wItemPixWidth;
}

void HexDump::appendDescriptor(int width, QString title, bool clickable, ColumnDescriptor_t descriptor)
{
    addColumnAt(width, title, clickable);
    mDescriptor.append(descriptor);
}

//Clears the descriptors, append a new descriptor and fix the tableOffset (use this instead of clearDescriptors()
void HexDump::appendResetDescriptor(int width, QString title, bool clickable, ColumnDescriptor_t descriptor)
{
    mAllowPainting = false;
    if(mDescriptor.size())
    {
        dsint wRVA = getTableOffset() * getBytePerRowCount() - mByteOffset;
        clearDescriptors();
        appendDescriptor(width, title, clickable, descriptor);
        printDumpAt(rvaToVa(wRVA), true, false);
    }
    else
        appendDescriptor(width, title, clickable, descriptor);
    mAllowPainting = true;
}

void HexDump::clearDescriptors()
{
    deleteAllColumns();
    mDescriptor.clear();
    int charwidth = getCharWidth();
    addColumnAt(8 + charwidth * 2 * sizeof(duint), "Address", false); //address
}

void HexDump::debugStateChanged(DBGSTATE state)
{
    if(state == stopped)
    {
        mMemPage->setAttributes(0, 0);
        setRowCount(0);
        reloadData();
    }
}
