#include "EdgeGraphicsShapeItem.h"

EdgeGraphicsShapeItem::EdgeGraphicsShapeItem(QGraphicsItem* startItem, QGraphicsItem* endItem, ogdf::DPolyline const & bends)
    : _startItem(startItem), _endItem(endItem), _clr(Qt::blue), _bends(bends)
{
    setZValue(1.0);
    computeCoordinates();
}

QPainterPath EdgeGraphicsShapeItem::shape(void) const
{
    QPainterPath path;
    path.addPath(_line);
    path.addPath(_head);
    return path;
}

QRectF EdgeGraphicsShapeItem::boundingRect(void) const
{
    return shape().boundingRect();
}

void EdgeGraphicsShapeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget /*= 0*/)
{
    computeCoordinates();
    std::vector<QPointF> points;
    bool revLine = (_startItem->y() > _endItem->y()) ? true : false;
    _clr = revLine ? Qt::red : Qt::blue;

    painter->setRenderHint(QPainter::Antialiasing);
    QPen pen(_clr);
    pen.setWidth(2);
    QBrush brs(_clr);

    painter->setPen(pen);
    painter->drawPath(_line);
    painter->setBrush(brs);
    painter->drawPath(_head);
}

void EdgeGraphicsShapeItem::computeCoordinates(void)
{
    prepareGeometryChange();
    std::vector<QPointF> points;
    points.reserve(2 + _bends.size());
    auto startRect = _startItem->boundingRect();
    auto endRect   = _endItem->boundingRect();
    bool revLine = (_startItem->y() > _endItem->y()) ? true : false;

    // Retrieve points
    if(revLine)
        points.push_back(QPointF(_endItem->x() + endRect.width() / 2, _endItem->y() + endRect.height()));
    else
        points.push_back(QPointF(_endItem->x() + endRect.width() / 2, _endItem->y()));

    for(auto it = _bends.begin(); it.valid(); ++it)
        //points.push_back(QPointF(endRect.width() / 2 + (*it).m_x, endRect.height() / 2 + (*it).m_y));
        points.push_back(QPointF((*it).m_x, (*it).m_y));

    if(revLine)
        points.push_back(QPointF(_startItem->x() + startRect.width() / 2, _startItem->y()));
    else
        points.push_back(QPointF(_startItem->x() + startRect.width() / 2, _startItem->y() + startRect.height()));

    // Retrieve lines and boundingRect
    std::list<QLineF> lines;
    auto iterPt = std::begin(points);
    QPointF startPt;
    startPt = *iterPt;
    ++iterPt;
    for(; iterPt != std::end(points); ++iterPt)
    {
        lines.push_back(QLineF(startPt, *iterPt));
        startPt = *iterPt;
    }

    // Generate path for line
    _line = QPainterPath();
    if(points.size() == 2)
    {
        QPolygonF polyLine;
        polyLine << points.front() << points.back();
        _line.addPolygon(polyLine);
    }
    else
    {
        _line.moveTo(points.front());
        for(size_t i = 0; i + 2 < points.size(); ++i)
            _line.cubicTo(points[i], points[i + 1], points[i + 2]);
    }

    // Generate path for head
    static const qreal Pi = 3.14;
    static const qreal arrowSize = 10.0;
    auto refLine = revLine ? lines.front() : lines.front();
    double angle = ::acos(refLine.dx() / refLine.length());
    if(revLine)
        angle = (Pi * 2) - angle;
    QPointF arrowPt = refLine.p1();
    QPointF arrowP1 = arrowPt + QPointF(::sin(angle + Pi / 3)      * arrowSize, ::cos(angle + Pi / 3)      * arrowSize);
    QPointF arrowP2 = arrowPt + QPointF(::sin(angle + Pi - Pi / 3) * arrowSize, ::cos(angle + Pi - Pi / 3) * arrowSize);
    QPolygonF head;
    head << arrowPt << arrowP1 << arrowP2;
    _head = QPainterPath();
    _head.addPolygon(head);
    _head.setFillRule(Qt::WindingFill);
}
