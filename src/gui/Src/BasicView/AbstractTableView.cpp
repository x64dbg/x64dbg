#include "AbstractTableView.h"
#include <QStyleOptionButton>
#include "Configuration.h"
#include "ColumnReorderDialog.h"
#include "CachedFontMetrics.h"
#include "Bridge.h"
#include <windows.h>

int AbstractTableView::mMouseWheelScrollDelta = 0;

AbstractTableScrollBar::AbstractTableScrollBar(QScrollBar* scrollbar)
{
    setOrientation(scrollbar->orientation());
    setParent(scrollbar->parentWidget());
}

void AbstractTableScrollBar::enterEvent(QEvent* event)
{
    Q_UNUSED(event);
    QApplication::setOverrideCursor(Qt::ArrowCursor);
}

void AbstractTableScrollBar::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    QApplication::restoreOverrideCursor();
}

AbstractTableView::AbstractTableView(QWidget* parent)
    : QAbstractScrollArea(parent),
      mFontMetrics(nullptr)
{
    // Class variable initialization
    mTableOffset = 0;
    mPrevTableOffset = mTableOffset + 1;
    Header_t data;
    data.isVisible = true;
    data.height = 20;
    data.activeButtonIndex = -1;
    mHeader = data;

    // Paint cell content only when debugger is running
    setDrawDebugOnly(true);

    mRowCount = 0;

    mHeaderButtonSytle.setStyleSheet(" QPushButton {\n     background-color: rgb(192, 192, 192);\n     border-style: outset;\n     border-width: 2px;\n     border-color: rgb(128, 128, 128);\n }\n QPushButton:pressed {\n     background-color: rgb(192, 192, 192);\n     border-style: inset;\n }");

    mNbrOfLineToPrint = 0;

    memset(&mColResizeData, 0, sizeof(mColResizeData));

    mGuiState = AbstractTableView::NoState;

    mShouldReload = true;
    mAllowPainting = true;
    mDrawDebugOnly = false;

    // ScrollBar Init
    setVerticalScrollBar(new AbstractTableScrollBar(verticalScrollBar()));
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    memset(&mScrollBarAttributes, 0, sizeof(mScrollBarAttributes));
    horizontalScrollBar()->setRange(0, 0);
    horizontalScrollBar()->setPageStep(650);
    if(mMouseWheelScrollDelta == 0)
    {
        //Initialize scroll delta from registry. Windows-specific
        HKEY hDesktop;
        if(RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop\\", 0, STANDARD_RIGHTS_READ | KEY_QUERY_VALUE, &hDesktop) != ERROR_SUCCESS)
            mMouseWheelScrollDelta = 4; // Failed to open the registry. Use a default value;
        else
        {
            wchar_t Data[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            DWORD regType = 0;
            DWORD cbData = sizeof(Data) - sizeof(wchar_t);
            if(RegQueryValueExW(hDesktop, L"WheelScrollLines", nullptr, &regType, (LPBYTE)&Data, &cbData) == ERROR_SUCCESS)
            {
                if(regType == REG_SZ) // Don't process other types of data
                    mMouseWheelScrollDelta = _wtoi(Data);
                if(mMouseWheelScrollDelta == 0)
                    mMouseWheelScrollDelta = 4; // Malformed registry value. Use a default value.
            }
            else
                mMouseWheelScrollDelta = 4; // Failed to query the registry. Use a default value;
            RegCloseKey(hDesktop);
        }
    }
    setMouseTracking(true);

    // Slots
    connect(verticalScrollBar(), SIGNAL(actionTriggered(int)), this, SLOT(vertSliderActionSlot(int)));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(slot_updateColors()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(slot_updateFonts()));
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(slot_updateShortcuts()));

    // todo: try Qt::QueuedConnection to init
    Initialize();
}

AbstractTableView::~AbstractTableView()
{
}

void AbstractTableView::slot_close()
{
    if(ConfigBool("Gui", "SaveColumnOrder"))
        saveColumnToConfig();
}

/************************************************************************************
                           Configuration
************************************************************************************/

void AbstractTableView::Initialize()
{
    // Required to be called by each constructor because
    // of VTable changes
    //
    // Init all other updates once
    updateColors();
    updateFonts();
    updateShortcuts();
}

void AbstractTableView::updateColors()
{
    backgroundColor = ConfigColor("AbstractTableViewBackgroundColor");
    textColor = ConfigColor("AbstractTableViewTextColor");
    separatorColor = ConfigColor("AbstractTableViewSeparatorColor");
    headerTextColor = ConfigColor("AbstractTableViewHeaderTextColor");
    selectionColor = ConfigColor("AbstractTableViewSelectionColor");
}

void AbstractTableView::updateFonts()
{
    setFont(ConfigFont("AbstractTableView"));
    invalidateCachedFont();
    mHeader.height = mFontMetrics->height() + 4;
}

void AbstractTableView::invalidateCachedFont()
{
    delete mFontMetrics;
    mFontMetrics = new CachedFontMetrics(this, font());
}

void AbstractTableView::slot_updateColors()
{
    updateColors();
}

void AbstractTableView::slot_updateFonts()
{
    updateFonts();
}

void AbstractTableView::slot_updateShortcuts()
{
    updateShortcuts();
}

void AbstractTableView::loadColumnFromConfig(const QString & viewName)
{
    int columnCount = getColumnCount();
    for(int i = 0; i < columnCount; i++)
    {
        duint width = ConfigUint("Gui", QString("%1ColumnWidth%2").arg(viewName).arg(i).toUtf8().constData());
        duint hidden = ConfigUint("Gui", QString("%1ColumnHidden%2").arg(viewName).arg(i).toUtf8().constData());
        duint order = ConfigUint("Gui", QString("%1ColumnOrder%2").arg(viewName).arg(i).toUtf8().constData());
        if(width != 0)
            setColumnWidth(i, width);
        if(hidden != 2)
            setColumnHidden(i, !!hidden);
        if(order != 0)
            mColumnOrder[i] = order - 1;
    }
    mViewName = viewName;
    connect(Bridge::getBridge(), SIGNAL(close()), this, SLOT(slot_close()));
}

void AbstractTableView::saveColumnToConfig()
{
    if(mViewName.length() == 0)
        return;
    int columnCount = getColumnCount();
    auto cfg = Config();
    for(int i = 0; i < columnCount; i++)
    {
        cfg->setUint("Gui", QString("%1ColumnWidth%2").arg(mViewName).arg(i).toUtf8().constData(), getColumnWidth(i));
        cfg->setUint("Gui", QString("%1ColumnHidden%2").arg(mViewName).arg(i).toUtf8().constData(), getColumnHidden(i) ? 1 : 0);
        cfg->setUint("Gui", QString("%1ColumnOrder%2").arg(mViewName).arg(i).toUtf8().constData(), mColumnOrder[i] + 1);
    }
}

void AbstractTableView::setupColumnConfigDefaultValue(QMap<QString, duint> & map, const QString & viewName, int columnCount)
{
    for(int i = 0; i < columnCount; i++)
    {
        map.insert(QString("%1ColumnWidth%2").arg(viewName).arg(i), 0);
        map.insert(QString("%1ColumnHidden%2").arg(viewName).arg(i), 2);
        map.insert(QString("%1ColumnOrder%2").arg(viewName).arg(i), 0);
    }
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
    if(!mAllowPainting)
        return;

    if(getColumnCount()) //make sure the last column is never smaller than the window
    {
        int totalWidth = 0;
        int lastWidth = totalWidth;
        int last = 0;
        for(int i = 0; i < getColumnCount(); i++)
        {
            if(getColumnHidden(mColumnOrder[i]))
                continue;
            last = mColumnOrder[i];
            lastWidth = getColumnWidth(last);
            totalWidth += lastWidth;
        }
        lastWidth = totalWidth - lastWidth;
        int width = this->viewport()->width();
        lastWidth = width > lastWidth ? width - lastWidth : 0;
        if(totalWidth < width)
            setColumnWidth(last, lastWidth);
        else
            setColumnWidth(last, getColumnWidth(last));
    }

    Q_UNUSED(event);
    QPainter wPainter(this->viewport());
    int wViewableRowsCount = getViewableRowsCount();

    int scrollValue = -horizontalScrollBar()->value();

    int x = scrollValue;
    int y = 0;

    // Reload data if needed
    if(mPrevTableOffset != mTableOffset || mShouldReload == true)
    {
        prepareData();
        mPrevTableOffset = mTableOffset;
        mShouldReload = false;
    }

    // Paints background
    wPainter.fillRect(wPainter.viewport(), QBrush(backgroundColor));

    // Paints header
    if(mHeader.isVisible == true)
    {
        for(int j = 0; j < getColumnCount(); j++)
        {
            int i = mColumnOrder[j];
            if(getColumnHidden(i))
                continue;
            int width = getColumnWidth(i);
            QStyleOptionButton wOpt;
            if((mColumnList[i].header.isPressed == true) && (mColumnList[i].header.isMouseOver == true)
                    || (mGuiState == AbstractTableView::HeaderButtonReordering && mColumnOrder[mHoveredColumnDisplayIndex] == i))
                wOpt.state = QStyle::State_Sunken;
            else
                wOpt.state = QStyle::State_Enabled;

            wOpt.rect = QRect(x, y, width, getHeaderHeight());

            mHeaderButtonSytle.style()->drawControl(QStyle::CE_PushButton, &wOpt, &wPainter, &mHeaderButtonSytle);

            wPainter.setPen(headerTextColor);
            wPainter.drawText(QRect(x + 4, y, width - 8, getHeaderHeight()), Qt::AlignVCenter | Qt::AlignLeft, mColumnList[i].title);

            x += width;
        }
    }

    x = scrollValue;
    y = getHeaderHeight();

    // Iterate over all columns and cells
    QString wStr;
    for(int k = 0; k < getColumnCount(); k++)
    {
        int j = mColumnOrder[k];
        if(getColumnHidden(j))
            continue;
        for(int i = 0; i < wViewableRowsCount; i++)
        {
            //  Paints cell contents
            if(i < mNbrOfLineToPrint)
            {
                // Don't draw cells if the flag is set, and no process is running
                if(!mDrawDebugOnly || DbgIsDebugging())
                {
                    wStr = paintContent(&wPainter, mTableOffset, i, j, x, y, getColumnWidth(j), getRowHeight());

                    if(wStr.length())
                    {
                        wPainter.setPen(textColor);
                        wPainter.drawText(QRect(x + 4, y, getColumnWidth(j) - 4, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, wStr);
                    }
                }
            }

            if(getColumnCount() > 1)
            {
                // Paints cell right borders
                wPainter.setPen(separatorColor);
                wPainter.drawLine(x + getColumnWidth(j) - 1, y, x + getColumnWidth(j) - 1, y + getRowHeight() - 1);
            }

            // Update y for the next iteration
            y += getRowHeight();
        }

        y = getHeaderHeight();
        x += getColumnWidth(j);
    }
    //emit repainted();
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
    if(getColumnCount() <= 1)
        return;
    int wColIndex = getColumnIndexFromX(event->x());
    int wDisplayIndex = getColumnDisplayIndexFromX(event->x());
    int wStartPos = getColumnPosition(wDisplayIndex); // Position X of the start of column
    int wEndPos = wStartPos + getColumnWidth(wColIndex); // Position X of the end of column
    bool wHandle = ((wColIndex != 0) && (event->x() >= wStartPos) && (event->x() <= (wStartPos + 2))) || ((event->x() <= wEndPos) && (event->x() >= (wEndPos - 2)));
    if(wColIndex == getColumnCount() - 1 && event->x() > viewport()->width()) //last column
        wHandle = false;

    switch(mGuiState)
    {
    case AbstractTableView::NoState:
    {
        if(event->buttons() == Qt::NoButton)
        {
            bool wHasCursor = cursor().shape() == Qt::SplitHCursor ? true : false;

            if((wHandle == true) && (wHasCursor == false))
            {
                setCursor(Qt::SplitHCursor);
                mColResizeData.splitHandle = true;
                mGuiState = AbstractTableView::ReadyToResize;
            }
            if((wHandle == false) && (wHasCursor == true))
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
    }
    break;

    case AbstractTableView::ReadyToResize:
    {
        if(event->buttons() == Qt::NoButton)
        {
            if((wHandle == false) && (mGuiState == AbstractTableView::ReadyToResize))
            {
                unsetCursor();
                mColResizeData.splitHandle = false;
                mGuiState = AbstractTableView::NoState;
            }
        }
    }
    break;

    case AbstractTableView::ResizeColumnState:
    {
        int delta = event->x() - mColResizeData.lastPosX;
        bool bCanResize = (getColumnWidth(mColumnOrder[mColResizeData.index]) + delta) >= 20;
        if(bCanResize)
        {
            int wNewSize = getColumnWidth(mColumnOrder[mColResizeData.index]) + delta;
            setColumnWidth(mColumnOrder[mColResizeData.index], wNewSize);
            mColResizeData.lastPosX = event->x();
            updateViewport();
        }
    }
    break;

    case AbstractTableView::HeaderButtonPressed:
    {
        int wColIndex = getColumnIndexFromX(event->x());

        if(wColIndex == mHeader.activeButtonIndex)
        {
            mColumnList[mHeader.activeButtonIndex].header.isMouseOver = (event->y() <= getHeaderHeight()) && (event->y() >= 0);
            break;
        }
        else
        {
            mGuiState = AbstractTableView::HeaderButtonReordering;
        }
    }
    // no break
    case AbstractTableView::HeaderButtonReordering:
    {
        mHoveredColumnDisplayIndex = getColumnDisplayIndexFromX(event->x());
        updateViewport();
    }
    break;

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
            int wColIndex = getColumnDisplayIndexFromX(event->x());
            int wDisplayIndex = getColumnDisplayIndexFromX(event->x());
            int wStartPos = getColumnPosition(wDisplayIndex); // Position X of the start of column

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
        else if(mHeader.isVisible && getColumnCount() && (event->y() <= getHeaderHeight()) && (event->y() >= 0))
        {
            mReorderStartX = event->x();

            int wColIndex = getColumnIndexFromX(event->x());
            if(mColumnList[wColIndex].header.isClickable)
            {
                //qDebug() << "Button " << wColIndex << "has been pressed.";
                emit headerButtonPressed(wColIndex);

                mColumnList[wColIndex].header.isPressed = true;
                mColumnList[wColIndex].header.isMouseOver = true;

                mHeader.activeButtonIndex = wColIndex;

                mGuiState = AbstractTableView::HeaderButtonPressed;

                updateViewport();
            }
        }
    }
    else //right/middle click
    {
        if(event->y() < getHeaderHeight())
        {
            ColumnReorderDialog reorderDialog(this);
            reorderDialog.setWindowTitle(tr("Edit columns"));
            reorderDialog.exec();
            event->accept();
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
                //qDebug() << "Button " << mHeader.activeButtonIndex << "has been released.";
                emit headerButtonReleased(mHeader.activeButtonIndex);
            }
            mGuiState = AbstractTableView::NoState;
        }
        else if(mGuiState == AbstractTableView::HeaderButtonReordering)
        {
            int temp;
            int wReorderFrom = getColumnDisplayIndexFromX(mReorderStartX);
            int wReorderTo = getColumnDisplayIndexFromX(event->x());
            temp = mColumnOrder[wReorderFrom];
            mColumnOrder[wReorderFrom] = mColumnOrder[wReorderTo];
            mColumnOrder[wReorderTo] = temp;
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

        updateViewport();
    }
}

void AbstractTableView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->y() < getHeaderHeight())
    {
        ColumnReorderDialog reorderDialog(this);
        reorderDialog.setWindowTitle(tr("Edit columns"));
        reorderDialog.exec();
        event->accept();
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
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15;

    if(numSteps > 0)
    {
        if(mMouseWheelScrollDelta > 0)
            for(int i = 0; i < mMouseWheelScrollDelta * numSteps; i++)
                verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        else // -1 : one screen at a time
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    }
    else
    {
        if(mMouseWheelScrollDelta > 0)
            for(int i = 0; i < mMouseWheelScrollDelta * numSteps * -1; i++)
                verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        else // -1 : one screen at a time
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
    }
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
        updateScrollBarRange(getRowCount());
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
    if(event->modifiers())
        return;

    if(wKey == Qt::Key_Up)
    {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
    }
    else if(wKey == Qt::Key_Down)
    {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
    }
    else if(wKey == Qt::Key_PageUp)
    {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    }
    else if(wKey == Qt::Key_PageDown)
    {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
    }
    else if(wKey == Qt::Key_Return || wKey == Qt::Key_Enter) //user pressed enter
        emit enterPressedSignal();
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
    dsint wDelta = 0;
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
        wDelta = 30;
        break;
    case QAbstractSlider::SliderPageStepSub:
        wDelta = -30;
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

    //this emit causes massive lag in the GUI
    //emit tableOffsetChanged(mTableOffset);

    // Scale the new table offset to the 32bits scrollbar range
#ifdef _WIN64
    wNewScrollBarValue = scaleFromUint64ToScrollBarRange(mTableOffset);
#else
    wNewScrollBarValue = mTableOffset;
#endif

    //this emit causes massive lag in the GUI
    //emit repainted();

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
dsint AbstractTableView::sliderMovedHook(int type, dsint value, dsint delta)
{
    Q_UNUSED(type);
    dsint wValue = value + delta;
    dsint wMax = getRowCount() - getViewableRowsCount() + 1;

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
int AbstractTableView::scaleFromUint64ToScrollBarRange(dsint value)
{
    if(mScrollBarAttributes.is64 == true)
    {
        dsint wValue = ((dsint)value) >> mScrollBarAttributes.rightShiftCount;
        dsint wValueMax = ((dsint)getRowCount() - 1) >> mScrollBarAttributes.rightShiftCount;

        if(value == ((dsint)getRowCount() - 1))
            return (int)(verticalScrollBar()->maximum());
        else
            return (int)((dsint)((dsint)verticalScrollBar()->maximum() * (dsint)wValue) / (dsint)wValueMax);
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
dsint AbstractTableView::scaleFromScrollBarRangeToUint64(int value)
{
    if(mScrollBarAttributes.is64 == true)
    {
        dsint wValueMax = ((dsint)getRowCount() - 1) >> mScrollBarAttributes.rightShiftCount;

        if(value == (int)0x7FFFFFFF)
            return (dsint)(getRowCount() - 1);
        else
            return (dsint)(((dsint)((dsint)wValueMax * (dsint)value) / (dsint)0x7FFFFFFF) << mScrollBarAttributes.rightShiftCount);
    }
    else
    {
        return (dsint)value;
    }
}
#endif

/**
 * @brief       This method updates the vertical scrollbar range and pre-computes some attributes for the 32<->64bits conversion methods.
 *
 * @param[in]   range New table range (size)
 *
 * @return      none.
 */
void AbstractTableView::updateScrollBarRange(dsint range)
{
    dsint wMax = range - getViewableRowsCount() + 1;

    if(wMax > 0)
    {
#ifdef _WIN64
        if((duint)wMax < (duint)0x0000000080000000)
        {
            mScrollBarAttributes.is64 = false;
            mScrollBarAttributes.rightShiftCount = 0;
            verticalScrollBar()->setRange(0, wMax);
        }
        else
        {
            duint wMask = 0x8000000000000000;
            int wLeadingZeroCount;

            // Count leading zeros
            for(wLeadingZeroCount = 0; wLeadingZeroCount < 64; wLeadingZeroCount++)
            {
                if((duint)wMax < wMask)
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
    else
        verticalScrollBar()->setRange(0, 0);
    verticalScrollBar()->setSingleStep(getRowHeight());
    verticalScrollBar()->setPageStep(getViewableRowsCount() * getRowHeight());
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
    int wX = -horizontalScrollBar()->value();
    int wColIndex = 0;

    while(wColIndex < getColumnCount())
    {
        int col = mColumnOrder[wColIndex];
        if(getColumnHidden(col))
        {
            wColIndex++;
            continue;
        }
        wX += getColumnWidth(col);

        if(x <= wX)
        {
            return mColumnOrder[wColIndex];
        }
        else if(wColIndex < getColumnCount())
        {
            wColIndex++;
        }
    }
    return getColumnCount() > 0 ? mColumnOrder[getColumnCount() - 1] : -1;
}

/**
 * @brief       Returns the displayed index of the column corresponding to the given x coordinate.
 *
 * @param[in]   x      Pixel offset starting from the left of the table
 *
 * @return      Displayed index.
 */
int AbstractTableView::getColumnDisplayIndexFromX(int x)
{
    int wX = -horizontalScrollBar()->value();
    int wColIndex = 0;

    while(wColIndex < getColumnCount())
    {
        int col = mColumnOrder[wColIndex];
        if(getColumnHidden(col))
        {
            wColIndex++;
            continue;
        }
        wX += getColumnWidth(col);

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
    int posX = -horizontalScrollBar()->value();

    if((index >= 0) && (index < getColumnCount()))
    {
        for(int i = 0; i < index; i++)
            if(!getColumnHidden(mColumnOrder[i]))
                posX += getColumnWidth(mColumnOrder[i]);
        return posX;
    }
    else
    {
        return -1;
    }
}

/**
 * @brief       Substracts the header height from the given y.
 *
 * @param[in]   y      y coordinate
 *
 * @return      y - getHeaderHeight().
 */
int AbstractTableView::transY(int y)
{
    return y - getHeaderHeight();
}

/**
 * @brief       Returns the number of viewable rows in the current window (Partially viewable rows are aslo counted).
 *
 * @return      Number of viewable rows.
 */
int AbstractTableView::getViewableRowsCount()
{
    int wTableHeight = this->viewport()->height() - getHeaderHeight();
    int wCount = wTableHeight / getRowHeight();

    wCount += (wTableHeight % getRowHeight()) > 0 ? 1 : 0;

    emit viewableRows(wCount);
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
    dsint wRemainingRowsCount = getRowCount() - mTableOffset;
    int wCount = (dsint)wRemainingRowsCount > (dsint)wViewableRowsCount ? (int)wViewableRowsCount : (int)wRemainingRowsCount;
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
 * @param[in]   sortFn          The sort function to use for this column. Defaults to case insensitve text search
 * @return      Nothing.
 */
void AbstractTableView::addColumnAt(int width, const QString & title, bool isClickable, SortBy::t sortFn)
{
    HeaderButton_t wHeaderButton;
    Column_t wColumn;
    int wCurrentCount;

    // Fix invisible columns near the edge of the screen
    if(width < 20)
        width = 20;

    wHeaderButton.isPressed = false;
    wHeaderButton.isClickable = isClickable;
    wHeaderButton.isMouseOver = false;

    wColumn.header = wHeaderButton;
    wColumn.width = width;
    wColumn.hidden = false;
    wColumn.title = title;
    wColumn.sortFunction = sortFn;
    wCurrentCount = mColumnList.length();
    mColumnList.append(wColumn);
    mColumnOrder.append(wCurrentCount);
}

void AbstractTableView::setRowCount(dsint count)
{
    updateScrollBarRange(count);
    if(mRowCount != count)
        mShouldReload = true;
    mRowCount = count;
}

void AbstractTableView::deleteAllColumns()
{
    mColumnList.clear();
    mColumnOrder.clear();
}

void AbstractTableView::setColTitle(int index, const QString & title)
{
    if(mColumnList.size() > 0 && index >= 0 && index < mColumnList.size())
    {
        Column_t wColum = mColumnList.takeAt(index);
        wColum.title = title;
        mColumnList.insert(index - 1, wColum);
    }
}

QString AbstractTableView::getColTitle(int index)
{
    if(mColumnList.size() > 0 && index >= 0 && index < mColumnList.size())
        return mColumnList[index].title;
    return QString();
}

/************************************************************************************
                                Getter & Setter
************************************************************************************/
dsint AbstractTableView::getRowCount() const
{
    return mRowCount;
}

int AbstractTableView::getColumnCount() const
{
    return mColumnList.size();
}

int AbstractTableView::getRowHeight()
{
    return mFontMetrics->height() | 1;
}

int AbstractTableView::getColumnWidth(int index)
{
    if(index < 0)
        return -1;
    else if(index < getColumnCount())
        return mColumnList[index].width;
    return 0;
}

bool AbstractTableView::getColumnHidden(int col)
{
    if(col < 0)
        return true;
    else if(col < getColumnCount())
        return mColumnList[col].hidden;
    else
        return true;
}

AbstractTableView::SortBy::t AbstractTableView::getColumnSortBy(int col) const
{
    if(col < getColumnCount() && col >= 0)
        return mColumnList[col].sortFunction;
    return SortBy::AsText;
}

void AbstractTableView::setColumnHidden(int col, bool hidden)
{
    if(col < getColumnCount() && col >= 0)
        mColumnList[col].hidden = hidden;
}

void AbstractTableView::setColumnWidth(int index, int width)
{
    int totalWidth = 0;
    for(int i = 0; i < getColumnCount(); i++)
        if(!getColumnHidden(i))
            totalWidth += getColumnWidth(i);
    if(totalWidth > this->viewport()->width())
        horizontalScrollBar()->setRange(0, totalWidth - this->viewport()->width());
    else
        horizontalScrollBar()->setRange(0, 0);

    mColumnList[index].width = width;
}

void AbstractTableView::setColumnOrder(int pos, int index)
{
    if(index != 0)
        mColumnOrder[pos] = index - 1;
}

int AbstractTableView::getColumnOrder(int index)
{
    return mColumnOrder[index] + 1;
}

int AbstractTableView::getHeaderHeight()
{
    if(mHeader.isVisible == true)
        return mHeader.height;
    else
        return 0;
}

int AbstractTableView::getTableHeight()
{
    return this->viewport()->height() - getHeaderHeight();
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

void AbstractTableView::setShowHeader(bool show)
{
    mHeader.isVisible = show;
}

int AbstractTableView::getCharWidth()
{
    return QFontMetrics(this->font()).width(QChar(' '));
}

/************************************************************************************
                           Content drawing control
************************************************************************************/

bool AbstractTableView::getDrawDebugOnly()
{
    return mDrawDebugOnly;
}

void AbstractTableView::setDrawDebugOnly(bool value)
{
    mDrawDebugOnly = value;
}

/************************************************************************************
                           Table offset management
************************************************************************************/
dsint AbstractTableView::getTableOffset()
{
    return mTableOffset;
}

void AbstractTableView::setTableOffset(dsint val)
{
    dsint wMaxOffset = getRowCount() - getViewableRowsCount() + 1;
    wMaxOffset = wMaxOffset > 0 ? getRowCount() : 0;
    if(val > wMaxOffset)
        return;

    // If val is within the last ViewableRows, then set RVA to rowCount - ViewableRows + 1
    if(wMaxOffset && val >= (getRowCount() - getViewableRowsCount() + 1))
        mTableOffset = (getRowCount() - getViewableRowsCount()) + 1;
    else
        mTableOffset = val;

    emit tableOffsetChanged(val);

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
    emit tableOffsetChanged(mTableOffset);
    this->viewport()->update();
}

void AbstractTableView::updateViewport()
{
    this->viewport()->update();
}

/**
 * @brief       This method is called when data have to be reloaded (e.g. When table offset changes).
 *
 * @return      Nothing.
 */
void AbstractTableView::prepareData()
{
    int wViewableRowsCount = getViewableRowsCount();
    dsint wRemainingRowsCount = getRowCount() - mTableOffset;
    mNbrOfLineToPrint = (dsint)wRemainingRowsCount > (dsint)wViewableRowsCount ? (int)wViewableRowsCount : (int)wRemainingRowsCount;
}

bool AbstractTableView::SortBy::AsText(const QString & a, const QString & b)
{
    auto i = QString::compare(a, b);
    if(i < 0)
        return true;
    if(i > 0)
        return false;
    return duint(&a) < duint(&b);
}

bool AbstractTableView::SortBy::AsInt(const QString & a, const QString & b)
{
    if(a.toLongLong() < b.toLongLong())
        return true;
    if(a.toLongLong() > b.toLongLong())
        return false;
    return duint(&a) < duint(&b);
}

bool AbstractTableView::SortBy::AsHex(const QString & a, const QString & b)
{
    if(a.toLongLong(0, 16) < b.toLongLong(0, 16))
        return true;
    if(a.toLongLong(0, 16) > b.toLongLong(0, 16))
        return false;
    return duint(&a) < duint(&b);
}
