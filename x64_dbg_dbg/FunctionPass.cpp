#include "FunctionPass.h"

FunctionPass::FunctionPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks)
    : AnalysisPass(VirtualStart, VirtualEnd, MainBlocks)
{
}

FunctionPass::~FunctionPass()
{
}

bool FunctionPass::Analyse()
{
    return false;
}