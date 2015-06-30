#pragma once

#include "AnalysisPass.h"
#include "BasicBlock.h"

class FunctionPass : public AnalysisPass
{
public:
    FunctionPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks);
    virtual ~FunctionPass();

    virtual const char* GetName() override;
    virtual bool Analyse() override;

private:
};