#pragma once
#include "../_global.h"
#include "Meta.h"
#include <set>


namespace fa
{
class Edge_t;

typedef std::set<Edge_t*> EdgeSet;

class Node_t
{
public:
    EdgeSet outgoing;
    EdgeSet incoming;

    const Instruction_t* instruction;
    duint parent_va;

    duint va;
    bool hasInstr;

    Node_t(duint t);
    ~Node_t();

    bool operator==(const Node_t & rhs) const;
    bool operator<(const Node_t & rhs) const;

    Edge_t* firstEdge();

};

};