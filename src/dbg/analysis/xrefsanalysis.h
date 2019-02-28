#pragma once

#include "analysis.h"

class XrefsAnalysis : public Analysis
{
public:
    XrefsAnalysis(duint base, duint size);

    void Analyse() override;
    void SetMarkers() override;

private:
    struct XREF
    {
        duint addr;
        duint from;
    };

    std::vector<XREF> mXrefs;

    duint modbase = 0;
    duint modsize = 0;

    bool inModRange(duint addr) const
    {
        return modbase ? (addr >= modbase && addr < modbase + modsize) : inRange(addr);
    }
};