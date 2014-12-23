#ifndef ABSTRACTTABLEVIEW_H
#define ABSTRACTTABLEVIEW_H

#include <QScrollBar>
#include <QAbstractScrollArea>
#include <QPushButton>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include "NewTypes.h"

//Hacky class that fixes a really annoying cursor problem
class AbstractTableScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    AbstractTableScrollBar(QScrollBar* scrollbar)
    {
        setOrientation(scrollbar->orientation());
        setParent(scrollbar->parentWidget());
    }

    void enterEvent(QEvent* event)
    {
        Q_UNUSED(event);
        QApplication::setOverrideCursor(Qt::ArrowCursor);
    }

    void leaveEvent(QEvent* event)
    {
        Q_UNUSED(event);
        QApplication::restoreOverrideCursor();
    }
};

class AbstractTableView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    enum GuiState_t {NoState, ReadyToResize, ResizeColumnState, HeaderButtonPressed};

    // Constructor
    explicit AbstractTableView(QWidget* parent = 0);

    //config updates
    virtual void colorsUpdated();
    virtual void fontsUpdated();

    // Pure Virtual Methods
    virtual QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h) = 0;

    // Painting Stuff
    void paintEvent(QPaintEvent* event);

    // Mouse Management
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);
    void resizeEvent(QResizeEvent* event);
    void keyPressEvent(QKeyEvent* event);

    // ScrollBar Management
    virtual int_t sliderMovedHook(int type, int_t value, int_t delta);
    int scaleFromUint64ToScrollBarRange(int_t value);
    int_t scaleFromScrollBarRangeToUint64(int value);
    void updateScrollBarRange(int_t range);


    // Coordinates Utils
    int getIndexOffsetFromY(int y);
    int getColumnIndexFromX(int x);
    int getColumnPosition(int index);
    int transY(int y);
    int getViewableRowsCount();
    virtual int getLineToPrintcount();

    // New Columns/New Size
    virtual void addColumnAt(int width, QString title, bool isClickable);
    virtual void setRowCount(int_t count);
    virtual void deleteAllColumns();
    void setColTitle(int index, QString title);
    QString getColTitle(int index);

    // Getter & Setter
    int_t getRowCount();
    int getColumnCount();
    int getRowHeight();
    int getColumnWidth(int index);
    void setColumnWidth(int index, int width);
    int getHeaderHeight();
    int getTableHeigth();
    int getGuiState();
    int getNbrOfLineToPrint();
    void setNbrOfLineToPrint(int parNbrOfLineToPrint);
    void setShowHeader(bool show);
    int getCharWidth();

    // Table Offset Management
    int_t getTableOffset();
    void setTableOffset(int_t val);

    // Update/Reload/Refresh/Repaint
    virtual void prepareData();

signals:
    void enterPressedSignal();
    void headerButtonPressed(int col);
    void headerButtonReleased(int col);
    void tableOffsetChanged(int_t i);
    void viewableRows(int rows);
    void repainted();

public slots:
    void colorsUpdatedSlot();
    void fontsUpdatedSlot();

    // Update/Reload/Refresh/Repaint
    virtual void reloadData();
    void repaint();

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
        HeaderButton_t header;
        QString title;
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

    int_t mRowCount;

    int mMouseWheelScrollDelta;

    int_t mTableOffset;
    Header_t mHeader;

    int mNbrOfLineToPrint;

    int_t mPrevTableOffset;

    bool mShouldReload;

    ScrollBar64_t mScrollBarAttributes;

protected:
    QColor backgroundColor;
    QColor textColor;
    QColor separatorColor;
    QColor headerTextColor;
    QColor selectionColor;
};

#endif // ABSTRACTTABLEVIEW_H
