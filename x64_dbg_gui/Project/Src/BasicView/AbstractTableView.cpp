#include "AbstractTableView.h"

AbstractTableView::AbstractTableView(QWidget *parent) : QAbstractScrollArea(parent)
{
    // Class variable initialization
    mTableOffset = 0;
    mPrevTableOffset = mTableOffset + 1;
    Header_t data;
    data.isVisible=true;
    data.height=20;
    data.activeButtonIndex=-1;
    mHeader = data;

    QFont font("Monospace", 8);
    //QFont font("Terminal", 6);
    //font.setBold(true); //bold

    font.setFixedPitch(true);
    //font.setStyleHint(QFont::Monospace);
    this->setFont(font);


    int wRowsHeight = QFontMetrics(this->font()).height();
    wRowsHeight = (wRowsHeight * 105) / 100;
    wRowsHeight = (wRowsHeight % 2) == 0 ? wRowsHeight : wRowsHeight + 1;
    mRowHeight = wRowsHeight;

    mRowCount = 0;

    mHeaderButtonSytle.setStyleSheet(" QPushButton {\n     background-color: rgb(192, 192, 192);\n     border-style: outset;\n     border-width: 2px;\n     border-color: rgb(128, 128, 128);\n }\n QPushButton:pressed {\n     background-color: rgb(192, 192, 192);\n     border-style: inset;\n }");

    mNbrOfLineToPrint = 0;

    mColResizeData = (ColumnResizingData_t){false, 0, 0};

    mGuiState = AbstractTableView::NoState;

    mShouldReload = true;

    // ScrollBar Init
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    mScrollBarAttributes = (ScrollBar64_t){false, 0};

    setMouseTracking(true);

    // Signals/Slots Connections
    connect(verticalScrollBar(), SIGNAL(actionTriggered(int)), this, SLOT(vertSliderActionSlot(int)));
}


/************************************************************************************
                            Painting Stuff
************************************************************************************/
/**
 * @brief       This method has been reimplemented. It paints the whole table.
 *
 * @param[in]   event       Paint event
 *
 * @return      Nothing.
 */
void AbstractTableView::paintEvent(QPaintEvent* event)
{
    QPainter wPainter(this->viewport());
    int wViewableRowsCount = getViewableRowsCount();

    int x = 0;
    int y = 0;

    // Reload data if needed
    if(mPrevTableOffset != mTableOffset || mShouldReload == true)
    {
        prepareData();
        mPrevTableOffset = mTableOffset;
    }

    // Paints background
    wPainter.fillRect(wPainter.viewport(), QBrush(QColor(255, 251, 240)));

    // Paints header
    if(mHeader.isVisible == true)
    {
        for(int i = 0; i < getColumnCount(); i++)
        {
            QStyleOptionButton wOpt;

            if((mColumnList[i].header.isPressed == true) && (mColumnList[i].header.isMouseOver == true))
                wOpt.state = QStyle::State_Sunken;
            else
                wOpt.state = QStyle::State_Enabled;

            wOpt.rect = QRect(x, y, getColumnWidth(i), getHeaderHeigth());

            mHeaderButtonSytle.style()->drawControl(QStyle::CE_PushButton, &wOpt, &wPainter,&mHeaderButtonSytle);

            x += getColumnWidth(i);
        }
        x = 0;
        y = getHeaderHeigth();
    }

    // Iterate over all columns and cells
    for(int j = 0; j < getColumnCount(); j++)
    {
        for(int i = 0; i < wViewableRowsCount; i++)
        {
            //  Paints cell contents
            if(i < mNbrOfLineToPrint)
            {
               QString wStr = paintContent(&wPainter, mTableOffset, i, j, x, y, getColumnWidth(j), getRowHeight());
               wPainter.drawText(QRect(x + 4, y, getColumnWidth(j) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, wStr);
            }

            // Paints cell right borders
            wPainter.save() ;
            wPainter.setPen(QColor(128, 128, 128));
            wPainter.drawLine(x + getColumnWidth(j) - 1, y, x + getColumnWidth(j) - 1, y + getRowHeight() - 1);
            wPainter.restore();

            // Update y for the next iteration
            y += getRowHeight();
        }

        y = getHeaderHeigth();
        x += getColumnWidth(j);
    }
}


/************************************************************************************
                            Mouse Management
************************************************************************************/
/**
 * @brief       This method has been reimplemented. It manages the following actions:
 *               - Column resizing
 *               - Header button
 *
 * @param[in]   event       Mouse event
 *
 * @return      Nothing.
 */
void AbstractTableView::mouseMoveEvent(QMouseEvent* event)
{
   // qDebug() << "mouseMoveEvent";

    switch (mGuiState)
    {
        case AbstractTableView::NoState:
        {
            //qDebug() << "State = NoState";

            int wColIndex = getColumnIndexFromX(event->x());
            int wStartPos = getColumnPosition(wColIndex); // Position X of the start of column
            int wEndPos = getColumnPosition(wColIndex) + getColumnWidth(wColIndex); // Position X of the end of column

            if(event->buttons() == Qt::NoButton)
            {
                bool wHandle = true;
                bool wHasCursor;

                wHasCursor = cursor().shape() == Qt::SplitHCursor ? true : false;

                if(((wColIndex != 0) && (event->x() >= wStartPos) && (event->x() <= (wStartPos + 2))) || ((wColIndex != (getColumnCount() - 1)) && (event->x() <= wEndPos) && (event->x() >= (wEndPos - 2))))
                {
                    wHandle = true;
                }
                else
                {
                    wHandle = false;
                }

                if((wHandle == true) && (wHasCursor == false))
                {
                    setCursor(Qt::SplitHCursor);
                    mColResizeData.splitHandle = true;
                    mGuiState = AbstractTableView::ReadyToResize;
                }
                if ((wHandle == false) && (wHasCursor == true))
                {
                    unsetCursor();
                    mColResizeData.splitHandle = false;
                    mGuiState = AbstractTableView::NoState;
                }
            }
            else
            {
                QWidget::mouseMoveEvent(event);
            }
            break;
        }
        case AbstractTableView::ReadyToResize:
        {
            //qDebug() << "State = ReadyToResize";

            int wColIndex = getColumnIndexFromX(event->x());
            int wStartPos = getColumnPosition(wColIndex); // Position X of the start of column
            int wEndPos = getColumnPosition(wColIndex) + getColumnWidth(wColIndex); // Position X of the end of column

            if(event->buttons() == Qt::NoButton)
            {
                bool wHandle = true;

                if(((wColIndex != 0) && (event->x() >= wStartPos) && (event->x() <= (wStartPos + 2))) || ((wColIndex != (getColumnCount() - 1)) && (event->x() <= wEndPos) && (event->x() >= (wEndPos - 2))))
                {
                    wHandle = true;
                }
                else
                {
                    wHandle = false;
                }

                if ((wHandle == false) && (mGuiState == AbstractTableView::ReadyToResize))
                {
                    unsetCursor();
                    mColResizeData.splitHandle = false;
                    mGuiState = AbstractTableView::NoState;
                }
            }
            break;
        }
        case AbstractTableView::ResizeColumnState:
        {
            //qDebug() << "State = ResizeColumnState";

            int delta = event->x() - mColResizeData.lastPosX;

            int wNewSize = ((getColumnWidth(mColResizeData.index) + delta) >= 20) ? (getColumnWidth(mColResizeData.index) + delta) : (20);

            setColumnWidth(mColResizeData.index, wNewSize);

            mColResizeData.lastPosX = event->x();

            repaint();

            break;
        }
        case AbstractTableView::HeaderButtonPressed:
        {
            //qDebug() << "State = HeaderButtonPressed";

            int wColIndex = getColumnIndexFromX(event->x());

            if((wColIndex == mHeader.activeButtonIndex) && (event->y() <= getHeaderHeigth()) && (event->y() >= 0))
            {
                mColumnList[mHeader.activeButtonIndex].header.isMouseOver = true;
            }
            else
            {
                mColumnList[mHeader.activeButtonIndex].header.isMouseOver = false;
            }

            repaint();
        }
        default:
            break;
    }
}


/**
 * @brief       This method has been reimplemented. It manages the following actions:
 *               - Column resizing
 *               - Header button
 *
 * @param[in]   event       Mouse event
 *
 * @return      Nothing.
 */
void AbstractTableView::mousePressEvent(QMouseEvent* event)
{
    if(((event->buttons() & Qt::LeftButton) != 0) && ((event->buttons() & Qt::RightButton) == 0))
    {
        if(mColResizeData.splitHandle == true)
        {
            int wColIndex = getColumnIndexFromX(event->x());
            int wStartPos = getColumnPosition(wColIndex); // Position X of the start of column

            mGuiState = AbstractTableView::ResizeColumnState;

            if(event->x() <= (wStartPos + 2))
            {
                mColResizeData.index = wColIndex - 1;
            }
            else
            {
                mColResizeData.index = wColIndex;
            }

            mColResizeData.lastPosX = event->x();
        }
        else if((mHeader.isVisible == true) && (event->y() <= getHeaderHeigth()) && (event->y() >= 0))
        {
            int wColIndex = getColumnIndexFromX(event->x());

            qDebug() << "Button " << wColIndex << "has been pressed.";
            emit headerButtonPressed(wColIndex);

            mColumnList[wColIndex].header.isPressed = true;
            mColumnList[wColIndex].header.isMouseOver = true;

            mHeader.activeButtonIndex = wColIndex;

            mGuiState = AbstractTableView::HeaderButtonPressed;

            repaint();
        }
    }

    //QWidget::mousePressEvent(event);
}


/**
 * @brief       This method has been reimplemented. It manages the following actions:
 *               - Column resizing
 *               - Header button
 *
 * @param[in]   event       Mouse event
 *
 * @return      Nothing.
 */
void AbstractTableView::mouseReleaseEvent(QMouseEvent* event)
{
    if((event->buttons() & Qt::LeftButton) == 0)
    {
        if(mGuiState == AbstractTableView::ResizeColumnState)
        {
            mGuiState = AbstractTableView::NoState;
        }
        else if(mGuiState == AbstractTableView::HeaderButtonPressed)
        {
            if(mColumnList[mHeader.activeButtonIndex].header.isMouseOver == true)
            {
                qDebug() << "Button " << mHeader.activeButtonIndex << "has been released.";
                emit headerButtonReleased(mHeader.activeButtonIndex);
            }
            mGuiState = AbstractTableView::NoState;
        }
        else
        {
            QWidget::mouseReleaseEvent(event);
        }

        // Release all buttons
        for(int i = 0; i < getColumnCount(); i++)
        {
            mColumnList[i].header.isPressed = false;
        }

        repaint();
    }
}


/**
 * @brief       This method has been reimplemented. It manages the following actions:
 *               - Mouse wheel
 *
 * @param[in]   event       Wheel event
 *
 * @return      Nothing.
 */
void AbstractTableView::wheelEvent(QWheelEvent* event)
{
    //qDebug() << "wheelEvent";

    if(event->delta() > 0)
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    else
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);

    //QAbstractScrollArea::wheelEvent(event);
}


/**
 * @brief       This method has been reimplemented. It repaints the table when the height changes.
 *
 * @param[in]   event       Resize event
 *
 * @return      Nothing.
 */
void AbstractTableView::resizeEvent(QResizeEvent* event)
{
    if(event->size().height() != event->oldSize().height())
    {
        mShouldReload = true;
    }

    QWidget::resizeEvent(event);
}


/************************************************************************************
                            Keyboard Management
************************************************************************************/
/**
 * @brief       This method has been reimplemented. It manages the following actions:
 *               - Pressed keys
 *
 * @param[in]   event       Key event
 *
 * @return      Nothing.
 */
void AbstractTableView::keyPressEvent(QKeyEvent* event)
{
    int wKey = event->key();

    if(wKey == Qt::Key_Up)
    {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
    }
    else if(wKey == Qt::Key_Down)
    {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
    }
}


/************************************************************************************
                            ScrollBar Management
 ***********************************************************************************/
/**
 * @brief       This method is the slot connected to the actionTriggered signal of the vertical scrollbar.
 *
 * @param[in]   action      Slider action type
 *
 * @return      Nothing.
 */
void AbstractTableView::vertSliderActionSlot(int action)
{
    int_t wDelta;
    int wSliderPos = verticalScrollBar()->sliderPosition();
    int wNewScrollBarValue;

    // Bounding
    wSliderPos = wSliderPos > verticalScrollBar()->maximum() ? verticalScrollBar()->maximum() : wSliderPos;
    wSliderPos = wSliderPos < 0 ? 0 : wSliderPos;

    // Determine the delta
    switch(action)
    {
        case QAbstractSlider::SliderNoAction:
            break;
        case QAbstractSlider::SliderSingleStepAdd:
            wDelta = 1;
            break;
        case QAbstractSlider::SliderSingleStepSub:
            wDelta = -1;
            break;
        case QAbstractSlider::SliderPageStepAdd:
            wDelta = 3;
            break;
        case QAbstractSlider::SliderPageStepSub:
            wDelta = -3;
            break;
        case QAbstractSlider::SliderToMinimum:
        case QAbstractSlider::SliderToMaximum:
        case QAbstractSlider::SliderMove:
#ifdef _WIN64
            wDelta = scaleFromScrollBarRangeToUint64(wSliderPos) - mTableOffset;
#else
            wDelta = wSliderPos - mTableOffset;
#endif
            break;
        default:
            break;
    }

    // Call the hook (Usefull for disassembly)
    mTableOffset = sliderMovedHook(action, mTableOffset, wDelta);

    // Scale the new table offset to the 32bits scrollbar range
#ifdef _WIN64
    wNewScrollBarValue = scaleFromUint64ToScrollBarRange(mTableOffset);
#else
    wNewScrollBarValue = mTableOffset;
#endif

    // Update scrollbar attributes
    verticalScrollBar()->setValue(wNewScrollBarValue);
    verticalScrollBar()->setSliderPosition(wNewScrollBarValue);
}


/**
 * @brief       This virtual method is called at the end of the vertSliderActionSlot(...) method.
 *              It allows changing the table offset according to the action type, the old table offset
 *              and delta between the old and the new table offset.
 *
 * @param[in]   type      Type of action (Refer to the QAbstractSlider::SliderAction enum)
 * @param[in]   value     Old table offset
 * @param[in]   delta     Scrollbar value delta compared to the previous state
 *
 * @return      Return the value of the new table offset.
 */
int_t AbstractTableView::sliderMovedHook(int type, int_t value, int_t delta)
{
    int_t wValue = value + delta;
    int_t wMax = getRowCount() - 1;

    // Bounding
    wValue = wValue > wMax ? wMax : wValue;
    wValue = wValue < 0 ? 0 : wValue;

    return wValue;
}


/**
 * @brief       This method scale the given 64bits integer to the scrollbar range (32bits).
 *
 * @param[in]   value      64bits integer to rescale
 *
 * @return      32bits integer.
 */
#ifdef _WIN64
int AbstractTableView::scaleFromUint64ToScrollBarRange(int_t value)
{
    if(mScrollBarAttributes.is64 == true)
    {
        int_t wValue = ((int_t)value) >> mScrollBarAttributes.rightShiftCount;
        int_t wValueMax = ((int_t)getRowCount() - 1) >> mScrollBarAttributes.rightShiftCount;

        if(value == ((int_t)getRowCount() - 1))
            return (int)(verticalScrollBar()->maximum());
        else
            return (int)((int_t)((int_t)verticalScrollBar()->maximum() * (int_t)wValue) / (int_t)wValueMax);
    }
    else
    {
        return (int)value;
    }
}
#endif


/**
 * @brief       This method scale the given 32bits integer to the table range (64bits).
 *
 * @param[in]   value      32bits integer to rescale
 *
 * @return      64bits integer.
 */
#ifdef _WIN64
int_t AbstractTableView::scaleFromScrollBarRangeToUint64(int value)
{
    if(mScrollBarAttributes.is64 == true)
    {
        int_t wValueMax = ((int_t)getRowCount() - 1) >> mScrollBarAttributes.rightShiftCount;

        if(value == (int)0x7FFFFFFF)
            return (int_t)(getRowCount() - 1);
        else
            return (int_t)(((int_t)((int_t)wValueMax * (int_t)value) / (int_t)0x7FFFFFFF) << mScrollBarAttributes.rightShiftCount);
    }
    else
    {
        return (int_t)value;
    }
}
#endif


/**
 * @brief       This method updates the scrollbar range and pre-computes some attributes for the 32<->64bits conversion methods.
 *
 * @param[in]   range New table range (size)
 *
 * @return      32bits integer.
 */
void AbstractTableView::updateScrollBarRange(int_t range)
{
    int_t wMax = range--;

    if(wMax > 0)
    {
#ifdef _WIN64
        if((uint_t)wMax < (uint_t)0x0000000080000000)
        {
            mScrollBarAttributes.is64 = false;
            mScrollBarAttributes.rightShiftCount = 0;
            verticalScrollBar()->setRange(0, wMax);
        }
        else
        {
            uint_t wMask = 0x8000000000000000;
            int wLeadingZeroCount;

            // Count leading zeros
            for(wLeadingZeroCount = 0; wLeadingZeroCount < 64; wLeadingZeroCount++)
            {
                if((uint_t)wMax < wMask)
                {
                    wMask = wMask >> 1;
                }
                else
                {
                    break;
                }
            }

            mScrollBarAttributes.is64 = true;
            mScrollBarAttributes.rightShiftCount = 32 - wLeadingZeroCount;
            verticalScrollBar()->setRange(0, 0x7FFFFFFF);
        }
#else
        verticalScrollBar()->setRange(0, wMax);
#endif
    }
}


/************************************************************************************
                            Coordinates Utils
************************************************************************************/
/**
 * @brief       Returns the index offset (relative to the table offset) corresponding to the given y coordinate.
 *
 * @param[in]   y      Pixel offset starting from the top of the table (without the header)
 *
 * @return      row index offset.
 */
int AbstractTableView::getIndexOffsetFromY(int y)
{
    return (y / getRowHeight());
}


/**
 * @brief       Returns the index of the column corresponding to the given x coordinate.
 *
 * @param[in]   x      Pixel offset starting from the left of the table
 *
 * @return      Column index.
 */
int AbstractTableView::getColumnIndexFromX(int x)
{
    int wX = 0;
    int wColIndex = 0;

    while(wColIndex < getColumnCount())
    {
        wX += getColumnWidth(wColIndex);

        if(x <= wX)
        {
            return wColIndex;
        }
        else if(wColIndex < getColumnCount())
        {
            wColIndex++;
        }
    }
    return getColumnCount() - 1;
}


/**
 * @brief       Returns the x coordinate of the beginning of the column at index index.
 *
 * @param[in]   index      Column index.
 *
 * @return      X coordinate of the column index.
 */
int AbstractTableView::getColumnPosition(int index)
{
    int posX = 0;

    if((index >= 0) && (index < getColumnCount()))
    {
        for(int i = 0; i <= (index - 1); i++)
        {
            posX += getColumnWidth(i);
        }

        return posX;
    }
    else
    {
        return -1;
    }
}


/**
 * @brief       Substracts the header heigth from the given y.
 *
 * @param[in]   y      y coordinate
 *
 * @return      y - getHeaderHeigth().
 */
int AbstractTableView::transY(int y)
{
    return y - getHeaderHeigth();
}


/**
 * @brief       Returns the number of viewable rows in the current window (Partially viewable rows are aslo counted).
 *
 * @return      Number of viewable rows.
 */
int AbstractTableView::getViewableRowsCount()
{
    int wTableHeight = this->height() - getHeaderHeigth();
    int wCount = wTableHeight / getRowHeight();

    wCount += (wTableHeight % getRowHeight()) > 0 ? 1 : 0;

    return wCount;
}


/**
 * @brief       This virtual method returns the number of remaining lines to print.
 *
 * @return      Number of remaining lines to print.
 */
int AbstractTableView::getLineToPrintcount()
{
    int wViewableRowsCount = getViewableRowsCount();
    int_t wRemainingRowsCount = getRowCount() - mTableOffset;
    int wCount = (int_t)wRemainingRowsCount > (int_t)wViewableRowsCount ? (int)wViewableRowsCount : (int)wRemainingRowsCount;
    return wCount;
}


/************************************************************************************
                                New Columns/New Size
************************************************************************************/
/**
 * @brief       This mehtod adds a new column to the table.
 *
 * @param[in]   width           Width of the column in pixel
 * @param[in]   isClickable     Boolean that tells whether the header is clickable or not
 *
 * @return      Nothing.
 */
void AbstractTableView::addColumnAt(int width, bool isClickable)
{
    HeaderButton_t wHeaderButton;
    Column_t wColumn;

    wHeaderButton.isPressed = false;
    wHeaderButton.isClickable = isClickable;
    wHeaderButton.isMouseOver = false;

    wColumn.header = wHeaderButton;
    wColumn.width = width;

    mColumnList.append(wColumn);
}


void AbstractTableView::setRowCount(int_t count)
{
    updateScrollBarRange(count);
    mRowCount = count;
}

/************************************************************************************
                                Getter & Setter
************************************************************************************/
int_t AbstractTableView::getRowCount()
{
    return mRowCount;
}


int AbstractTableView::getColumnCount()
{
    return mColumnList.size();
}


int AbstractTableView::getRowHeight()
{
    return mRowHeight;
}


int AbstractTableView::getColumnWidth(int index)
{
    if(index < 0)
    {
        return -1;
    }
    else if(index < (getColumnCount() - 1))
    {
        return mColumnList.at(index).width;
    }
    else if(index == (getColumnCount() - 1))
    {
        int wGlobWidth = 0;

        for(int i = 0; i < getColumnCount() - 1; i++)
            wGlobWidth += getColumnWidth(i);

        return this->width() - wGlobWidth;
    }

    return 0;
}


void AbstractTableView::setColumnWidth(int index, int width)
{
    mColumnList[index].width = width;
}


int AbstractTableView::getHeaderHeigth()
{
    if(mHeader.isVisible == true)
        return mHeader.height;
    else
        return 0;
}


int AbstractTableView::getTableHeigth()
{
    return this->height() - getHeaderHeigth();
}


int AbstractTableView::getGuiState()
{
    return mGuiState;
}


int AbstractTableView::getNbrOfLineToPrint()
{
    return mNbrOfLineToPrint;
}


void AbstractTableView::setNbrOfLineToPrint(int parNbrOfLineToPrint)
{
    mNbrOfLineToPrint = parNbrOfLineToPrint;
}


/************************************************************************************
                           Table Offset Management
************************************************************************************/
int_t AbstractTableView::getTableOffset()
{
    return mTableOffset;
}


void AbstractTableView::setTableOffset(int_t val)
{
    mTableOffset = val;

#ifdef _WIN64
    int wNewValue = scaleFromUint64ToScrollBarRange(mTableOffset);
    verticalScrollBar()->setValue(wNewValue);
    verticalScrollBar()->setSliderPosition(wNewValue);
#else
    verticalScrollBar()->setValue(val);
    verticalScrollBar()->setSliderPosition(val);
#endif
}


/************************************************************************************
                         Update/Reload/Refresh/Repaint
************************************************************************************/
void AbstractTableView::reloadData()
{
    mShouldReload = true;
    repaint();
}


void AbstractTableView::repaint()
{
    this->viewport()->repaint();
}


/**
 * @brief       This method is called when data have to be reloaded (e.g. When table offset changes).
 *
 * @return      Nothing.
 */
void AbstractTableView::prepareData()
{
    int wViewableRowsCount = getViewableRowsCount();
    int_t wRemainingRowsCount = getRowCount() - mTableOffset;
    mNbrOfLineToPrint = (int_t)wRemainingRowsCount > (int_t)wViewableRowsCount ? (int)wViewableRowsCount : (int)wRemainingRowsCount;
}
