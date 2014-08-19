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
			(*start.first)->out_edges.insert(edge);
			(*end.first)->in_edges.insert(edge);

			// update edge
			edge->start = (*start.first);
			edge->end = (*end.first);

			return edges.insert(edge);


		}



}
