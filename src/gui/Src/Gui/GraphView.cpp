
#include "GraphView.h"
#include "ui_GraphView.h"
#include "GraphEdge.h"
#include "Configuration.h"
#include <QDebug>


GraphView::GraphView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GraphView)
{
    ui->setupUi(this);
    mG = nullptr;
    mGA = nullptr;
    mSL = nullptr;
    mOHL = nullptr;
    mTree = nullptr;
    mDisas = new QBeaEngine(-1);
    mScene = new QGraphicsScene(this);
    mParentsInfo = nullptr;
    mBasicBlockInfo = nullptr;
    mGraphNodeVector = new GRAPHNODEVECTOR;
    bProgramInitialized = false;

    mScene->setBackgroundBrush(ConfigColor("DisassemblyBackgroundColor"));
//    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
//    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->graphicsView->setScene(mScene);

    connect(mScene, SIGNAL(changed(QList<QRectF>)), this, SLOT(sceneChangedSlot(QList<QRectF>)));
    connect(Bridge::getBridge(), SIGNAL(setControlFlowInfos(duint*)), this, SLOT(setControlFlowInfosSlot(duint*)));
    connect(Bridge::getBridge(), SIGNAL(disassembleAt(dsint,dsint)), this, SLOT(disassembleAtSlot(dsint, dsint)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChangedSlot(DBGSTATE)));
}

void GraphView::startControlFlowAnalysis()
{
    DbgCmdExec(QString("cfanalyze").toUtf8().constData());
}

void GraphView::showEvent(QShowEvent *event)
{
//    ui->graphicsView->fitInView(mScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void GraphView::setupGraph()
{
    using namespace ogdf;

    //initialize graph
    mG = new Graph;
    mGA = new GraphAttributes(*mG, GraphAttributes::nodeGraphics |
                              GraphAttributes::edgeGraphics |
                              GraphAttributes::nodeLabel |
                              GraphAttributes::nodeStyle |
                              GraphAttributes::edgeType |
                              GraphAttributes::edgeArrow |
                              GraphAttributes::edgeStyle);


    //add nodes
    mTree = new Tree<GraphNode*>(mG, mGA);

//    setupTree();
    setupTree();

    addGraphToScene();

    //make sure there is some spacing
    QRectF sceneRect = ui->graphicsView->sceneRect();
    sceneRect.adjust(-20, -20, 20, 20);
    ui->graphicsView->setSceneRect(sceneRect);

    ui->graphicsView->show();
}


void GraphView::setupTree(duint va)
{
    using namespace ogdf;

    QTextStream out(stdout);
    BASICBLOCKMAP::iterator it;
    QByteArray byteArray(MAX_STRING_SIZE, 0);
    std::vector<Instruction_t> instructionsVector;

    // Clear graph and tree
    mG->clear();
    mTree->clear();

    if(!va)
        it = mBasicBlockInfo->begin();
    else
        it = mBasicBlockInfo->find(va);

    // Couldn't find the basic block of the VA
    if(it == mBasicBlockInfo->end())
        return;

    // Disassemble the instruction at the address
    duint addr = it->first;
    duint startAddr = it->second.start;
    duint endAddr = it->second.end;
    duint baseAddr = DbgMemFindBaseAddr(addr, 0);

    for(startAddr; startAddr <= endAddr;)
    {
        if(!DbgMemRead(startAddr, (unsigned char*)byteArray.data(), 16))
            return;

        Instruction_t wInstruction = mDisas->DisassembleAt((byte_t*)byteArray.data(), byteArray.length(), 0, baseAddr, startAddr-baseAddr);

        // Add instruction to the vector
        instructionsVector.push_back(wInstruction);
        startAddr += wInstruction.length;
    }

    // Add root node first
    mGraphNodeVector->push_back(new GraphNode(instructionsVector, addr));
    connect(mGraphNodeVector->back(), SIGNAL(drawGraphAt(duint)), this, SLOT(drawGraphAtSlot(duint)), Qt::QueuedConnection);

    Node<GraphNode *> *rootNode = mTree->newNode(mGraphNodeVector->back());
    addAllNodes(it, rootNode);
}

void GraphView::setupGraphLayout()
{
    using namespace ogdf;

    // TODO : Better way to do this
    if(mOHL && mSL)
    {
        mSL->call(*mGA);
        return;
    }


    mOHL = new OptimalHierarchyLayout;
    mOHL->nodeDistance(25.0);
    mOHL->layerDistance(50.0);
    mOHL->fixedLayerDistance(false);
    mOHL->weightBalancing(0.0);
    mOHL->weightSegments(0.0);

    mSL = new SugiyamaLayout;
    mSL->setRanking(new OptimalRanking);
    mSL->setCrossMin(new MedianHeuristic);
    mSL->alignSiblings(false);
    mSL->setLayout(mOHL);
    mSL->call(*mGA);
}

void GraphView::addGraphToScene()
{
    using namespace ogdf;

    mScene->clear();

    // adjust node size
    node v;
    forall_nodes(v, *mG)
    {
        Node<GraphNode* > *node = mTree->findNode(v);
        if (node)
        {
            QRectF rect = node->data()->boundingRect();
            mGA->width(v) = rect.width();
            mGA->height(v) = rect.height();
        }
    }

    setupGraphLayout();

    //draw widget contents (nodes)
    forall_nodes(v, *mG)
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

    //draw edges
    edge e;
    forall_edges(e, *mG)
    {
        const auto bends = mGA->bends(e);
        const auto source = e->source();
        const auto target = e->target();

        GraphNode* sourceGraphNode = mTree->findNode(source)->data();
        GraphNode* targetGraphNode = mTree->findNode(target)->data();

        QRectF sourceRect = sourceGraphNode->geometry();
        sourceRect.adjust(-4, -4, 4, 4);
        QRectF targetRect = targetGraphNode->geometry();
        targetRect.adjust(-4, -4, 4, 4);

        QPointF start(mGA->x(source), mGA->y(source));
        QPointF end(mGA->x(target), mGA->y(target));

        GraphEdge* edge = nullptr;
        if(mTree->findNode(source)->left() && mTree->findNode(source)->left()->data()->address() == targetGraphNode->address())
            edge = new GraphEdge(start, end, bends, sourceRect, targetRect, Qt::green);
        else
            edge = new GraphEdge(start, end, bends, sourceRect, targetRect, Qt::red);

        mScene->addItem(edge);
    }

//    mScene->setSceneRect(mScene->itemsBoundingRect());

//    mScene->setSceneRect(0, 0, mGA->boundingBox().width(), mGA->boundingBox().height());
    QTextStream out(stdout);
    out << "*-----------------------------------*"<< endl;
    out << mScene->sceneRect().x() << endl;
    out << mScene->sceneRect().y() << endl;
    out << mScene->sceneRect().width() << endl;
    out << mScene->sceneRect().height() << endl;
    ui->graphicsView->ensureVisible(mScene->itemsBoundingRect());
    ui->graphicsView->setSceneRect(mScene->sceneRect());

//    ui->graphicsView->fitInView(mScene->sceneRect(), Qt::KeepAspectRatio);
}

void GraphView::addAllNodes(BASICBLOCKMAP::iterator it, Node<GraphNode *> *parentNode)
{
    QTextStream out(stdout);
    QByteArray byteArray(MAX_STRING_SIZE, 0);
    duint addr  = it->first; // Parent Basic Block addr
    duint left  = it->second.left; // Left child addr
    duint right = it->second.right; // Right child addr

    // No childs
    if(!left && !right)
        return;

    for(int i=0; i < 2; i++)
    {
        if((i == 0 && !left) || (i == 1 && !right))
            continue;

        BASICBLOCKMAP::iterator itChild;
        Node<GraphNode *> *node = nullptr;

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
            // If the Node doesn't exist yet;
            std::vector<Instruction_t> instructionsVector;
            if(node == nullptr)
            {
                duint startAddr = itChild->second.start;
                duint endAddr   = itChild->second.end;

                // BaseAddr..
                duint baseAddr = DbgMemFindBaseAddr(addr, 0);

                // Disassemble the BasicBlock instructions
                for(startAddr; startAddr <= endAddr;)
                {
                    DbgMemRead(startAddr, (unsigned char*)byteArray.data(), 16);
                    Instruction_t wInstruction = mDisas->DisassembleAt((byte_t*)byteArray.data(), MAX_STRING_SIZE,0, baseAddr, startAddr-baseAddr);

                    instructionsVector.push_back(wInstruction);
                    startAddr += wInstruction.length;
                }
            }

            // Add node to left of parentNode
            if(i == 0)
            {
                // Node does not exist, create it
                if(node == nullptr)
                {
                    mGraphNodeVector->push_back(new GraphNode(instructionsVector, left));
                    node = mTree->newNode(mGraphNodeVector->back());
                    connect(mGraphNodeVector->back(), SIGNAL(drawGraphAt(duint)), this, SLOT(drawGraphAtSlot(duint)), Qt::QueuedConnection);
                }

                Node<GraphNode *> *newParentNode = nullptr;

                // Edge already exists between parentNode and left / right, we've been here before..
                if(parentNode->left() && (parentNode->left()->data()->address() == left || parentNode->left()->data()->address() == right))
                    return;
                else
                    newParentNode = parentNode->setLeft(node);

                addAllNodes(itChild, newParentNode);
            }

            // Add node to right of parentNode
            else if(i == 1)
            {
                // Node does not exist, create it
                if(node == nullptr)
                {
                    mGraphNodeVector->push_back(new GraphNode(instructionsVector, right));
                    node = mTree->newNode(mGraphNodeVector->back());
                    connect((GraphNode*)(mGraphNodeVector->back()), SIGNAL(drawGraphAt(duint)), this, SLOT(drawGraphAtSlot(duint)), Qt::QueuedConnection);
                }

                Node<GraphNode *> *newParentNode = nullptr;

                // Edge already exists between parentNode and left / right
                if(parentNode->right() && (parentNode->right()->data()->address() == right || parentNode->right()->data()->address() == left))
                    return;
                else
                    newParentNode = parentNode->setRight(node);

                addAllNodes(itChild, newParentNode);
            }
        }
    }


}

GraphView::~GraphView()
{
    delete ui;
}

void GraphView::setControlFlowInfosSlot(duint *controlFlowInfos)
{
    if(controlFlowInfos)
    {
        mParentsInfo = (PARENTMAP*)(((CONTROLFLOWINFOS*)controlFlowInfos)->parents);
        mBasicBlockInfo = (BASICBLOCKMAP*)(((CONTROLFLOWINFOS*)controlFlowInfos)->blocks);
        setupGraph();
    }
}

void GraphView::dbgStateChangedSlot(DBGSTATE state)
{
    if(state == initialized)
        bProgramInitialized = false;
}

bool GraphView::findBasicBlock(duint &va)
{
    if(!mBasicBlockInfo)
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

void GraphView::drawGraphAtSlot(duint va)
{
    bool bFound = findBasicBlock(va);

    if(!bFound)
        return;

    setupTree(va);
    addGraphToScene();

//    if(mScene->itemsBoundingRect().height() < ui->graphicsView->height())
//        ui->graphicsView->verticalScrollBar()->hide();
//    else
//        ui->graphicsView->verticalScrollBar()->show();
//    ui->graphicsView->fitInView(mScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void GraphView::disassembleAtSlot(dsint parVA, dsint CIP)
{
    Q_UNUSED(CIP)
    Q_UNUSED(parVA)

    if(!bProgramInitialized)
    {
        startControlFlowAnalysis();
        bProgramInitialized = true;
    }
}
