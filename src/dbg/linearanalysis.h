#ifndef _LINEARANALYSIS_H
#define _LINEARANALYSIS_H

#include "_global.h"
#include "analysis.h"

class LinearAnalysis : public Analysis
{
public:
    explicit LinearAnalysis(uint base, uint size);
    void Analyse() override;
    void SetMarkers() override;

private:
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

    std::vector<FunctionInfo> _functions;

    void SortCleanup();
    void PopulateReferences();
    void AnalyseFunctions();
    uint FindFunctionEnd(uint start, uint maxaddr);
    uint GetReferenceOperand();
};

#endif //_LINEARANALYSIS_H