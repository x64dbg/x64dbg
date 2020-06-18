#include "AbstractStdTable.h"
#include "Bridge.h"
#include "RichTextPainter.h"

AbstractStdTable::AbstractStdTable(QWidget* parent) : AbstractTableView(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(Bridge::getBridge(), SIGNAL(repaintTableView()), this, SLOT(reloadData()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequestedSlot(QPoint)));
    connect(this, SIGNAL(headerButtonPressed(int)), this, SLOT(headerButtonPressedSlot(int)));

    Initialize();
}

QString AbstractStdTable::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    bool isaddr = DbgIsDebugging() && getRowCount() > 0 && col == mAddressColumn;
    bool wIsSelected = isSelected(rowBase, rowOffset);
    QString text = getCellContent(rowBase + rowOffset, col);

    duint wVA = isaddr ? duint(text.toULongLong(&isaddr, 16)) : 0;
    auto wIsTraced = isaddr && DbgFunctions()->GetTraceRecordHitCount(wVA) != 0;
    QColor lineBackgroundColor;
    bool isBackgroundColorSet;
    if(wIsSelected && wIsTraced)
    {
        lineBackgroundColor = mTracedSelectedAddressBackgroundColor;
        isBackgroundColorSet = true;
    }
    else if(wIsSelected)
    {
        lineBackgroundColor = mSelectionColor;
        isBackgroundColorSet = true;
    }
    else if(wIsTraced)
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
        if(bAddressLabel && DbgGetLabelAt(wVA, SEG_DEFAULT, label)) //has label
        {
            char module[MAX_MODULE_SIZE] = "";
            if(DbgGetModuleAt(wVA, module) && !QString(label).startsWith("JMP.&"))
                text += " <" + QString(module) + "." + QString(label) + ">";
            else
                text += " <" + QString(label) + ">";
        }
        BPXTYPE bpxtype = DbgGetBpxTypeAt(wVA);
        bool isbookmark = DbgGetBookmarkAt(wVA);

        duint cip = Bridge::getBridge()->mLastCip;
        if(bCipBase)
        {
            duint base = DbgFunctions()->ModBaseFromAddr(cip);
            if(base)
                cip = base;
        }

        if(DbgIsDebugging() && wVA == cip) //debugging + cip
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
                        if(wIsSelected)
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
                            if(wIsSelected)
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
    else if(mHighlightText.length() && text.contains(mHighlightText, Qt::CaseInsensitive)) // TODO: case sensitive + regex highlighting
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
        QStringList split = text.split(QChar('\1'), QString::SkipEmptyParts, Qt::CaseInsensitive);

        //create rich text list
        RichTextPainter::CustomRichText_t curRichText;
        curRichText.flags = RichTextPainter::FlagColor;
        curRichText.textColor = getCellColor(rowBase + rowOffset, col);
        curRichText.highlightColor = ConfigColor("SearchListViewHighlightColor");
        RichTextPainter::List richText;
        foreach(QString str, split)
        {
            curRichText.text = str;
            curRichText.highlight = !str.compare(mHighlightText, Qt::CaseInsensitive);
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
    bool wAccept = true;
    int y = transY(event->y());

    if(mGuiState == AbstractStdTable::MultiRowsSelectionState)
    {
        //qDebug() << "State = MultiRowsSelectionState";

        if(y >= 0 && y <= this->getTableHeight())
        {
            int wRowIndex = getTableOffset() + getIndexOffsetFromY(y);

            if(wRowIndex < getRowCount())
            {
                if(mIsMultiSelectionAllowed)
                    expandSelectionUpTo(wRowIndex);
                else
                    setSingleSelection(wRowIndex);

                updateViewport();

                wAccept = false;
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

    if(wAccept)
        AbstractTableView::mouseMoveEvent(event);
}

void AbstractStdTable::mousePressEvent(QMouseEvent* event)
{
    bool wAccept = false;

    if(((event->buttons() & Qt::LeftButton) != 0) && ((event->buttons() & Qt::RightButton) == 0))
    {
        if(getGuiState() == AbstractTableView::NoState)
        {
            if(event->y() > getHeaderHeight())
            {
                int wRowIndex = getTableOffset() + getIndexOffsetFromY(transY(event->y()));

                if(wRowIndex < getRowCount())
                {
                    if(mIsMultiSelectionAllowed && (event->modifiers() & Qt::ShiftModifier))
                        expandSelectionUpTo(wRowIndex);
                    else
                        setSingleSelection(wRowIndex);

                    mGuiState = AbstractStdTable::MultiRowsSelectionState;

                    updateViewport();

                    wAccept = true;
                }
            }
        }
    }

    if(!wAccept)
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
    bool wAccept = true;

    if((event->buttons() & Qt::LeftButton) == 0)
    {
        if(mGuiState == AbstractStdTable::MultiRowsSelectionState)
        {
            mGuiState = AbstractStdTable::NoState;

            updateViewport();

            wAccept = false;
        }
    }

    if(wAccept)
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
            key == Qt::Key_A)
    {
        dsint wBotIndex = getTableOffset();
        dsint wTopIndex = wBotIndex + getNbrOfLineToPrint() - 1;

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
        }

        if(getInitialSelection() < wBotIndex)
        {
            setTableOffset(getInitialSelection());
        }
        else if(getInitialSelection() >= wTopIndex)
        {
            setTableOffset(getInitialSelection() - getNbrOfLineToPrint() + 2);
        }

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
void AbstractStdTable::expandSelectionUpTo(int to)
{
    if(to < mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = to;
        mSelection.toIndex = mSelection.firstSelectedIndex;
        emit selectionChangedSignal(to);
    }
    else if(to > mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = mSelection.firstSelectedIndex;
        mSelection.toIndex = to;
        emit selectionChangedSignal(to);
    }
    else if(to == mSelection.firstSelectedIndex)
    {
        setSingleSelection(to);
    }
}

void AbstractStdTable::expandUp()
{
    int wRowIndex = mSelection.firstSelectedIndex - 1;
    if(wRowIndex >= 0)
    {
        if(wRowIndex < mSelection.fromIndex)
        {
            mSelection.fromIndex = wRowIndex;
            mSelection.firstSelectedIndex = wRowIndex;

        }
        else
        {
            mSelection.firstSelectedIndex = wRowIndex;
            mSelection.toIndex = wRowIndex;
        }

        emit selectionChangedSignal(wRowIndex);
    }
}

void AbstractStdTable::expandDown()
{
    int wRowIndex = mSelection.firstSelectedIndex + 1;
    int endIndex = getRowCount() - 1;
    if(wRowIndex <= endIndex)
    {

        if(wRowIndex > mSelection.toIndex)
        {
            mSelection.firstSelectedIndex = wRowIndex;
            mSelection.toIndex = wRowIndex;

        }
        else
        {
            mSelection.fromIndex = wRowIndex;
            mSelection.firstSelectedIndex = wRowIndex;
        }


        emit selectionChangedSignal(wRowIndex);
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
    int endIndex = getRowCount() - 1;
    if(endIndex >= 0)
    {
        expandSelectionUpTo(endIndex);
    }
}

void AbstractStdTable::setSingleSelection(int index)
{
    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = index;
    emit selectionChangedSignal(index);
}

int AbstractStdTable::getInitialSelection() const
{
    return mSelection.firstSelectedIndex;
}

QList<int> AbstractStdTable::getSelection() const
{
    QList<int> selection;
    selection.reserve(mSelection.toIndex - mSelection.fromIndex);
    for(int i = mSelection.fromIndex; i <= mSelection.toIndex; i++)
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
    int endIndex = getRowCount() - 1;
    if(endIndex >= 0)
    {
        setSingleSelection(endIndex);
    }
}

void AbstractStdTable::selectNext()
{
    int wNext = getInitialSelection() + 1;

    // Bounding
    wNext = wNext > getRowCount() - 1 ? getRowCount() - 1 : wNext;
    wNext = wNext < 0  ? 0 : wNext;

    setSingleSelection(wNext);
}

void AbstractStdTable::selectPrevious()
{
    int wNext = getInitialSelection() - 1;

    // Bounding
    wNext = wNext > getRowCount() - 1 ? getRowCount() - 1 : wNext;
    wNext = wNext < 0  ? 0 : wNext;

    setSingleSelection(wNext);
}

void AbstractStdTable::selectAll()
{
    int index = 0;
    int indexEnd = getRowCount() - 1;

    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = indexEnd;

    emit selectionChangedSignal(index);
}

bool AbstractStdTable::isSelected(int base, int offset) const
{
    int wIndex = base + offset;

    if(wIndex >= mSelection.fromIndex && wIndex <= mSelection.toIndex)
        return true;
    else
        return false;
}

bool AbstractStdTable::scrollSelect(int offset)
{
    if(!isValidIndex(offset, 0))
        return false;

    int rangefrom = getTableOffset();
    int rangeto = rangefrom + getViewableRowsCount() - 1;
    if(offset < rangefrom) //offset lays before the current view
        setTableOffset(offset);
    else if(offset > (rangeto - 1)) //offset lays after the current view
        setTableOffset(offset - getViewableRowsCount() + 2);
    setSingleSelection(offset);
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
    int colCount = getColumnCount();
    QString finalText = "";
    if(colCount == 1)
        finalText = getCellContent(getInitialSelection(), 0);
    else
    {
        for(int selected : getSelection())
        {
            for(int i = 0; i < colCount; i++)
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
    int colCount = getColumnCount();
    int selected = getInitialSelection();
    QString finalText = "";
    if(colCount == 1)
        finalText = getCellContent(selected, 0);
    else
    {
        for(int i = 0; i < colCount; i++)
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
    int colCount = getColumnCount();
    int rowCount = getRowCount();
    QString finalText = "";
    if(colCount == 1)
    {
        for(int i = 0; i < rowCount; i++)
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
        for(int i = 0; i < colCount; i++)
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
        for(int i = 0; i < rowCount; i++)
        {
            QString finalRowText = "";
            for(int j = 0; j < colCount; j++)
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
    int rowCount = getRowCount();
    int colCount = getColumnCount();
    for(int i = 0; i < colCount; i++)
    {
        int max = getCellContent(0, i).length();
        for(int j = 1; j < rowCount; j++)
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
        int max = getCellContent(0, i).length();
        for(int j = 1; j < rowCount; j++)
            max = std::max(getCellContent(j, i).length(), max);
        colWidths.push_back(max);
    }
    emit Bridge::getBridge()->addMsgToLog(copyTable(colWidths).toUtf8());
}

void AbstractStdTable::copyTableToReferencesSlot()
{
    GuiReferenceInitialize(tr("Copied table").toUtf8().constData());
    auto colCount = getColumnCount();
    for(int col = 0; col < colCount; col++)
        GuiReferenceAddColumn(getColumnWidth(col) / getCharWidth(), getColTitle(col).toUtf8().constData());
    auto rowCount = (int)getRowCount();
    GuiReferenceSetRowCount(rowCount);
    for(auto row = 0; row < rowCount; row++)
        for(int col = 0; col < colCount; col++)
            GuiReferenceSetCellContent(row, col, getCellContent(row, col).toUtf8().constData());
    GuiReferenceReloadData();
}

void AbstractStdTable::copyEntrySlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(!action)
        return;
    int col = action->objectName().toInt();
    QString finalText;
    for(auto row : getSelection())
    {
        if(!finalText.isEmpty())
            finalText += "\r\n";
        finalText += getCellContent(row, col).trimmed();
    }
    Bridge::CopyToClipboard(finalText);
}

void AbstractStdTable::setupCopyMenu(QMenu* copyMenu)
{
    if(!getColumnCount())
        return;
    copyMenu->setIcon(DIcon("copy.png"));
    //Copy->Whole Line
    QAction* mCopyLine = new QAction(DIcon("copy_table_line.png"), tr("&Line"), copyMenu);
    connect(mCopyLine, SIGNAL(triggered()), this, SLOT(copyLineSlot()));
    copyMenu->addAction(mCopyLine);
    //Copy->Cropped Table
    QAction* mCopyTable = new QAction(DIcon("copy_cropped_table.png"), tr("Cropped &Table"), copyMenu);
    connect(mCopyTable, SIGNAL(triggered()), this, SLOT(copyTableSlot()));
    copyMenu->addAction(mCopyTable);
    //Copy->Full Table
    QAction* mCopyTableResize = new QAction(DIcon("copy_full_table.png"), tr("&Full Table"), copyMenu);
    connect(mCopyTableResize, SIGNAL(triggered()), this, SLOT(copyTableResizeSlot()));
    copyMenu->addAction(mCopyTableResize);
    //Copy->Separator
    copyMenu->addSeparator();
    //Copy->Whole Line To Log
    QAction* mCopyLineToLog = new QAction(DIcon("copy_table_line.png"), tr("Line, To Log"), copyMenu);
    connect(mCopyLineToLog, SIGNAL(triggered()), this, SLOT(copyLineToLogSlot()));
    copyMenu->addAction(mCopyLineToLog);
    //Copy->Cropped Table To Log
    QAction* mCopyTableToLog = new QAction(DIcon("copy_cropped_table.png"), tr("Cropped Table, To Log"), copyMenu);
    connect(mCopyTableToLog, SIGNAL(triggered()), this, SLOT(copyTableToLogSlot()));
    copyMenu->addAction(mCopyTableToLog);
    //Copy->Full Table To Log
    QAction* mCopyTableResizeToLog = new QAction(DIcon("copy_full_table.png"), tr("Full Table, To Log"), copyMenu);
    connect(mCopyTableResizeToLog, SIGNAL(triggered()), this, SLOT(copyTableResizeToLogSlot()));
    copyMenu->addAction(mCopyTableResizeToLog);
    //Copy->Separator
    copyMenu->addSeparator();
    if(mAddressColumn != -1)
    {
        //Copy->Full Table, To References
        QAction* mCopyFullTableToReferences = new QAction(DIcon("references.png"), tr("Full Table, To References"), copyMenu);
        connect(mCopyFullTableToReferences, SIGNAL(triggered(bool)), this, SLOT(copyTableToReferencesSlot()));
        copyMenu->addAction(mCopyFullTableToReferences);
        //Copy->Separator
        copyMenu->addSeparator();
    }
    //Copy->ColName
    setupCopyColumnMenu(copyMenu);
}

void AbstractStdTable::setupCopyColumnMenu(QMenu* copyMenu)
{
    for(int i = 0; i < getColumnCount(); i++)
    {
        if(!getCellContent(getInitialSelection(), i).length()) //skip empty cells
            continue;
        QString title = mCopyTitles.at(i);
        if(!title.length()) //skip empty copy titles
            continue;
        QAction* mCopyAction = new QAction(DIcon("copy_item.png"), title, copyMenu);
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
    copyMenu->addAction(makeAction(DIcon("copy_table_line.png"), tr("&Line"), SLOT(copyLineSlot())));
    //Copy->Cropped Table
    copyMenu->addAction(makeAction(DIcon("copy_cropped_table.png"), tr("Cropped &Table"), SLOT(copyTableSlot())));
    //Copy->Full Table
    copyMenu->addAction(makeAction(DIcon("copy_full_table.png"), tr("&Full Table"), SLOT(copyTableResizeSlot())));
    //Copy->Separator
    copyMenu->addSeparator();
    //Copy->Whole Line To Log
    copyMenu->addAction(makeAction(DIcon("copy_table_line.png"), tr("Line, To Log"), SLOT(copyLineToLogSlot())));
    //Copy->Cropped Table
    copyMenu->addAction(makeAction(DIcon("copy_cropped_table.png"), tr("Cropped Table, To Log"), SLOT(copyTableToLogSlot())));
    //Copy->Full Table
    copyMenu->addAction(makeAction(DIcon("copy_full_table.png"), tr("Full Table, To Log"), SLOT(copyTableResizeToLogSlot())));
    //Copy->Separator
    copyMenu->addSeparator();
    if(mAddressColumn != -1)
    {
        //Copy->Full Table, To References
        copyMenu->addAction(makeAction(DIcon("references.png"), tr("Full Table, To References"), SLOT(copyTableToReferencesSlot())));
        //Copy->Separator
        copyMenu->addSeparator();
    }
    //Copy->ColName
    setupCopyColumnMenu(copyMenu);
}

void AbstractStdTable::setupCopyColumnMenu(MenuBuilder* copyMenu)
{
    copyMenu->addBuilder(new MenuBuilder(this, [this](QMenu * menu)
    {
        setupCopyColumnMenu(menu);
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
    if(!mCopyMenuOnly)
    {
        emit contextMenuSignal(pos);
        return;
    }
    if(mCopyMenuDebugOnly && !DbgIsDebugging())
        return;
    QMenu wMenu(this);
    QMenu wCopyMenu(tr("&Copy"), this);
    setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu.addSeparator();
        wMenu.addMenu(&wCopyMenu);
        wMenu.exec(mapToGlobal(pos));
    }
}

void AbstractStdTable::headerButtonPressedSlot(int col)
{
    if(!mIsColumnSortingAllowed)
        return;
    if(mSort.column != col)
    {
        mSort.column = col;
        mSort.ascending = true;
    }
    else
        mSort.ascending = !mSort.ascending;
    reloadData();
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

duint AbstractStdTable::getDisassemblyPopupAddress(int mousex, int mousey)
{
    if(!bDisassemblyPopupEnabled) //No disassembly popup is meaningful for this table
        return 0;
    int c = getColumnIndexFromX(mousex);
    int r = getTableOffset() + getIndexOffsetFromY(transY(mousey));
    if(r < getRowCount())
    {
        QString cell = getCellContent(r, c);
        duint addr;
        bool ok = false;
#ifdef _WIN64
        addr = cell.toULongLong(&ok, 16);
#else //x86
        addr = cell.toULong(&ok, 16);
#endif //_WIN64
        if(!ok)
            return 0;
        else
            return addr;
    }
    else
        return 0;
}
