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

private:
    uint m_MaximumThreads;
    std::vector<BasicBlock> m_InitialBlocks;

    void AnalysisWorker(uint Start, uint End, std::vector<BasicBlock>* Blocks);
};