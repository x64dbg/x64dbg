#pragma once

#include <QScrollBar>
#include <QAbstractScrollArea>
#include <QPushButton>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QMenu>
#include "StringUtil.h"
#include "Configuration.h"
#include "MenuBuilder.h"
#include "MiscUtil.h"
#include "ActionHelpers.h"

class CachedFontMetrics;
class ColumnReorderDialog;

//Hacky class that fixes a really annoying cursor problem
class AbstractTableScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit AbstractTableScrollBar(QScrollBar* scrollbar);
    bool event(QEvent* event) override;
};

class AbstractTableView;
class AbstractTableView : public QAbstractScrollArea, public ActionHelper<AbstractTableView>
{
    Q_OBJECT

public:
    explicit AbstractTableView(QWidget* parent = nullptr);
    virtual ~AbstractTableView() = default;

    // Configuration
    virtual void Initialize();
    virtual void updateColors();
    virtual void updateFonts();

    // Pure Virtual Methods
    virtual QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) = 0;
    virtual QColor getCellColor(duint row, duint col);

    // Painting Stuff
    void paintEvent(QPaintEvent* event) override;

    // Mouse Management
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    // ScrollBar Management
    virtual duint sliderMovedHook(QScrollBar::SliderAction action, duint prevTableOffset, dsint delta); // can be made protected
    int scaleFromUint64ToScrollBarRange(duint value); // can be made private
    duint scaleFromScrollBarRangeToUint64(int value); // can be made private

    void updateScrollBarRange(duint range); // setRowCount+resizeEvent needs this, can be made private

    // Coordinates Utils
    dsint getIndexOffsetFromY(int y) const; // can be made protected
    duint getColumnIndexFromX(int x) const; // can be made protected
    int getColumnPosition(duint column) const; // can be made protected
    int transY(int y) const; // can be made protected

    // TODO: this should probably be uint32_t?
    duint getViewableRowsCount() const; // can be made protected
    duint getMaxTableOffset() const;

    // New Columns/New Size
    virtual void addColumnAt(int width, const QString & title, bool isClickable);
    virtual void setRowCount(duint count);
    virtual void deleteAllColumns(); // can be made protected, although it makes sense as a public API
    void setColTitle(duint col, const QString & title); // can be deleted, although it makes sense as a public API
    QString getColTitle(duint col) const; // can be deleted, although it makes sense as a public API

    enum GuiState
    {
        NoState,
        ReadyToResize,
        ResizeColumnState,
        HeaderButtonPressed,
        HeaderButtonReordering
    };

    // Getter & Setter
    duint getRowCount() const;
    duint getColumnCount() const;
    int getRowHeight() const;
    int getColumnWidth(duint col) const;
    void setColumnWidth(duint col, int width);
    void setColumnOrder(duint col, duint colNew);
    duint getColumnOrder(duint col) const;
    int getHeaderHeight() const; // can be made protected
    int getTableHeight() const; // can be made protected
    GuiState getGuiState() const; // can be made protected
    duint getNbrOfLineToPrint() const; // TODO: should this be signed?
    void setNbrOfLineToPrint(duint parNbrOfLineToPrint);
    void setShowHeader(bool show);
    int getCharWidth() const;
    int calculateColumnWidth(int characterCount) const;
    bool getColumnHidden(duint col) const;
    void setColumnHidden(duint col, bool hidden);
    bool getDrawDebugOnly() const;
    void setDrawDebugOnly(bool value);
    bool getAllowPainting() const;
    void setAllowPainting(bool allow);

    // UI customization
    void loadColumnFromConfig(const QString & viewName);
    void saveColumnToConfig();
    static void setupColumnConfigDefaultValue(QMap<QString, duint> & map, const QString & viewName, int columnCount);

    // Table offset management
    duint getTableOffset() const; // TODO: duint
    void setTableOffset(duint val); // TODO: duint

    // Update/Reload/Refresh/Repaint
    virtual void prepareData();

    virtual duint getAddressForPosition(int x, int y);

signals:
    void enterPressedSignal();
    void headerButtonPressed(duint col);
    void headerButtonReleased(duint col);
    void tableOffsetChanged(duint i);
    void viewableRowsChanged(duint rowCount);
    void repainted();

public slots:
    // Update/Reload/Refresh/Repaint
    virtual void reloadData();
    void updateViewport();

    // ScrollBar Management
    void vertSliderActionSlot(int action);

    void editColumnDialog();

private slots:
    // Configuration
    void updateColorsSlot();
    void updateFontsSlot();
    void updateShortcutsSlot();
    void closeSlot();

private:
    GuiState mGuiState = NoState;

    struct ColumnResizeState
    {
        bool splitHandle = false;
        int index = -1;
        int lastPosX = -1;
    } mColResizeData;

    struct HeaderButton
    {
        bool isClickable = false;
        bool isPressed = false;
        bool isMouseOver = false;
    };

    struct Column
    {
        int width = 0;
        int paintedWidth = -1;
        bool hidden = false;
        HeaderButton header;
        QString title;
    };

    struct HeaderConfig
    {
        bool isVisible = true;
        int height = 20;
        int activeButtonIndex = -1;
    } mHeader;

    int mMinColumnWidth = 5;
    QList<Column> mColumnList;
    QList<duint> mColumnOrder;
    int mReorderStartX = -1;
    duint mHoveredColumnDisplayIndex = 0;

    duint mRowCount = 0;
    duint mTableOffset = 0;
    duint mPrevTableOffset = -1;
    duint mNbrOfLineToPrint = 0;

    bool mShouldReload = true;
    bool mDrawDebugOnly = false;

    // State for accumulating scroll events
    enum ScrollDirection
    {
        ScrollUnknown,
        ScrollVertical,
        ScrollHorizontal,
    } mPixelScrollDirection = ScrollUnknown;
    QPoint mPixelScrollDelta;
    QPoint mAngleScrollDelta;

    struct ScrollBarAttributes
    {
        bool is64 = false;
        int rightShiftCount = 0;
    } mScrollBarAttributes;

    duint getColumnDisplayIndexFromX(int x);
    friend class ColumnReorderDialog;

    void updateLastColumnWidth();

protected:
    bool mAllowPainting = true;

    // Configuration
    QColor mTextColor;
    QColor mBackgroundColor;
    QColor mHeaderTextColor;
    QColor mHeaderBackgroundColor;
    QColor mSeparatorColor;
    QColor mSelectionColor;
    QString mViewName; // TODO: this is needed during construction

    // Font metrics
    CachedFontMetrics* mFontMetrics = nullptr;
    void invalidateCachedFont();

    ColumnReorderDialog* mReorderDialog = nullptr;
};
