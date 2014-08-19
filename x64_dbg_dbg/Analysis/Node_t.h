#pragma once
#include "../_global.h"
#include "Meta.h"
#include <set>

namespace fa
{
	class Edge_t;
	class Node_t{
	public:
		std::set<Edge_t*> out_edges;  //  all outgoing edges
		std::set<Edge_t*> in_edges;   // all incoming edges
		UInt64 virtualAddress;  //  a node is an instruction

		Node_t(UINT64 addr);
		~Node_t();
		bool operator==(const Node_t & rhs) const;
		bool operator<(const Node_t & rhs) const;

	};

};