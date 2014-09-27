#ifndef FLOWGRAPHGRAPHICSVIEW_H
#define FLOWGRAPHGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QtCore>
#include <QtGui>

// src: http://www.qtcentre.org/threads/35738-How-to-zooming-like-in-AutoCAD-with-QGraphicsView
class FlowGraphGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    FlowGraphGraphicsView(QWidget* parent = 0) : QGraphicsView(parent), _isMoving(false), _lastPos(), _lastCursor(Qt::ArrowCursor) {}
    void zoom(qreal factor, QPointF centerPt);

protected:
    virtual void wheelEvent(QWheelEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

private:
    bool    _isMoving;
    QPoint  _lastPos;
    QCursor _lastCursor;
};

#endif // FLOWGRAPHGRAPHICSVIEW_H
