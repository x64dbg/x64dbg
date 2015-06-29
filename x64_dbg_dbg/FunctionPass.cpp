#include "FunctionPass.h"

FunctionPass::FunctionPass(uint VirtualStart, uint VirtualEnd)
    : AnalysisPass(VirtualStart, VirtualEnd)
{
}

FunctionPass::~FunctionPass()
{
}

bool FunctionPass::Analyse()
{
    return false;
}