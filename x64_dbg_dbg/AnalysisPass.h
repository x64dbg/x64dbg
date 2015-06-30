#pragma once

#include <assert.h>
#include "_global.h"
#include "BasicBlock.h"

class AnalysisPass
{
public:
    AnalysisPass(uint VirtualStart, uint VirtualEnd);
    virtual ~AnalysisPass();

    virtual bool Analyse() = 0;

    virtual std::vector<BasicBlock> & GetModifiedBlocks() = 0;
    //virtual std::vector<Function>& GetModifiedFunctions() = 0;

protected:
    uint m_VirtualStart;
    uint m_VirtualEnd;
    uint m_DataSize;
    unsigned char* m_Data;

    inline unsigned char* TranslateAddress(uint Address)
    {
        assert(ValidateAddress(Address));

        return &m_Data[Address - m_VirtualStart];
    }

    inline bool ValidateAddress(uint Address)
    {
        return (Address >= m_VirtualStart && Address < m_VirtualEnd);
    }
};