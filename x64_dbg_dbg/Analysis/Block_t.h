#pragma once
#include "../_global.h"
#include "Meta.h"
#include "Node_t.h"
#include "FlowGraph.h"
#include <set>

namespace fa
{
struct instrPtr_compare
{
    bool operator()(const Instruction_t* lhs, const Instruction_t* rhs) const
    {
        return lhs->BeaStruct.VirtualAddr < rhs->BeaStruct.VirtualAddr;
    }
};
typedef std::set<const Instruction_t*, instrPtr_compare> InstructionPtrSet;
// block containing all instructions between nearby nodes
class Block_t
{
public:
    Node_t* top;   // where is the entry into the block
    Node_t* bottom;     // where is the end of the block
    InstructionPtrSet instructions;


    Block_t(FlowGraph* Graph, Node_t* topNode, Node_t* bottomNode);

    bool operator==(const Block_t & rhs) const;
    bool operator<(const Block_t & rhs) const;


};

};