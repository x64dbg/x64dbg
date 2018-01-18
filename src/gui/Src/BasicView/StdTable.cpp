#include "StdTable.h"
#include "Bridge.h"

StdTable::StdTable(QWidget* parent) : AbstractTableView(parent)
{
    SelectionData_t data;
    memset(&data, 0, sizeof(SelectionData_t));
    mSelection = data;

    mIsMultiSelectionAllowed = false;
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
    if(isSelected(rowBase, rowOffset))
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
                    if(mIsMultiSelectionAllowed && (event->modifiers() & Qt::ShiftModifier))
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

    if(!wAccept)
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

    if(wAccept)
        AbstractTableView::mouseReleaseEvent(event);
}

void StdTable::keyPressEvent(QKeyEvent* event)
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

void StdTable::enableMultiSelection(bool enabled)
{
    mIsMultiSelectionAllowed = enabled;
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

void StdTable::expandUp()
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

void StdTable::expandDown()
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

void StdTable::expandTop()
{
    if(getRowCount() > 0)
    {
        expandSelectionUpTo(0);
    }
}

void StdTable::expandBottom()
{
    int endIndex = getRowCount() - 1;
    if(endIndex >= 0)
    {
        expandSelectionUpTo(endIndex);
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

void StdTable::selectStart()
{
    if(getRowCount() > 0)
    {
        setSingleSelection(0);
    }
}

void StdTable::selectEnd()
{
    int endIndex = getRowCount() - 1;
    if(endIndex >= 0)
    {
        setSingleSelection(endIndex);
    }
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

void StdTable::selectAll()
{
    int index = 0;
    int indexEnd = getRowCount() - 1;

    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = indexEnd;

    emit selectionChangedSignal(index);
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
    for(size_t i = 0; i < mData.size(); i++)
        mData[i].push_back(CellData());

    //Append copy title
    if(!copyTitle.length())
        mCopyTitles.push_back(title);
    else
        mCopyTitles.push_back(copyTitle);
}

void StdTable::setRowCount(int count)
{
    int wRowToAddOrRemove = count - int(mData.size());
    for(int i = 0; i < qAbs(wRowToAddOrRemove); i++)
    {
        if(wRowToAddOrRemove > 0)
        {
            mData.push_back(std::vector<CellData>());
            for(int j = 0; j < getColumnCount(); j++)
                mData[mData.size() - 1].push_back(CellData());
        }
        else
            mData.pop_back();
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
    if(isValidIndex(r, c))
        mData[r][c].text = s;
}

QString StdTable::getCellContent(int r, int c)
{
    if(isValidIndex(r, c))
        return mData[r][c].text;
    else
        return QString("");
}

void StdTable::setCellUserdata(int r, int c, duint userdata)
{
    if(isValidIndex(r, c))
        mData[r][c].userdata = userdata;
}

duint StdTable::getCellUserdata(int r, int c)
{
    return isValidIndex(r, c) ? mData[r][c].userdata : 0;
}

bool StdTable::isValidIndex(int r, int c)
{
    if(r < 0 || c < 0 || r >= int(mData.size()))
        return false;
    return c < int(mData.at(r).size());
}

void StdTable::copyLineSlot()
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
    emit Bridge::getBridge()->addMsgToLog(finalText.toUtf8());
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
    emit Bridge::getBridge()->addMsgToLog(copyTable(colWidths).toUtf8());
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
    emit Bridge::getBridge()->addMsgToLog(copyTable(colWidths).toUtf8());
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
        QAction* mCopyAction = new QAction(DIcon("copy_item.png"), title, copyMenu);
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
            QAction* action = new QAction(DIcon("copy_item.png"), title, menu);
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
    {
        auto sortFn = getColumnSortBy(mSort.first);
        std::stable_sort(mData.begin(), mData.end(), [this, &sortFn](const std::vector<CellData> & a, const std::vector<CellData> & b)
        {
            auto less = sortFn(a.at(mSort.first).text, b.at(mSort.first).text);
            return mSort.second ? !less : less;
        });
    }
    AbstractTableView::reloadData();
}
