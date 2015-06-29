#pragma once

#include "_global.h"

class AnalysisPass
{
public:
    AnalysisPass(uint VirtualStart, uint VirtualEnd);
    virtual ~AnalysisPass();

    virtual bool Analyse() = 0;

protected:
    uint m_VirtualStart;
    uint m_VirtualEnd;
    uint m_DataSize;
    unsigned char* m_Data;

    unsigned char* TranslateAddress(uint Address);
    bool ValidateAddress(uint Address);
};