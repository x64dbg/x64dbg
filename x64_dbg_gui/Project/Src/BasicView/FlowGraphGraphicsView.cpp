#include "FlowGraphGraphicsView.h"

void FlowGraphGraphicsView::zoom(qreal factor, QPointF centerPt)
{
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    scale(factor, factor);
    centerOn(centerPt);
}

void FlowGraphGraphicsView::wheelEvent(QWheelEvent* event)
{
    if(event->modifiers().testFlag(Qt::ControlModifier))
    {
        int numSteps = event->delta() / 15 / 8;

        if(numSteps != 0)
        {
            qreal sc = pow(1.25, numSteps);
            zoom(sc, mapToScene(event->pos()));
            event->accept();
        }
    }
    QGraphicsView::wheelEvent(event);
}

void FlowGraphGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if(_isMoving && event->button() == Qt::LeftButton)
    {
        _lastCursor = cursor();
        _lastPos    = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void FlowGraphGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if(_isMoving)
    {
        _isMoving = false;
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void FlowGraphGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    if(_isMoving)
    {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (event->pos().x() - _lastPos.x()));
        verticalScrollBar()  ->setValue(verticalScrollBar()->value()   - (event->pos().y() - _lastPos.y()));
        _lastPos = event->pos();
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void FlowGraphGraphicsView::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Space && !_isMoving)
    {
        _isMoving   = true;
        _lastCursor = Qt::ArrowCursor;
        setCursor(Qt::OpenHandCursor);
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void FlowGraphGraphicsView::keyReleaseEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Space && _isMoving)
    {
        setCursor(_lastCursor);
        _isMoving   = false;
        _lastCursor = Qt::ArrowCursor;
        return;
    }
    QGraphicsView::keyReleaseEvent(event);
}

