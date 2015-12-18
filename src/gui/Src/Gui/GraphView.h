#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <set>
#include <map>
#include <QWidget>
#include <QWheelEvent>
#include <QGraphicsScene>
#include "QBeaEngine.h"
#include "Bridge.h"
#include "Tree.h"
#include "GraphNode.h"
#include "capstone_gui.h"
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/layered/SugiyamaLayout.h>
#include <ogdf/layered/OptimalRanking.h>
#include <ogdf/layered/MedianHeuristic.h>
#include <ogdf/layered/OptimalHierarchyLayout.h>

namespace Ui
{
class GraphView;
}


struct BasicBlock
{
    duint start;
    duint end;
    duint left;
    duint right;
    duint function;

    BasicBlock()
    {
        this->start = 0;
        this->end = 0;
        this->left = 0;
        this->right = 0;
        this->function = 0;
    }

    BasicBlock(duint start, duint end, duint left, duint right)
    {
        this->start = start;
        this->end = end;
        this->left = std::min(left, right);
        this->right = std::max(left, right);
        this->function = 0;
    }

    std::string toString()
    {
        char buffer[MAX_STRING_SIZE];
        sprintf_s(buffer, "start:%p,end:%p,left:%p,right:%p,func:%p", start, end, left, right, function);
        return std::string(buffer);
    }
};

typedef std::map<duint, BasicBlock> BASICBLOCKMAP; //start of block -> block
typedef std::map<duint, std::set<duint> > PARENTMAP; //start child -> parents
typedef std::vector<GraphNode* > GRAPHNODEVECTOR;
typedef std::map<duint, Node<GraphNode *> * > NODEMAP;

class GraphView : public QWidget
{
    Q_OBJECT

public:
    explicit GraphView(QWidget *parent = 0);
    void startControlFlowAnalysis();
    void showEvent(QShowEvent *event);
    ~GraphView();

public slots:
    void drawGraphAtSlot(duint va);
    void disassembleAtSlot(dsint parVA, dsint CIP);
    void setControlFlowInfosSlot(duint *controlFlowInfos);
    void dbgStateChangedSlot(DBGSTATE state);
    void sceneChangedSlot(QList<QRectF> rectList);

private:
    void setupGraph();
    void setupGraphLayout();
    void setupTree(duint va = 0);
    void addGraphToScene();
    void addAllNodes(BASICBLOCKMAP::iterator it, Node<GraphNode *> *parentNode);
    bool findBasicBlock(duint &va);
    QBeaEngine *mDisas;
    PARENTMAP *mParentsInfo;
    BASICBLOCKMAP *mBasicBlockInfo;
    GRAPHNODEVECTOR *mGraphNodeVector;
    ogdf::Graph *mG;
    ogdf::GraphAttributes *mGA;
    ogdf::SugiyamaLayout *mSL;
    ogdf::OptimalHierarchyLayout *mOHL;
    QGraphicsScene *mScene;
    Tree<GraphNode*> *mTree;
    Ui::GraphView *ui;
    bool bProgramInitialized;
    int mIndex = 0;

};

#endif // GRAPHVIEW_H
