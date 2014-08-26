#include "FlowGraph.h"
#include "Node_t.h"
#include "Edge_t.h"
#include "../console.h"

namespace fa
{

	FlowGraph::FlowGraph(AnalysisRunner * ana) : analysis(ana)
	{

	}
	FlowGraph::~FlowGraph(void)
	{

	}

	bool FlowGraph::insertNode(Node_t* node)
	{
//          if (!contains(nodes,(duint)node->vaddr))
//          {
//              return (nodes.insert(std::make_pair<duint,Node_t*>(node->vaddr,node))).second;
//          }
		return true;

	}
	bool FlowGraph::insertEdge(Node_t* startNode, Node_t* endNode, EdgeType currentEdgeType)
	{	
		Node_t *workStart;
		Node_t *workEnd;
		if(!contains(nodes,startNode->vaddr)){
			nodes.insert(std::pair<duint,Node_t*>(startNode->vaddr, startNode));
		}
		if(!contains(nodes,endNode->vaddr)){
			nodes.insert(std::pair<duint,Node_t*>(endNode->vaddr, endNode));
		}

		workStart = &*(nodes.find(startNode->vaddr)->second);
		workEnd = &*(nodes.find(endNode->vaddr)->second);


		if (startNode->hasInstr && !((nodes.find(startNode->vaddr)->second))->hasInstr){
			ttDebug("updating old startnode");
			workStart->instruction = startNode->instruction;
		}

		// now workStart and workEnd are most up-to-date information


		Edge_t* edge = new Edge_t(workStart, workEnd, currentEdgeType);

		edges.insert(std::pair<duint,Edge_t*>(workStart->vaddr,edge));
		std::map<duint,Edge_t*>::iterator ansiter = edges.find(workStart->vaddr);
		edge = &*(ansiter->second);
		edge->start = workStart;
		edge->end = workEnd;
		workStart->outEdge = edge;
		workEnd->inEdges.insert(edge);



		return true;
	}


	void FlowGraph::clean()
	{
		// first delete all edges whoses aks for it
		std::map<duint, Edge_t*>::iterator e = edges.begin();
		while(e != edges.end())
		{
			std::map<duint, Edge_t*>::iterator current = e++;
			if((*current->second).askForRemove)
			{
				delete current->second;
				edges.erase(current);
			}
		}


		// find and delete isolated nodes, i.e. nodes without incoming and outgoing edges
		std::map<duint, Node_t*>::iterator n = nodes.begin();
		while(n != nodes.end())
		{
			std::map<duint, Node_t*>::iterator current = n++;
			if((!(*current->second).outEdge)  && ((*current->second).inEdges.size() == 0))
			{
				delete current->second;
				nodes.erase(current);
			}
		}
	}
	bool FlowGraph::find(const duint va , Node_t* node)
	{
		if(contains(nodes, va))
		{
			std::map<duint, Node_t*>::iterator iter = nodes.find(va);
			node = (iter->second);

			tDebug("-> found node at "fhex" \n", node->vaddr);
			tDebug("Node_t address  is  "fhex" \n", node);

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
		for(std::map<duint, Node_t*>::iterator i=nodes.begin();i!=nodes.end();i++){
			i->second->hasInstr=true;
			i->second->instruction.BeaStruct = analysis->instruction_t(i->first).BeaStruct;
			if(i->second->instruction.BeaStruct.Instruction.Opcode == 0xFF)
				ttDebug("ext jump at %x",i->first);
		}
	}



}
