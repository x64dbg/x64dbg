#include "BlockGraphicsItem.h"
#include <QObject>
#include <sstream>

BlockGraphicsItem::BlockGraphicsItem(QObject* parent, qreal width, qreal height, int id)
    : _parent(parent), _width(width), _height(height), _isPress(false), _id(id), _z(zValue()), _fx(new QGraphicsDropShadowEffect(this))
{
    setFlags(ItemIsMovable | ItemIsSelectable);
    _fx->setBlurRadius(25.0);
    setGraphicsEffect(_fx);
}

QRectF BlockGraphicsItem::boundingRect(void) const
{
    return QRectF(0, 0, _width, _height);
}

void BlockGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget /*= 0*/)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);
    QRectF rect = boundingRect();
    QColor clr = Qt::lightGray;
    qreal opacity = 1.0;

    if(_isPress)
    {
        clr = Qt::blue;
        opacity = 0.7;
    }

    QBrush brush(clr);

    setOpacity(opacity);
    painter->fillRect(rect, brush);
    _fx->setColor(clr);
    std::ostringstream oss;
    oss << "bb_" << _id;
    painter->drawText(10, 10, QString::fromStdString(oss.str()));
    painter->drawRect(rect);
}

void BlockGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    _isPress = true;
    setZValue(1.0);
    update();
    QGraphicsItem::mousePressEvent(event);
}

void BlockGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    _isPress = false;
    setZValue(_z);
    update();
}
