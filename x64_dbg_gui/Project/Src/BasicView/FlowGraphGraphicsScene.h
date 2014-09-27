#ifndef FLOWGRAPHGRAPHICSSCENE_H
#define FLOWGRAPHGRAPHICSSCENE_H

#include <QtCore>
#include <QtGui>
#include <QGraphicsScene>

class FlowGraphGraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit FlowGraphGraphicsScene(QObject* parent = 0) : QGraphicsScene(parent) {}
private:
};

#endif // FLOWGRAPHGRAPHICSSCENE_H
