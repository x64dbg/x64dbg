#ifndef _FUNCTIONANALYSIS_H
#define _FUNCTIONANALYSIS_H

#include "_global.h"
#include "capstone_wrapper.h"

class FunctionAnalysis
{
public:
    explicit FunctionAnalysis(uint base, uint size);
    FunctionAnalysis(const FunctionAnalysis & that) = delete;
    ~FunctionAnalysis();
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

private:
    uint _base;
    uint _size;
    unsigned char* _data;
    std::vector<FunctionInfo> _functions;
    Capstone _cp;

    void SortCleanup();
    void PopulateReferences();
    void AnalyseFunctions();
    uint FindFunctionEnd(uint start, uint maxaddr);
    uint GetReferenceOperand();
};

#endif //_FUNCTIONANALYSIS_H