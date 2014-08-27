#include "../_global.h"
#include "../console.h"
#include "Meta.h"
#include "Edge_t.h"
#include "Node_t.h"

namespace fa
{

	Node_t::Node_t(duint address)
	{
		outgoing = NULL;
		va = address;
		hasInstr = false;
	}

	Node_t::~Node_t()
	{

	}

	// nodes are unique with respect to their virtual address
	bool Node_t::operator==(const Node_t & rhs) const
	{
		return static_cast<bool>(va == rhs.va);
	}
	bool Node_t::operator<(const Node_t & rhs) const
	{
		return static_cast<bool>(va < rhs.va);
	}

};