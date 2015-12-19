#ifndef _GRAPH_EDGE_H
#define _GRAPH_EDGE_H

#include <ogdf/basic/geometry.h>
#include <QGraphicsItem>
#include <QPainter>
#include <QDebug>

class GraphEdge : public QAbstractGraphicsShapeItem
{
public:
    enum EDGE_TYPE
    {
        EDGE_RIGHT,
        EDGE_LEFT
    } ;

    GraphEdge(QPointF start, QPointF end, ogdf::DPolyline bends, QRectF sourceRect, QRectF targetRect, EDGE_TYPE edgeType);
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    qreal calculateDistance(QPointF p1, QPointF p2);
    QPointF calculateNearestIntersect(QRectF rect, QPointF p1, QPointF p2);
    QList<QPointF> calculateLine(QPointF start, QPointF end, ogdf::DPolyline bends, QRectF sourceRect, QRectF targetRect);
    QList<QPointF> calculateArrow(const QList<QPointF> & linePoints);
    QRectF calculateBoundingRect(const QList<QPointF> & linePoints, const QList<QPointF> & arrowPoints);
    void preparePainterPaths(const QList<QPointF> & linePoints, const QList<QPointF> & arrowPoints);
    void setEdgeColor(QBrush edgeColor);
    GraphEdge::EDGE_TYPE getEdgeType();

private:
    QPainterPath _line;
    QPainterPath _arrow;
    QRectF _boundingRect;
    QBrush _edgeColor;
    EDGE_TYPE _edgeType;
};

#endif //_GRAPH_EDGE_H
