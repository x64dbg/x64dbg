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

    std::vector<FunctionInfo> _functions;

    void SortCleanup();
    void PopulateReferences();
    void AnalyseFunctions();
    duint FindFunctionEnd(duint start, duint maxaddr);
    duint GetReferenceOperand();
};

#endif //_LINEARANALYSIS_H