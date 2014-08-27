#include "../_global.h"
#include "../console.h"
#include "Meta.h"
#include "Edge_t.h"
#include "Node_t.h"


namespace fa
{

Edge_t::Edge_t(Node_t* startNode, Node_t* endNode, EdgeType btype)
{
    // remember node
    start = startNode;
    end = endNode;
    type = btype;
}

Edge_t::~Edge_t()
{

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