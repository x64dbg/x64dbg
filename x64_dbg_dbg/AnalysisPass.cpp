#include <assert.h>
#include "AnalysisPass.h"
#include "capstone_wrapper.h"
#include "memory.h"

AnalysisPass::AnalysisPass(uint VirtualStart, uint VirtualEnd)
{
    // Internal class data
    m_VirtualStart = VirtualStart;
    m_VirtualEnd = VirtualEnd;

    // Read remote instruction data to local memory
    m_DataSize = VirtualEnd - VirtualStart;
    m_Data = (unsigned char*)BridgeAlloc(m_DataSize);

    if(!MemRead((PVOID)VirtualStart, m_Data, m_DataSize, nullptr))
    {
        BridgeFree(m_Data);
        m_Data = nullptr;

        assert(false);
    }
}

AnalysisPass::~AnalysisPass()
{
    if(m_Data)
        BridgeFree(m_Data);
}

unsigned char* AnalysisPass::TranslateAddress(uint Address)
{
    assert(ValidateAddress(Address));

    return &m_Data[Address - m_VirtualStart];
}

bool AnalysisPass::ValidateAddress(uint Address)
{
    return (Address >= m_VirtualStart && Address < m_VirtualEnd);
}