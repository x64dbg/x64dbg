#pragma once
#include <set>
#include "../_global.h"
#include "Node_t.h"
#include "Edge_t.h"
#include "AnalysisRunner.h"

namespace fa
{
// mapping from virtual address to edge and node. The key is the virtual adress
// of the startnode of an edge
typedef std::map<duint, Edge_t*> EdgeMap;
typedef std::map<duint, Node_t*> NodeMap;

/*
the idea is that a program flow can be represented as a cyclic directed graph.
Each instruction is a node an each possible branch in "JMP, JNE, ... , CALL, RET"
represents an edge.
Since we do not really need each instruction to be a node, we only store instructions
as a node, that cause a branch like "call, jmp" or that is the instruction at the target
address value of these branches.
*/


class FlowGraph
{

    // all existing edges
    EdgeMap edges;
    // all existing nodes
    NodeMap nodes;
    // pointer to main analysis class to outreach a pointer to information like api-data (parameters)
    AnalysisRunner* analysis;
public:
    FlowGraph(AnalysisRunner* ana);
    ~FlowGraph(void);
    void fillNodes();
    // insert a new node an returns the existing node if there was already the node
    // WARNING: this should only be called from the corresponding edge!!!
    bool insertNode(Node_t* node);
    // insert a new edge an returns the existing edge if there was already the edge
    void insertEdge(duint startAddress, duint endAddress, duint parentAddress, EdgeType btype);
    // try to find a node by virtual address and return "if the node exists"
    bool find(const duint va, Node_t* node);
    // direct access to node (WARNING: can be NULL)
    Node_t* FlowGraph::node(const duint va);
    // pointer to all information like api-call documentation
    const AnalysisRunner* information() const;
    // get the start address of the subroutine for displaying the graph
    bool getSubroutine(duint va, Node_t* n);
    Node_t* FlowGraph::nearestNodeBefore(duint va);
    void FlowGraph::correctGraph();
};

}
