#include <assert.h>
#include "AnalysisPass.h"
#include "capstone_wrapper.h"
#include "memory.h"

AnalysisPass::AnalysisPass(uint VirtualStart, uint VirtualEnd)
{
    assert(VirtualEnd > VirtualStart);

    // Internal class data
    m_VirtualStart = VirtualStart;
    m_VirtualEnd = VirtualEnd;

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