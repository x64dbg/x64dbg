#include "../_global.h"
#include "../console.h"
#include "Meta.h"
#include "Edge_t.h"
#include "Node_t.h"

namespace fa
{
	Node_t::Node_t(UINT64 addr){
		virtualAddress = addr;
	}
	Node_t::~Node_t(){
		// destroying a node will clear all outgoing edges
		for each(Edge_t* e in out_edges){
			delete e;
		} 
		out_edges.clear();
		// destroying a node will clear all incoming edges
		for each(Edge_t* e in in_edges){
			delete e;
		} 
		in_edges.clear();
	}
	// nodes are unique with respect to their virtual address
	bool Node_t::operator==(const Node_t & rhs) const
	{
		return static_cast<bool>(virtualAddress == rhs.virtualAddress);
	}
	bool Node_t::operator<(const Node_t & rhs) const
	{
		return static_cast<bool>(virtualAddress < rhs.virtualAddress);
	}

};