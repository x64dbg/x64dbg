#ifndef _GRAPH_EDGE_H
#define _GRAPH_EDGE_H

#include <ogdf/basic/geometry.h>
#include <QGraphicsItem>
#include <QDebug>

class GraphEdge : public QAbstractGraphicsShapeItem
{
public:
    GraphEdge(QPointF start, QPointF end, ogdf::DPolyline bends, QRectF sourceRect, QRectF targetRect) : QAbstractGraphicsShapeItem()
    {
        QList<QPointF> linePoints = calculateLine(start, end, bends, sourceRect, targetRect);
        for(auto p : linePoints)
            qDebug() << p;
        QList<QPointF> arrowPoints = calculateArrow(linePoints);
        _boundingRect = calculateBoundingRect(linePoints, arrowPoints);
        preparePainterPaths(linePoints, arrowPoints);
    }

    QRectF boundingRect() const
    {
        return _boundingRect;
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        //save painter
        painter->save();

#if _DEBUG
        //draw bounding rect
        painter->setPen(QPen(Qt::red, 1));
        painter->drawRect(boundingRect());
#endif //_DEBUG

        //set painter options
        painter->setRenderHint(QPainter::Antialiasing);

        int lineSize = 2;

        //draw line
        painter->setPen(QPen(Qt::green, lineSize));
        painter->drawPath(_line);

        //draw arrow
        painter->setPen(QPen(Qt::green, lineSize));
        painter->drawPath(_arrow);

        //restore painter
        painter->restore();
    }

    qreal calculateDistance(QPointF p1, QPointF p2)
    {
        QPointF d = p2 - p1;
        return sqrt(d.x() * d.x() + d.y() * d.y());
    }

    QPointF calculateNearestIntersect(QRectF rect, QPointF p1, QPointF p2)
    {
        /*qDebug() << "calculateNearest";
        qDebug() << "rect" << rect.topLeft() << rect.bottomRight();
        qDebug() << "p1" << p1;
        qDebug() << "p2" << p2;*/

        //y=a*x+b
        //a = dy/dx = (p1.y-p2.y)/(p1.x-p2.x)
        //b = p1.y - p1.x;

        qreal div = p1.x()-p2.x();

        if(div == 0)
        {
            QPointF i1(p1.x(), rect.top());
            //qDebug() << "i1" << i1;
            QPointF i2(p1.x(), rect.bottom());
            //qDebug() << "i2" << i2;

            if(p2.y() < p1.y())
                return i1;
            else
                return i2;
        }
        else
        {
            QPointF result;
            qreal bestDist = 10e99;

            qreal a = (p1.y()-p2.y()) / div;
            qreal b = p1.y() - a * p1.x();
            //qDebug() << "a" << a << "b" << b;

            //intersect 1
            //rect.top() = a*x+b;
            //x = (b - rect.top()) / -a
            QPointF i1((b - rect.top()) / -a, rect.top());
            //qDebug() << "i1" << i1;
            //qDebug() << "consider?" << rect.contains(i1);
            if(rect.contains(i1))
            {
                qreal dist = calculateDistance(p2, i1);
                if(dist < bestDist)
                {
                    bestDist = dist;
                    result = i1;
                }
            }

            //intersect 2
            //rect.bottom() = a*x+b
            //x = (b - rect.bottom()) / -a
            QPointF i2((b - rect.bottom()) / -a, rect.bottom());
            //qDebug() << "i2" << i2;
            //qDebug() << "consider?" << rect.contains(i2);
            if(rect.contains(i2))
            {
                qreal dist = calculateDistance(p2, i2);
                if(dist < bestDist)
                {
                    bestDist = dist;
                    result = i2;
                }
            }

            //intersect 3
            //x=rect.left()
            QPointF i3(rect.left(), a * rect.left() + b);
            //qDebug() << "i3" << i3;
            //qDebug() << "consider?" << rect.contains(i3);
            if(rect.contains(i3))
            {
                qreal dist = calculateDistance(p2, i3);
                if(dist < bestDist)
                {
                    bestDist = dist;
                    result = i3;
                }
            }

            //intersect 4
            //x=rect.right()
            QPointF i4(rect.right(), a * rect.right() + b);
            //qDebug() << "i4" << i4;
            //qDebug() << "consider?" << rect.contains(i4);
            if(rect.contains(i4))
            {
                qreal dist = calculateDistance(p2, i4);
                if(dist < bestDist)
                {
                    bestDist = dist;
                    result = i4;
                }
            }
            return result;
        }
        //qDebug() << " ";
    }

    QList<QPointF> calculateLine(QPointF start, QPointF end, ogdf::DPolyline bends, QRectF sourceRect, QRectF targetRect)
    {
        QList<QPointF> linePoints;
        linePoints << start;
        for(auto p : bends)
            linePoints << QPointF(p.m_x, p.m_y);
        linePoints << end;

        QPointF nearestI = calculateNearestIntersect(sourceRect, linePoints[0], linePoints[1]);
        linePoints[0]=nearestI;
        int len = linePoints.length();
        nearestI = calculateNearestIntersect(targetRect, linePoints[len-1], linePoints[len-2]);
        linePoints[len-1]=nearestI;

        return linePoints;
    }

    QList<QPointF> calculateArrow(const QList<QPointF> & linePoints)
    {
        //arrow
        int len=linePoints.length();
        QLineF perpLine = QLineF(linePoints[len-1], linePoints[len-2]).normalVector();

        qreal arrowLen = 6;

        QLineF a;
        a.setP1(linePoints[len-1]);
        a.setAngle(perpLine.angle() - 45);
        a.setLength(arrowLen);

        QLineF b;
        b.setP1(linePoints[len-1]);
        b.setAngle(perpLine.angle() - 135);
        b.setLength(arrowLen);

        QLineF c;
        c.setP1(a.p2());
        c.setP2(b.p2());

        QList<QPointF> arrowPoints;
        arrowPoints << a.p1() << a.p2() << b.p1() << b.p2() << c.p1() << c.p2();
        return arrowPoints;
    }

    QRectF calculateBoundingRect(const QList<QPointF> & linePoints, const QList<QPointF> & arrowPoints)
    {
        QList<QPointF> allPoints;
        allPoints << linePoints << arrowPoints;
        //find top-left and bottom-right points for the bounding rect
        QPointF topLeft = allPoints[0];
        QPointF bottomRight = topLeft;
        for(auto p : allPoints)
        {
            qreal x = p.x();
            qreal y = p.y();

            if(x < topLeft.x())
                topLeft.setX(x);
            if(y < topLeft.y())
                topLeft.setY(y);

            if(x > bottomRight.x())
                bottomRight.setX(x);
            if(y > bottomRight.y())
                bottomRight.setY(y);
        }
        return QRectF(topLeft, bottomRight);
    }

    void preparePainterPaths(const QList<QPointF> & linePoints, const QList<QPointF> & arrowPoints)
    {
        //edge line
        QPolygonF polyLine;
        for(auto p : linePoints)
            polyLine << p;
        _line.addPolygon(polyLine);

        //arrow
        QPolygonF polyArrow;
        for(auto p : arrowPoints)
            polyArrow << p;
        _arrow.addPolygon(polyArrow);
    }

private:
    QPainterPath _line;
    QPainterPath _arrow;
    QRectF _boundingRect;
};

#endif //_GRAPH_EDGE_H
