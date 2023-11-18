#include <assert.h>
#include <thread>
#include "AnalysisPass.h"
#include "memory.h"

AnalysisPass::AnalysisPass(duint VirtualStart, duint VirtualEnd, BBlockArray & MainBlocks) : m_MainBlocks(MainBlocks)
{
    assert(VirtualEnd > VirtualStart);

    // Internal class data
    m_VirtualStart = VirtualStart;
    m_VirtualEnd = VirtualEnd;
    m_InternalMaxThreads = 0;

    // Read remote instruction data to local memory
    m_DataSize = VirtualEnd - VirtualStart;
    m_Data = (unsigned char*)VirtualAlloc(nullptr, m_DataSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if(!MemRead(VirtualStart, m_Data, m_DataSize))
    {
        VirtualFree(m_Data, 0, MEM_RELEASE);
        assert(false);
    }
}

AnalysisPass::~AnalysisPass()
{
    if(m_Data)
        VirtualFree(m_Data, 0, MEM_RELEASE);
}

BasicBlock* AnalysisPass::FindBBlockInRange(duint Address)
{
    // NOTE: __MUST__ BE A SORTED VECTOR
    //
    // Use a binary search
    duint indexLo = 0;
    duint indexHi = m_MainBlocks.size();

    // Get a pointer to pure data
    const auto blocks = m_MainBlocks.data();

    while(indexHi > indexLo)
    {
        duint indexMid = (indexLo + indexHi) / 2;
        auto entry = &blocks[indexMid];

        if(Address < entry->VirtualStart)
        {
            // Continue search in lower half
            indexHi = indexMid;
        }
        else if(Address >= entry->VirtualEnd)
        {
            // Continue search in upper half
            indexLo = indexMid + 1;
        }
        else
        {
            // Address is within limits, return entry
            return entry;
        }
    }

    // Not found
    return nullptr;
}

duint AnalysisPass::FindBBlockIndex(BasicBlock* Block)
{
    // Fast pointer arithmetic to find index
    return ((duint)Block - (duint)m_MainBlocks.data()) / sizeof(BasicBlock);
}

duint AnalysisPass::IdealThreadCount()
{
    if(m_InternalMaxThreads == 0)
    {
        // Determine the maximum hardware thread count at once
        duint maximumThreads = max(GetThreadCount(), 1);

        // Don't consume 100% of the CPU, adjust accordingly
        if(maximumThreads > 1)
            maximumThreads -= 1;

        SetIdealThreadCount(maximumThreads);
    }

    return m_InternalMaxThreads;
}

void AnalysisPass::SetIdealThreadCount(duint Count)
{
    m_InternalMaxThreads = (BYTE)min(Count, 255);
}