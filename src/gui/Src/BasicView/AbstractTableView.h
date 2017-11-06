#ifndef ABSTRACTTABLEVIEW_H
#define ABSTRACTTABLEVIEW_H

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

//Hacky class that fixes a really annoying cursor problem
class AbstractTableScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    AbstractTableScrollBar(QScrollBar* scrollbar);
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event);
};

class AbstractTableView;
class AbstractTableView : public QAbstractScrollArea, public ActionHelper<AbstractTableView>
{
    Q_OBJECT

public:
    enum GuiState_t {NoState, ReadyToResize, ResizeColumnState, HeaderButtonPressed, HeaderButtonReordering};

    // Constructor
    explicit AbstractTableView(QWidget* parent = 0);
    ~AbstractTableView();

    // Configuration
    virtual void Initialize();
    virtual void updateColors();
    virtual void updateFonts();

    // Pure Virtual Methods
    virtual QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) = 0;

    // Painting Stuff
    void paintEvent(QPaintEvent* event);

    // Mouse Management
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);
    void resizeEvent(QResizeEvent* event);
    void keyPressEvent(QKeyEvent* event);

    // ScrollBar Management
    virtual dsint sliderMovedHook(int type, dsint value, dsint delta);
    int scaleFromUint64ToScrollBarRange(dsint value);
    dsint scaleFromScrollBarRangeToUint64(int value);
    void updateScrollBarRange(dsint range);


    // Coordinates Utils
    int getIndexOffsetFromY(int y);
    int getColumnIndexFromX(int x);
    int getColumnPosition(int index);
    int transY(int y);
    int getViewableRowsCount();
    virtual int getLineToPrintcount();

    struct SortBy
    {
        typedef std::function<bool(const QString &, const QString &)> t;
        static bool AsText(const QString & a, const QString & b);
        static bool AsInt(const QString & a, const QString & b);
        static bool AsHex(const QString & a, const QString & b);
    };

    // New Columns/New Size
    virtual void addColumnAt(int width, const QString & title, bool isClickable, SortBy::t sortFn = SortBy::AsText);
    virtual void setRowCount(dsint count);
    virtual void deleteAllColumns();
    void setColTitle(int index, const QString & title);
    QString getColTitle(int index);

    // Getter & Setter
    dsint getRowCount() const;
    int getColumnCount() const;
    int getRowHeight();
    int getColumnWidth(int index);
    void setColumnWidth(int index, int width);
    void setColumnOrder(int pos, int index);
    int getColumnOrder(int index);
    int getHeaderHeight();
    int getTableHeight();
    int getGuiState();
    int getNbrOfLineToPrint();
    void setNbrOfLineToPrint(int parNbrOfLineToPrint);
    void setShowHeader(bool show);
    int getCharWidth();
    bool getColumnHidden(int col);
    void setColumnHidden(int col, bool hidden);
    SortBy::t getColumnSortBy(int idx) const;

    // UI customization
    void loadColumnFromConfig(const QString & viewName);
    void saveColumnToConfig();
    static void setupColumnConfigDefaultValue(QMap<QString, duint> & map, const QString & viewName, int columnCount);

    // Content drawing control
    bool getDrawDebugOnly();
    void setDrawDebugOnly(bool value);

    // Table offset management
    dsint getTableOffset();
    void setTableOffset(dsint val);

    // Update/Reload/Refresh/Repaint
    virtual void prepareData();

signals:
    void enterPressedSignal();
    void headerButtonPressed(int col);
    void headerButtonReleased(int col);
    void tableOffsetChanged(dsint i);
    void viewableRows(int rows);
    void repainted();

public slots:
    // Configuration
    void slot_updateColors();
    void slot_updateFonts();
    void slot_updateShortcuts();
    void slot_close();

    // Update/Reload/Refresh/Repaint
    virtual void reloadData();
    void updateViewport();

    // ScrollBar Management
    void vertSliderActionSlot(int action);

private:
    typedef struct _ColumnResizingData_t
    {
        bool splitHandle;
        int index;
        int lastPosX;
    } ColumnResizingData_t;

    typedef struct _HeaderButton_t
    {
        bool isClickable;
        bool isPressed;
        bool isMouseOver;
    } HeaderButton_t;

    typedef struct _Column_t
    {
        int width;
        bool hidden;
        HeaderButton_t header;
        QString title;
        SortBy::t sortFunction;
    } Column_t;

    typedef struct _Header_t
    {
        bool isVisible;
        int height;
        int activeButtonIndex;
    } Header_t;

    typedef struct _ScrollBar64_t
    {
        bool is64;
        int rightShiftCount;
    } ScrollBar64_t;

    GuiState_t mGuiState;

    ColumnResizingData_t mColResizeData;

    QPushButton mHeaderButtonSytle;

    QList<Column_t> mColumnList;

    QList<int> mColumnOrder;
    int mReorderStartX;
    int mHoveredColumnDisplayIndex;

    dsint mRowCount;

    static int mMouseWheelScrollDelta;

    dsint mTableOffset;
    Header_t mHeader;

    int mNbrOfLineToPrint;

    dsint mPrevTableOffset;

    bool mShouldReload;

    ScrollBar64_t mScrollBarAttributes;

    int getColumnDisplayIndexFromX(int x);
    friend class ColumnReorderDialog;
protected:
    bool mAllowPainting;
    bool mDrawDebugOnly;

    // Configuration
    QColor backgroundColor;
    QColor textColor;
    QColor separatorColor;
    QColor headerTextColor;
    QColor selectionColor;
    QString mViewName;

    // Font metrics
    CachedFontMetrics* mFontMetrics;
    void invalidateCachedFont();
};

#endif // ABSTRACTTABLEVIEW_H
