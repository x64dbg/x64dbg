#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include "_global.h"
#include "capstone_wrapper.h"

class Analysis
{
public:
    explicit Analysis(uint base, uint size);
    Analysis(const Analysis & that) = delete;
    ~Analysis();
    virtual void Analyse() = 0;
    virtual void SetMarkers() = 0;

protected:
    uint _base;
    uint _size;
    unsigned char* _data;
    Capstone _cp;

    bool IsValidAddress(uint addr);
    const unsigned char* TranslateAddress(uint addr);
};

#endif //_ANALYSIS_H