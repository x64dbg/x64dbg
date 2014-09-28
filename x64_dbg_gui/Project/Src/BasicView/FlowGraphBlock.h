#ifndef BLOCKGRAPHICSITEM_H
#define BLOCKGRAPHICSITEM_H

#include <QObject>
#include <QPainter>
#include <QGraphicsItem>
#include <QGraphicsDropShadowEffect>

// this class should represent a basic block containing connected disassembled instruction
// = the blocks you can see in IDA
class FlowGraphBlock : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    FlowGraphBlock(QObject* parent, unsigned int width, unsigned int height, int id);

    QRectF boundingRect(void) const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

    const unsigned int with() const;
    const unsigned int height() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

signals:
    void moved(void);

private:
    QObject*     mParent;
    unsigned int mWidth;
    unsigned int mHeight;
    bool mMouseDown;
    int internalId;
    unsigned int mZValue;
    QGraphicsDropShadowEffect* _fx;
};

#endif // BLOCKGRAPHICSITEM_H
