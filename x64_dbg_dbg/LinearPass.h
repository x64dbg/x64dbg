#pragma once

#include <vector>
#include <thread>

#include "AnalysisPass.h"
#include "BasicBlock.h"
#include "capstone_wrapper.h"

class LinearPass : public AnalysisPass
{
public:
    LinearPass(uint VirtualStart, uint VirtualEnd);
    virtual ~LinearPass();

    virtual bool Analyse() override;
    virtual std::vector<BasicBlock> & GetModifiedBlocks() override;

private:
    uint m_MaximumThreads;
    std::vector<BasicBlock> m_InitialBlocks;

    void AnalysisWorker(uint Start, uint End, std::vector<BasicBlock>* Blocks);
    BasicBlock* CreateBlockWorker(std::vector<BasicBlock>* Blocks, uint Start, uint End, bool Call, bool Jmp, bool Ret, bool Intr);
};