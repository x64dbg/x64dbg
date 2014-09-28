#ifndef GRAPHDIALOG_H
#define GRAPHDIALOG_H

#include <QWidget>
#include <QDialog>
#include "FlowGraphScene.h"
#include "FlowGraphView.h"


class FlowGraphWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FlowGraphWidget(QWidget* parent = 0);
    ~FlowGraphWidget();
protected:
    void demoScene();
private:
    FlowGraphView* canvas;
    FlowGraphScene* scene;
    typedef std::vector<QGraphicsItem*> VectorItem;
    VectorItem items;
};

#endif // GRAPHDIALOG_H
