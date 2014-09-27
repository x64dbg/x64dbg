#include "GraphWidget.h"
#include "ui_GraphWidget.h"

#include "ogdf/basic/Graph.h"
#include "ogdf/basic/GraphAttributes.h"
#include "ogdf/layered/SugiyamaLayout.h"
#include "ogdf/layered/MedianHeuristic.h"
#include "ogdf/layered/OptimalRanking.h"
#include "ogdf/layered/FastHierarchyLayout.h"

GraphWidget::GraphWidget(QWidget* parent) : QWidget(parent)
{
    ui = new Ui::GraphWidget();
    graphicsView = new FlowGraphGraphicsView(this);
    graphicsView->setObjectName("graphicsView");
    graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    ui->setupUi(this);
}

GraphWidget::~GraphWidget()
{
    delete ui;
}
