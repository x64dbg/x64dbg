#pragma once
#include <set>
#include "../_global.h"
#include "Node_t.h"
#include "Edge_t.h"
#include "AnalysisRunner.h"

namespace fa
{



class FlowGraph
{
    // this class represents the program flow including branches likes JMP, JNE, ... , CALL, RET
    // all existing edges
    std::map<duint, Edge_t*> edges;
    // all existing nodes
    std::map<duint, Node_t*> nodes;

    AnalysisRunner* analysis;
public:
    FlowGraph(AnalysisRunner* ana);
    ~FlowGraph(void);
    void fillNodes();
    // insert a new node an returns the existing node if there was already the node
    // WARNING: this should only be called from the corresponding edge!!!
    bool insertNode(Node_t* node);
    // insert a new edge an returns the existing edge if there was already the edge
    void insertEdge(duint startAddress, duint endAddress, EdgeType btype);
    bool find(const duint va, Node_t* node);
    Node_t* FlowGraph::node(const duint va);


};

}
