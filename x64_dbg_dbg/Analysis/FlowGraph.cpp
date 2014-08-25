#include "FlowGraph.h"
#include "Node_t.h"
#include "Edge_t.h"
#include "../console.h"

namespace fa{
	
		FlowGraph::FlowGraph(void){

		}
		FlowGraph::~FlowGraph(void){

		}

		bool FlowGraph::insertNode( Node_t* node )
		{
			if (!contains(nodes,(UInt64)node->vaddr))
			{
				return (nodes.insert(std::make_pair<UInt64,Node_t*>(node->vaddr,node))).second;
			}
			return true;
			
		}

		bool FlowGraph::insertEdge( Edge_t* edge )
		{
			// until here a node just contains an address!
			// we search for these address to get most updated information (like incoming edges)

			dprintf("try to insert edge from "fhex" to "fhex" \n", edge->start->vaddr, edge->end->vaddr);
			return true;
			insertNode(edge->start);
			std::map<UInt64,Node_t*>::iterator it = nodes.find(edge->start->vaddr);
			it->second->outEdge = edge;

			insertNode(edge->end);
			std::map<UInt64,Node_t*>::iterator it2 = nodes.find(edge->end->vaddr);
			it2->second->inEdges.insert(edge);

			edge->start = (it->second);
			edge->end = (it2->second);
			bool ans = edges.insert(std::make_pair<UInt64,Edge_t*>(edge->start->vaddr,edge)).second;
			std::map<UInt64,Edge_t*>::iterator e = edges.find(edge->start->vaddr);
		
			return ans;


		}

		void FlowGraph::clean()
		{
			// first delete all edges whoses aks for it
			std::map<UInt64,Edge_t*>::iterator e = edges.begin();
			while(e != edges.end()) {
				std::map<UInt64,Edge_t*>::iterator current = e++;
				if((*current->second).askForRemove){
					delete current->second;
					edges.erase(current);
				}
			}


			// find and delete isolated nodes, i.e. nodes without incoming and outgoing edges
			std::map<UInt64,Node_t*>::iterator n = nodes.begin();
			while(n != nodes.end()) {
				std::map<UInt64,Node_t*>::iterator current = n++;
				if( (!(*current->second).outEdge)  && ((*current->second).inEdges.size()==0) ){
					delete current->second;
					nodes.erase(current);
				}
			}
		}
		bool FlowGraph::find(const UInt64 va , Node_t *node)
		{
			// try to find a node
			std::map<UInt64,Node_t*>::iterator it = nodes.find(va);
			Node_t* n = it->second;
			node = n;
			return (it != nodes.end());
		}



}
