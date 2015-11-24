// Qt includes
#include <QApplication>
#include <QMouseEvent>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPaintDevice>
#include <QDrag>
#include <QMimeData>

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
    setElideMode(Qt::ElideRight);
    setSelectionBehaviorOnRemove(QTabBar::SelectLeftTab);
    setMovable(true);
}

//////////////////////////////////////////////////////////////
// Default Destructor
//////////////////////////////////////////////////////////////
MHTabBar::~MHTabBar(void)
{
}

void MHTabBar::contextMenuEvent(QContextMenuEvent* event)
{
    if(!mAllowDetach && !mAllowDelete)
        return;
    QMenu wMenu(this);
    QAction wDetach("&Detach", this);
    if(mAllowDetach)
        wMenu.addAction(&wDetach);
    QAction wDelete("&Delete", this);
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
