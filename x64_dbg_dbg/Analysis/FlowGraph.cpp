#include "FlowGraph.h"
#include "Node_t.h"
#include "Edge_t.h"
#include "../console.h"
#include <set>

namespace fa
{

FlowGraph::FlowGraph(AnalysisRunner* ana) : analysis(ana)
{

}
FlowGraph::~FlowGraph(void)
{

}



void FlowGraph::insertEdge(duint startAddress, duint endAddress, duint parentAddress, EdgeType btype)
{
    // each edge is directed from an instruction start: jmp end to "end"-address
    // since these nodes are potentially new (we provide the info about their parent address)
    Node_t* workStart = new Node_t(startAddress);
    workStart->parent_va = parentAddress;
    Node_t* workEnd = new Node_t(endAddress);
    workEnd->parent_va = parentAddress;

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
    // force the starting node to save their parent address
    workStart->parent_va = parentAddress;
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

void FlowGraph::correctGraph()
{
    // we just have to add an edge from each "uncond. jmp" to its following instruction
    /*
    0x122 cmp eax, 0
    0x123 jne  foo
    0x124
    edge from "0x123 --> foo" allready exists, we now insert the
    edge from "0x123 --> 0x124"
    */
    for(NodeMap::iterator iter = nodes.begin(); iter != nodes.end(); iter++)
    {
        if(iter->second->firstEdge()->type == CONDJMP)
        {
            // this can be written in less chars, but somebody has to understand it
            InstructionMap::const_iterator currentInstr = analysis->instructionIter(iter->second->va);
            const duint startAddr = currentInstr->second.BeaStruct.VirtualAddr;
            currentInstr++;
            const duint endAddr = currentInstr->second.BeaStruct.VirtualAddr;
            insertEdge(startAddr, endAddr, iter->second->parent_va, CONDJMP);
        }
    }


}



bool FlowGraph::find(const duint va , Node_t* node)
{
    // search for a node at given virtual address (nodes are sparse!)
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
    // same as find, but directly returns node (however this works better)
    if(contains(nodes, va))
    {
        NodeMap::iterator iter = nodes.find(va);
        Node_t* node = (iter->second);
        return node;
    }
    return NULL;
}

Node_t* FlowGraph::nearestNodeBefore(duint va)
{
    // since there is no node foreach address
    // we can at least provide a node that directly comes before this address
    // if the address has a node --> this ndoe will be returned
    NodeMap::iterator iter = nodes.lower_bound(va);
    Node_t* node = (iter->second);
    return node;
}

void FlowGraph::fillNodes()
{
    // use Mr.Exodia interpretation of the BeaStruct
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
    // just forward the analysis class
    return analysis;
}

bool FlowGraph::getSubroutine(duint va, Node_t* n)
{
    // foreach address we can provide the information about the subroutine that contains
    // the node
    n = nearestNodeBefore(va);
    //n->
    // is node == OEP --> return OEP
    if(n->va == analysis->oep())
        return true;
    // go backwards until oep or a call
    //if (n->va == analysis->oep())

}



}
