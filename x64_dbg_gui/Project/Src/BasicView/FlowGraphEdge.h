#ifndef EDGEGRAPHICSSHAPEITEM_H
#define EDGEGRAPHICSSHAPEITEM_H

#include <QAbstractGraphicsShapeItem>
#include <QPen>
#include <QPainter>
#include "ThirdPartyLibs/OGDF/basic/geometry.h"
class FlowGraphEdge : public QAbstractGraphicsShapeItem
{
public:
    enum { Type = UserType + 1 };

    FlowGraphEdge(QGraphicsItem* startItem, QGraphicsItem* endItem, ogdf::DPolyline const & bends);

    int type(void) const;
    virtual QRectF boundingRect(void) const;
    virtual QPainterPath shape() const;
    void computeCoordinates(void);

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

private:
    QGraphicsItem* mStartBlock, *mEndBlock;
    ogdf::DPolyline _bends;
    QColor mLineColor;
    QPainterPath mLine;
    QPainterPath mHead;
};

#endif // EDGEGRAPHICSSHAPEITEM_H
