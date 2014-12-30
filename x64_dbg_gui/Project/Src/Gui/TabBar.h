#ifndef __MHTABBAR_H__
#define __MHTABBAR_H__

// Qt includes
#include <QTabBar>
#include <QMenu>
#include <QAction>

// Qt forward class definitions
class MHTabBar;
class QMainWindow;

//////////////////////////////////////////////////////////////////////////////
// Summary:
//    MHTabBar implements the a Tab Bar with tear-off functionality.
//////////////////////////////////////////////////////////////////////////////
class MHTabBar: public QTabBar
{
    Q_OBJECT
public:
    MHTabBar(QWidget* parent, bool allowDetach, bool allowDelete);
    ~MHTabBar(void);

protected:
    void contextMenuEvent(QContextMenuEvent* event);
    /*
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    void dragMoveEvent(QDragMoveEvent* event);
    void dropEvent(QDropEvent* event);
    */

signals:
    // Detach Tab
    void OnDetachTab(int index, QPoint & dropPoint);
    // Move Tab
    void OnMoveTab(int fromIndex, int toIndex);
    // Delete Tab
    void OnDeleteTab(int index);

private:
    bool mAllowDetach;
    bool mAllowDelete;
    /*
    QPoint       m_dragStartPos;
    QPoint       m_dragMovedPos;
    QPoint       m_dragDroppedPos;
    bool         m_dragInitiated;
    int          m_dragCurrentIndex;
    */
};

#endif // __MHTABBAR_H__
