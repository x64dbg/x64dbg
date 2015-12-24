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
#include "QActionLambda.h"

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

    // Configuration
    virtual void Initialize();
    virtual void updateColors();
    virtual void updateFonts();
    virtual void updateShortcuts();

    // Pure Virtual Methods
    virtual QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) = 0;

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

    // New Columns/New Size
    virtual void addColumnAt(int width, QString title, bool isClickable);
    virtual void setRowCount(dsint count);
    virtual void deleteAllColumns();
    void setColTitle(int index, QString title);
    QString getColTitle(int index);

    // Getter & Setter
    dsint getRowCount();
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

    dsint mRowCount;

    int mMouseWheelScrollDelta;

    dsint mTableOffset;
    Header_t mHeader;

    int mNbrOfLineToPrint;

    dsint mPrevTableOffset;

    bool mShouldReload;

    ScrollBar64_t mScrollBarAttributes;

protected:
    bool mAllowPainting;
    bool mDrawDebugOnly;

    // Configuration
    QColor backgroundColor;
    QColor textColor;
    QColor separatorColor;
    QColor headerTextColor;
    QColor selectionColor;

    //action helpers
private:
    struct ActionShortcut
    {
        QAction* action;
        const char* shortcut;

        ActionShortcut(QAction* action, const char* shortcut)
            : action(action),
              shortcut(shortcut)
        {
        }
    };

    std::vector<ActionShortcut> actionShortcutPairs;

    inline QAction* connectAction(QAction* action, const char* slot)
    {
        connect(action, SIGNAL(triggered(bool)), this, slot);
        return action;
    }

    inline QAction* connectAction(QAction* action, QActionLambda::TriggerCallback callback)
    {
        auto lambda = new QActionLambda(action->parent(), callback);
        connect(action, SIGNAL(triggered(bool)), lambda, SLOT(triggeredSlot()));
        return action;
    }

    inline QAction* connectShortcutAction(QAction* action, const char* shortcut)
    {
        actionShortcutPairs.push_back(ActionShortcut(action, shortcut));
        action->setShortcut(ConfigShortcut(shortcut));
        action->setShortcutContext(Qt::WidgetShortcut);
        addAction(action);
        return action;
    }

    inline QAction* connectMenuAction(QMenu* menu, QAction* action)
    {
        menu->addAction(action);
        return action;
    }

protected:
    inline QMenu* makeMenu(const QString & title)
    {
        return new QMenu(title, this);
    }

    inline QMenu* makeMenu(const QIcon & icon, const QString & title)
    {
        QMenu* menu = new QMenu(title, this);
        menu->setIcon(icon);
        return menu;
    }

    template<typename T>
    inline QAction* makeAction(const QString & text, T slot)
    {
        return connectAction(new QAction(text, this), slot);
    }

    template<typename T>
    inline QAction* makeAction(const QIcon & icon, const QString & text, T slot)
    {
        return connectAction(new QAction(icon, text, this), slot);
    }

    template<typename T>
    inline QAction* makeShortcutAction(const QString & text, T slot, const char* shortcut)
    {
        return connectShortcutAction(makeAction(text, slot), shortcut);
    }

    template<typename T>
    inline QAction* makeShortcutAction(const QIcon & icon, const QString & text, T slot, const char* shortcut)
    {
        return connectShortcutAction(makeAction(icon, text, slot), shortcut);
    }

    template<typename T>
    inline QAction* makeMenuAction(QMenu* menu, const QString & text, T slot)
    {
        return connectMenuAction(menu, makeAction(text, slot));
    }

    template<typename T>
    inline QAction* makeMenuAction(QMenu* menu, const QIcon & icon, const QString & text, T slot)
    {
        return connectMenuAction(menu, makeAction(icon, text, slot));
    }

    template<typename T>
    inline QAction* makeShortcutMenuAction(QMenu* menu, const QString & text, T slot, const char* shortcut)
    {
        return connectShortcutAction(makeMenuAction(menu, text, slot), shortcut);
    }

    template<typename T>
    inline QAction* makeShortcutMenuAction(QMenu* menu, const QIcon & icon, const QString & text, T slot, const char* shortcut)
    {
        return connectShortcutAction(makeMenuAction(menu, icon, text, slot), shortcut);
    }
};

#endif // ABSTRACTTABLEVIEW_H
