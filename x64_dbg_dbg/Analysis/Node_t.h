#pragma once
#include "../_global.h"
#include "Meta.h"
#include <set>


namespace fa
{
class Edge_t;
class Node_t
{
public:
    Edge_t* outgoing;
    std::set<Edge_t*> incoming;

    const Instruction_t* instruction;

    duint va;
    bool hasInstr;

    Node_t(duint t);
    ~Node_t();

    bool operator==(const Node_t & rhs) const;
    bool operator<(const Node_t & rhs) const;

};

};