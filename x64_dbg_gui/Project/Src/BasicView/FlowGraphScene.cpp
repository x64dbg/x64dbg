#include "FlowGraphScene.h"

FlowGraphScene::FlowGraphScene(QObject* parent) : QGraphicsScene(parent)
{
    setBackgroundBrush(QBrush(ConfigColor("FlowGraphBackground")));
}
