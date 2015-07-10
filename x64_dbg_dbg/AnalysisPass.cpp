#include <assert.h>
#include <thread>
#include "AnalysisPass.h"
#include "memory.h"

AnalysisPass::AnalysisPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks) : m_MainBlocks(MainBlocks)
{
    assert(VirtualEnd > VirtualStart);

    // Shared lock init
    InitializeSRWLock(&m_InternalLock);

    // Internal class data
    m_VirtualStart = VirtualStart;
    m_VirtualEnd = VirtualEnd;
    m_InternalMaxThreads = 0;

    // Read remote instruction data to local memory
    m_DataSize = VirtualEnd - VirtualStart;
    m_Data = (unsigned char*)BridgeAlloc(m_DataSize);

    if(!MemRead((PVOID)VirtualStart, m_Data, m_DataSize, nullptr))
    {
        BridgeFree(m_Data);
        assert(false);
    }
}

AnalysisPass::~AnalysisPass()
{
    if(m_Data)
        BridgeFree(m_Data);
}

BasicBlock* AnalysisPass::FindBBlockInRange(uint Address)
{
    // NOTE: __MUST__ BE A SORTED VECTOR
    //
    // Use a binary search
    uint indexLo = 0;
    uint indexHi = m_MainBlocks.size();

    // Get a pointer to pure data
    const auto blocks = m_MainBlocks.data();

    while(indexHi > indexLo)
    {
        uint indexMid = (indexLo + indexHi) / 2;
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

uint AnalysisPass::FindBBlockIndex(BasicBlock* Block)
{
    // Fast pointer arithmetic to find index
    return ((uint)Block - (uint)m_MainBlocks.data()) / sizeof(BasicBlock);
}

void AnalysisPass::AcquireReadLock()
{
    AcquireSRWLockShared(&m_InternalLock);
}

void AnalysisPass::ReleaseReadLock()
{
    ReleaseSRWLockShared(&m_InternalLock);
}

void AnalysisPass::AcquireExclusiveLock()
{
    AcquireSRWLockExclusive(&m_InternalLock);
}

void AnalysisPass::ReleaseExclusiveLock()
{
    ReleaseSRWLockExclusive(&m_InternalLock);
}

uint AnalysisPass::IdealThreadCount()
{
    if(m_InternalMaxThreads == 0)
    {
        // Determine the maximum hardware thread count at once
        uint maximumThreads = max(std::thread::hardware_concurrency(), 1);

        // Don't consume 100% of the CPU, adjust accordingly
        if(maximumThreads > 1)
            maximumThreads -= 1;

        SetIdealThreadCount(maximumThreads);
    }

    return m_InternalMaxThreads;
}

void AnalysisPass::SetIdealThreadCount(uint Count)
{
    m_InternalMaxThreads = (BYTE)min(Count, 255);
}