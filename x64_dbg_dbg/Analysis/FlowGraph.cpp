#include "FlowGraph.h"
#include "Node_t.h"
#include "Edge_t.h"
#include "../console.h"

namespace fa
{

FlowGraph::FlowGraph(AnalysisRunner* ana) : analysis(ana)
{

}
FlowGraph::~FlowGraph(void)
{

}



void FlowGraph::insertEdge(duint startAddress, duint endAddress, EdgeType btype)
{
    // each edge is directed from an instruction start: jmp end to "end"-address
    Node_t* workStart = new Node_t(startAddress);
    Node_t* workEnd = new Node_t(endAddress);

    // insert node or get the existing node
    std::pair<NodeMap::iterator, bool> sn = nodes.insert(std::pair<duint, Node_t*>(startAddress, workStart));
    std::pair<NodeMap::iterator, bool> en = nodes.insert(std::pair<duint, Node_t*>(endAddress, workEnd));

    if(!sn.second)
    {
        // if the node alreay exists -> use it
        delete workStart;
        workStart = (sn.first)->second;
    }
    if(!en.second)
    {
        // if the node alreay exists -> use it
        delete workEnd;
        workEnd = (en.first)->second;
    }
    // create new edge
    Edge_t* edge = new Edge_t(workStart, workEnd, btype);
    // insert or find edge
    std::pair<EdgeMap::iterator, bool> e = edges.insert(std::pair<duint, Edge_t*>(startAddress, edge));

    if(!e.second)
    {
        // delete tmp edge, if it already exists
        delete edge;
        edge = (e.first)->second;
    }
    // "edge" is existing or new edge --> let it know the nodes
    edge->start = workStart;
    edge->end = workEnd;
    // both nodes are the existing or new nodes --> let them know the edge
    workStart->outgoing.insert(edge);
    workEnd->incoming.insert(edge);
}




bool FlowGraph::find(const duint va , Node_t* node)
{
    if(contains(nodes, va))
    {
        NodeMap::iterator iter = nodes.find(va);
        node = (iter->second);
        return true;
    }
    return false;

}
Node_t* FlowGraph::node(const duint va)
{
    if(contains(nodes, va))
    {
        NodeMap::iterator iter = nodes.find(va);
        Node_t* node = (iter->second);
        return node;
    }
    return NULL;

}

void FlowGraph::fillNodes()
{
    for(NodeMap::iterator i = nodes.begin(); i != nodes.end(); i++)
    {
        i->second->hasInstr = true;
        i->second->instruction = analysis->instruction(i->first);
        fillbasicinfo(&(i->second->instruction->BeaStruct), &i->second->instruction->BasicInfo);
        i->second->instruction->BasicInfo.size = i->second->instruction->Length;
    }
}

const AnalysisRunner* FlowGraph::information() const
{
    return analysis;
}



}
