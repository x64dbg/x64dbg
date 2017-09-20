#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include "_global.h"
#include <zydis_wrapper.h>

class Analysis
{
public:
    explicit Analysis(duint base, duint size);
    Analysis(const Analysis & that) = delete;
    virtual ~Analysis();
    virtual void Analyse() = 0;
    virtual void SetMarkers() = 0;

protected:
    duint mBase;
    duint mSize;
    unsigned char* mData;
    Zydis mCp;

    bool inRange(duint addr) const
    {
        return addr >= mBase && addr < mBase + mSize;
    }

    const unsigned char* translateAddr(duint addr) const
    {
        return inRange(addr) ? mData + (addr - mBase) : nullptr;
    }
};

#endif //_ANALYSIS_H