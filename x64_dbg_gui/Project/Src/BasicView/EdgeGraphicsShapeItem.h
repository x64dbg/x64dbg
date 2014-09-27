#ifndef EDGEGRAPHICSSHAPEITEM_H
#define EDGEGRAPHICSSHAPEITEM_H

#include <QAbstractGraphicsShapeItem>
#include <QPen>
#include <QPainter>
#include "ThirdPartyLibs/OGDF/basic/geometry.h"
class EdgeGraphicsShapeItem : public QAbstractGraphicsShapeItem
{
public:
    enum { Type = UserType + 1 };

    EdgeGraphicsShapeItem(QGraphicsItem* startItem, QGraphicsItem* endItem, ogdf::DPolyline const & bends);

    int type(void) const
    {
        return Type;
    }
    virtual QRectF boundingRect(void) const;
    virtual QPainterPath shape() const;
    void computeCoordinates(void);

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

private:
    QGraphicsItem* _startItem, *_endItem;
    ogdf::DPolyline _bends;
    QColor _clr;
    QPainterPath _line;
    QPainterPath _head;
};

#endif // EDGEGRAPHICSSHAPEITEM_H
