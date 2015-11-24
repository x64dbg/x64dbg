#pragma once

#include "AnalysisPass.h"
#include "BasicBlock.h"

class LinearPass : public AnalysisPass
{
public:
    LinearPass(duint VirtualStart, duint VirtualEnd, BBlockArray & MainBlocks);
    virtual ~LinearPass();

    virtual const char* GetName() override;
    virtual bool Analyse() override;
    void AnalyseOverlaps();

private:
    void AnalysisWorker(duint Start, duint End, BBlockArray* Blocks);
    void AnalysisOverlapWorker(duint Start, duint End, BBlockArray* Insertions);
    BasicBlock* CreateBlockWorker(BBlockArray* Blocks, duint Start, duint End, bool Call, bool Jmp, bool Ret, bool Pad);
};