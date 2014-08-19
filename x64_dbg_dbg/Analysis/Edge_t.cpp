#include "../_global.h"
#include "../console.h"
#include "Meta.h"
#include "Edge_t.h"
#include "Node_t.h"


namespace fa
{

	Edge_t::Edge_t(Node_t* start, Node_t* end, EdgeType btype){
		// remember node
		start = start;
		end = end;
		type = btype;
	}

	Edge_t::~Edge_t(){
		// if the node at the end would be isolated without this edge, we delete it
		if( (end->in_edges.size() == 1) && (end->out_edges.size()==0)  )
			delete end;
	}

	// all edges are unique with respect to their virtual start address
	bool Edge_t::operator==(const Edge_t & rhs) const
	{
		return static_cast<bool>(start == rhs.start);
	}
	bool Edge_t::operator<(const Edge_t & rhs) const
	{
		return static_cast<bool>(start < rhs.start);
	}

};