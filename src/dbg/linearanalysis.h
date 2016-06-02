#ifndef _LINEARANALYSIS_H
#define _LINEARANALYSIS_H

#include "_global.h"
#include "analysis.h"

class LinearAnalysis : public Analysis
{
public:
    explicit LinearAnalysis(duint base, duint size);
    void Analyse() override;
    void SetMarkers() override;

private:
    struct FunctionInfo
    {
        duint start;
        duint end;

        bool operator<(const FunctionInfo & b) const
        {
            return start < b.start;
        }

        bool operator==(const FunctionInfo & b) const
        {
            return start == b.start;
        }
    };

    struct ReferenceInfo
    {
        duint from;
        duint to;
        bool iscall;
    };

    std::vector<FunctionInfo> mFunctions;

    std::vector<ReferenceInfo> mReferences;

    void sortCleanup();
    void populateReferences();
    void analyseFunctions();
    duint findFunctionEnd(duint start, duint maxaddr);
    duint getReferenceOperand(bool* iscall) const;
};

#endif //_LINEARANALYSIS_H