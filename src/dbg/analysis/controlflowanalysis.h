#ifndef _CONTROLFLOWANALYSIS_H
#define _CONTROLFLOWANALYSIS_H

#include "_global.h"
#include "analysis.h"
#include "addrinfo.h"
#include <functional>
#include <unordered_set>

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
            : BasicBlock(0, 0, 0, 0)
        {
        }

        BasicBlock(duint start, duint end, duint left, duint right)
            : start(start),
              end(end),
              left(std::min(left, right)),
              right(std::min(left, right)),
              function(0)
        {
        }

        String toString() const
        {
            return StringUtils::sprintf("start:%p,end:%p,left:%p,right:%p,func:%p", start, end, left, right, function);
        }
    };

    typedef std::unordered_set<duint> UintSet;

    duint mModuleBase;
    duint mFunctionInfoSize;
    void* mFunctionInfoData;

    UintSet mBlockStarts;
    UintSet mFunctionStarts;
    std::unordered_map<duint, BasicBlock> mBlocks; //start of block -> block
    std::unordered_map<duint, UintSet> mParentMap; //start child -> parents
    std::unordered_map<duint, UintSet> mFunctions; //function start -> function block starts
    std::vector<Range> mFunctionRanges; //function start -> function range TODO: smarter stuff with overlapping ranges

    void BasicBlockStarts();
    void BasicBlocks();
    void Functions();
    void FunctionRanges();
    void insertBlock(const BasicBlock & block);
    const BasicBlock* findBlock(duint start) const;
    void insertParent(duint child, duint parent);
    const UintSet* findParents(duint child) const;
    duint findFunctionStart(const BasicBlock* block, const UintSet* parents) const;
    static String blockToString(const BasicBlock* block);
    duint getReferenceOperand() const;

#ifdef _WIN64
    void enumerateFunctionRuntimeEntries64(const std::function<bool(PRUNTIME_FUNCTION)> & Callback) const;
#endif // _WIN64
};

#endif //_CONTROLFLOWANALYSIS_H