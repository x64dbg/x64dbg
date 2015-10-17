#ifndef _CONTROLFLOWANALYSIS_H
#define _CONTROLFLOWANALYSIS_H

#include "_global.h"
#include "analysis.h"
#include "addrinfo.h"
#include <functional>

class ControlFlowAnalysis : public Analysis
{
public:
    explicit ControlFlowAnalysis(duint base, duint size, bool exceptionDirectory);
    ~ControlFlowAnalysis();
    void Analyse() override;
    void SetMarkers() override;

private:
    struct BasicBlock
    {
        duint start;
        duint end;
        duint left;
        duint right;
        duint function;

        BasicBlock()
        {
            this->start = 0;
            this->end = 0;
            this->left = 0;
            this->right = 0;
            this->function = 0;
        }

        BasicBlock(duint start, duint end, duint left, duint right)
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

    typedef std::set<duint> UintSet;

    duint _moduleBase;
    duint _functionInfoSize;
    void* _functionInfoData;

    UintSet _blockStarts;
    UintSet _functionStarts;
    std::map<duint, BasicBlock> _blocks; //start of block -> block
    std::map<duint, UintSet> _parentMap; //start child -> parents
    std::map<duint, UintSet> _functions; //function start -> function block starts
    std::vector<Range> _functionRanges; //function start -> function range TODO: smarter stuff with overlapping ranges

    void BasicBlockStarts();
    void BasicBlocks();
    void Functions();
    void FunctionRanges();
    void insertBlock(BasicBlock block);
    BasicBlock* findBlock(duint start);
    void insertParent(duint child, duint parent);
    UintSet* findParents(duint child);
    duint findFunctionStart(BasicBlock* block, UintSet* parents);
    String blockToString(BasicBlock* block);
    duint GetReferenceOperand();
#ifdef _WIN64
    void EnumerateFunctionRuntimeEntries64(std::function<bool(PRUNTIME_FUNCTION)> Callback);
#endif // _WIN64
};

#endif //_CONTROLFLOWANALYSIS_H