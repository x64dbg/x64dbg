#pragma once

// Qt includes
#include <QTabBar>

//////////////////////////////////////////////////////////////////////////////
// Summary:
//    MHTabBar implements the a Tab Bar with detach functionality.
//////////////////////////////////////////////////////////////////////////////
class MHTabBar: public QTabBar
{
    Q_OBJECT

public:
    MHTabBar(QWidget* parent, bool allowDetach, bool allowDelete);
    ~MHTabBar();

protected:
    void contextMenuEvent(QContextMenuEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);

signals:
    // Detach Tab
    void OnDetachTab(int index, const QPoint & dropPoint);
    // Move Tab
    void OnMoveTab(int fromIndex, int toIndex);
    // Delete Tab
    void OnDeleteTab(int index);
    // Double Click on Tab, Get Index
    void OnDoubleClickTabIndex(int index);

private:
    bool mAllowDetach;
    bool mAllowDelete;
};
