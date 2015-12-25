#include "ControlFlowGraph.h"
#include "Configuration.h"

void deleteGraphNodeVector(GRAPHNODEVECTOR* graphNodeVector)
{
    for(auto& ptr : *graphNodeVector)
        ptr->deleteLater();
}

ControlFlowGraph::ControlFlowGraph(QWidget *parent) : QWidget(parent),
    mParentsInfo(nullptr),
    mBasicBlockInfo(nullptr),
    mDisas(new QBeaEngine(-1)),
    mScene(new QGraphicsScene()),
    mGraphicsView(new QGraphicsView()),
    bProgramInitialized(false),
    mVLayout(new QVBoxLayout()),
    mGraphNodeVector(new GRAPHNODEVECTOR, deleteGraphNodeVector)
{

    mScene->setBackgroundBrush(ConfigColor("DisassemblyBackgroundColor"));
    mGraphicsView->setScene(mScene);

    mVLayout->addWidget(mGraphicsView);
    setLayout(mVLayout);


    connect(Bridge::getBridge(), SIGNAL(setControlFlowInfos(duint*)), this, SLOT(setControlFlowInfosSlot(duint*)));
    connect(Bridge::getBridge(), SIGNAL(disassembleAt(dsint,dsint)), this, SLOT(disassembleAtSlot(dsint, dsint)));

}


void ControlFlowGraph::startControlFlowAnalysis()
{
    DbgCmdExec(QString("cfanalyze").toUtf8().constData());
}

void ControlFlowGraph::setUnconditionalBranchEdgeColor()
{
    for(auto const &nodeGraphEdge : mNodeGraphEdge)
    {
        if(nodeGraphEdge.second.size() == 1)
            nodeGraphEdge.second.at(0)->setEdgeColor(Qt::blue);
    }
}

ControlFlowGraph::~ControlFlowGraph()
{
    mGraphNodeVector.reset();
    mGraphicsView->deleteLater();
    mScene->deleteLater();
    mVLayout->deleteLater();
    mTree.reset();
    mOHL.reset();
    mSL.reset();
    mGA.reset();
    delete mBasicBlockInfo;
    delete mParentsInfo;
    delete mDisas;
}

void ControlFlowGraph::setupGraph()
{
    using namespace ogdf;
    using namespace std;


    //initialize graph
    mGA = make_unique<GraphAttributes>(mG, GraphAttributes::nodeGraphics |
                                       GraphAttributes::edgeGraphics |
                                       GraphAttributes::nodeLabel |
                                       GraphAttributes::nodeStyle |
                                       GraphAttributes::edgeType |
                                       GraphAttributes::edgeArrow |
                                       GraphAttributes::edgeStyle);


    //add nodes
    mTree = make_unique<Tree<GraphNode*>>(&mG, mGA.get());

    setupTree();

    addGraphToScene();

    // Make sure there is some spacing
    QRectF sceneRect = mGraphicsView->sceneRect();
    sceneRect.adjust(-20, -20, 20, 20);
    mGraphicsView->setSceneRect(sceneRect);

    mGraphicsView->show();
}


void ControlFlowGraph::setupTree(duint va)
{
    using namespace ogdf;
    using namespace std;

    QTextStream out(stdout);
    BASICBLOCKMAP::iterator it;
    std::vector<Instruction_t> instructionsVector;

    // Clear graph and tree
    mG.clear();
    mTree->clear();
    mGraphNodeVector->clear();

    if(!va)
        it = mBasicBlockInfo->begin();
    else
        it = mBasicBlockInfo->find(va);

    // Couldn't find the basic block of the VA
    if(it == mBasicBlockInfo->end())
        return;


    // Disassemble the instruction at the address
    readBasicBlockInstructions(it, instructionsVector);

    // Add root node first
    duint addr = it->first;
    mGraphNodeVector->push_back( make_unique<GraphNode>(instructionsVector, addr) );
    connect(mGraphNodeVector->back().get(), SIGNAL(drawGraphAt(duint)), this, SLOT(drawGraphAtSlot(duint)), Qt::QueuedConnection);

    Node<GraphNode *> *rootNode = mTree->newNode(mGraphNodeVector->back().get());
    addAllNodes(it, rootNode);
}

void ControlFlowGraph::setupGraphLayout()
{
    using namespace ogdf;
    using namespace std;


    // TODO : Better way to do this
    if(mOHL.get() && mSL.get())
    {
        mSL->call(*mGA);
        return;
    }


    mOHL = make_unique<OptimalHierarchyLayout>();
    mOHL->nodeDistance(25.0);
    mOHL->layerDistance(50.0);
    mOHL->fixedLayerDistance(false);
    mOHL->weightBalancing(0.0);
    mOHL->weightSegments(0.0);

    mSL = make_unique<SugiyamaLayout>();
    mSL->setRanking(new OptimalRanking);
    mSL->setCrossMin(new MedianHeuristic);
    mSL->alignSiblings(false);
    mSL->setLayout(mOHL.get());
    mSL->call(*mGA);
}

void ControlFlowGraph::adjustNodesSize()
{
    // Adjust node size
    ogdf::node v;
    forall_nodes(v, mG)
    {
        Node<GraphNode* > *node = mTree->findNode(v);
        if (node)
        {
            QRectF rect = node->data()->boundingRect();
            mGA->width(v) = rect.width();
            mGA->height(v) = rect.height();
        }
    }
}

void ControlFlowGraph::addGraphToScene()
{
    using namespace ogdf;

    mNodeGraphEdge.clear();
    mScene->clear();

    adjustNodesSize();

    // Apply the graph layout after we've set the nodes sizes
    setupGraphLayout();

    addNodesToScene();

    addEdgesToScene();

    // Change unconditionalBranches colors to something different than for conditional branches
    setUnconditionalBranchEdgeColor();

    mGraphicsView->ensureVisible(mScene->itemsBoundingRect());

    // Make sure there is some spacing
    mScene->sceneRect().adjust(-20, -20, 20, 20);
    mGraphicsView->setSceneRect(mScene->sceneRect());
}

void ControlFlowGraph::addNodesToScene()
{
    ogdf::node v;
    forall_nodes(v, mG)
    {
        Node<GraphNode* > *node = mTree->findNode(v);
        if (node)
        {
            //draw node using x,y
            QRectF rect = node->data()->boundingRect();
            qreal x = mGA->x(v) - (rect.width()/2);
            qreal y = mGA->y(v) - (rect.height()/2);
            node->data()->setGeometry(x, y, rect.width(), rect.height());
            mScene->addWidget(node->data());
        }
    }
}

void ControlFlowGraph::addEdgesToScene()
{
    //draw edges
    ogdf::edge e;
    forall_edges(e, mG)
    {
        const auto bends = mGA->bends(e);
        const auto source = e->source();
        const auto target = e->target();

        GraphNode* sourceGraphNode = mTree->findNode(source)->data();
        GraphNode* targetGraphNode = mTree->findNode(target)->data();

        QRectF sourceRect = sourceGraphNode->geometry();
        QRectF targetRect = targetGraphNode->geometry();
        sourceRect.adjust(-4, -4, 4, 4);
        targetRect.adjust(-4, -4, 4, 4);

        QPointF start(mGA->x(source), mGA->y(source));
        QPointF end(mGA->x(target), mGA->y(target));

        GraphEdge* edge = nullptr;
        auto const sourceNodeLeft = mTree->findNode(source)->left();

        if(sourceNodeLeft && sourceNodeLeft->data()->address() == targetGraphNode->address())
            edge = new GraphEdge(start, end, bends, sourceRect, targetRect, GraphEdge::EDGE_LEFT);
        else
            edge = new GraphEdge(start, end, bends, sourceRect, targetRect, GraphEdge::EDGE_RIGHT);

        mNodeGraphEdge[source].push_back(std::unique_ptr<GraphEdge>(edge));
        mScene->addItem(edge);
    }
}

void ControlFlowGraph::addAllNodes(BASICBLOCKMAP::iterator it, Node<GraphNode *> *parentNode)
{
    using namespace std;

    QByteArray byteArray(MAX_STRING_SIZE, 0);
    duint left  = it->second.left; // Left child addr
    duint right = it->second.right; // Right child addr

    // No childs
    if(!left && !right)
        return;

    for(int i=0; i < 2; i++)
    {
        // If not left or right child continue..
        if((i == 0 && !left) || (i == 1 && !right))
            continue;

        BASICBLOCKMAP::iterator itChild;
        Node<GraphNode*>* node = nullptr;

        // Left
        if(i == 0)
        {
            itChild = mBasicBlockInfo->find(left);
            node = mTree->findNodeByAddress(left);
        }
        //Right
        else
        {
            itChild = mBasicBlockInfo->find(right);
            node = mTree->findNodeByAddress(right);
        }

        // If we found the basicblock for left or right
        if(itChild != mBasicBlockInfo->end())
        {
            duint childNodeAddr;

            if(i == 0) // Add node to left of parentNode
                childNodeAddr = left;
            else       // Add node to right of parentNode
                childNodeAddr = right;

            // Node does not exist, create it
            if(node == nullptr)
            {
                std::vector<Instruction_t> instructionsVector;
                readBasicBlockInstructions(itChild, instructionsVector);

                mGraphNodeVector->push_back( make_unique<GraphNode>(instructionsVector, childNodeAddr) );

                node = mTree->newNode(mGraphNodeVector->back().get());

                connect(mGraphNodeVector->back().get(), SIGNAL(drawGraphAt(duint)), this, SLOT(drawGraphAtSlot(duint)), Qt::QueuedConnection);
            }

            Node<GraphNode*>* newParentNode = nullptr;
            Node <GraphNode*>* parentNodeLeftRight  = nullptr;

            if(i == 0)
                parentNodeLeftRight = parentNode->left();
            else
                parentNodeLeftRight = parentNode->right();

            // Edge already exists between parentNode and left / right, we've been here before..
            if(parentNodeLeftRight && (parentNodeLeftRight->data()->address() == left || parentNodeLeftRight->data()->address() == right))
                return;

            if(i == 0)
                newParentNode = parentNode->setLeft(node);
            else
                newParentNode = parentNode->setRight(node);

            addAllNodes(itChild, newParentNode);
        }
    }


}

void ControlFlowGraph::setControlFlowInfosSlot(duint *controlFlowInfos)
{
    if(controlFlowInfos)
    {
        mParentsInfo = (PARENTMAP*)(((CONTROLFLOWINFOS*)controlFlowInfos)->parents);
        mBasicBlockInfo = (BASICBLOCKMAP*)(((CONTROLFLOWINFOS*)controlFlowInfos)->blocks);
        setupGraph();
    }
}

bool ControlFlowGraph::findBasicBlock(duint& va)
{
    if(mBasicBlockInfo == nullptr)
        return false;

    if(!mBasicBlockInfo->size())
        return false;

    bool bFound = false;
    BASICBLOCKMAP::iterator it = mBasicBlockInfo->find(va);

    // If the block was not found by addr, check if it belongs to a basic block
    if(it == mBasicBlockInfo->end())
    {
        for(it = mBasicBlockInfo->begin(); it != mBasicBlockInfo->end(); it++)
        {
            if(va >= it->second.start && va <= it->second.end)
            {
                va = it->first;
                bFound = true;
                break;
            }
        }
    }
    else
        bFound = true;

    return bFound;
}

void ControlFlowGraph::readBasicBlockInstructions(BASICBLOCKMAP::iterator it, std::vector<Instruction_t> &instructionsVector)
{
    duint addr = it->first;
    duint startAddr = it->second.start;
    duint endAddr = it->second.end;
    duint baseAddr = DbgMemFindBaseAddr(addr, 0);
    QByteArray byteArray(MAX_STRING_SIZE, 0);

    // Read basic block instructions
    for(startAddr; startAddr <= endAddr;)
    {
        if(!DbgMemRead(startAddr, reinterpret_cast<unsigned char*>(byteArray.data()), 16))
            return;

        // Add instruction to the vector
        Instruction_t wInstruction = mDisas->DisassembleAt(reinterpret_cast<byte_t*>(byteArray.data()), byteArray.length(), 0, baseAddr, startAddr-baseAddr);

        instructionsVector.push_back(wInstruction);
        startAddr += wInstruction.length;
    }
}

void ControlFlowGraph::drawGraphAtSlot(duint va)
{
    bool bFound = findBasicBlock(va);

    if(!bFound)
        return;

    setupTree(va);
    addGraphToScene();
}


