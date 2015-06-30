#include "FunctionPass.h"

FunctionPass::FunctionPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks)
    : AnalysisPass(VirtualStart, VirtualEnd, MainBlocks)
{
}

FunctionPass::~FunctionPass()
{
}

const char* FunctionPass::GetName()
{
    return "Function Analysis";
}

bool FunctionPass::Analyse()
{
    return false;
}