#ifndef BLOCKGRAPHICSITEM_H
#define BLOCKGRAPHICSITEM_H

#include <QObject>
#include <QPainter>
#include <QGraphicsItem>
#include <QGraphicsDropShadowEffect>

class BlockGraphicsItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    BlockGraphicsItem(QObject* parent, qreal width, qreal height, int id);

    QRectF boundingRect(void) const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

signals:
    void moved(void);

private:
    QObject*                   _parent;
    qreal                      _width, _height;
    bool                       _isPress;
    int                        _id;
    qreal                      _z;
    QGraphicsDropShadowEffect* _fx;
};

#endif // BLOCKGRAPHICSITEM_H
