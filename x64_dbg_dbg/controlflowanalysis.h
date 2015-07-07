#ifndef _CONTROLFLOWANALYSIS_H
#define _CONTROLFLOWANALYSIS_H

#include "_global.h"
#include "capstone_wrapper.h"

class ControlFlowAnalysis
{
public:
    explicit ControlFlowAnalysis(uint base, uint size);
    ControlFlowAnalysis(const ControlFlowAnalysis & that) = delete;
    ~ControlFlowAnalysis();
    bool IsValidAddress(uint addr);
    const unsigned char* TranslateAddress(uint addr);
    void Analyse();
    void SetMarkers();

    struct FunctionInfo
    {
        uint start;
        uint end;

        bool operator<(const FunctionInfo & b) const
        {
            return start < b.start;
        }

        bool operator==(const FunctionInfo & b) const
        {
            return start == b.start;
        }
    };

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

private:
    uint _base;
    uint _size;
    unsigned char* _data;
    std::set<uint> _blockStarts;
    std::vector<BasicBlock> _blocks;
    Capstone _cp;

    void BasicBlockStarts();
    void BasicBlocks();
    uint GetBranchOperand();
};

#endif //_CONTROLFLOWANALYSIS_H