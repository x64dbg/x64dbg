#include "../_global.h"
#include "../console.h"
#include "Meta.h"
#include "FlowGraph.h"
#include "AnalysisRunner.h"
#include "Block_t.h"



namespace fa
{



Block_t::Block_t(FlowGraph* Graph, Node_t* topNode, Node_t* bottomNode)
{
    top = topNode;
    bottom = bottomNode;

    std::map<duint, Instruction_t>::const_iterator itStart = Graph->information()->instructionIter(topNode->va);
    std::map<duint, Instruction_t>::const_iterator itEnd = Graph->information()->instructionIter(bottomNode->va);

    do
    {
        instructions.insert(&itStart->second);
        itStart++;
    }
    while(itStart != itEnd);

}

bool Block_t::operator==(const Block_t & rhs) const
{
    return (top->va == rhs.top->va);
}

bool Block_t::operator<(const Block_t & rhs) const
{
    return (top->va < rhs.top->va);
}

};