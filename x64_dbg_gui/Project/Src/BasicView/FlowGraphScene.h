#ifndef FLOWGRAPHGRAPHICSSCENE_H
#define FLOWGRAPHGRAPHICSSCENE_H

#include <QtCore>
#include <QtGui>
#include <QGraphicsScene>
#include "Configuration.h"

class FlowGraphScene : public QGraphicsScene
{
    Q_OBJECT
public:
    FlowGraphScene(QObject* parent = 0);
private:
};

#endif // FLOWGRAPHGRAPHICSSCENE_H
