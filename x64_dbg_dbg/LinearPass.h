#pragma once

#include <vector>
#include <thread>

#include "AnalysisPass.h"
#include "BasicBlock.h"
#include "capstone_wrapper.h"

class LinearPass : public AnalysisPass
{
public:
    LinearPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks);
    virtual ~LinearPass();

    virtual bool Analyse() override;

    virtual const char* GetName() override;

private:
    uint m_MaximumThreads;

    void AnalysisWorker(uint Start, uint End, std::vector<BasicBlock>* Blocks);
    BasicBlock* CreateBlockWorker(std::vector<BasicBlock>* Blocks, uint Start, uint End, bool Call, bool Jmp, bool Ret, bool Intr);
};