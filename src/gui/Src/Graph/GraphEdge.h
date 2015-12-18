#ifndef _GRAPH_EDGE_H
#define _GRAPH_EDGE_H

#include <ogdf/basic/geometry.h>
#include <QGraphicsItem>
#include <QPainter>
#include <QDebug>

class GraphEdge : public QAbstractGraphicsShapeItem
{
public:
    GraphEdge(QPointF start, QPointF end, ogdf::DPolyline bends, QRectF sourceRect, QRectF targetRect, QBrush lineColor = Qt::green);
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    qreal calculateDistance(QPointF p1, QPointF p2);
    QPointF calculateNearestIntersect(QRectF rect, QPointF p1, QPointF p2);
    QList<QPointF> calculateLine(QPointF start, QPointF end, ogdf::DPolyline bends, QRectF sourceRect, QRectF targetRect);
    QList<QPointF> calculateArrow(const QList<QPointF> & linePoints);
    QRectF calculateBoundingRect(const QList<QPointF> & linePoints, const QList<QPointF> & arrowPoints);
    void preparePainterPaths(const QList<QPointF> & linePoints, const QList<QPointF> & arrowPoints);

private:
    QPainterPath _line;
    QPainterPath _arrow;
    QRectF _boundingRect;
    QBrush _edgeColor;
};

#endif //_GRAPH_EDGE_H
