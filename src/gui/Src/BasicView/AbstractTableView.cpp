#include "AbstractTableView.h"
#include <QStyleOptionButton>
#include "Configuration.h"
#include "ColumnReorderDialog.h"
#include "CachedFontMetrics.h"
#include "Bridge.h"
#include "MethodInvoker.h"

AbstractTableScrollBar::AbstractTableScrollBar(QScrollBar* scrollbar)
{
    setOrientation(scrollbar->orientation());
    setParent(scrollbar->parentWidget());
}

bool AbstractTableScrollBar::event(QEvent* event)
{
    switch(event->type())
    {
    case QEvent::Enter:
        QApplication::setOverrideCursor(Qt::ArrowCursor);
        break;
    case QEvent::Leave:
        QApplication::restoreOverrideCursor();
        break;
    default:
        break;
    }
    return QScrollBar::event(event);
}

AbstractTableView::AbstractTableView(QWidget* parent)
    : QAbstractScrollArea(parent)
{
    // ScrollBar Init
    setVerticalScrollBar(new AbstractTableScrollBar(verticalScrollBar()));
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    memset(&mScrollBarAttributes, 0, sizeof(mScrollBarAttributes));
    horizontalScrollBar()->setRange(0, 0);
    horizontalScrollBar()->setPageStep(650); // TODO: random value
    setMouseTracking(true);

    // Slots
    connect(verticalScrollBar(), SIGNAL(actionTriggered(int)), this, SLOT(vertSliderActionSlot(int)));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateColorsSlot()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(updateFontsSlot()));
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(updateShortcutsSlot()));
    connect(Bridge::getBridge(), SIGNAL(close()), this, SLOT(closeSlot()));

    // todo: try Qt::QueuedConnection to init
    Initialize();
}

void AbstractTableView::closeSlot()
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
    mTextColor = ConfigColor("AbstractTableViewTextColor");
    mBackgroundColor = ConfigColor("AbstractTableViewBackgroundColor");

    mHeaderTextColor = ConfigColor("AbstractTableViewHeaderTextColor");
    mHeaderBackgroundColor = ConfigColor("AbstractTableViewHeaderBackgroundColor");

    mSelectionColor = ConfigColor("AbstractTableViewSelectionColor");
    mSeparatorColor = ConfigColor("AbstractTableViewSeparatorColor");
}

void AbstractTableView::updateFonts()
{
    setFont(ConfigFont("AbstractTableView"));
    invalidateCachedFont();
    mHeader.height = mFontMetrics->height() + 4;
}

QColor AbstractTableView::getCellColor(duint row, duint col)
{
    Q_UNUSED(row);
    Q_UNUSED(col);
    return mTextColor;
}

void AbstractTableView::invalidateCachedFont()
{
    delete mFontMetrics;
    mFontMetrics = new CachedFontMetrics(this, font());
}

void AbstractTableView::updateColorsSlot()
{
    updateColors();
}

void AbstractTableView::updateFontsSlot()
{
    auto oldCharWidth = getCharWidth();
    updateFonts();
    auto newCharWidth = getCharWidth();

    // Scale the column widths to the new font
    for(duint col = 0; col < getColumnCount(); col++)
    {
        auto width = getColumnWidth(col);
        auto charCount = width / oldCharWidth;
        auto padding = width % oldCharWidth;
        setColumnWidth(col, charCount * newCharWidth + padding);
    }
}

void AbstractTableView::updateShortcutsSlot()
{
    updateShortcuts();
}

void AbstractTableView::loadColumnFromConfig(const QString & viewName)
{
    duint columnCount = getColumnCount();
    for(duint i = 0; i < columnCount; i++)
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

void AbstractTableView::editColumnDialog()
{
    delete mReorderDialog;
    mReorderDialog = new ColumnReorderDialog(this);
    mReorderDialog->setWindowTitle(tr("Edit columns"));
    mReorderDialog->open();
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

    Q_UNUSED(event);
    QPainter painter(this->viewport());
    painter.setFont(font());
    painter.setLayoutDirection(Qt::LayoutDirectionAuto);
    auto viewableRowsCount = getViewableRowsCount();

    int scrollValue = -horizontalScrollBar()->value();

    // Reload data if needed
    if(mPrevTableOffset != mTableOffset || mShouldReload)
    {
        updateScrollBarRange(getRowCount());
        prepareData();
        mPrevTableOffset = mTableOffset;
        mShouldReload = false;
    }

    // TODO: report if mTableOffset is out of view

    // Paints background
    if(mBackgroundColor.alpha() == 255) // The secret code to allow the user to set a background image in style.css
        painter.fillRect(painter.viewport(), QBrush(mBackgroundColor));

    // Paints header
    if(mHeader.isVisible)
    {
        int x = scrollValue;
        int y = 0;

        QPen textPen(mHeaderTextColor);
        QPen separatorPen(mSeparatorColor, 2);
        QBrush backgroundBrush(mHeaderBackgroundColor);

        for(duint j = 0; j < getColumnCount(); j++)
        {
            int i = mColumnOrder[j];
            if(getColumnHidden(i))
                continue;
            int width = getColumnWidth(i);
            int height = getHeaderHeight();

            const auto isPressed = (mColumnList[i].header.isPressed && mColumnList[i].header.isMouseOver)
                                   || (mGuiState == AbstractTableView::HeaderButtonReordering && mColumnOrder[mHoveredColumnDisplayIndex] == i);

            if(isPressed)
            {
                painter.fillRect(x, y, width, height, QBrush(mSeparatorColor));
            }
            else
            {
                painter.fillRect(x, y, width, height, backgroundBrush);
            }

            painter.setPen(textPen);
            painter.drawText(QRect(x + 4, y, width - 8, height), Qt::AlignVCenter | Qt::AlignLeft, mColumnList[i].title);

            if(isPressed)
            {
                painter.setPen(QPen(mTextColor, 2));
            }
            else
            {
                painter.setPen(separatorPen);
            }

            painter.drawLine(x, y + height - separatorPen.width() + 1, x + width - separatorPen.width(), y + height - separatorPen.width() + 1);
            painter.drawLine(x + width - separatorPen.width() + 1, y, x + width - separatorPen.width() + 1, y + height - separatorPen.width());

            x += width;
        }
    }

    int x = scrollValue;
    int y = getHeaderHeight();

    // Iterate over all columns and cells
    for(duint k = 0; k < getColumnCount(); k++)
    {
        int j = mColumnOrder[k];
        if(getColumnHidden(j))
            continue;
        for(duint i = 0; i < viewableRowsCount; i++)
        {
            // Paints cell contents
            if(i < mNbrOfLineToPrint)
            {
                // Don't draw cells if the flag is set, and no process is running
                if(!mDrawDebugOnly || DbgIsDebugging())
                {
                    QString str = paintContent(&painter, mTableOffset + i, j, x, y, getColumnWidth(j), getRowHeight());

                    if(str.length())
                    {
                        painter.setPen(getCellColor(mTableOffset + i, j));
                        painter.drawText(QRectF(x + 4, y, getColumnWidth(j) - 5, getRowHeight()), Qt::AlignVCenter | Qt::AlignLeft, str);
                    }
                }
            }

            if(getColumnCount() > 1)
            {
                // Paints cell right borders
                painter.setPen(mSeparatorColor);
                painter.drawLine(x + getColumnWidth(j) - 1, y, x + getColumnWidth(j) - 1, y + getRowHeight() - 1);
            }

            // Update y for the next iteration
            y += getRowHeight();
        }

        y = getHeaderHeight();
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
    if(getColumnCount() <= 1)
        return;
    auto colIndex = getColumnIndexFromX(event->x());
    auto displayIndex = getColumnDisplayIndexFromX(event->x());
    int startPos = getColumnPosition(displayIndex); // Position X of the start of column
    int endPos = startPos + getColumnWidth(colIndex); // Position X of the end of column
    bool onHandle = ((colIndex != 0) && (event->x() >= startPos) && (event->x() <= (startPos + 2))) || ((event->x() <= endPos) && (event->x() >= (endPos - 2)));
    if(colIndex == getColumnCount() - 1 && event->x() > viewport()->width()) //last column
        onHandle = false;

    switch(mGuiState)
    {
    case AbstractTableView::NoState:
    {
        if(event->buttons() == Qt::NoButton)
        {
            bool hasCursor = cursor().shape() == Qt::SplitHCursor ? true : false;

            if(onHandle && !hasCursor)
            {
                setCursor(Qt::SplitHCursor);
                mColResizeData.splitHandle = true;
                mGuiState = AbstractTableView::ReadyToResize;
            }
            else if(!onHandle && hasCursor)
            {
                unsetCursor();
                mColResizeData.splitHandle = false;
                mGuiState = AbstractTableView::NoState;
            }
        }
    }
    break;

    case AbstractTableView::ReadyToResize:
    {
        if(event->buttons() == Qt::NoButton)
        {
            if(!onHandle && mGuiState == AbstractTableView::ReadyToResize)
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
        bool bCanResize = (getColumnWidth(mColumnOrder[mColResizeData.index]) + delta) >= mMinColumnWidth;
        if(bCanResize)
        {
            int newSize = getColumnWidth(mColumnOrder[mColResizeData.index]) + delta;
            setColumnWidth(mColumnOrder[mColResizeData.index], newSize);
            mColResizeData.lastPosX = event->x();
            updateViewport();
        }
    }
    break;

    case AbstractTableView::HeaderButtonPressed:
    {
        auto colIndex = getColumnIndexFromX(event->x());

        if(colIndex == mHeader.activeButtonIndex)
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

    QAbstractScrollArea::mouseMoveEvent(event);
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
        if(mColResizeData.splitHandle)
        {
            int colIndex = getColumnDisplayIndexFromX(event->x());
            int displayIndex = getColumnDisplayIndexFromX(event->x());
            int startPos = getColumnPosition(displayIndex); // Position X of the start of column

            mGuiState = AbstractTableView::ResizeColumnState;

            if(event->x() <= (startPos + 2))
            {
                mColResizeData.index = colIndex - 1;
            }
            else
            {
                mColResizeData.index = colIndex;
            }

            mColResizeData.lastPosX = event->x();
        }
        else if(mHeader.isVisible && getColumnCount() && (event->y() <= getHeaderHeight()) && (event->y() >= 0))
        {
            mReorderStartX = event->x();

            auto colIndex = getColumnIndexFromX(event->x());
            if(mColumnList[colIndex].header.isClickable)
            {
                //qDebug() << "Button " << colIndex << "has been pressed.";
                emit headerButtonPressed(colIndex);

                mColumnList[colIndex].header.isPressed = true;
                mColumnList[colIndex].header.isMouseOver = true;

                mHeader.activeButtonIndex = colIndex;

                mGuiState = AbstractTableView::HeaderButtonPressed;

                updateViewport();
            }
        }
    }
    else //right/middle click
    {
        if(event->y() < getHeaderHeight())
        {
            editColumnDialog();
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
            if(mColumnList[mHeader.activeButtonIndex].header.isMouseOver)
            {
                emit headerButtonReleased(mHeader.activeButtonIndex);
            }
            mGuiState = AbstractTableView::NoState;
        }
        else if(mGuiState == AbstractTableView::HeaderButtonReordering)
        {
            int reorderFrom = getColumnDisplayIndexFromX(mReorderStartX);
            int reorderTo = getColumnDisplayIndexFromX(event->x());
            std::swap(mColumnOrder[reorderFrom], mColumnOrder[reorderTo]);
            mGuiState = AbstractTableView::NoState;
            updateLastColumnWidth();
        }
        else
        {
            return;
        }

        // Release all buttons
        for(duint i = 0; i < getColumnCount(); i++)
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
        editColumnDialog();
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
    const auto lineAngleTick = 120 / QApplication::wheelScrollLines();
    mAngleScrollDelta += event->angleDelta();

    auto linesY = mAngleScrollDelta.y() / lineAngleTick;
    if(linesY != 0)
    {
        mAngleScrollDelta = QPoint(0, mAngleScrollDelta.y() % lineAngleTick);
    }

    auto linesX = mAngleScrollDelta.x() / lineAngleTick;
    if(linesX != 0)
    {
        mAngleScrollDelta = QPoint(mAngleScrollDelta.x() % lineAngleTick, 0);
    }

    if(event->modifiers() == Qt::NoModifier)
    {
#ifdef Q_OS_DARWIN
        const auto enableTouchPad = true;
#else
        // For now this functionality is only enabled on macOS, because the Qt::ScrollBegin phase
        // is only available there. Implementing this on another OS would require a timer to determine
        // when the user stops scrolling.
        const auto enableTouchPad = false;
#endif // Q_OS_DARWIN

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        if(enableTouchPad && event->device()->type() == QInputDevice::DeviceType::TouchPad)
#else
        if(enableTouchPad && event->source() == Qt::MouseEventSynthesizedBySystem)
#endif // QT_VERSION
        {
            QPoint pixelDelta = event->pixelDelta();

            // When a touchpad is used the scroll events are pixel-based (vs mouse wheels which are angle-based).
            // The scroll direction is determined with a getRowHeight threshold and after that the direction is
            // locked. Without doing this you get weird diagonal scrolling and a bad user experience.
            if(event->phase() == Qt::ScrollBegin)
            {
                mPixelScrollDelta = pixelDelta;
                mPixelScrollDirection = ScrollUnknown;
            }
            else
            {
                mPixelScrollDelta += pixelDelta;
            }

            auto deltaY = mPixelScrollDelta.y();
            const auto tickY = getRowHeight();
            auto stepsY = deltaY / tickY;
            if(stepsY != 0 && mPixelScrollDirection != ScrollHorizontal)
            {
                mPixelScrollDirection = ScrollVertical;
                mPixelScrollDelta = QPoint(0, deltaY % tickY);
                if(stepsY > 0)
                {
                    for(int i = 0; i < stepsY; i++)
                        verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
                }
                else
                {
                    for(int i = 0; i < -stepsY; i++)
                        verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
                }
            }

            auto deltaX = mPixelScrollDelta.x();
            const auto tickX = getRowHeight();
            if(mPixelScrollDirection == ScrollUnknown && (deltaX / tickX) != 0)
            {
                mPixelScrollDirection = ScrollHorizontal;
            }

            if(mPixelScrollDirection == ScrollHorizontal)
            {
                if(deltaX > 0)
                {
                    for(int i = 0; i < deltaX; i++)
                        horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
                }
                else
                {
                    for(int i = 0; i < -deltaX; i++)
                        horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
                }
                mPixelScrollDelta = QPoint(0, 0);
            }
        }
        else
        {
            if(linesY > 0)
            {
                for(int i = 0; i < linesY; i++)
                    verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
            }
            else if(linesY < 0)
            {
                for(int i = 0; i < -linesY; i++)
                    verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
            }
            else if(linesX > 0)
            {
                for(int i = 0; i < linesX * 20; i++)
                    horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
            }
            else if(linesX < 0)
            {
                for(int i = 0; i < -linesX * 20; i++)
                    horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
            }
        }
    }
    else if(event->modifiers() == Qt::ControlModifier) // Zoom
    {
        Config()->zoomFont("AbstractTableView", event);
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
    if(event->size().width() != event->oldSize().width())
    {
        updateLastColumnWidth();
    }

    if(event->size().height() != event->oldSize().height())
    {
        emit viewableRowsChanged(getViewableRowsCount());
        mShouldReload = true;
    }
    QAbstractScrollArea::resizeEvent(event);
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
    auto key = event->key();
    if(event->modifiers() != Qt::NoModifier && event->modifiers() != Qt::KeypadModifier)
        return QAbstractScrollArea::keyPressEvent(event);

    if(key == Qt::Key_Up)
    {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
    }
    else if(key == Qt::Key_Down)
    {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
    }
    else if(key == Qt::Key_PageUp)
    {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    }
    else if(key == Qt::Key_PageDown)
    {
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
    }
    else if(key == Qt::Key_Return || key == Qt::Key_Enter) //user pressed enter
    {
        emit enterPressedSignal();
    }
    else
    {
        QAbstractScrollArea::keyPressEvent(event);
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
    dsint delta = 0;
    int sliderPos = verticalScrollBar()->sliderPosition();

    // Bounding
    sliderPos = sliderPos > verticalScrollBar()->maximum() ? verticalScrollBar()->maximum() : sliderPos;
    sliderPos = sliderPos < 0 ? 0 : sliderPos;

    // Determine the delta
    switch(action)
    {
    case QAbstractSlider::SliderNoAction:
        break;
    case QAbstractSlider::SliderSingleStepAdd:
        delta = 1;
        break;
    case QAbstractSlider::SliderSingleStepSub:
        delta = -1;
        break;
    case QAbstractSlider::SliderPageStepAdd:
        delta = 30;
        break;
    case QAbstractSlider::SliderPageStepSub:
        delta = -30;
        break;
    case QAbstractSlider::SliderToMinimum:
    case QAbstractSlider::SliderToMaximum:
    case QAbstractSlider::SliderMove:
        delta = scaleFromScrollBarRangeToUint64(sliderPos) - mTableOffset;
        break;
    default:
        break;
    }

    // TODO: negative?
    // Call the hook (Usefull for disassembly)
    mTableOffset = sliderMovedHook((QScrollBar::SliderAction)action, mTableOffset, delta);

    //this emit causes massive lag in the GUI
    //emit tableOffsetChanged(mTableOffset);

    // Scale the new table offset to the 32bits scrollbar range
    auto newScrollBarValue = scaleFromUint64ToScrollBarRange(mTableOffset);

    //this emit causes massive lag in the GUI
    //emit repainted();

    // Update scrollbar attributes
    verticalScrollBar()->setValue(newScrollBarValue);
    verticalScrollBar()->setSliderPosition(newScrollBarValue);
}

/**
 * @brief       This virtual method is called at the end of the vertSliderActionSlot(...) method.
 *              It allows changing the table offset according to the action type, the old table offset
 *              and delta between the old and the new table offset.
 *
 * @param[in]   action              Type of action (Refer to the QAbstractSlider::SliderAction enum)
 * @param[in]   prevTableOffset     Previous table offset
 * @param[in]   delta               Scrollbar value delta compared to the previous state
 *
 * @return      Return the value of the new table offset.
 */
duint AbstractTableView::sliderMovedHook(QScrollBar::SliderAction action, duint prevTableOffset, dsint delta)
{
    Q_UNUSED(action);

    // TODO: fix this signed/unsigned logic
    dsint value = prevTableOffset + delta;
    dsint maxTableOffset = getMaxTableOffset();

    // Bounding
    value = value > maxTableOffset ? maxTableOffset : value;
    value = value < 0 ? 0 : value;

    return value;
}

/**
 * @brief       This method scale the given 64bits integer to the scrollbar range (32bits).
 *
 * @param[in]   value      64bits integer to rescale
 *
 * @return      32bits integer.
 */
int AbstractTableView::scaleFromUint64ToScrollBarRange(duint value)
{
    if(mScrollBarAttributes.is64)
    {
        // TODO: this needs to be rewritten
        value = ((dsint)value) >> mScrollBarAttributes.rightShiftCount;
        dsint valueMax = ((dsint)getRowCount() - 1) >> mScrollBarAttributes.rightShiftCount;

        if(value == ((dsint)getRowCount() - 1))
        {
            return verticalScrollBar()->maximum();
        }
        else
        {
            if(valueMax == 0)
            {
                printf("valueMax: %lli, value: %llu, rightShiftCount: %d\n", valueMax, value, mScrollBarAttributes.rightShiftCount);
            }
            // TODO: division by zero
            return (int)((dsint)((dsint)verticalScrollBar()->maximum() * (dsint)value) / (dsint)valueMax);
        }
    }
    else
    {
        return (int)value;
    }
}

/**
 * @brief       This method scale the given 32bits integer to the table range (64bits).
 *
 * @param[in]   value      32bits integer to rescale
 *
 * @return      64bits integer.
 */
duint AbstractTableView::scaleFromScrollBarRangeToUint64(int value)
{
    if(mScrollBarAttributes.is64)
    {
        dsint valueMax = ((dsint)getRowCount() - 1) >> mScrollBarAttributes.rightShiftCount;

        if(value == (int)0x7FFFFFFF)
            return (dsint)(getRowCount() - 1);
        else
            return (dsint)(((dsint)((dsint)valueMax * (dsint)value) / (dsint)0x7FFFFFFF) << mScrollBarAttributes.rightShiftCount);
    }
    else
    {
        return (dsint)value;
    }
}

/**
 * @brief       This method updates the vertical scrollbar range and pre-computes some attributes for the 32<->64bits conversion methods.
 *
 * @param[in]   range New table range (size)
 *
 * @return      none.
 */
void AbstractTableView::updateScrollBarRange(duint range)
{
    int rangeMin = 0;
    int rangeMax = 0;

    auto viewableRows = getViewableRowsCount();
    if(range > viewableRows)
    {
        auto maxTableOffset = range - viewableRows + 1; // TODO: why +1?
        if(maxTableOffset < (duint)INT_MAX + 1)
        {
            mScrollBarAttributes.is64 = false;
            mScrollBarAttributes.rightShiftCount = 0;

            rangeMin = 0;
            rangeMax = maxTableOffset;
        }
        else
        {
            // Count leading zeros
            int leadingZeroCount = 0;
            for(uint64_t mask = 0x8000000000000000; mask != 0; mask >>= 1)
            {
                if((maxTableOffset & mask) != 0)
                {
                    break;
                }
                else
                {
                    leadingZeroCount++;
                }
            }

            mScrollBarAttributes.is64 = true;
            mScrollBarAttributes.rightShiftCount = 32 - leadingZeroCount;

            rangeMin = 0;
            rangeMax = INT_MAX;
        }
    }

    verticalScrollBar()->setRange(rangeMin, rangeMax);
    verticalScrollBar()->setSingleStep(getRowHeight());
    verticalScrollBar()->setPageStep(viewableRows * getRowHeight());
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
dsint AbstractTableView::getIndexOffsetFromY(int y) const
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
duint AbstractTableView::getColumnIndexFromX(int x) const
{
    int scrollX = -horizontalScrollBar()->value();

    for(duint colIndex = 0; colIndex < getColumnCount(); colIndex++)
    {
        auto col = mColumnOrder[colIndex];
        if(getColumnHidden(col))
        {
            continue;
        }

        scrollX += getColumnWidth(col);
        if(x <= scrollX)
        {
            return mColumnOrder[colIndex];
        }
    }

    return getColumnCount() > 0 ? mColumnOrder[getColumnCount() - 1] : - 1;
}

/**
 * @brief       Returns the displayed index of the column corresponding to the given x coordinate.
 *
 * @param[in]   x      Pixel offset starting from the left of the table
 *
 * @return      Displayed index.
 */
duint AbstractTableView::getColumnDisplayIndexFromX(int x)
{
    int scrollX = -horizontalScrollBar()->value();

    for(duint colIndex = 0; colIndex < getColumnCount(); colIndex++)
    {
        auto col = mColumnOrder[colIndex];
        if(getColumnHidden(col))
        {
            continue;
        }

        scrollX += getColumnWidth(col);
        if(x <= scrollX)
        {
            return colIndex;
        }
    }

    // TODO: what if there are no columns?
    return getColumnCount() > 0 ? getColumnCount() - 1 : -1;
}

void AbstractTableView::updateLastColumnWidth()
{
    // Make sure the last column is never smaller than the viewport
    if(getColumnCount())
    {
        int totalWidth = 0;
        int lastWidth = totalWidth;
        int last = 0;
        for(duint i = 0; i < getColumnCount(); i++)
        {
            if(getColumnHidden(mColumnOrder[i]))
                continue;
            last = mColumnOrder[i];
            auto & column = mColumnList[last];
            column.paintedWidth = -1;
            lastWidth = column.width;
            totalWidth += lastWidth;
        }

        lastWidth = totalWidth - lastWidth;
        int width = viewport()->width();
        lastWidth = width > lastWidth ? width - lastWidth : 0;
        if(totalWidth < width)
            mColumnList[last].paintedWidth = lastWidth;
    }

    MethodInvoker::invokeMethod([this]()
    {
        int totalWidth = 0;
        for(duint i = 0; i < getColumnCount(); i++)
            if(!getColumnHidden(i))
                totalWidth += getColumnWidth(i);

        if(totalWidth > viewport()->width())
            horizontalScrollBar()->setRange(0, totalWidth - viewport()->width());
        else
            horizontalScrollBar()->setRange(0, 0);
    });
}

/**
 * @brief       Returns the x coordinate of the beginning of the column at index index.
 *
 * @param[in]   index      Column index.
 *
 * @return      X coordinate of the column index.
 */
int AbstractTableView::getColumnPosition(duint index) const
{
    int posX = -horizontalScrollBar()->value();

    if((index >= 0) && (index < getColumnCount()))
    {
        for(duint i = 0; i < index; i++)
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
int AbstractTableView::transY(int y) const
{
    return y - getHeaderHeight();
}

/**
 * @brief       Returns the number of viewable rows in the current window (Partially viewable rows are also counted).
 *
 * @return      Number of viewable rows.
 */
duint AbstractTableView::getViewableRowsCount() const
{
    auto tableHeight = viewport()->height() - getHeaderHeight();
    auto count = tableHeight / getRowHeight();

    count += (tableHeight % getRowHeight()) > 0 ? 1 : 0;

    return count;
}

duint AbstractTableView::getMaxTableOffset() const
{
    // TODO: is the +1 correct?
    return getRowCount() - getViewableRowsCount() + 1;
}

/************************************************************************************
                                New Columns/New Size
************************************************************************************/
/**
 * @brief       This mehtod adds a new column to the table.
 *
 * @param[in]   width           Width of the column in pixel
 * @param[in]   isClickable     Boolean that tells whether the header is clickable or not
 * @return      Nothing.
 */
void AbstractTableView::addColumnAt(int width, const QString & title, bool isClickable)
{
    Column column;
    column.header.isClickable = isClickable;
    column.width = std::max(mMinColumnWidth, width);
    column.hidden = false;
    column.title = title;
    mColumnOrder.append(mColumnList.length());
    mColumnList.append(column);

    updateLastColumnWidth();
}

void AbstractTableView::setRowCount(duint count)
{
    if(mRowCount != count)
        mShouldReload = true;
    mRowCount = count;

    MethodInvoker::invokeMethod([this]()
    {
        updateScrollBarRange(getRowCount());
    });

    // TODO: report if mTableOffset is out of view
}

void AbstractTableView::deleteAllColumns()
{
    mColumnList.clear();
    mColumnOrder.clear();
}

void AbstractTableView::setColTitle(duint col, const QString & title)
{
    if(mColumnList.size() > 0 && col < (duint)mColumnList.size())
    {
        Column column = mColumnList.takeAt(col);
        column.title = title;
        mColumnList.insert(col - 1, column);
    }
}

QString AbstractTableView::getColTitle(duint col) const
{
    if(mColumnList.size() > 0 && col < (duint)mColumnList.size())
        return mColumnList[col].title;
    return QString();
}

/************************************************************************************
                                Getter & Setter
************************************************************************************/
duint AbstractTableView::getRowCount() const
{
    return mRowCount;
}

duint AbstractTableView::getColumnCount() const
{
    return mColumnList.size();
}

int AbstractTableView::getRowHeight() const
{
    return mFontMetrics->height() | 1;
}

int AbstractTableView::getColumnWidth(duint col) const
{
    if(col < getColumnCount())
    {
        const auto & column = mColumnList[col];
        if(column.paintedWidth != -1)
            return column.paintedWidth;
        else
            return column.width;
    }
    return 0;
}

bool AbstractTableView::getColumnHidden(duint col) const
{
    if(col < getColumnCount())
        return mColumnList[col].hidden;
    else
        return true;
}

void AbstractTableView::setColumnHidden(duint col, bool hidden)
{
    if(col < getColumnCount() && col >= 0)
        mColumnList[col].hidden = hidden;
}

void AbstractTableView::setColumnWidth(duint col, int width)
{
    if(col >= getColumnCount())
        return;

    mColumnList[col].width = std::max(width, mMinColumnWidth);

    updateLastColumnWidth();
}

void AbstractTableView::setColumnOrder(duint col, duint colNew)
{
    if(colNew != 0)
        mColumnOrder[col] = colNew - 1;
}

duint AbstractTableView::getColumnOrder(duint col) const
{
    return mColumnOrder[col] + 1;
}

int AbstractTableView::getHeaderHeight() const
{
    if(mHeader.isVisible)
        return mHeader.height;
    else
        return 0;
}

int AbstractTableView::getTableHeight() const
{
    return this->viewport()->height() - getHeaderHeight();
}

AbstractTableView::GuiState AbstractTableView::getGuiState() const
{
    return mGuiState;
}

duint AbstractTableView::getNbrOfLineToPrint() const
{
    return mNbrOfLineToPrint;
}

void AbstractTableView::setNbrOfLineToPrint(duint parNbrOfLineToPrint)
{
    mNbrOfLineToPrint = parNbrOfLineToPrint;
}

void AbstractTableView::setShowHeader(bool show)
{
    mHeader.isVisible = show;
}

int AbstractTableView::getCharWidth() const
{
    return mFontMetrics->width(' ');
}

int AbstractTableView::calculateColumnWidth(int characterCount) const
{
    return 7 + getCharWidth() * characterCount;
}

/************************************************************************************
                           Content drawing control
************************************************************************************/

bool AbstractTableView::getDrawDebugOnly() const
{
    return mDrawDebugOnly;
}

void AbstractTableView::setDrawDebugOnly(bool value)
{
    mDrawDebugOnly = value;
}

void AbstractTableView::setAllowPainting(bool allow)
{
    mAllowPainting = allow;
}

bool AbstractTableView::getAllowPainting() const
{
    return mAllowPainting;
}

/************************************************************************************
                           Table offset management
************************************************************************************/
duint AbstractTableView::getTableOffset() const
{
    return mTableOffset;
}

void AbstractTableView::setTableOffset(duint val)
{
    auto rowCount = getRowCount();
    auto viewableRows = getViewableRowsCount();
    if(rowCount <= viewableRows)
    {
        mTableOffset = 0;
        return;
    }
    auto maxTableOffset = getMaxTableOffset();

    // If val is within the last viewable rows
    if(maxTableOffset > 0 && val >= maxTableOffset)
        mTableOffset = maxTableOffset;
    else
        mTableOffset = val;

    emit tableOffsetChanged(val);

    MethodInvoker::invokeMethod([this]()
    {
        int newValue = scaleFromUint64ToScrollBarRange(mTableOffset);
        verticalScrollBar()->setValue(newValue);
        verticalScrollBar()->setSliderPosition(newValue);
    });
}


/************************************************************************************
                         Update/Reload/Refresh/Repaint
************************************************************************************/
void AbstractTableView::reloadData()
{
    mShouldReload = true;
    emit tableOffsetChanged(mTableOffset);
    updateViewport();
}

void AbstractTableView::updateViewport()
{
    MethodInvoker::invokeMethod([this]()
    {
        viewport()->update();
    });
}

/**
 * @brief       This method is called when data have to be reloaded (e.g. When table offset changes).
 *
 * @return      Nothing.
 */
void AbstractTableView::prepareData()
{
    auto viewableRowsCount = getViewableRowsCount();
    auto remainingRowsCount = getRowCount() - mTableOffset;
    mNbrOfLineToPrint = qMin(remainingRowsCount, viewableRowsCount);
}

duint AbstractTableView::getAddressForPosition(int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    return 0;
}
