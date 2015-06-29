#include "Int3CoagulatorPass.h"

Int3CoagulatorPass::Int3CoagulatorPass(uint VirtualStart, uint VirtualEnd)
    : AnalysisPass(VirtualStart, VirtualEnd)
{
}

Int3CoagulatorPass::~Int3CoagulatorPass()
{
}

bool Int3CoagulatorPass::Analyse()
{
    return false;
}