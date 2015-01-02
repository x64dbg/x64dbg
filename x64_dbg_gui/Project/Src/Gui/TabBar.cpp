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


/*
//////////////////////////////////////////////////////////////////////////////
void MHTabBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPos = event->pos();

    m_dragDroppedPos.setX(0);
    m_dragDroppedPos.setY(0);
    m_dragMovedPos.setX(0);
    m_dragMovedPos.setY(0);

    m_dragInitiated = false;

    QTabBar::mousePressEvent(event);
}

//////////////////////////////////////////////////////////////////////////////
void MHTabBar::mouseMoveEvent(QMouseEvent* event)
{
    // Distinguish a drag
    if ( !m_dragStartPos.isNull() &&
         ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance()) )
    {
        m_dragInitiated = true;
    }

    // The left button is pressed
    // And the move could also be a drag
    // And the mouse moved outside the tab bar
    if ((event->buttons() & Qt::LeftButton) && m_dragInitiated && !geometry().contains(event->pos()))
    {
        // Stop the move to be able to convert to a drag
        {
            QMouseEvent finishMoveEvent(QEvent::MouseMove, event->pos(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            QTabBar::mouseMoveEvent(&finishMoveEvent);
        }

        // A crude way to distinguish tab-reordering drops from other ones
        QMimeData* mimeData = new QMimeData;
        mimeData->setData("action", "application/tab-detach");

        // Initiate Drag
        QDrag* drag = new QDrag(this);
        drag->setMimeData(mimeData);

        // Create transparent screen dump
        QPixmap pixmap = QPixmap::grabWindow(dynamic_cast<MHTabWidget*>(parentWidget())->currentWidget()->winId()).scaled(640, 480, Qt::KeepAspectRatio);
        QPixmap targetPixmap(pixmap.size());

        QPainter painter(&targetPixmap);
        painter.setOpacity(0.5);
        painter.drawPixmap(0, 0, pixmap);
        painter.end();

        drag->setPixmap(targetPixmap);

        // Handle Detach and Move
        Qt::DropAction dragged = drag->exec(Qt::MoveAction | Qt::CopyAction);

        if (dragged == Qt::IgnoreAction)
        {
            event->accept();
            OnDetachTab(tabAt(m_dragStartPos), QCursor::pos());
        }
        else if (dragged == Qt::MoveAction)
        {
            if (!m_dragDroppedPos.isNull())
            {
                event->accept();
                OnMoveTab(tabAt(m_dragStartPos), tabAt(m_dragDroppedPos));
            }
        }

        delete drag;
    }
    else
    {
        QTabBar::mouseMoveEvent(event);
    }
}

//////////////////////////////////////////////////////////////////////////////
void MHTabBar::dragEnterEvent(QDragEnterEvent* event)
{
    // Only accept if it's an tab-reordering request (not a detach request)
    const QMimeData* m = event->mimeData();

    if (m->formats().contains("action") && (m->data("action") != "application/tab-detach"))
    {
        event->acceptProposedAction();
    }

    QTabBar::dragEnterEvent(event);
}

//////////////////////////////////////////////////////////////////////////////
void MHTabBar::dragMoveEvent(QDragMoveEvent* event)
{
    // Only accept if it's an tab-reordering request (not a detach request)
    const QMimeData* m = event->mimeData();

    if (m->formats().contains("action") && (m->data("action") != "application/tab-detach"))
    {
        m_dragMovedPos = event->pos();
        event->acceptProposedAction();
    }

    QTabBar::dragMoveEvent(event);
}

//////////////////////////////////////////////////////////////////////////////
void MHTabBar::dropEvent(QDropEvent* event)
{
    // If a dragged Event is dropped within this widget it is not a drag but a move.
    m_dragDroppedPos = event->pos();

    QTabBar::dropEvent(event);
}
*/
