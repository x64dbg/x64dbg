#include "FlowGraphWidget.h"
#include "FlowGraphView.h"
#include "FlowGraphScene.h"
#include "FlowGraphBlock.h"
#include "FlowGraphEdge.h"

#include "ogdf/basic/Graph.h"
#include "ogdf/basic/GraphAttributes.h"
#include "ogdf/layered/SugiyamaLayout.h"
#include "ogdf/layered/MedianHeuristic.h"
#include "ogdf/layered/OptimalRanking.h"
#include "ogdf/layered/FastHierarchyLayout.h"

FlowGraphWidget::FlowGraphWidget(QWidget* parent) : QWidget(parent)
{

    QGridLayout* layout = new QGridLayout(this);

    canvas = new FlowGraphView(this);
    canvas->setObjectName("graphicsView");
    canvas->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    scene = new FlowGraphScene(this);
    canvas->setScene(scene);
    layout->addWidget(canvas, 0, 0);
    demoScene();
}

FlowGraphWidget::~FlowGraphWidget()
{

}

void FlowGraphWidget::demoScene()
{
    // we use OGDF as a backend library

    // ------------------------------ INIT GRAPH --------------------------------
    // this Graph contains our current graph
    ogdf::Graph G;
    // some information about drawing
    const unsigned int graphFlags = ogdf::GraphAttributes::nodeGraphics |
                                    ogdf::GraphAttributes::edgeGraphics ;

    ogdf::GraphAttributes flowgraphAttributes(G, graphFlags);

    // fill some random stuff
    const int numberOfNodes = 15;

    // reserve memory for our graph G=(V,E)
    std::vector<ogdf::node> V;
    V.reserve(numberOfNodes);
    std::vector<ogdf::edge> E;
    E.reserve(numberOfNodes * 2);

    for(int i = 0; i < numberOfNodes; ++i)
    {
        // create node
        ogdf::node newNode = G.newNode();
        // set random width
        // TODO: estimate values from instructions
        const unsigned int width = 100 + rand() % 500;
        const unsigned int height = 100 + rand() % 500;
        flowgraphAttributes.width()[newNode] = width;
        flowgraphAttributes.height()[newNode] = height;
        // insert node into set of nodes
        V.push_back(newNode);
    }
    for(int i = 0; i < numberOfNodes * 2; ++i)
    {
        const unsigned int startNodeId = rand() % numberOfNodes;
        const unsigned int endNodeId = rand() % numberOfNodes;
        // insert edge
        ogdf::edge edge = G.newEdge(V[startNodeId], V[endNodeId]);
        E.push_back(edge);
    }

    // ------------------------------ OPTIMIZE LAYOUT GRAPH --------------------------------
    // layout optimization = the magic for nice automatic layouts
    ogdf::FastHierarchyLayout* fastHierarchy = new ogdf::FastHierarchyLayout;
    fastHierarchy->nodeDistance(25.0);
    fastHierarchy->layerDistance(100.0);

    ogdf::SugiyamaLayout sugiyamaHeuristic;
    sugiyamaHeuristic.setRanking(new ogdf::OptimalRanking);
    sugiyamaHeuristic.setCrossMin(new ogdf::MedianHeuristic);
    sugiyamaHeuristic.alignSiblings(false);
    sugiyamaHeuristic.setLayout(fastHierarchy);
    flowgraphAttributes.writeGML("flowgraphextract.gml");
    sugiyamaHeuristic.call(flowgraphAttributes);

    // ------------------------------ FILL BLOCKS WITH INFORMATION --------------------------
    ogdf::node node;
    std::vector<QGraphicsItem*> items;
    int i = 0;
    // fill gray blocks
    forall_nodes(node, G)
    {
        ogdf::node cV = V[i];
        const unsigned int blockWidth = flowgraphAttributes.width()[cV];
        const unsigned int blockHeight = flowgraphAttributes.height()[cV];
        FlowGraphBlock* block = new FlowGraphBlock(scene, blockWidth, blockHeight , i);
        i++;
        items.push_back(block);
        block->moveBy(flowgraphAttributes.x(node), flowgraphAttributes.y(node));
        scene->addItem(block);
    }
    // fill edges
    forall_nodes(node, G)
    {
        ogdf::edge e;
        forall_adj_edges(e, node)
        {
            ogdf::DPolyline bends = flowgraphAttributes.bends(e);
            FlowGraphEdge* edge = new FlowGraphEdge(items[e->source()->index()], items[e->target()->index()], bends);
            scene->addItem(edge);
        }
    }

}


