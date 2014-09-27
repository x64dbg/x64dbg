#ifndef GRAPHDIALOG_H
#define GRAPHDIALOG_H

#include <QWidget>
#include "FlowGraphGraphicsScene.h"
#include "FlowGraphGraphicsView.h"

namespace Ui
{
class GraphWidget;
}

class GraphWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GraphWidget(QWidget* parent = 0);
    ~GraphWidget();

private:
    Ui::GraphWidget* ui;
    FlowGraphGraphicsScene* scene;
    typedef std::vector<QGraphicsItem*> VectorItem;
    VectorItem items;
};

#endif // GRAPHDIALOG_H
