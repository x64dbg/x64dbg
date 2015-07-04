#include <assert.h>
#include <thread>
#include "AnalysisPass.h"
#include "capstone_wrapper.h"
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

        m_InternalMaxThreads = (BYTE)min(maximumThreads, 255);
    }

    return m_InternalMaxThreads;
}