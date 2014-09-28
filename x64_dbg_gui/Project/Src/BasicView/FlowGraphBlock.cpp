#include "FlowGraphBlock.h"
#include "Configuration.h"
#include <QObject>
#include <sstream>

FlowGraphBlock::FlowGraphBlock(QObject* parent, unsigned int width, unsigned int height, int id)
    : mParent(parent), mWidth(width), mHeight(height), mMouseDown(false), internalId(id), mZValue(zValue()), _fx(new QGraphicsDropShadowEffect(this))
{
    setFlags(ItemIsMovable | ItemIsSelectable);
    _fx->setBlurRadius(25.0);
    setGraphicsEffect(_fx);
}

QRectF FlowGraphBlock::boundingRect(void) const
{
    return QRectF(0, 0, mWidth, mHeight);
}

void FlowGraphBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget /*= 0*/)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);

    QRectF boundingRectangle = boundingRect();
    QColor backgroundColor = mMouseDown ? ConfigColor("FlowGraphBlockBackgroundHover") : ConfigColor("FlowGraphBlockBackground");

    painter->fillRect(boundingRectangle, QBrush(backgroundColor));
    _fx->setColor(backgroundColor);

    std::ostringstream oss;
    oss << "block id:" << internalId;

    painter->drawText(10, 10, QString::fromStdString(oss.str()));
    painter->drawRect(boundingRectangle);
}

const unsigned int FlowGraphBlock::with() const
{
    return mWidth;
}

const unsigned int FlowGraphBlock::height() const
{
    return mHeight;
}

void FlowGraphBlock::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    mMouseDown = true;
    setZValue(1.0);
    update();
    QGraphicsItem::mousePressEvent(event);
}

void FlowGraphBlock::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    mMouseDown = false;
    setZValue(mZValue);
    update();
}
