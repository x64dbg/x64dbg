#pragma once
#include <set>
#include "../_global.h"


namespace fa{

	class Edge_t;
	class Node_t;
	class FlowGraph
	{
		// this class represents the program flow including branches likes JMP, JNE, ... , CALL, RET
		// all existing edges
		std::set<Edge_t*> edges;
		// all existing nodes
		std::set<Node_t*> nodes;
	public:
		FlowGraph(void);
		~FlowGraph(void);
		void clean();
		// insert a new node an returns the existing node if there was already the node
		// WARNING: this should only be called from the corresponding edge!!!
		std::pair<std::set<Node_t*>::iterator,bool> insertNode(Node_t* node);
		// insert a new edge an returns the existing edge if there was already the edge
		std::pair<std::set<Edge_t*>::iterator,bool> FlowGraph::insertEdge( Edge_t* edge );

		bool find(const UInt64 va , Node_t *node);
	};

}
