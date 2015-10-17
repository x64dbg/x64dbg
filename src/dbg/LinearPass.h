#pragma once

#include "AnalysisPass.h"
#include "BasicBlock.h"

class LinearPass : public AnalysisPass
{
public:
    LinearPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks);
    virtual ~LinearPass();

    virtual const char* GetName() override;
    virtual bool Analyse() override;
    void AnalyseOverlaps();

private:
    void AnalysisWorker(uint Start, uint End, BBlockArray* Blocks);
    void AnalysisOverlapWorker(uint Start, uint End, BBlockArray* Insertions);
    BasicBlock* CreateBlockWorker(BBlockArray* Blocks, uint Start, uint End, bool Call, bool Jmp, bool Ret, bool Pad);
};