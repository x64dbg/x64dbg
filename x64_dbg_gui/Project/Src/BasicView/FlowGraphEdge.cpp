#include "FlowGraphEdge.h"
#include "Configuration.h"

FlowGraphEdge::FlowGraphEdge(QGraphicsItem* startBlock, QGraphicsItem* endBlock, ogdf::DPolyline const & bends)
    : mStartBlock(startBlock), mEndBlock(endBlock), mLineColor(ConfigColor("FlowGraphEdgeTrue")), _bends(bends)
{
    setZValue(1.0);
    computeCoordinates();
}

int FlowGraphEdge::type() const
{
    return Type;
}

QPainterPath FlowGraphEdge::shape(void) const
{
    QPainterPath path;
    path.addPath(mLine);
    path.addPath(mHead);
    return path;
}

QRectF FlowGraphEdge::boundingRect(void) const
{
    return shape().boundingRect();
}

void FlowGraphEdge::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget /*= 0*/)
{
    computeCoordinates();
    std::vector<QPointF> points;
    bool revLine = (mStartBlock->y() > mEndBlock->y()) ? true : false;
    mLineColor = revLine ? ConfigColor("FlowGraphEdgeFalse") : ConfigColor("FlowGraphEdgeTrue");

    painter->setRenderHint(QPainter::Antialiasing);
    QPen pen(mLineColor);
    pen.setWidth(2);
    QBrush brs(mLineColor);

    painter->setPen(pen);
    painter->drawPath(mLine);
    painter->setBrush(brs);
    painter->drawPath(mHead);
}

void FlowGraphEdge::computeCoordinates(void)
{
    prepareGeometryChange();
    std::vector<QPointF> points;
    points.reserve(2 + _bends.size());
    auto startRect = mStartBlock->boundingRect();
    auto endRect   = mEndBlock->boundingRect();
    bool revLine = (mStartBlock->y() > mEndBlock->y()) ? true : false;

    // Retrieve points
    if(revLine)
        points.push_back(QPointF(mEndBlock->x() + endRect.width() / 2, mEndBlock->y() + endRect.height()));
    else
        points.push_back(QPointF(mEndBlock->x() + endRect.width() / 2, mEndBlock->y()));

    for(auto it = _bends.begin(); it.valid(); ++it)
        points.push_back(QPointF((*it).m_x, (*it).m_y));

    if(revLine)
        points.push_back(QPointF(mStartBlock->x() + startRect.width() / 2, mStartBlock->y()));
    else
        points.push_back(QPointF(mStartBlock->x() + startRect.width() / 2, mStartBlock->y() + startRect.height()));

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
    mLine = QPainterPath();
    if(points.size() == 2)
    {
        QPolygonF polyLine;
        polyLine << points.front() << points.back();
        mLine.addPolygon(polyLine);
    }
    else
    {
        mLine.moveTo(points.front());
        for(size_t i = 0; i + 2 < points.size(); ++i)
            mLine.cubicTo(points[i], points[i + 1], points[i + 2]);
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
    mHead = QPainterPath();
    mHead.addPolygon(head);
    mHead.setFillRule(Qt::WindingFill);
}
