#include "GraphView.h"
#include "ui_GraphView.h"

#include "Tree.h"
#include "GraphEdge.h"
#include "GraphNode.h"
#include "QGraphScene.h"

#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/layered/SugiyamaLayout.h>
#include <ogdf/layered/OptimalRanking.h>
#include <ogdf/layered/MedianHeuristic.h>
#include <ogdf/layered/OptimalHierarchyLayout.h>

#include "Bridge.h"

GraphView::GraphView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GraphView)
{
    ui->setupUi(this);
    setupGraph();
}

void GraphView::setupGraph()
{
    using namespace ogdf;

    //initialize graph
    Graph G;
    GraphAttributes GA(G, GraphAttributes::nodeGraphics |
                       GraphAttributes::edgeGraphics |
                       GraphAttributes::nodeLabel |
                       GraphAttributes::nodeStyle |
                       GraphAttributes::edgeType |
                       GraphAttributes::edgeArrow |
                       GraphAttributes::edgeStyle);

    //add nodes
    Tree<GraphNode*> tree(&G, &GA);
    auto root = tree.newNode(new GraphNode("rp"));
    auto left = root->setLeft(tree.newNode(new GraphNode("left 1")));
    left->setLeft(tree.newNode(new GraphNode("left 2")));
    left->setRight(tree.newNode(new GraphNode("right 2")));
    auto right = root->setRight(tree.newNode(new GraphNode("right 1")));
    right->setLeft(tree.newNode(new GraphNode("left 3")))->setRight(left);
    right->setRight(tree.newNode(new GraphNode("right 3")))->setRight(tree.newNode(new GraphNode("nice long text :)")))->setLeft(root);

    //adjust node size
    node v;
    forall_nodes(v, G)
    {
        auto node = tree.findNode(v);
        if (node)
        {
            auto rect = node->data()->boundingRect();
            GA.width(v) = rect.width();
            GA.height(v) = rect.height();
        }
    }

    //do layout
    OptimalHierarchyLayout* OHL = new OptimalHierarchyLayout;
    OHL->nodeDistance(25.0);
    OHL->layerDistance(50.0);
    OHL->weightBalancing(0.0);
    OHL->weightSegments(0.0);

    SugiyamaLayout SL;
    SL.setRanking(new OptimalRanking);
    SL.setCrossMin(new MedianHeuristic);
    SL.alignSiblings(false);
    SL.setLayout(OHL);
    SL.call(GA);

    QGraphScene* scene = new QGraphScene(this);

    //draw widget contents (nodes)
    forall_nodes(v, G)
    {
        auto node = tree.findNode(v);
        if (node)
        {
            //draw node using x,y
            auto rect = node->data()->boundingRect();
            qreal x = GA.x(v) - (rect.width()/2);
            qreal y = GA.y(v) - (rect.height()/2);
            node->data()->setGeometry(x, y, rect.width(), rect.height());
            scene->addWidget(node->data());
        }
    }

    //draw edges
    edge e;
    forall_edges(e, G)
    {
        const auto bends = GA.bends(e);
        const auto source = e->source();
        const auto target = e->target();

        GraphNode* sourceGraphNode = tree.findNode(source)->data();
        GraphNode* targetGraphNode = tree.findNode(target)->data();
        qDebug() << "edge" << sourceGraphNode->label() << "->" << targetGraphNode->label();

        QRectF sourceRect = sourceGraphNode->geometry();
        sourceRect.adjust(-4, -4, 4, 4);
        QRectF targetRect = targetGraphNode->geometry();
        targetRect.adjust(-4, -4, 4, 4);

        QPointF start(GA.x(source), GA.y(source));
        QPointF end(GA.x(target), GA.y(target));
        GraphEdge* edge = new GraphEdge(start, end, bends, sourceRect, targetRect);

        scene->addItem(edge);
    }

    //draw scene
    scene->setBackgroundBrush(QBrush(Qt::darkGray));

    ui->graphicsView->setScene(scene);

    //make sure there is some spacing
    QRectF sceneRect = ui->graphicsView->sceneRect();
    sceneRect.adjust(-20, -20, 20, 20);
    ui->graphicsView->setSceneRect(sceneRect);

    ui->graphicsView->show();



    //qDebug() << "sceneRect()" << ui->graphicsView->sceneRect();

    GraphIO::drawSVG(GA, "test.svg");
}

GraphView::~GraphView()
{
    delete ui;
}
