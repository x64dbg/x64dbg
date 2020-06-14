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
class DisassemblyPopup;

//Hacky class that fixes a really annoying cursor problem
class AbstractTableScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit AbstractTableScrollBar(QScrollBar* scrollbar);
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
};

class AbstractTableView;
class AbstractTableView : public QAbstractScrollArea, public ActionHelper<AbstractTableView>
{
    Q_OBJECT

public:
    enum GuiState
    {
        NoState,
        ReadyToResize,
        ResizeColumnState,
        HeaderButtonPressed,
        HeaderButtonReordering
    };

    // Constructor
    explicit AbstractTableView(QWidget* parent = 0);
    virtual ~AbstractTableView() = default;

    // Configuration
    virtual void Initialize();
    virtual void updateColors();
    virtual void updateFonts();

    // Pure Virtual Methods
    virtual QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) = 0;
    virtual QColor getCellColor(int r, int c);

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
    void leaveEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

    // ScrollBar Management
    virtual dsint sliderMovedHook(int type, dsint value, dsint delta); // can be made protected
    int scaleFromUint64ToScrollBarRange(dsint value); // can be made private
    dsint scaleFromScrollBarRangeToUint64(int value); // can be made private

    void updateScrollBarRange(dsint range); // setRowCount+resizeEvent needs this, can be made private

    // Coordinates Utils
    int getIndexOffsetFromY(int y) const; // can be made protected
    int getColumnIndexFromX(int x) const; // can be made protected
    int getColumnPosition(int index) const; // can be made protected
    int transY(int y) const; // can be made protected
    int getViewableRowsCount() const; // can be made protected
    virtual int getLineToPrintcount() const;

    // New Columns/New Size
    virtual void addColumnAt(int width, const QString & title, bool isClickable);
    virtual void setRowCount(dsint count);
    virtual void deleteAllColumns(); // can be made protected, although it makes sense as a public API
    void setColTitle(int index, const QString & title); // can be deleted, although it makes sense as a public API
    QString getColTitle(int index) const; // can be deleted, although it makes sense as a public API

    // Getter & Setter
    dsint getRowCount() const;
    int getColumnCount() const;
    int getRowHeight() const;
    int getColumnWidth(int index) const;
    void setColumnWidth(int index, int width);
    void setColumnOrder(int pos, int index);
    int getColumnOrder(int index) const;
    int getHeaderHeight() const; // can be made protected
    int getTableHeight() const; // can be made protected
    int getGuiState() const; // can be made protected
    int getNbrOfLineToPrint() const;
    void setNbrOfLineToPrint(int parNbrOfLineToPrint);
    void setShowHeader(bool show);
    int getCharWidth() const;
    bool getColumnHidden(int col) const;
    void setColumnHidden(int col, bool hidden);
    bool getDrawDebugOnly() const;
    void setDrawDebugOnly(bool value);
    bool getAllowPainting() const;
    void setAllowPainting(bool allow);
    void setDisassemblyPopupEnabled(bool enable);

    // UI customization
    void loadColumnFromConfig(const QString & viewName);
    void saveColumnToConfig();
    static void setupColumnConfigDefaultValue(QMap<QString, duint> & map, const QString & viewName, int columnCount);

    // Table offset management
    dsint getTableOffset() const;
    void setTableOffset(dsint val);

    // Update/Reload/Refresh/Repaint
    virtual void prepareData();

    virtual duint getDisassemblyPopupAddress(int mousex, int mousey);

signals:
    void enterPressedSignal();
    void headerButtonPressed(int col);
    void headerButtonReleased(int col);
    void tableOffsetChanged(dsint i);
    void viewableRowsChanged(int rows);
    void repainted();

public slots:
    // Update/Reload/Refresh/Repaint
    virtual void reloadData();
    void updateViewport();

    // ScrollBar Management
    void vertSliderActionSlot(int action);

protected slots:
    void ShowDisassemblyPopup(duint addr, int x, int y); // this should probably be a slot, but doesn't need emit fixes (it's already used correctly)

private slots:
    // Configuration
    void updateColorsSlot();
    void updateFontsSlot();
    void updateShortcutsSlot();
    void closeSlot();

private:
    struct ColumnResizingData
    {
        bool splitHandle;
        int index;
        int lastPosX;
    };

    struct HeaderButton
    {
        bool isClickable;
        bool isPressed;
        bool isMouseOver;
    };

    struct Column
    {
        int width;
        bool hidden;
        HeaderButton header;
        QString title;
    };

    struct Header
    {
        bool isVisible;
        int height;
        int activeButtonIndex;
    };

    struct ScrollBar64
    {
        bool is64;
        int rightShiftCount;
    };

    GuiState mGuiState;

    Header mHeader;
    QPushButton mHeaderButtonSytle;

    QList<Column> mColumnList;
    ColumnResizingData mColResizeData;

    QList<int> mColumnOrder;
    int mReorderStartX;
    int mHoveredColumnDisplayIndex;

    dsint mRowCount;
    dsint mTableOffset;
    dsint mPrevTableOffset;
    int mNbrOfLineToPrint;

    bool mShouldReload;
    bool mDrawDebugOnly;
    bool mPopupEnabled;
    bool mAllowPainting;

    static int mMouseWheelScrollDelta;
    ScrollBar64 mScrollBarAttributes;

    int getColumnDisplayIndexFromX(int x);
    friend class ColumnReorderDialog;

protected:
    // Configuration
    QColor mBackgroundColor;
    QColor mTextColor;
    QColor mSeparatorColor;
    QColor mHeaderTextColor;
    QColor mSelectionColor;
    QString mViewName;

    // Font metrics
    CachedFontMetrics* mFontMetrics;
    void invalidateCachedFont();

    // Disassembly Popup
    DisassemblyPopup* mDisassemblyPopup;
};

#endif // ABSTRACTTABLEVIEW_H
