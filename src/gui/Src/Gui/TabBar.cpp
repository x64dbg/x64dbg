// Qt includes
#include <QApplication>
#include <QMouseEvent>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPaintDevice>
#include <QDrag>
#include <QMimeData>
#include <QMenu>

#include "tabbar.h"
#include "tabwidget.h"

//////////////////////////////////////////////////////////////
// Default Constructor
//////////////////////////////////////////////////////////////
MHTabBar::MHTabBar(QWidget* parent, bool allowDetach, bool allowDelete) : QTabBar(parent)
{
    mAllowDetach = allowDetach;
    mAllowDelete = allowDelete;
    setAcceptDrops(true);
    setElideMode(Qt::ElideNone);
    setSelectionBehaviorOnRemove(QTabBar::SelectLeftTab);
    setMovable(true);
    setDrawBase(false);
}

//////////////////////////////////////////////////////////////
// Default Destructor
//////////////////////////////////////////////////////////////
MHTabBar::~MHTabBar()
{
}

void MHTabBar::contextMenuEvent(QContextMenuEvent* event)
{
    if(!mAllowDetach && !mAllowDelete)
        return;
    QMenu wMenu(this);
    QAction wDetach(tr("&Detach"), this);
    if(mAllowDetach)
        wMenu.addAction(&wDetach);
    QAction wDelete(tr("&Close"), this);
    if(mAllowDelete)
        wMenu.addAction(&wDelete);
    QAction* executed = wMenu.exec(event->globalPos());
    if(executed == &wDetach)
    {
        QPoint p(0, 0);
        OnDetachTab((int)tabAt(event->pos()), p);
    }
    else if(executed == &wDelete)
    {
        OnDeleteTab((int)tabAt(event->pos()));
    }
}

void MHTabBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    // On tab double click emit the index of the tab that was double clicked
    if(event->button() == Qt::LeftButton)
    {
        int tabIndex = tabAt(event->pos());

        if(tabIndex != -1)
            emit OnDoubleClickTabIndex(tabIndex);
    }
}
