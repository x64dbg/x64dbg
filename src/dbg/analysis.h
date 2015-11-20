#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include "_global.h"
#include <capstone_wrapper.h>

class Analysis
{
public:
    explicit Analysis(duint base, duint size);
    Analysis(const Analysis & that) = delete;
    virtual ~Analysis();
    virtual void Analyse() = 0;
    virtual void SetMarkers() = 0;

protected:
    duint _base;
    duint _size;
    unsigned char* _data;
    Capstone _cp;

    bool IsValidAddress(duint addr);
    const unsigned char* TranslateAddress(duint addr);
};

#endif //_ANALYSIS_H