#include "FlowGraph.h"
#include "Node_t.h"
#include "Edge_t.h"

namespace fa{
	
		FlowGraph::FlowGraph(void){

		}
		FlowGraph::~FlowGraph(void){

		}

		std::pair<std::set<Node_t*>::iterator,bool> FlowGraph::insertNode( Node_t* node )
		{
			return nodes.insert(node);
		}

		std::pair<std::set<Edge_t*>::iterator,bool> FlowGraph::insertEdge( Edge_t* edge )
		{
			// until here a node just contains an address!
			// we search for these address to get most updated information (like incoming edges)
			std::pair<std::set<Node_t*>::iterator,bool> start = insertNode(edge->start);
			std::pair<std::set<Node_t*>::iterator,bool> end = insertNode(edge->end);

			// insert current edge into these nodes
			(*start.first)->outEdge.insert(edge);
			(*end.first)->inEdges.insert(edge);

			// update edge
			edge->start = (*start.first);
			edge->end = (*end.first);

			return edges.insert(edge);


		}

		void FlowGraph::clean()
		{
			// first delete all edges whoses aks for it
			std::set<Edge_t*>::iterator e = edges.begin();
			while(e != edges.end()) {
				std::set<Edge_t*>::iterator current = e++;
				if((*current)->askForRemove){
					delete *current;
					edges.erase(current);
				}
			}


			// find and delete isolated nodes, i.e. nodes without incoming and outgoing edges
			std::set<Node_t*>::iterator n = nodes.begin();
			while(n != nodes.end()) {
				std::set<Node_t*>::iterator current = n++;
				if( (!(*current)->outEdge)  && ((*current)->inEdges.size()==0) ){
					delete *current;
					nodes.erase(current);
				}
			}
		}

		bool FlowGraph::find(const UInt64 va , Node_t *node)
		{
			// try to find a node
			std::set<Node_t*>::iterator it = nodes.find(Node_t(va));
			node = *it;
			return (it != nodes.end());
		}



}
