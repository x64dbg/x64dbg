#ifndef _CONTROLFLOWANALYSIS_H
#define _CONTROLFLOWANALYSIS_H

#include "_global.h"
#include "capstone_wrapper.h"
#include "analysis.h"

class ControlFlowAnalysis : public Analysis
{
public:
    explicit ControlFlowAnalysis(uint base, uint size);
    void Analyse();
    void SetMarkers();

private:
    struct BasicBlock
    {
        uint start;
        uint end;
        uint left;
        uint right;

        BasicBlock()
        {
            this->start = 0;
            this->end = 0;
            this->left = 0;
            this->right = 0;
        }

        BasicBlock(uint start, uint end, uint left, uint right)
        {
            this->start = start;
            this->end = end;
            this->left = min(left, right);
            this->right = max(left, right);
        }
    };

    std::set<uint> _blockStarts;
    std::vector<BasicBlock> _blocks;

    void BasicBlockStarts();
    void BasicBlocks();
    uint GetBranchOperand();
};

#endif //_CONTROLFLOWANALYSIS_H