#pragma once

#include <vector>
#include <thread>

#include "AnalysisPass.h"
#include "BasicBlock.h"

class Int3CoagulatorPass : public AnalysisPass
{
public:
    Int3CoagulatorPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks);
    virtual ~Int3CoagulatorPass();

    virtual const char* GetName() override;
    virtual bool Analyse() override;

private:
    void AnalysisWorker(uint Start, uint End, std::vector<BasicBlock>* Blocks);
};