#pragma once

#include "AnalysisPass.h"
#include "BasicBlock.h"

class FunctionPass : public AnalysisPass
{
public:
    FunctionPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks);
    virtual ~FunctionPass();

    virtual bool Analyse() override;

private:
};