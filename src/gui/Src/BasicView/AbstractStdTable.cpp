#include "AbstractStdTable.h"
#include "Bridge.h"
#include "RichTextPainter.h"

AbstractStdTable::AbstractStdTable(QWidget* parent) : AbstractTableView(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(Bridge::getBridge(), SIGNAL(repaintTableView()), this, SLOT(reloadData()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequestedSlot(QPoint)));
    connect(this, SIGNAL(headerButtonPressed(duint)), this, SLOT(headerButtonPressedSlot(duint)));

    Initialize();

    // Set up copy menu
    mCopyLine = makeShortcutAction(DIcon("copy_table_line"), tr("&Line"), SLOT(copyLineSlot()), "ActionCopy");
    mCopyTable = makeShortcutAction(DIcon("copy_cropped_table"), tr("Cropped &Table"), SLOT(copyTableSlot()), "ActionCopyCroppedTable");
    mCopyTableResize = makeShortcutAction(DIcon("copy_full_table"), tr("&Full Table"), SLOT(copyTableResizeSlot()), "ActionCopyTable");
    mCopyLineToLog = makeShortcutAction(DIcon("copy_table_line"), tr("Line, To Log"), SLOT(copyLineToLogSlot()), "ActionCopyLineToLog");
    mCopyTableToLog = makeShortcutAction(DIcon("copy_cropped_table"), tr("Cropped Table, To Log"), SLOT(copyTableToLogSlot()), "ActionCopyCroppedTableToLog");
    mCopyTableResizeToLog = makeShortcutAction(DIcon("copy_full_table"), tr("Full Table, To Log"), SLOT(copyTableResizeToLogSlot()), "ActionCopyTableToLog");
    mExportTableCSV = makeShortcutAction(DIcon("database-export"), tr("&Export Table"), SLOT(exportTableSlot()), "ActionExport");
}

QString AbstractStdTable::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    bool isaddr = DbgIsDebugging() && getRowCount() > 0 && col == mAddressColumn;
    bool rowSelected = isSelected(row);
    QString text = getCellContent(row, col);

    duint va = isaddr ? duint(text.toULongLong(&isaddr, 16)) : 0;
    auto rowTraced = isaddr && DbgFunctions()->GetTraceRecordHitCount(va) != 0;
    QColor lineBackgroundColor;
    bool isBackgroundColorSet;
    if(rowSelected && rowTraced)
    {
        lineBackgroundColor = mTracedSelectedAddressBackgroundColor;
        isBackgroundColorSet = true;
    }
    else if(rowSelected)
    {
        lineBackgroundColor = mSelectionColor;
        isBackgroundColorSet = true;
    }
    else if(rowTraced)
    {
        lineBackgroundColor = mTracedBackgroundColor;
        isBackgroundColorSet = true;
    }
    else
    {
        isBackgroundColorSet = false;
    }
    if(isBackgroundColorSet)
        painter->fillRect(QRect(x, y, w, h), QBrush(lineBackgroundColor));

    if(col == mAddressColumn && isaddr)
    {
        char label[MAX_LABEL_SIZE] = "";
        if(bAddressLabel && DbgGetLabelAt(va, SEG_DEFAULT, label)) //has label
        {
            char module[MAX_MODULE_SIZE] = "";
            if(DbgGetModuleAt(va, module) && !QString(label).startsWith("JMP.&"))
                text += " <" + QString(module) + "." + QString(label) + ">";
            else
                text += " <" + QString(label) + ">";
        }
        BPXTYPE bpxtype = DbgGetBpxTypeAt(va);
        bool isbookmark = DbgGetBookmarkAt(va);

        duint cip = Bridge::getBridge()->mLastCip;
        if(bCipBase)
        {
            duint base = DbgFunctions()->ModBaseFromAddr(cip);
            if(base)
                cip = base;
        }

        if(DbgIsDebugging() && va == cip) //debugging + cip
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(mCipBackgroundColor));
            if(!isbookmark) //no bookmark
            {
                if(bpxtype & bp_normal) //normal breakpoint
                {
                    QColor & bpColor = mBreakpointBackgroundColor;
                    if(!bpColor.alpha()) //we don't want transparent text
                        bpColor = mBreakpointColor;
                    if(bpColor == mCipBackgroundColor)
                        bpColor = mCipColor;
                    painter->setPen(bpColor);
                }
                else if(bpxtype & bp_hardware) //hardware breakpoint only
                {
                    QColor hwbpColor = mHardwareBreakpointBackgroundColor;
                    if(!hwbpColor.alpha()) //we don't want transparent text
                        hwbpColor = mHardwareBreakpointColor;
                    if(hwbpColor == mCipBackgroundColor)
                        hwbpColor = mCipColor;
                    painter->setPen(hwbpColor);
                }
                else //no breakpoint
                {
                    painter->setPen(mCipColor);
                }
            }
            else //bookmark
            {
                QColor bookmarkColor = mBookmarkBackgroundColor;
                if(!bookmarkColor.alpha()) //we don't want transparent text
                    bookmarkColor = mBookmarkColor;
                if(bookmarkColor == mCipBackgroundColor)
                    bookmarkColor = mCipColor;
                painter->setPen(bookmarkColor);
            }
        }
        else //non-cip address
        {
            if(!isbookmark) //no bookmark
            {
                if(*label) //label
                {
                    if(bpxtype == bp_none) //label only : fill label background
                    {
                        painter->setPen(mLabelColor); //red -> address + label text
                        painter->fillRect(QRect(x, y, w, h), QBrush(mLabelBackgroundColor)); //fill label background
                    }
                    else //label + breakpoint
                    {
                        if(bpxtype & bp_normal) //label + normal breakpoint
                        {
                            painter->setPen(mBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //label + hardware breakpoint only
                        {
                            painter->setPen(mHardwareBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill ?
                        }
                        else //other cases -> do as normal
                        {
                            painter->setPen(mLabelColor); //red -> address + label text
                            painter->fillRect(QRect(x, y, w, h), QBrush(mLabelBackgroundColor)); //fill label background
                        }
                    }
                }
                else //no label
                {
                    if(bpxtype == bp_none) //no label, no breakpoint
                    {
                        QColor background;
                        if(rowSelected)
                        {
                            background = mSelectedAddressBackgroundColor;
                            painter->setPen(mSelectedAddressColor); //black address (DisassemblySelectedAddressColor)
                        }
                        else
                        {
                            background = mAddressBackgroundColor;
                            painter->setPen(mAddressColor); //DisassemblyAddressColor
                        }
                        if(background.alpha())
                            painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background
                    }
                    else //breakpoint only
                    {
                        if(bpxtype & bp_normal) //normal breakpoint
                        {
                            painter->setPen(mBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //hardware breakpoint only
                        {
                            painter->setPen(mHardwareBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill red
                        }
                        else //other cases (memory breakpoint in disassembly) -> do as normal
                        {
                            QColor background;
                            if(rowSelected)
                            {
                                background = mSelectedAddressBackgroundColor;
                                painter->setPen(mSelectedAddressColor); //black address (DisassemblySelectedAddressColor)
                            }
                            else
                            {
                                background = mAddressBackgroundColor;
                                painter->setPen(mAddressColor);
                            }
                            if(background.alpha())
                                painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background
                        }
                    }
                }
            }
            else //bookmark
            {
                if(*label) //label + bookmark
                {
                    if(bpxtype == bp_none) //label + bookmark
                    {
                        painter->setPen(mLabelColor); //red -> address + label text
                        painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill label background
                    }
                    else //label + breakpoint + bookmark
                    {
                        QColor color = mBookmarkBackgroundColor;
                        if(!color.alpha()) //we don't want transparent text
                            color = mAddressColor;
                        painter->setPen(color);
                        if(bpxtype & bp_normal) //label + bookmark + normal breakpoint
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //label + bookmark + hardware breakpoint only
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill ?
                        }
                    }
                }
                else //bookmark, no label
                {
                    if(bpxtype == bp_none) //bookmark only
                    {
                        painter->setPen(mBookmarkColor); //black address
                        painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill bookmark color
                    }
                    else //bookmark + breakpoint
                    {
                        QColor color = mBookmarkBackgroundColor;
                        if(!color.alpha()) //we don't want transparent text
                            color = mAddressColor;
                        painter->setPen(color);
                        if(bpxtype & bp_normal) //bookmark + normal breakpoint
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //bookmark + hardware breakpoint only
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill red
                        }
                        else //other cases (bookmark + memory breakpoint in disassembly) -> do as normal
                        {
                            painter->setPen(mBookmarkColor); //black address
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill bookmark color
                        }
                    }
                }
            }
        }
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, text);
        text.clear();
    }
    else if(mHighlightText.length() && col >= mMinimumHighlightColumn && text.contains(mHighlightText, Qt::CaseInsensitive)) // TODO: case sensitive + regex highlighting
    {
        //super smart way of splitting while keeping the delimiters (thanks to cypher for guidance)
        int index = -2;
        do
        {
            index = text.indexOf(mHighlightText, index + 2, Qt::CaseInsensitive);
            if(index != -1)
            {
                text = text.insert(index + mHighlightText.length(), QChar('\1'));
                text = text.insert(index, QChar('\1'));
            }
        }
        while(index != -1);
        QStringList split = text.split(QChar('\1'), Qt::SkipEmptyParts, Qt::CaseInsensitive);

        //create rich text list
        RichTextPainter::CustomRichText_t curRichText;
        curRichText.flags = RichTextPainter::FlagColor;
        QColor textColor = getCellColor(row, col);
        QColor textBackgroundColor = Qt::transparent;
        QColor highlightColor = ConfigColor("SearchListViewHighlightColor");
        QColor highlightBackgroundColor = ConfigColor("SearchListViewHighlightBackgroundColor");
        curRichText.textColor = textColor;
        curRichText.underline = false;
        RichTextPainter::List richText;
        foreach(QString str, split)
        {
            curRichText.text = str;
            if(!str.compare(mHighlightText, Qt::CaseInsensitive))
            {
                curRichText.textColor = highlightColor;
                curRichText.textBackground = highlightBackgroundColor;
            }
            else
            {
                curRichText.textColor = textColor;
                curRichText.textBackground = textBackgroundColor;
            }
            richText.push_back(curRichText);
        }

        //paint the rich text
        RichTextPainter::paintRichText(painter, x + 1, y, w, h, 4, richText, mFontMetrics);
        text.clear();
    }
    return text;
}

void AbstractStdTable::updateColors()
{
    AbstractTableView::updateColors();

    mCipBackgroundColor = ConfigColor("DisassemblyCipBackgroundColor");
    mCipColor = ConfigColor("DisassemblyCipColor");
    mBreakpointBackgroundColor = ConfigColor("DisassemblyBreakpointBackgroundColor");
    mBreakpointColor = ConfigColor("DisassemblyBreakpointColor");
    mHardwareBreakpointBackgroundColor = ConfigColor("DisassemblyHardwareBreakpointBackgroundColor");
    mHardwareBreakpointColor = ConfigColor("DisassemblyHardwareBreakpointColor");
    mBookmarkBackgroundColor = ConfigColor("DisassemblyBookmarkBackgroundColor");
    mBookmarkColor = ConfigColor("DisassemblyBookmarkColor");
    mLabelColor = ConfigColor("DisassemblyLabelColor");
    mLabelBackgroundColor = ConfigColor("DisassemblyLabelBackgroundColor");
    mSelectedAddressBackgroundColor = ConfigColor("DisassemblySelectedAddressBackgroundColor");
    mSelectedAddressColor = ConfigColor("DisassemblySelectedAddressColor");
    mAddressBackgroundColor = ConfigColor("DisassemblyAddressBackgroundColor");
    mAddressColor = ConfigColor("DisassemblyAddressColor");
    mTracedBackgroundColor = ConfigColor("DisassemblyTracedBackgroundColor");

    auto a = mSelectionColor, b = mTracedBackgroundColor;
    mTracedSelectedAddressBackgroundColor = QColor((a.red() + b.red()) / 2, (a.green() + b.green()) / 2, (a.blue() + b.blue()) / 2);
}

void AbstractStdTable::mouseMoveEvent(QMouseEvent* event)
{
    bool accept = true;
    int y = transY(event->y());

    if(mGuiState == AbstractStdTable::MultiRowsSelectionState)
    {
        //qDebug() << "State = MultiRowsSelectionState";

        if(y >= 0 && y <= this->getTableHeight())
        {
            auto rowIndex = getTableOffset() + getIndexOffsetFromY(y);

            if(rowIndex < getRowCount())
            {
                if(mIsMultiSelectionAllowed)
                    expandSelectionUpTo(rowIndex);
                else
                    setSingleSelection(rowIndex);

                // TODO: only update if the selection actually changed
                updateViewport();

                accept = false;
            }
        }
        else if(y < 0)
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        }
        else if(y > getTableHeight())
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        }
    }

    if(accept)
        AbstractTableView::mouseMoveEvent(event);
}

void AbstractStdTable::mousePressEvent(QMouseEvent* event)
{
    bool accept = false;

    if(((event->buttons() & Qt::LeftButton) != 0) && ((event->buttons() & Qt::RightButton) == 0))
    {
        if(getGuiState() == AbstractTableView::NoState)
        {
            if(event->y() > getHeaderHeight())
            {
                auto rowIndex = getTableOffset() + getIndexOffsetFromY(transY(event->y()));

                if(rowIndex < getRowCount())
                {
                    if(mIsMultiSelectionAllowed && (event->modifiers() & Qt::ShiftModifier))
                        expandSelectionUpTo(rowIndex);
                    else
                        setSingleSelection(rowIndex);

                    mGuiState = AbstractStdTable::MultiRowsSelectionState;

                    // TODO: only update if the selection actually changed
                    updateViewport();

                    accept = true;
                }
            }
        }
    }

    if(!accept)
        AbstractTableView::mousePressEvent(event);
}

void AbstractStdTable::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->y() > getHeaderHeight() && event->button() == Qt::LeftButton)
        emit doubleClickedSignal();
    AbstractTableView::mouseDoubleClickEvent(event);
}

void AbstractStdTable::mouseReleaseEvent(QMouseEvent* event)
{
    bool accept = true;

    if((event->buttons() & Qt::LeftButton) == 0)
    {
        if(mGuiState == AbstractStdTable::MultiRowsSelectionState)
        {
            mGuiState = AbstractStdTable::NoState;

            accept = false;
        }
    }

    if(accept)
        AbstractTableView::mouseReleaseEvent(event);
}

void AbstractStdTable::keyPressEvent(QKeyEvent* event)
{
    emit keyPressedSignal(event);
    int key = event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();

    if(key == Qt::Key_Up ||
            key == Qt::Key_Down ||
            key == Qt::Key_Home ||
            key == Qt::Key_End ||
            key == Qt::Key_A ||
            key == Qt::Key_C)
    {
        auto botIndex = getTableOffset();
        auto topIndex = botIndex + getNbrOfLineToPrint() - 1;

        switch(key)
        {
        case Qt::Key_Up:
            if(mIsMultiSelectionAllowed && modifiers == Qt::ShiftModifier) //Shift+Up -> expand selection upwards
            {
                expandUp();
            }
            else //Up -> select previous
            {
                selectPrevious();
            }
            break;

        case Qt::Key_Down:
            if(mIsMultiSelectionAllowed && modifiers == Qt::ShiftModifier) //Shift+Down -> expand selection downwards
            {
                expandDown();
            }
            else //Down -> select next
            {
                selectNext();
            }
            break;

        case Qt::Key_Home:
            if(mIsMultiSelectionAllowed && modifiers == Qt::ShiftModifier) //Shift+Home -> expand selection to top
            {
                expandTop();
            }
            else if(modifiers == Qt::NoModifier) //Home -> select first line
            {
                selectStart();
            }
            break;

        case Qt::Key_End:
            if(mIsMultiSelectionAllowed && modifiers == Qt::ShiftModifier) //Shift+End -> expand selection to bottom
            {
                expandBottom();
            }
            else if(modifiers == Qt::NoModifier) //End -> select last line
            {
                selectEnd();
            }
            break;

        case Qt::Key_A:
            if(mIsMultiSelectionAllowed && modifiers == Qt::ControlModifier) //Ctrl+A -> select all
            {
                selectAll();
            }
            break;

        case Qt::Key_C:
            if(modifiers == Qt::ControlModifier) //Ctrl+C -> copy
            {
                copyLineSlot();
            }
            break;
        }

        if(getInitialSelection() < botIndex)
        {
            setTableOffset(getInitialSelection());
        }
        else if(getInitialSelection() >= topIndex)
        {
            setTableOffset(getInitialSelection() - getNbrOfLineToPrint() + 2);
        }

        // TODO: only update if the selection actually changed
        updateViewport();
    }
    else
    {
        AbstractTableView::keyPressEvent(event);
    }
}

void AbstractStdTable::enableMultiSelection(bool enabled)
{
    mIsMultiSelectionAllowed = enabled;
}

void AbstractStdTable::enableColumnSorting(bool enabled)
{
    mIsColumnSortingAllowed = enabled;
}

/************************************************************************************
                                Selection Management
************************************************************************************/
void AbstractStdTable::expandSelectionUpTo(duint to)
{
    if(to < mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = to;
        mSelection.toIndex = mSelection.firstSelectedIndex;
        emit selectionChanged(to);
    }
    else if(to > mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = mSelection.firstSelectedIndex;
        mSelection.toIndex = to;
        emit selectionChanged(to);
    }
    else if(to == mSelection.firstSelectedIndex)
    {
        setSingleSelection(to);
    }
}

void AbstractStdTable::expandUp()
{
    auto rowIndex = mSelection.firstSelectedIndex - 1;
    if(rowIndex >= 0)
    {
        if(rowIndex < mSelection.fromIndex)
        {
            mSelection.fromIndex = rowIndex;
            mSelection.firstSelectedIndex = rowIndex;

        }
        else
        {
            mSelection.firstSelectedIndex = rowIndex;
            mSelection.toIndex = rowIndex;
        }

        emit selectionChanged(rowIndex);
    }
}

void AbstractStdTable::expandDown()
{
    auto rowIndex = mSelection.firstSelectedIndex + 1;
    auto endIndex = getRowCount() - 1;
    if(rowIndex <= endIndex)
    {

        if(rowIndex > mSelection.toIndex)
        {
            mSelection.firstSelectedIndex = rowIndex;
            mSelection.toIndex = rowIndex;

        }
        else
        {
            mSelection.fromIndex = rowIndex;
            mSelection.firstSelectedIndex = rowIndex;
        }


        emit selectionChanged(rowIndex);
    }
}

void AbstractStdTable::expandTop()
{
    if(getRowCount() > 0)
    {
        expandSelectionUpTo(0);
    }
}

void AbstractStdTable::expandBottom()
{
    auto rowCount = getRowCount();
    if(rowCount > 0)
    {
        expandSelectionUpTo(rowCount - 1);
    }
}

void AbstractStdTable::setSingleSelection(duint index)
{
    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = index;
    emit selectionChanged(index);
}

duint AbstractStdTable::getInitialSelection() const
{
    return mSelection.firstSelectedIndex;
}

QList<duint> AbstractStdTable::getSelection() const
{
    QList<duint> selection;
    selection.reserve(mSelection.toIndex - mSelection.fromIndex);
    for(duint i = mSelection.fromIndex; i <= mSelection.toIndex; i++)
    {
        selection.append(i);
    }
    return selection;
}

void AbstractStdTable::selectStart()
{
    if(getRowCount() > 0)
    {
        setSingleSelection(0);
    }
}

void AbstractStdTable::selectEnd()
{
    auto endIndex = getRowCount() - 1;
    if(endIndex >= 0)
    {
        setSingleSelection(endIndex);
    }
}

void AbstractStdTable::selectNext()
{
    // TODO: fix the signed/unsigned
    duint next = getInitialSelection() + 1;

    // Bounding
    next = next > getRowCount() - 1 ? getRowCount() - 1 : next;
    next = next < 0  ? 0 : next;

    setSingleSelection(next);
}

void AbstractStdTable::selectPrevious()
{
    duint next = getInitialSelection() - 1;

    // Bounding
    next = next > getRowCount() - 1 ? getRowCount() - 1 : next;
    next = next < 0  ? 0 : next;

    setSingleSelection(next);
}

void AbstractStdTable::selectAll()
{
    duint index = 0;
    duint indexEnd = getRowCount() - 1;

    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = indexEnd;

    emit selectionChanged(index);
}

bool AbstractStdTable::isSelected(duint row) const
{
    return row >= mSelection.fromIndex && row <= mSelection.toIndex;
}

bool AbstractStdTable::scrollSelect(duint row)
{
    if(!isValidIndex(row, 0))
        return false;

    auto rangefrom = getTableOffset();
    auto rangeto = rangefrom + getViewableRowsCount() - 1;
    if(row < rangefrom) //offset lays before the current view
        setTableOffset(row);
    else if(row > (rangeto - 1)) //offset lays after the current view
        setTableOffset(row - getViewableRowsCount() + 2);
    setSingleSelection(row);
    return true;
}

/************************************************************************************
                                Data Management
************************************************************************************/
void AbstractStdTable::addColumnAt(int width, QString title, bool isClickable, QString copyTitle)
{
    AbstractTableView::addColumnAt(width, title, isClickable);

    //Append copy title
    if(!copyTitle.length())
        mCopyTitles.push_back(title);
    else
        mCopyTitles.push_back(copyTitle);
}

void AbstractStdTable::deleteAllColumns()
{
    setRowCount(0);
    AbstractTableView::deleteAllColumns();
    mCopyTitles.clear();
}

void AbstractStdTable::copyLineSlot()
{
    auto colCount = getColumnCount();
    QString finalText = "";
    if(colCount == 1)
        finalText = getCellContent(getInitialSelection(), 0);
    else
    {
        for(auto selected : getSelection())
        {
            for(duint i = 0; i < colCount; i++)
            {
                QString cellContent = getCellContent(selected, i);
                if(!cellContent.length()) //skip empty cells
                    continue;
                QString title = mCopyTitles.at(i);
                if(title.length())
                    finalText += title + "=";
                finalText += cellContent.trimmed();;
                finalText += "\r\n";
            }
        }
    }
    Bridge::CopyToClipboard(finalText);
}

void AbstractStdTable::copyLineToLogSlot()
{
    auto colCount = getColumnCount();
    auto selected = getInitialSelection();
    QString finalText = "";
    if(colCount == 1)
        finalText = getCellContent(selected, 0);
    else
    {
        for(duint i = 0; i < colCount; i++)
        {
            QString cellContent = getCellContent(selected, i);
            if(!cellContent.length()) //skip empty cells
                continue;
            QString title = mCopyTitles.at(i);
            if(title.length())
                finalText += title + "=";
            finalText += cellContent.trimmed();;
            finalText += "\r\n";
        }
    }
    emit Bridge::getBridge()->addMsgToLog(finalText.toUtf8());
}

QString AbstractStdTable::copyTable(const std::vector<int> & colWidths)
{
    auto colCount = getColumnCount();
    auto rowCount = getRowCount();
    QString finalText = "";
    if(colCount == 1)
    {
        for(duint i = 0; i < rowCount; i++)
        {
            QString cellContent = getCellContent(i, 0);
            if(!cellContent.length()) //skip empty cells
                continue;
            finalText += cellContent + "\r\n";
        }
    }
    else
    {
        //std::vector<int> colWidths;
        //for(int i = 0; i < colCount; i++)
        //    colWidths.push_back(getMaxColumnLength(i));
        for(duint i = 0; i < colCount; i++)
        {
            if(i)
                finalText += " ";
            int colWidth = colWidths[i];
            if(colWidth)
                finalText += getColTitle(i).leftJustified(colWidth, QChar(' '), true);
            else
                finalText += getColTitle(i);
        }
        finalText += "\r\n";
        for(duint i = 0; i < rowCount; i++)
        {
            QString finalRowText = "";
            for(duint j = 0; j < colCount; j++)
            {
                if(j)
                    finalRowText += " ";
                QString cellContent = getCellContent(i, j);
                int colWidth = colWidths[j];
                if(colWidth && j != colCount - 1)
                    finalRowText += cellContent.leftJustified(colWidth, QChar(' '), true);
                else
                    finalRowText += cellContent;
            }
            finalText += finalRowText + "\r\n";
        }
    }
    return finalText;
}

void AbstractStdTable::copyTableSlot()
{
    std::vector<int> colWidths;
    int colCount = getColumnCount();
    for(int i = 0; i < colCount; i++)
        colWidths.push_back(getColumnWidth(i) / getCharWidth());
    Bridge::CopyToClipboard(copyTable(colWidths));
}

void AbstractStdTable::copyTableToLogSlot()
{
    std::vector<int> colWidths;
    int colCount = getColumnCount();
    for(int i = 0; i < colCount; i++)
        colWidths.push_back(getColumnWidth(i) / getCharWidth());
    emit Bridge::getBridge()->addMsgToLog(copyTable(colWidths).toUtf8());
}

void AbstractStdTable::copyTableResizeSlot()
{
    std::vector<int> colWidths;
    auto rowCount = getRowCount();
    auto colCount = getColumnCount();
    for(duint i = 0; i < colCount; i++)
    {
        auto max = getCellContent(0, i).length();
        for(duint j = 1; j < rowCount; j++)
            max = std::max(getCellContent(j, i).length(), max);
        colWidths.push_back(max);
    }
    Bridge::CopyToClipboard(copyTable(colWidths));
}

void AbstractStdTable::copyTableResizeToLogSlot()
{
    std::vector<int> colWidths;
    int rowCount = getRowCount();
    int colCount = getColumnCount();
    for(int i = 0; i < colCount; i++)
    {
        auto max = getCellContent(0, i).length();
        for(int j = 1; j < rowCount; j++)
            max = std::max(getCellContent(j, i).length(), max);
        colWidths.push_back(max);
    }
    emit Bridge::getBridge()->addMsgToLog(copyTable(colWidths).toUtf8());
}

void AbstractStdTable::copyEntrySlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(!action)
        return;
    int col = action->objectName().toInt();
    QString finalText;
    for(int row : getSelection())
    {
        if(!finalText.isEmpty())
        {
            finalText += "\n";
        }
        finalText += getCellContent(row, col);
        while(finalText.endsWith(' '))
            finalText.chop(1);
    }
    Bridge::CopyToClipboard(finalText);
}

void AbstractStdTable::exportTableSlot()
{
    std::vector<QString> headers;
    headers.reserve(getColumnCount());
    for(duint i = 0; i < getColumnCount(); i++)
        headers.push_back(getColTitle(i));
    ExportCSV(getRowCount(), getColumnCount(), headers, [this](duint row, duint column)
    {
        return getCellContent(row, column);
    });
}

void AbstractStdTable::setupCopyMenu(QMenu* copyMenu)
{
    if(!getColumnCount())
        return;
    copyMenu->setIcon(DIcon("copy"));
    //Copy->Whole Line
    copyMenu->addAction(mCopyLine);
    //Copy->Cropped Table
    copyMenu->addAction(mCopyTable);
    //Copy->Full Table
    copyMenu->addAction(mCopyTableResize);
    //Copy->Separator
    copyMenu->addSeparator();
    //Copy->Whole Line To Log
    copyMenu->addAction(mCopyLineToLog);
    //Copy->Cropped Table To Log
    copyMenu->addAction(mCopyTableToLog);
    //Copy->Full Table To Log
    copyMenu->addAction(mCopyTableResizeToLog);
    //Copy->Export Table
    copyMenu->addAction(mExportTableCSV);
    //Copy->Separator
    copyMenu->addSeparator();
    //Copy->ColName
    setupCopyColumnMenu(copyMenu);
}

void AbstractStdTable::setupCopyColumnMenu(QMenu* copyMenu)
{
    for(duint i = 0; i < getColumnCount(); i++)
    {
        if(!getCellContent(getInitialSelection(), i).length()) //skip empty cells
            continue;
        QString title = mCopyTitles.at(i);
        if(!title.length()) //skip empty copy titles
            continue;
        QAction* mCopyAction = new QAction(DIcon("copy_item"), title, copyMenu);
        mCopyAction->setObjectName(QString::number(i));
        connect(mCopyAction, SIGNAL(triggered()), this, SLOT(copyEntrySlot()));
        copyMenu->addAction(mCopyAction);
    }
}

void AbstractStdTable::setupCopyMenu(MenuBuilder* copyMenu)
{
    if(!getColumnCount())
        return;
    //Copy->Whole Line
    copyMenu->addAction(mCopyLine);
    //Copy->Cropped Table
    copyMenu->addAction(mCopyTable);
    //Copy->Full Table
    copyMenu->addAction(mCopyTableResize);
    //Copy->Separator
    copyMenu->addSeparator();
    //Copy->Whole Line To Log
    copyMenu->addAction(mCopyLineToLog);
    //Copy->Cropped Table
    copyMenu->addAction(mCopyTableToLog);
    //Copy->Full Table
    copyMenu->addAction(mCopyTableResizeToLog);
    //Copy->Export Table
    copyMenu->addAction(mExportTableCSV);
    //Copy->Separator
    copyMenu->addSeparator();
    //Copy->ColName
    setupCopyColumnMenu(copyMenu);
}

void AbstractStdTable::setupCopyColumnMenu(MenuBuilder* copyMenu)
{
    copyMenu->addBuilder(new MenuBuilder(this, [this](QMenu * menu)
    {
        for(duint i = 0; i < getColumnCount(); i++)
        {
            if(!getCellContent(getInitialSelection(), i).length()) //skip empty cells
                continue;
            QString title = mCopyTitles.at(i);
            if(!title.length()) //skip empty copy titles
                continue;
            QAction* action = new QAction(DIcon("copy_item"), title, menu);
            action->setObjectName(QString::number(i));
            connect(action, SIGNAL(triggered()), this, SLOT(copyEntrySlot()));
            menu->addAction(action);
        }
        return true;
    }));
}

void AbstractStdTable::setCopyMenuOnly(bool bSet, bool bDebugOnly)
{
    mCopyMenuOnly = bSet;
    mCopyMenuDebugOnly = bDebugOnly;
}

void AbstractStdTable::contextMenuRequestedSlot(const QPoint & pos)
{
    if(pos.y() < getHeaderHeight())
        return;
    if(!mCopyMenuOnly)
    {
        emit contextMenuSignal(pos);
        return;
    }
    if(mCopyMenuDebugOnly && !DbgIsDebugging())
        return;
    auto menu = new QMenu(this);
    auto copyMenu = new QMenu(tr("&Copy"), this);
    setupCopyMenu(copyMenu);
    if(copyMenu->actions().length())
    {
        menu->addSeparator();
        menu->addMenu(copyMenu);
        menu->popup(mapToGlobal(pos));
    }
}

void AbstractStdTable::headerButtonPressedSlot(duint col)
{
    if(!mIsColumnSortingAllowed)
        return;

    if(mSort.column == -1 && col == mAddressColumn)
    {
        mSort.column = col;
        mSort.ascending = false;
    }
    else if(mSort.column != col)
    {
        mSort.column = col;
        mSort.ascending = true;
    }
    else
        mSort.ascending = !mSort.ascending;
    reloadData();

    emit sortChangedSignal();
}

void AbstractStdTable::reloadData()
{
    //TODO: do this on request, not every time reloadData is called...
    if(mSort.column != -1) //re-sort if the user wants to sort
    {
        sortRows(mSort.column, mSort.ascending);
    }
    AbstractTableView::reloadData();
}

duint AbstractStdTable::getAddressForPosition(int x, int y)
{
    auto c = getColumnIndexFromX(x);
    auto r = getTableOffset() + getIndexOffsetFromY(transY(y));
    if(r < getRowCount())
    {
        QString cell = getCellContent(r, c);
        duint addr;
        bool ok = false;
        addr = cell.toULongLong(&ok, 16);
        if(!ok)
            return 0;
        else
            return addr;
    }
    else
        return 0;
}
