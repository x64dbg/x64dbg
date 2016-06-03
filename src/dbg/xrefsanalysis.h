#pragma once

#include "analysis.h"

class XrefsAnalysis : public Analysis
{
public:
    XrefsAnalysis(duint base, duint size)
        : Analysis(base, size)
    {
    }

    void Analyse() override;
    void SetMarkers() override;

private:
    struct XREF
    {
        duint addr;
        duint from;
    };

    std::vector<XREF> mXrefs;
};