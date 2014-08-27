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
    Node_t* workStart = new Node_t(startAddress);
    Node_t* workEnd = new Node_t(endAddress);


    std::pair<std::map<duint, Node_t*>::iterator, bool> sn = nodes.insert(std::pair<duint, Node_t*>(startAddress, workStart));
    std::pair<std::map<duint, Node_t*>::iterator, bool> en = nodes.insert(std::pair<duint, Node_t*>(endAddress, workEnd));

    if(!sn.second)
    {
        delete workStart;
        workStart = (sn.first)->second;
    }
    if(!en.second)
    {
        delete workEnd;
        workEnd = (en.first)->second;
    }

    Edge_t* edge = new Edge_t(workStart, workEnd, btype);

    std::pair<std::map<duint, Edge_t*>::iterator, bool> e = edges.insert(std::pair<duint, Edge_t*>(startAddress, edge));

    if(!e.second)
    {
        delete edge;
        edge = (e.first)->second;
    }

    edge->start = workStart;
    edge->end = workEnd;
    workStart->outgoing = edge;
    workEnd->incoming.insert(edge);
}




bool FlowGraph::find(const duint va , Node_t* node)
{
    if(contains(nodes, va))
    {
        std::map<duint, Node_t*>::iterator iter = nodes.find(va);
        node = (iter->second);
        return true;
    }
    return false;

}
Node_t* FlowGraph::node(const duint va)
{
    if(contains(nodes, va))
    {
        std::map<duint, Node_t*>::iterator iter = nodes.find(va);
        Node_t* node = (iter->second);
        return node;
    }
    return NULL;

}

void FlowGraph::fillNodes()
{
    for(std::map<duint, Node_t*>::iterator i = nodes.begin(); i != nodes.end(); i++)
    {
        i->second->hasInstr = true;
        i->second->instruction = analysis->instruction(i->first);
        fillbasicinfo(&(i->second->instruction->BeaStruct), &i->second->instruction->BasicInfo);
        i->second->instruction->BasicInfo.size = i->second->instruction->Length;
    }
}



}
