#pragma once

#include "_global.h"
#include "BasicBlock.h"
#include <functional>

class AnalysisPass
{
public:
    AnalysisPass(duint VirtualStart, duint VirtualEnd, BBlockArray & MainBlocks);
    virtual ~AnalysisPass();

    virtual const char* GetName() = 0;
    virtual bool Analyse() = 0;

protected:
    duint m_VirtualStart;
    duint m_VirtualEnd;
    duint m_DataSize;
    unsigned char* m_Data;
    BBlockArray & m_MainBlocks;

    unsigned char* TranslateAddress(duint Address)
    {
        ASSERT_TRUE(ValidateAddress(Address));

        return &m_Data[Address - m_VirtualStart];
    }

    bool ValidateAddress(duint Address)
    {
        return (Address >= m_VirtualStart && Address < m_VirtualEnd);
    }

    BasicBlock* FindBBlockInRange(duint Address);
    duint FindBBlockIndex(BasicBlock* Block);
    duint IdealThreadCount();
    void SetIdealThreadCount(duint Count);
    void ParallelFor(duint ThreadCount, const std::function<void(duint)> & Worker);

private:
    duint m_InternalMaxThreads;
};