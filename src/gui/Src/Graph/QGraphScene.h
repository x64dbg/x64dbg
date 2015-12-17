#ifndef QGRAPHSCENE_H
#define QGRAPHSCENE_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

class QGraphScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit QGraphScene(QWidget* parent = 0);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event);

signals:

public slots:

};

#endif // QGRAPHSCENE_H
