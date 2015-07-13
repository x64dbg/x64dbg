#ifndef _CONTROLFLOWANALYSIS_H
#define _CONTROLFLOWANALYSIS_H

#include "_global.h"
#include "analysis.h"
#include "addrinfo.h"

class ControlFlowAnalysis : public Analysis
{
public:
    explicit ControlFlowAnalysis(uint base, uint size);
    void Analyse() override;
    void SetMarkers() override;

private:
    struct BasicBlock
    {
        uint start;
        uint end;
        uint left;
        uint right;
        uint function;

        BasicBlock()
        {
            this->start = 0;
            this->end = 0;
            this->left = 0;
            this->right = 0;
            this->function = 0;
        }

        BasicBlock(uint start, uint end, uint left, uint right)
        {
            this->start = start;
            this->end = end;
            this->left = min(left, right);
            this->right = max(left, right);
            this->function = 0;
        }

        String toString()
        {
            return StringUtils::sprintf("start:%p,end:%p,left:%p,right:%p,func:%p", start, end, left, right, function);
        }
    };

    typedef std::set<uint> UintSet;

    UintSet _blockStarts;
    UintSet _functionStarts;
    std::map<uint, BasicBlock> _blocks; //start of block -> block
    std::map<uint, UintSet> _parentMap; //start child -> parents
    std::map<uint, UintSet> _functions; //function start -> function block starts

    void BasicBlockStarts();
    void BasicBlocks();
    void Functions();
    void insertBlock(BasicBlock block);
    BasicBlock* findBlock(uint start);
    void insertParent(uint child, uint parent);
    UintSet* findParents(uint child);
    uint findFunctionStart(BasicBlock* block, UintSet* parents);
    String blockToString(BasicBlock* block);
    uint GetReferenceOperand();
};

#endif //_CONTROLFLOWANALYSIS_H