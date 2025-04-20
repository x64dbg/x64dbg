#include "HexDump.h"
#include "Configuration.h"
#include "Bridge.h"
#include "StringUtil.h"
#include <QMessageBox>
#include <QFloat16>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringDecoder>
#else
#include <QTextCodec>
#endif // QT_VERSION

static int getStringMaxLength(HexDump::DataDescriptor desc);
static int byteStringMaxLength(HexDump::ByteViewMode mode);
static int wordStringMaxLength(HexDump::WordViewMode mode);
static int dwordStringMaxLength(HexDump::DwordViewMode mode);
static int qwordStringMaxLength(HexDump::QwordViewMode mode);
static int twordStringMaxLength(HexDump::TwordViewMode mode);

HexDump::HexDump(Architecture* architecture, QWidget* parent, MemoryPage* memPage)
    : AbstractTableView(parent),
      mArchitecture(architecture)
{
    setDrawDebugOnly(true);

    mGuiState = HexDump::NoState;

    setRowCount(0);

    if(!memPage)
        mMemPage = new MemoryPage(0, 0, this);
    else
        mMemPage = memPage;
    mForceColumn = -1;

    clearDescriptors();

    mBackgroundColor = ConfigColor("HexDumpBackgroundColor");
    mTextColor = ConfigColor("HexDumpTextColor");
    mSelectionColor = ConfigColor("HexDumpSelectionColor");

    mRvaDisplayEnabled = false;
    mSyncAddrExpression = "";
    mNonprintReplace = QChar('.'); //QChar(0x25CA);
    mNullReplace = QChar('.'); //QChar(0x2022);

    const auto updateCacheDataSize = 0x1000;
    mUpdateCacheData.resize(updateCacheDataSize);
    mUpdateCacheTemp.resize(updateCacheDataSize);

    // Slots
    connect(Bridge::getBridge(), SIGNAL(updateDump()), this, SLOT(updateDumpSlot()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChanged(DBGSTATE)));
    setupCopyMenu();

    Initialize();
}

HexDump::~HexDump()
{
    delete mMemPage;
}

void HexDump::updateColors()
{
    AbstractTableView::updateColors();

    mBackgroundColor = ConfigColor("HexDumpBackgroundColor");
    mTextColor = ConfigColor("HexDumpTextColor");
    mSelectionColor = ConfigColor("HexDumpSelectionColor");

    mModifiedBytesColor = ConfigColor("HexDumpModifiedBytesColor");
    mModifiedBytesBackgroundColor = ConfigColor("HexDumpModifiedBytesBackgroundColor");
    mRestoredBytesColor = ConfigColor("HexDumpRestoredBytesColor");
    mRestoredBytesBackgroundColor = ConfigColor("HexDumpRestoredBytesBackgroundColor");
    mByte00Color = ConfigColor("HexDumpByte00Color");
    mByte00BackgroundColor = ConfigColor("HexDumpByte00BackgroundColor");
    mByte7FColor = ConfigColor("HexDumpByte7FColor");
    mByte7FBackgroundColor = ConfigColor("HexDumpByte7FBackgroundColor");
    mByteFFColor = ConfigColor("HexDumpByteFFColor");
    mByteFFBackgroundColor = ConfigColor("HexDumpByteFFBackgroundColor");
    mByteIsPrintColor = ConfigColor("HexDumpByteIsPrintColor");
    mByteIsPrintBackgroundColor = ConfigColor("HexDumpByteIsPrintBackgroundColor");

    mUserModuleCodePointerHighlightColor = ConfigColor("HexDumpUserModuleCodePointerHighlightColor");
    mUserModuleDataPointerHighlightColor = ConfigColor("HexDumpUserModuleDataPointerHighlightColor");
    mSystemModuleCodePointerHighlightColor = ConfigColor("HexDumpSystemModuleCodePointerHighlightColor");
    mSystemModuleDataPointerHighlightColor = ConfigColor("HexDumpSystemModuleDataPointerHighlightColor");
    mUnknownCodePointerHighlightColor = ConfigColor("HexDumpUnknownCodePointerHighlightColor");
    mUnknownDataPointerHighlightColor = ConfigColor("HexDumpUnknownDataPointerHighlightColor");

    reloadData();
}

void HexDump::updateFonts()
{
    duint setting;
    if(BridgeSettingGetUint("Gui", "NonprintReplaceCharacter", &setting))
        mNonprintReplace = QChar(uint(setting));
    if(BridgeSettingGetUint("Gui", "NullReplaceCharacter", &setting))
        mNullReplace = QChar(uint(setting));
    setFont(ConfigFont("HexDump"));
    invalidateCachedFont();
}

void HexDump::updateShortcuts()
{
    AbstractTableView::updateShortcuts();
    mCopyAddress->setShortcut(ConfigShortcut("ActionCopyAddress"));
    mCopyRva->setShortcut(ConfigShortcut("ActionCopyRva"));
    mCopySelection->setShortcut(ConfigShortcut("ActionCopy"));
}

void HexDump::updateDumpSlot()
{
    if(mSyncAddrExpression.length() && DbgFunctions()->ValFromString)
    {
        duint syncAddr;
        if(DbgFunctions()->ValFromString(mSyncAddrExpression.toUtf8().constData(), &syncAddr)
                && DbgMemIsValidReadPtr(syncAddr))
        {
            printDumpAt(syncAddr, false, false, true);
        }
    }
    UpdateCache cur;
    cur.memBase = mMemPage->getBase();
    cur.memSize = mMemPage->getSize();
    if(cur.memBase)
    {
        cur.rva = getTableOffsetRva();
        cur.size = getBytePerRowCount() * getViewableRowsCount();
        if(cur.size < mUpdateCacheData.size())
        {
            if(mMemPage->read(mUpdateCacheTemp.data(), cur.rva, cur.size))
            {
                if(mUpdateCache == cur && memcmp(mUpdateCacheData.data(), mUpdateCacheTemp.data(), cur.size) == 0)
                {
                    // same view and same data, do not reload
                    return;
                }
                else
                {
                    mUpdateCache = cur;
                    mUpdateCacheData.swap(mUpdateCacheTemp);
#ifdef DEBUG
                    OutputDebugStringA(QString("[x64dbg] %1[%2] %3[%4]").arg(ToPtrString(mUpdateCache.memBase)).arg(ToHexString(mUpdateCache.memSize)).arg(ToPtrString(mUpdateCache.rva)).arg(ToHexString(mUpdateCache.size)).toUtf8().constData());
#endif // DEBUG
                }
            }
        }
    }
    reloadData();
}

void HexDump::copySelectionSlot()
{
    Bridge::CopyToClipboard(makeCopyText());
}

void HexDump::printDumpAt(duint parVA, bool select, bool repaint, bool updateTableOffset)
{
    duint size = 0;
    auto base = DbgMemFindBaseAddr(parVA, &size); //get memory base
    if(size == 0)
        return;
    auto rva = parVA - base; //calculate rva
    auto bytePerRowCount = getBytePerRowCount(); //get the number of bytes per row

    // Byte offset used to be aligned on the given RVA
    // TODO: check this logic
    mByteOffset = (dsint)rva % (dsint)bytePerRowCount;
    mByteOffset = mByteOffset > 0 ? (dsint)bytePerRowCount - mByteOffset : 0;

    // Compute row count
    auto rowCount = size / bytePerRowCount;
    rowCount += mByteOffset > 0 ? 1 : 0;

    if(mRvaDisplayEnabled && mMemPage->getBase() != mRvaDisplayPageBase)
        mRvaDisplayEnabled = false;

    setRowCount(rowCount); //set the number of rows

    mMemPage->setAttributes(base, size);  // Set base and size (Useful when memory page changed)

    if(updateTableOffset)
    {
        setTableOffset(-1); //make sure the requested address is always first
        setTableOffset((rva + mByteOffset) / bytePerRowCount); //change the displayed offset
    }

    if(select)
    {
        setSingleSelection(rva);
        auto endingAddress = rva + getSizeOf(mDescriptor.at(0).data.itemSize) - 1;
        expandSelectionUpTo(endingAddress);
    }

    if(repaint)
        reloadData();
}

void HexDump::printDumpAt(duint parVA)
{
    printDumpAt(parVA, true);
}

void HexDump::gotoPreviousSlot()
{
    printDumpAt(mHistory.historyPrev());
}

void HexDump::gotoNextSlot()
{
    printDumpAt(mHistory.historyNext());
}

duint HexDump::rvaToVa(duint rva) const
{
    return mMemPage->va(rva);
}

duint HexDump::getTableOffsetRva() const
{
    return getTableOffset() * getBytePerRowCount() - mByteOffset;
}

QString HexDump::makeAddrText(duint va) const
{
    bool addr64 = mArchitecture->addr64();
    char label[MAX_LABEL_SIZE] = "";
    QString addrText = "";
    if(mRvaDisplayEnabled) //RVA display
    {
        dsint displayRva = va - mRvaDisplayBase;
        if(displayRva == 0)
        {
            if(addr64)
                addrText = "$ ==>            ";
            else
                addrText = "$ ==>    ";
        }
        else if(displayRva > 0)
        {
            if(addr64)
                addrText = "$+" + QString("%1").arg(displayRva, -15, 16, QChar(' ')).toUpper();
            else
                addrText = "$+" + QString("%1").arg(displayRva, -7, 16, QChar(' ')).toUpper();
        }
        else if(displayRva < 0)
        {
            if(addr64)
                addrText = "$-" + QString("%1").arg(-displayRva, -15, 16, QChar(' ')).toUpper();
            else
                addrText = "$-" + QString("%1").arg(-displayRva, -7, 16, QChar(' ')).toUpper();
        }
    }
    addrText += ToPtrString(va);
    if(DbgGetLabelAt(va, SEG_DEFAULT, label)) //has label
    {
        char module[MAX_MODULE_SIZE] = "";
        if(DbgGetModuleAt(va, module) && !QString(label).startsWith("JMP.&"))
            addrText += " <" + QString(module) + "." + QString(label) + ">";
        else
            addrText += " <" + QString(label) + ">";
    }
    else
        *label = 0;
    return std::move(addrText);
}

QString HexDump::makeCopyText()
{
    auto deltaRowBase = getSelectionStart() % getBytePerRowCount() + mByteOffset;
    if(deltaRowBase >= getBytePerRowCount())
        deltaRowBase -= getBytePerRowCount();
    auto curRow = getSelectionStart() - deltaRowBase;
    QString result;
    while(curRow <= getSelectionEnd())
    {
        for(duint col = 0; col < getColumnCount(); col++)
        {
            if(col)
                result += " ";
            RichTextPainter::List richText;
            getColumnRichText(col, curRow, richText);
            QString colText;
            for(auto & r : richText)
                colText += r.text;
            if(col + 1 == getColumnCount())
                result += colText;
            else
                result += colText.leftJustified(getColumnWidth(col) / getCharWidth(), QChar(' '), true);
        }
        curRow += getBytePerRowCount();
        result += "\n";
    }
    return std::move(result);
}

void HexDump::setupCopyMenu()
{
    // Copy -> Data
    mCopySelection = new QAction(DIcon("copy_selection"), tr("&Selected lines"), this);
    connect(mCopySelection, SIGNAL(triggered(bool)), this, SLOT(copySelectionSlot()));
    mCopySelection->setShortcutContext(Qt::WidgetShortcut);
    addAction(mCopySelection);

    // Copy -> Address
    mCopyAddress = new QAction(DIcon("copy_address"), tr("&Address"), this);
    connect(mCopyAddress, SIGNAL(triggered()), this, SLOT(copyAddressSlot()));
    mCopyAddress->setShortcutContext(Qt::WidgetShortcut);
    addAction(mCopyAddress);

    // Copy -> RVA
    mCopyRva = new QAction(DIcon("copy_address"), "&RVA", this);
    connect(mCopyRva, SIGNAL(triggered()), this, SLOT(copyRvaSlot()));
    mCopyRva->setShortcutContext(Qt::WidgetShortcut);
    addAction(mCopyRva);
}

void HexDump::copyAddressSlot()
{
    QString addrText = ToPtrString(rvaToVa(getInitialSelection()));
    Bridge::CopyToClipboard(addrText);
}

void HexDump::copyRvaSlot()
{
    duint addr = rvaToVa(getInitialSelection());
    duint base = DbgFunctions()->ModBaseFromAddr(addr);
    if(base)
    {
        QString addrText = ToHexString(addr - base);
        Bridge::CopyToClipboard(addrText);
    }
    else
        QMessageBox::warning(this, tr("Error!"), tr("Selection not in a module..."));
}

void HexDump::mouseMoveEvent(QMouseEvent* event)
{
    bool accept = true;

    int x = event->x();
    int y = event->y();

    if(mGuiState == HexDump::MultiRowsSelectionState)
    {
        //qDebug() << "State = MultiRowsSelectionState";

        if((transY(y) >= 0) && y <= this->height())
        {
            auto colIndex = getColumnIndexFromX(x);

            if(mForceColumn != -1)
            {
                colIndex = mForceColumn;
                x = getColumnPosition(mForceColumn) + 1;
            }

            if(colIndex > 0) // No selection for first column (addresses)
            {
                auto startingAddress = getItemStartingAddress(x, y);
                auto dataSize = getSizeOf(mDescriptor.at(colIndex - 1).data.itemSize) - 1;
                auto endingAddress = startingAddress + dataSize;

                if(endingAddress < mMemPage->getSize())
                {
                    if(startingAddress < getInitialSelection())
                    {
                        expandSelectionUpTo(startingAddress);
                        mSelection.toIndex += dataSize;
                        emit selectionUpdated();
                    }
                    else
                        expandSelectionUpTo(endingAddress);

                    mGuiState = HexDump::MultiRowsSelectionState;

                    // TODO: only update if the selection actually changed
                    updateViewport();
                }
            }
            else
            {
                auto startingAddress = getItemStartingAddress(getColumnPosition(1) + 1, y);
                auto dataSize = getSizeOf(mDescriptor.at(0).data.itemSize) * mDescriptor.at(0).itemCount - 1;
                auto endingAddress = startingAddress + dataSize;

                if(endingAddress < mMemPage->getSize())
                {
                    if(startingAddress < getInitialSelection())
                    {
                        expandSelectionUpTo(startingAddress);
                        mSelection.toIndex += dataSize;
                        emit selectionUpdated();
                    }
                    else
                        expandSelectionUpTo(endingAddress);

                    mGuiState = HexDump::MultiRowsSelectionState;

                    // TODO: only update if the selection actually changed
                    updateViewport();
                }
            }

            accept = true;
        }
        else if(y > this->height() && mGuiState == HexDump::MultiRowsSelectionState)
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        }
        else if(transY(y) < 0 && mGuiState == HexDump::MultiRowsSelectionState)
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        }
    }

    if(accept)
        AbstractTableView::mouseMoveEvent(event);
}

void HexDump::mousePressEvent(QMouseEvent* event)
{
    if(event->buttons() == Qt::MiddleButton) //copy address to clipboard
    {
        if(!DbgIsDebugging())
            return;
        QApplication::beep();
        QString addrText = ToPtrString(rvaToVa(getInitialSelection()));
        Bridge::CopyToClipboard(addrText);
        return;
    }
    //qDebug() << "HexDump::mousePressEvent";

    int x = event->x();
    int y = event->y();

    bool accept = false;

    if(((event->buttons() & Qt::LeftButton) != 0) && ((event->buttons() & Qt::RightButton) == 0))
    {
        if(getGuiState() == AbstractTableView::NoState)
        {
            if(y > getHeaderHeight() && y <= this->height())
            {
                auto colIndex = getColumnIndexFromX(x);

                if(mForceColumn != -1)
                {
                    colIndex = mForceColumn;
                    x = getColumnPosition(mForceColumn) + 1;
                }

                if(colIndex > 0 && mDescriptor.at(colIndex - 1).isData) // No selection for first column (addresses) and no data columns
                {
                    auto startingAddress = getItemStartingAddress(x, y);
                    auto dataSize = getSizeOf(mDescriptor.at(colIndex - 1).data.itemSize) - 1;
                    auto endingAddress = startingAddress + dataSize;

                    if(endingAddress < mMemPage->getSize())
                    {
                        bool bUpdateTo = false;
                        if(!(event->modifiers() & Qt::ShiftModifier))
                            setSingleSelection(startingAddress);
                        else if(getInitialSelection() > endingAddress)
                        {
                            endingAddress -= dataSize;
                            bUpdateTo = true;
                        }
                        expandSelectionUpTo(endingAddress);
                        if(bUpdateTo)
                        {
                            mSelection.toIndex += dataSize;
                            emit selectionUpdated();
                        }

                        mGuiState = HexDump::MultiRowsSelectionState;

                        // TODO: only update if the selection actually changed
                        updateViewport();
                    }
                }
                else if(colIndex == 0)
                {
                    auto startingAddress = getItemStartingAddress(getColumnPosition(1) + 1, y);
                    auto dataSize = getSizeOf(mDescriptor.at(0).data.itemSize) * mDescriptor.at(0).itemCount - 1;
                    auto endingAddress = startingAddress + dataSize;

                    if(endingAddress < mMemPage->getSize())
                    {
                        bool bUpdateTo = false;
                        if((event->modifiers() & Qt::ShiftModifier) == 0)
                            setSingleSelection(startingAddress);
                        else if(getInitialSelection() > endingAddress)
                        {
                            endingAddress -= dataSize;
                            bUpdateTo = true;
                        }
                        expandSelectionUpTo(endingAddress);
                        if(bUpdateTo)
                        {
                            mSelection.toIndex += dataSize;
                            emit selectionUpdated();
                        }

                        mGuiState = HexDump::MultiRowsSelectionState;

                        // TODO: only update if the selection actually changed
                        updateViewport();
                    }
                }

                accept = true;
            }
        }
    }

    if(!accept)
        AbstractTableView::mousePressEvent(event);
}

void HexDump::mouseReleaseEvent(QMouseEvent* event)
{
    bool accept = true;

    if((event->buttons() & Qt::LeftButton) == 0)
    {
        if(mGuiState == HexDump::MultiRowsSelectionState)
        {
            mGuiState = HexDump::NoState;

            accept = false;
        }
    }
    if((event->button() & Qt::BackButton) != 0) //Go to previous/next history
    {
        accept = true;
        printDumpAt(mHistory.historyPrev());
    }
    else if((event->button() & Qt::ForwardButton) != 0)
    {
        accept = true;
        printDumpAt(mHistory.historyNext());
    }

    if(accept)
        AbstractTableView::mouseReleaseEvent(event);
}

void HexDump::wheelEvent(QWheelEvent* event)
{
    if(event->modifiers() == Qt::NoModifier)
        AbstractTableView::wheelEvent(event);
    else if(event->modifiers() == Qt::ControlModifier) // Zoom
        Config()->zoomFont("HexDump", event);
}

void HexDump::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    auto selStart = getInitialSelection();
    dsint granularity = 1; //Size of a data word.
    dsint action = 0; //Where to scroll the scrollbar
    Qt::KeyboardModifiers modifiers = event->modifiers();
    for(int i = 0; i < mDescriptor.size(); i++) //Find the first data column
    {
        if(mDescriptor.at(i).isData)
        {
            granularity = getSizeOf(mDescriptor.at(i).data.itemSize);
            break;
        }
    }
    if(modifiers == Qt::NoModifier)
    {
        //selStart -= selStart % granularity; //Align the selection to word boundary. TODO: Unaligned data?
        switch(key)
        {
        case Qt::Key_Left:
        {
            selStart -= granularity;
            if(0 <= selStart)
                action = -1;
        }
        break;
        case Qt::Key_Right:
        {
            selStart += granularity;
            if(mMemPage->getSize() > selStart)
                action = 1;
        }
        break;
        case Qt::Key_Up:
        {
            selStart -= getBytePerRowCount();
            if(0 <= selStart)
                action = -1;
        }
        break;
        case Qt::Key_Down:
        {
            selStart += getBytePerRowCount();
            if(mMemPage->getSize() > selStart)
                action = 1;
        }
        break;
        default:
            AbstractTableView::keyPressEvent(event);
        }

        if(action != 0)
        {
            //Check if selection is out of viewport. Step the scrollbar if necessary. (TODO)
            if(action == 1 && selStart >= getViewableRowsCount() * getBytePerRowCount() + getTableOffsetRva())
                verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
            else if(action == -1 && selStart < getTableOffsetRva())
                verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
            setSingleSelection(selStart);
            if(granularity > 1)
                expandSelectionUpTo(selStart + granularity - 1);
            reloadData();
        }
    }
    else if(modifiers == Qt::ControlModifier || modifiers == (Qt::ControlModifier | Qt::AltModifier))
    {
        duint offsetVa = rvaToVa(getTableOffsetRva());
        switch(key)
        {
        case Qt::Key_Left:
            action = (modifiers & Qt::AltModifier) ? -1 : -granularity;
            break;
        case Qt::Key_Right:
            action = (modifiers & Qt::AltModifier) ? 1 : granularity;
            break;
        case Qt::Key_Up:
            action = 0;
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
            break;
        case Qt::Key_Down:
            action = 0;
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
            break;
        default:
            AbstractTableView::keyPressEvent(event);
        }
        if(action != 0)
        {
            offsetVa += action;
            if(mMemPage->inRange(offsetVa))
                printDumpAt(offsetVa, false);
        }
    }
    else if(modifiers == Qt::ShiftModifier)
    {
        //TODO
    }
    else
    {
        AbstractTableView::keyPressEvent(event);
    }
    /*
        Let's keep the old code for a while until nobody remembers previous behaviour.
        if(key == Qt::Key_Left || (key == Qt::Key_Up && control)) //TODO: see if Ctrl+Up is redundant
        {
            duint offsetVa = rvaToVa(getTableOffsetRva()) - 1;
            if(mMemPage->inRange(offsetVa))
                printDumpAt(offsetVa, false);
        }
        else if(key == Qt::Key_Right || (key == Qt::Key_Down && control)) //TODO: see if Ctrl+Down is redundant
        {
            duint offsetVa = rvaToVa(getTableOffsetRva()) + 1;
            if(mMemPage->inRange(offsetVa))
                printDumpAt(offsetVa, false);
        }
        else
            AbstractTableView::keyPressEvent(event);
    */
}

QString HexDump::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    // Reset byte offset when base address is reached
    // TODO: wtf?
    if(getTableOffset() == 0 && mByteOffset != 0)
        printDumpAt(mMemPage->getBase(), false, false);

    // Compute RVA
    auto bytePerRowCount = getBytePerRowCount();
    auto rva = row * bytePerRowCount - mByteOffset;

    if(col && mDescriptor.at(col - 1).isData)
        printSelected(painter, row, col, x, y, w, h);

    RichTextPainter::List richText;
    getColumnRichText(col, rva, richText);
    RichTextPainter::paintRichText(painter, x, y, w, h, 4, richText, mFontMetrics);

    return QString();
}

void HexDump::printSelected(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    if(col > 0 && col <= (duint)mDescriptor.size())
    {
        ColumnDescriptor curDescriptor = mDescriptor.at(col - 1);
        auto bytePerRowCount = getBytePerRowCount();
        duint rva = row * bytePerRowCount - mByteOffset;
        int itemPixWidth = getItemPixelWidth(curDescriptor);
        int charWidth = getCharWidth();
        if(itemPixWidth == charWidth)
            x += 4;

        for(int i = 0; i < curDescriptor.itemCount; i++)
        {
            int selectionX = x + i * itemPixWidth;
            if(isSelected(rva + i * getSizeOf(curDescriptor.data.itemSize)))
            {
                int selectionWidth = itemPixWidth > w - (selectionX - x) ? w - (selectionX - x) : itemPixWidth;
                selectionWidth = selectionWidth < 0 ? 0 : selectionWidth;
                painter->setPen(mTextColor);
                painter->fillRect(QRect(selectionX, y, selectionWidth, h), QBrush(mSelectionColor));
            }
            int separator = curDescriptor.separator;
            if(i && separator && !(i % separator))
            {
                painter->setPen(mSeparatorColor);
                painter->drawLine(selectionX, y, selectionX, y + h);
            }
        }
    }
}

/************************************************************************************
                                Selection Management
************************************************************************************/
void HexDump::expandSelectionUpTo(duint rva)
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

void HexDump::setSingleSelection(duint rva)
{
    mSelection.firstSelectedIndex = rva;
    mSelection.fromIndex = rva;
    mSelection.toIndex = rva;
    emit selectionUpdated();
}

duint HexDump::getInitialSelection() const
{
    return mSelection.firstSelectedIndex;
}

duint HexDump::getSelectionStart() const
{
    return mSelection.fromIndex;
}

duint HexDump::getSelectionEnd() const
{
    return mSelection.toIndex;
}

bool HexDump::isSelected(duint rva) const
{
    return rva >= mSelection.fromIndex && rva <= mSelection.toIndex;
}

void HexDump::getColumnRichText(duint col, duint rva, RichTextPainter::List & richText)
{
    RichTextPainter::CustomRichText_t curData;
    curData.underline = false;
    curData.flags = RichTextPainter::FlagAll;
    curData.textColor = mTextColor;
    curData.textBackground = Qt::transparent;
    curData.underlineColor = Qt::transparent;

    RichTextPainter::CustomRichText_t spaceData;
    spaceData.underline = false;
    spaceData.flags = RichTextPainter::FlagNone;
    spaceData.underlineColor = Qt::transparent;

    if(!col) //address
    {
        curData.text = makeAddrText(rvaToVa(rva));
        richText.push_back(curData);
    }
    else if(mDescriptor.at(col - 1).isData)
    {
        const ColumnDescriptor & desc = mDescriptor.at(col - 1);

        auto byteCount = getSizeOf(desc.data.itemSize);
        auto bufferByteCount = desc.itemCount * byteCount;

        bufferByteCount = bufferByteCount > (mMemPage->getSize() - rva) ? mMemPage->getSize() - rva : bufferByteCount;

        // TODO: reuse a member buffer for this?
        uint8_t* data = new uint8_t[bufferByteCount];
        mMemPage->read(data, rva, bufferByteCount);

        if(!desc.textEncoding.isEmpty()) //convert the row bytes to unicode
        {
            // TODO: decode backwards a bit to display the correct state
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            auto textDecoder = QStringDecoder(desc.textEncoding);
            curData.text = textDecoder.decode(QByteArrayView((const char*)data, (int)bufferByteCount));
#else
            auto textCodec = QTextCodec::codecForName(desc.textEncoding);
            curData.text = textCodec->toUnicode((const char*)data, (int)bufferByteCount);
#endif // QT_VERSION
            //This might produce invalid characters in variables-width encodings. This is currently ignored.
            curData.text.replace('\t', "\\t");
            curData.text.replace('\f', "\\f");
            curData.text.replace('\v', "\\v");
            curData.text.replace('\n', "\\n");
            curData.text.replace('\r', "\\r");
            richText.push_back(curData);
        }
        else
        {
            for(int i = 0; i < desc.itemCount && (rva + i) < mMemPage->getSize(); i++)
            {
                curData.text.clear();
                curData.textColor = mTextColor;
                curData.textBackground = Qt::transparent;
                curData.flags = RichTextPainter::FlagAll;

                int maxLen = getStringMaxLength(desc.data);
                if((rva + i + byteCount - 1) < mMemPage->getSize())
                {
                    toString(desc.data, rva + i * byteCount, data + i * byteCount, curData);
                    if(curData.text.length() < maxLen)
                    {
                        spaceData.text = QString(' ').repeated(maxLen - curData.text.length());
                        richText.push_back(spaceData);
                    }
                    if(mUnderliningEnabled && i % sizeof(duint) == 0 && byteCount == 1 && desc.data.byteMode == HexByte) //pointer underlining
                    {
                        auto ptr = *(duint*)(data + i * byteCount);
                        if((spaceData.underline = curData.underline = DbgMemIsValidReadPtr(ptr)))
                        {
                            auto codePage = DbgFunctions()->MemIsCodePage(ptr, false);
                            auto modbase = DbgFunctions()->ModBaseFromAddr(ptr);
                            if(modbase)
                            {
                                if(DbgFunctions()->ModGetParty(modbase) == 1) //system
                                    spaceData.underlineColor = curData.underlineColor = codePage ? mSystemModuleCodePointerHighlightColor : mSystemModuleDataPointerHighlightColor;
                                else //user
                                    spaceData.underlineColor = curData.underlineColor = codePage ? mUserModuleCodePointerHighlightColor : mUserModuleDataPointerHighlightColor;
                            }
                            else
                                spaceData.underlineColor = curData.underlineColor = codePage ? mUnknownCodePointerHighlightColor : mUnknownDataPointerHighlightColor;
                        }
                    }
                    richText.push_back(curData);
                    if(maxLen)
                    {
                        spaceData.text = QString(' ');
                        if(i % sizeof(duint) == sizeof(duint) - 1)
                            spaceData.underline = false;
                        richText.push_back(spaceData);
                    }
                }
                else
                {
                    curData.text = QString("?").rightJustified(maxLen, ' ');
                    if(maxLen)
                        curData.text.append(' ');
                    richText.push_back(curData);
                }
            }
        }

        auto dataStartAddr = rvaToVa(rva);
        auto dataEndAddr = dataStartAddr + bufferByteCount - 1;

        if(mUnderlineRangeStartVa && mUnderlineRangeEndVa)
        {
            // Check if the highlight ranges overlap
            if(mUnderlineRangeStartVa <= dataEndAddr && dataStartAddr <= mUnderlineRangeEndVa)
            {
                for(RichTextPainter::CustomRichText_t & token : richText)
                {
                    token.underline = true;
                    token.underlineColor = token.textColor;
                }
                while(richText.back().text == QStringLiteral(" "))
                    richText.pop_back();
            }
        }

        delete[] data;
    }
}

void HexDump::toString(DataDescriptor desc, duint rva, uint8_t* data, RichTextPainter::CustomRichText_t & richText) //convert data to string
{
    switch(desc.itemSize)
    {
    case Byte:
    {
        byteToString(rva, *((uint8_t*)data), desc.byteMode, richText);
    }
    break;

    case Word:
    {
        wordToString(rva, *((uint16_t*)data), desc.wordMode, richText);
    }
    break;

    case Dword:
    {
        dwordToString(rva, *((uint32_t*)data), desc.dwordMode, richText);
    }
    break;

    case Qword:
    {
        qwordToString(rva, *((uint64_t*)data), desc.qwordMode, richText);
    }
    break;

    case Tword:
    {
        twordToString(rva, data, desc.twordMode, richText);
    }
    break;

    default:
    {

    }
    break;
    }

    if(desc.itemSize == Byte) //byte patches are handled in byteToString
        return;

    duint start = rvaToVa(rva);
    duint end = start + getSizeOf(desc.itemSize) - 1;
    if(DbgFunctions()->PatchInRange(start, end))
        richText.textColor = ConfigColor("HexDumpModifiedBytesColor");
}

void HexDump::byteToString(duint rva, uint8_t byte, ByteViewMode mode, RichTextPainter::CustomRichText_t & richText)
{
    QString str = "";

    switch(mode)
    {
    case HexByte:
    {
        str = ToByteString((unsigned char)byte);
    }
    break;

    case AsciiByte:
    {
        QChar ch = QChar::fromLatin1((char)byte);

        if(ch.isPrint())
            str = QString(ch);
        else if(!ch.unicode())
            str = mNullReplace;
        else
            str = mNonprintReplace;
    }
    break;

    case SignedDecByte:
    {
        str = QString::number((int)((char)byte));
    }
    break;

    case UnsignedDecByte:
    {
        str = QString::number((unsigned int)byte);
    }
    break;

    default:
    {

    }
    break;
    }

    richText.text = str;

    DBGPATCHINFO patchInfo;
    if(DbgFunctions()->PatchGetEx(rvaToVa(rva), &patchInfo))
    {
        if(byte == patchInfo.newbyte)
        {
            richText.textColor = mModifiedBytesColor;
            richText.textBackground = mModifiedBytesBackgroundColor;
        }
        else
        {
            richText.textColor = mRestoredBytesColor;
            richText.textBackground = mRestoredBytesBackgroundColor;
        }
    }
    else
    {
        switch(byte)
        {
        case 0x00:
            richText.textColor = mByte00Color;
            richText.textBackground = mByte00BackgroundColor;
            break;
        case 0x7F:
            richText.textColor = mByte7FColor;
            richText.textBackground = mByte7FBackgroundColor;
            break;
        case 0xFF:
            richText.textColor = mByteFFColor;
            richText.textBackground = mByteFFBackgroundColor;
            break;
        default:
            if(isprint(byte) || isspace(byte))
            {
                richText.textColor = mByteIsPrintColor;
                richText.textBackground = mByteIsPrintBackgroundColor;
            }
            break;
        }
    }
}

void HexDump::wordToString(duint rva, uint16_t word, WordViewMode mode, RichTextPainter::CustomRichText_t & richText)
{
    Q_UNUSED(rva);
    QString str;

    switch(mode)
    {
    case HexWord:
    {
        str = ToWordString((unsigned short)word);
    }
    break;

    case UnicodeWord:
    {
        QChar ch = QChar::fromLatin1((char)word & 0xFF);
        if(ch.isPrint() && (word >> 8) == 0)
            str = QString(ch);
        else if(!ch.unicode())
            str = mNullReplace;
        else
            str = mNonprintReplace;
    }
    break;

    case SignedDecWord:
    {
        str = QString::number((int)((short)word));
    }
    break;

    case UnsignedDecWord:
    {
        str = QString::number((unsigned int)word);
    }
    break;

    case HalfFloatWord:
    {
        str = ToFloatingString<qfloat16>(&word, 3);
    }
    break;

    default:
    {

    }
    break;
    }

    richText.text = str;
}

void HexDump::dwordToString(duint rva, uint32_t dword, DwordViewMode mode, RichTextPainter::CustomRichText_t & richText)
{
    Q_UNUSED(rva);
    QString str;

    switch(mode)
    {
    case HexDword:
    {
        str = QString("%1").arg((unsigned int)dword, 8, 16, QChar('0')).toUpper();
    }
    break;

    case SignedDecDword:
    {
        str = QString::number((int)dword);
    }
    break;

    case UnsignedDecDword:
    {
        str = QString::number((unsigned int)dword);
    }
    break;

    case FloatDword:
    {
        str = ToFloatString(&dword);
    }
    break;

    default:
    {

    }
    break;
    }

    richText.text = str;
}

void HexDump::qwordToString(duint rva, uint64_t qword, QwordViewMode mode, RichTextPainter::CustomRichText_t & richText)
{
    Q_UNUSED(rva);
    QString str;

    switch(mode)
    {
    case HexQword:
    {
        str = QString("%1").arg((unsigned long long)qword, 16, 16, QChar('0')).toUpper();
    }
    break;

    case SignedDecQword:
    {
        str = QString::number((long long)qword);
    }
    break;

    case UnsignedDecQword:
    {
        str = QString::number((unsigned long long)qword);
    }
    break;

    case DoubleQword:
    {
        str = ToDoubleString(&qword);
    }
    break;

    default:
    {

    }
    break;
    }

    richText.text = str;
}

void HexDump::twordToString(duint rva, void* tword, TwordViewMode mode, RichTextPainter::CustomRichText_t & richText)
{
    Q_UNUSED(rva);
    QString str;

    switch(mode)
    {
    case FloatTword:
    {
        str = ToLongDoubleString(tword);
    }
    break;

    default:
    {

    }
    break;
    }

    richText.text = str;
}

size_t HexDump::getSizeOf(DataSize size)
{
    return size_t(size);
}

static int getStringMaxLength(HexDump::DataDescriptor desc)
{
    int length = 0;

    switch(desc.itemSize)
    {
    case HexDump::Byte:
    {
        length = byteStringMaxLength(desc.byteMode);
    }
    break;

    case HexDump::Word:
    {
        length = wordStringMaxLength(desc.wordMode);
    }
    break;

    case HexDump::Dword:
    {
        length = dwordStringMaxLength(desc.dwordMode);
    }
    break;

    case HexDump::Qword:
    {
        length = qwordStringMaxLength(desc.qwordMode);
    }
    break;

    case HexDump::Tword:
    {
        length = twordStringMaxLength(desc.twordMode);
    }
    break;

    default:
    {

    }
    break;
    }

    return length;
}

static int byteStringMaxLength(HexDump::ByteViewMode mode)
{
    int length = 0;

    switch(mode)
    {
    case HexDump::HexByte:
    {
        length = 2;
    }
    break;

    case HexDump::AsciiByte:
    {
        length = 0;
    }
    break;

    case HexDump::SignedDecByte:
    {
        length = 4;
    }
    break;

    case HexDump::UnsignedDecByte:
    {
        length = 3;
    }
    break;

    default:
    {

    }
    break;
    }

    return length;
}

static int wordStringMaxLength(HexDump::WordViewMode mode)
{
    int length = 0;

    switch(mode)
    {
    case HexDump::HexWord:
    {
        length = 4;
    }
    break;

    case HexDump::UnicodeWord:
    {
        length = 0;
    }
    break;

    case HexDump::SignedDecWord:
    {
        length = 6;
    }
    break;

    case HexDump::UnsignedDecWord:
    {
        length = 5;
    }
    break;

    case HexDump::HalfFloatWord:
    {
        length = 9;
    }
    break;

    default:
    {

    }
    break;
    }

    return length;
}

static int dwordStringMaxLength(HexDump::DwordViewMode mode)
{
    int length = 0;

    switch(mode)
    {
    case HexDump::HexDword:
    {
        length = 8;
    }
    break;

    case HexDump::SignedDecDword:
    {
        length = 11;
    }
    break;

    case HexDump::UnsignedDecDword:
    {
        length = 10;
    }
    break;

    case HexDump::FloatDword:
    {
        length = 13;
    }
    break;

    default:
    {

    }
    break;
    }

    return length;
}

static int qwordStringMaxLength(HexDump::QwordViewMode mode)
{
    int length = 0;

    switch(mode)
    {
    case HexDump::HexQword:
    {
        length = 16;
    }
    break;

    case HexDump::SignedDecQword:
    {
        length = 20;
    }
    break;

    case HexDump::UnsignedDecQword:
    {
        length = 20;
    }
    break;

    case HexDump::DoubleQword:
    {
        length = 23;
    }
    break;

    default:
    {

    }
    break;
    }

    return length;
}

static int twordStringMaxLength(HexDump::TwordViewMode mode)
{
    int length = 0;

    switch(mode)
    {
    case HexDump::FloatTword:
    {
        length = 29;
    }
    break;

    default:
    {

    }
    break;
    }

    return length;
}

int HexDump::getItemIndexFromX(int x) const
{
    auto colIndex = getColumnIndexFromX(x);

    if(colIndex > 0)
    {
        int colStartingPos = getColumnPosition(colIndex);
        int relativeX = x - colStartingPos;

        int itemPixWidth = getItemPixelWidth(mDescriptor.at(colIndex - 1));
        int charWidth = getCharWidth();
        if(itemPixWidth == charWidth)
            relativeX -= 4;

        int itemIndex = relativeX / itemPixWidth;

        itemIndex = itemIndex < 0 ? 0 : itemIndex;
        itemIndex = itemIndex > (mDescriptor.at(colIndex - 1).itemCount - 1) ? (mDescriptor.at(colIndex - 1).itemCount - 1) : itemIndex;

        return itemIndex;
    }
    else
    {
        return 0;
    }
}

duint HexDump::getItemStartingAddress(int x, int y)
{
    auto rowOffset = getIndexOffsetFromY(transY(y));
    auto itemIndex = getItemIndexFromX(x);
    auto colIndex = getColumnIndexFromX(x);
    duint startingAddress = 0;

    if(colIndex > 0)
    {
        colIndex -= 1;
        startingAddress = (getTableOffset() + rowOffset) * (mDescriptor.at(colIndex).itemCount * getSizeOf(mDescriptor.at(colIndex).data.itemSize)) + itemIndex * getSizeOf(mDescriptor.at(colIndex).data.itemSize) - mByteOffset;
    }

    return startingAddress;
}

size_t HexDump::getBytePerRowCount() const
{
    return mDescriptor.at(0).itemCount * getSizeOf(mDescriptor.at(0).data.itemSize);
}

int HexDump::getItemPixelWidth(ColumnDescriptor desc) const
{
    int charWidth = getCharWidth();
    int itemPixWidth = getStringMaxLength(desc.data) * charWidth + charWidth;

    return itemPixWidth;
}

void HexDump::appendDescriptor(int width, QString title, bool clickable, ColumnDescriptor descriptor)
{
    addColumnAt(width, title, clickable);
    mDescriptor.append(descriptor);
}

//Clears the descriptors, append a new descriptor and fix the tableOffset (use this instead of clearDescriptors()
void HexDump::appendResetDescriptor(int width, QString title, bool clickable, ColumnDescriptor descriptor)
{
    setAllowPainting(false);
    if(mDescriptor.size())
    {
        auto rva = getTableOffset() * getBytePerRowCount() - mByteOffset;
        clearDescriptors();
        appendDescriptor(width, title, clickable, descriptor);
        printDumpAt(rvaToVa(rva), true, false);
    }
    else
        appendDescriptor(width, title, clickable, descriptor);
    setAllowPainting(true);
}

void HexDump::clearDescriptors()
{
    deleteAllColumns();
    mDescriptor.clear();
    int charwidth = getCharWidth();
    addColumnAt(8 + charwidth * 2 * sizeof(duint), tr("Address"), false); //address
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
