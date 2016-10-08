#include "StdTable.h"
#include "Bridge.h"

StdTable::StdTable(QWidget* parent) : AbstractTableView(parent)
{
    SelectionData_t data;
    memset(&data, 0, sizeof(SelectionData_t));
    mSelection = data;

    mIsMultiSelctionAllowed = false;
    mIsColumnSortingAllowed = true;

    mData.clear();
    mSort.first = -1;

    mGuiState = StdTable::NoState;

    mCopyMenuOnly = false;
    mCopyMenuDebugOnly = true;
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(Bridge::getBridge(), SIGNAL(repaintTableView()), this, SLOT(reloadData()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequestedSlot(QPoint)));
    connect(this, SIGNAL(headerButtonPressed(int)), this, SLOT(headerButtonPressedSlot(int)));
}

QString StdTable::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    if(isSelected(rowBase, rowOffset) == true)
        painter->fillRect(QRect(x, y, w, h), QBrush(selectionColor));
    return getCellContent(rowBase + rowOffset, col);
}

void StdTable::mouseMoveEvent(QMouseEvent* event)
{
    bool wAccept = true;
    int y = transY(event->y());

    if(mGuiState == StdTable::MultiRowsSelectionState)
    {
        //qDebug() << "State = MultiRowsSelectionState";

        if(y >= 0 && y <= this->getTableHeigth())
        {
            int wRowIndex = getTableOffset() + getIndexOffsetFromY(y);

            if(wRowIndex < getRowCount())
            {
                if(mIsMultiSelctionAllowed == true)
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
        else if(y > getTableHeigth())
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        }
    }

    if(wAccept == true)
        AbstractTableView::mouseMoveEvent(event);
}

void StdTable::mousePressEvent(QMouseEvent* event)
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
                    if(mIsMultiSelctionAllowed && (event->modifiers() & Qt::ShiftModifier))
                        expandSelectionUpTo(wRowIndex);
                    else
                        setSingleSelection(wRowIndex);

                    mGuiState = StdTable::MultiRowsSelectionState;

                    updateViewport();

                    wAccept = true;
                }
            }
        }
    }

    if(wAccept == false)
        AbstractTableView::mousePressEvent(event);
}

void StdTable::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->y() > getHeaderHeight() && event->button() == Qt::LeftButton)
        emit doubleClickedSignal();
    AbstractTableView::mouseDoubleClickEvent(event);
}

void StdTable::mouseReleaseEvent(QMouseEvent* event)
{
    bool wAccept = true;

    if((event->buttons() & Qt::LeftButton) == 0)
    {
        if(mGuiState == StdTable::MultiRowsSelectionState)
        {
            mGuiState = StdTable::NoState;

            updateViewport();

            wAccept = false;
        }
    }

    if(wAccept == true)
        AbstractTableView::mouseReleaseEvent(event);
}

void StdTable::keyPressEvent(QKeyEvent* event)
{
    emit keyPressedSignal(event);
    int key = event->key();

    if(key == Qt::Key_Up || key == Qt::Key_Down)
    {
        dsint wBotIndex = getTableOffset();
        dsint wTopIndex = wBotIndex + getNbrOfLineToPrint() - 1;

        if(key == Qt::Key_Up)
            selectPrevious();
        else
            selectNext();

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

void StdTable::enableMultiSelection(bool enabled)
{
    mIsMultiSelctionAllowed = enabled;
}

void StdTable::enableColumnSorting(bool enabled)
{
    mIsColumnSortingAllowed = enabled;
}

/************************************************************************************
                                Selection Management
************************************************************************************/
void StdTable::expandSelectionUpTo(int to)
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

void StdTable::setSingleSelection(int index)
{
    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = index;
    emit selectionChangedSignal(index);
}

int StdTable::getInitialSelection()
{
    return mSelection.firstSelectedIndex;
}

QList<int> StdTable::getSelection()
{
    QList<int> selection;
    selection.reserve(mSelection.toIndex - mSelection.fromIndex);
    for(int i = mSelection.fromIndex; i <= mSelection.toIndex; i++)
    {
        selection.append(i);
    }
    return selection;
}

void StdTable::selectNext()
{
    int wNext = getInitialSelection() + 1;

    // Bounding
    wNext = wNext > getRowCount() - 1 ? getRowCount() - 1 : wNext;
    wNext = wNext < 0  ? 0 : wNext;

    setSingleSelection(wNext);
}

void StdTable::selectPrevious()
{
    int wNext = getInitialSelection() - 1;

    // Bounding
    wNext = wNext > getRowCount() - 1 ? getRowCount() - 1 : wNext;
    wNext = wNext < 0  ? 0 : wNext;

    setSingleSelection(wNext);
}

bool StdTable::isSelected(int base, int offset)
{
    int wIndex = base + offset;

    if(wIndex >= mSelection.fromIndex && wIndex <= mSelection.toIndex)
        return true;
    else
        return false;
}

bool StdTable::scrollSelect(int offset)
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
void StdTable::addColumnAt(int width, QString title, bool isClickable, QString copyTitle, SortBy::t sortFn)
{
    AbstractTableView::addColumnAt(width, title, isClickable, sortFn);

    //append empty column to list of rows
    for(int i = 0; i < mData.size(); i++)
        mData[i].append("");

    //Append copy title
    if(!copyTitle.length())
        mCopyTitles.append(title);
    else
        mCopyTitles.append(copyTitle);
}

void StdTable::setRowCount(int count)
{
    int wRowToAddOrRemove = count - mData.size();
    for(int i = 0; i < qAbs(wRowToAddOrRemove); i++)
    {
        if(wRowToAddOrRemove > 0)
        {
            mData.append(QList<QString>());
            for(int j = 0; j < getColumnCount(); j++)
                mData.last().append("");
        }
        else
            mData.removeLast();
    }
    AbstractTableView::setRowCount(count);
}

void StdTable::deleteAllColumns()
{
    setRowCount(0);
    AbstractTableView::deleteAllColumns();
    mCopyTitles.clear();
}

void StdTable::setCellContent(int r, int c, QString s)
{
    if(isValidIndex(r, c) == true)
        mData[r].replace(c, s);
}

QString StdTable::getCellContent(int r, int c)
{
    if(isValidIndex(r, c) == true)
        return mData[r][c];
    else
        return QString("");
}

bool StdTable::isValidIndex(int r, int c)
{
    if(r < 0 || c < 0 || r >= mData.size())
        return false;
    return c < mData.at(r).size();
}

void StdTable::copyLineSlot()
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
    Bridge::CopyToClipboard(finalText);
}

void StdTable::copyLineToLogSlot()
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
    emit Bridge::getBridge()->addMsgToLog(finalText);
}

QString StdTable::copyTable(const std::vector<int> & colWidths)
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

void StdTable::copyTableSlot()
{
    std::vector<int> colWidths;
    int colCount = getColumnCount();
    for(int i = 0; i < colCount; i++)
        colWidths.push_back(getColumnWidth(i) / getCharWidth());
    Bridge::CopyToClipboard(copyTable(colWidths));
}

void StdTable::copyTableToLogSlot()
{
    std::vector<int> colWidths;
    int colCount = getColumnCount();
    for(int i = 0; i < colCount; i++)
        colWidths.push_back(getColumnWidth(i) / getCharWidth());
    emit Bridge::getBridge()->addMsgToLog(copyTable(colWidths));
}

void StdTable::copyTableResizeSlot()
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

void StdTable::copyTableResizeToLogSlot()
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
    emit Bridge::getBridge()->addMsgToLog(copyTable(colWidths));
}

void StdTable::copyEntrySlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(!action)
        return;
    int col = action->objectName().toInt();
    QString finalText = getCellContent(getInitialSelection(), col);
    while(finalText.endsWith(" ")) finalText.chop(1);
    Bridge::CopyToClipboard(finalText);
}

void StdTable::setupCopyMenu(QMenu* copyMenu)
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
    //Copy->ColName
    for(int i = 0; i < getColumnCount(); i++)
    {
        if(!getCellContent(getInitialSelection(), i).length()) //skip empty cells
            continue;
        QString title = mCopyTitles.at(i);
        if(!title.length()) //skip empty copy titles
            continue;
        QAction* mCopyAction = new QAction(title, copyMenu);
        mCopyAction->setObjectName(QString::number(i));
        connect(mCopyAction, SIGNAL(triggered()), this, SLOT(copyEntrySlot()));
        copyMenu->addAction(mCopyAction);
    }
}

void StdTable::setupCopyMenu(MenuBuilder* copyMenu)
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
    //Copy->ColName
    copyMenu->addBuilder(new MenuBuilder(this, [this](QMenu * menu)
    {
        for(int i = 0; i < getColumnCount(); i++)
        {
            if(!getCellContent(getInitialSelection(), i).length()) //skip empty cells
                continue;
            QString title = mCopyTitles.at(i);
            if(!title.length()) //skip empty copy titles
                continue;
            QAction* action = new QAction(title, menu);
            action->setObjectName(QString::number(i));
            connect(action, SIGNAL(triggered()), this, SLOT(copyEntrySlot()));
            menu->addAction(action);
        }
        return true;
    }));
}

void StdTable::setCopyMenuOnly(bool bSet, bool bDebugOnly)
{
    mCopyMenuOnly = bSet;
    mCopyMenuDebugOnly = bDebugOnly;
}

void StdTable::contextMenuRequestedSlot(const QPoint & pos)
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

void StdTable::headerButtonPressedSlot(int col)
{
    if(!mIsColumnSortingAllowed)
        return;
    if(mSort.first != col) //first = column to sort
    {
        mSort.first = col;
        mSort.second = false; //second = ascending/descending
    }
    else
        mSort.second = !mSort.second;
    reloadData();
}

void StdTable::reloadData()
{
    if(mSort.first != -1) //re-sort if the user wants to sort
        qSort(mData.begin(), mData.end(), ColumnCompare(mSort.first, mSort.second, getColumnSortBy(mSort.first)));
    AbstractTableView::reloadData();
}
