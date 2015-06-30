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

    virtual bool Analyse() override;

    virtual const char* GetName() override;

private:
    void AnalysisWorker(uint Start, uint End, std::vector<BasicBlock>* Blocks);
};