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

#include "TabBar.h"
#include "TabWidget.h"

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
    QMenu menu(this);
    QAction detach(tr("&Detach"), this);
    if(mAllowDetach)
        menu.addAction(&detach);
    QAction close(tr("&Close"), this);
    if(mAllowDelete)
        menu.addAction(&close);
    QAction* executed = menu.exec(event->globalPos());
    if(executed == &detach)
    {
        QPoint p(0, 0);
        emit OnDetachTab((int)tabAt(event->pos()), p);
    }
    else if(executed == &close)
    {
        emit OnDeleteTab((int)tabAt(event->pos()));
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
