#include "../_global.h"
#include "../console.h"
#include "Meta.h"
#include "Edge_t.h"
#include "Node_t.h"


namespace fa
{

Edge_t::Edge_t(Node_t* start, Node_t* end, EdgeType btype)
{
    // remember node
    start = &*start;
    end = &*end;
    type = btype;
    askForRemove = false;
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

bool Edge_t::shouldBeRemoved() const
{
    return askForRemove;
}

void Edge_t::remove()
{
    // this edge should be delete from memory
    askForRemove = true;
    // say start node goodbye
    start->outEdge = NULL;
    // say end node goodbye
    end->inEdges.erase(this);
    // if the node at the end would be isolated without this edge, we flag it for removing
    if(end)
    {
        if((end->inEdges.size() == 1) && (!start->outEdge))
        {
            end->remove();
            // do NOT delete the node "end" here, since the graph is NOT acyclic!
        }
    }
}

};