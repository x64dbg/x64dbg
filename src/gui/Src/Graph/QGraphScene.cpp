#include "QGraphScene.h"

QGraphScene::QGraphScene(QWidget* parent) : QGraphicsScene(parent)
{
}

void QGraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QWidget* parent = (QWidget*)this->parent();
    parent->setWindowTitle(QString().sprintf("x:%f, y:%f", event->scenePos().x(), event->scenePos().y()));
    QGraphicsScene::mouseMoveEvent(event);
}
