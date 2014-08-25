#include "../_global.h"
#include "../console.h"
#include "Meta.h"
#include "Edge_t.h"
#include "Node_t.h"

namespace fa
{
	Node_t::Node_t(Instruction_t t){
		outEdge = NULL;
		instruction = t;
		vaddr = (duint)t.BeaStruct.VirtualAddr;
	}
	Node_t::Node_t(){
		outEdge = NULL;
		instruction = Instruction_t();
		vaddr = 0;
	}
	Node_t::Node_t(duint va){
		outEdge = NULL;
		instruction = Instruction_t();
		vaddr = va;
	}
	Node_t::~Node_t(){
		
	}
	void Node_t::remove(){
		outEdge->remove();
		outEdge = NULL;

		// destroying a node will clear all incoming edges
		for each(Edge_t* e in inEdges){
			e->remove();
		} 
		inEdges.clear();
	}

	// nodes are unique with respect to their virtual address
	bool Node_t::operator==(const Node_t & rhs) const
	{
		return static_cast<bool>(vaddr == rhs.vaddr);
		
	}
	bool Node_t::operator<(const Node_t & rhs) const
	{

			return static_cast<bool>(vaddr < rhs.vaddr);

		
	}

};