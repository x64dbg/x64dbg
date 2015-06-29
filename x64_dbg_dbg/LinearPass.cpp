#include "AnalysisPass.h"
#include "LinearPass.h"
#include "console.h"

LinearPass::LinearPass(uint VirtualStart, uint VirtualEnd)
    : AnalysisPass(VirtualStart, VirtualEnd)
{
    // Determine the maximum hardware thread count at once
    m_MaximumThreads = max(std::thread::hardware_concurrency(), 1);

    // Don't consume 100% of the CPU, so adjust accordingly
    if(m_MaximumThreads > 1)
        m_MaximumThreads -= 1;
}

LinearPass::~LinearPass()
{
}

bool LinearPass::Analyse()
{
    // Divide the work up between each thread
    // THREAD_WORK = (TOTAL / # THREADS)
    uint workCurrent = 0;
    uint workAmount = m_DataSize / m_MaximumThreads;

    // Initialize thread vector
    std::vector<std::thread> localThreads(m_MaximumThreads);
    std::vector<BasicBlock>* threadBlocks = new std::vector<BasicBlock>[m_MaximumThreads];

    for(uint i = 0; i < m_MaximumThreads; i++)
    {
        uint threadWorkStart = m_VirtualStart + workCurrent;
        uint threadWorkStop = min((threadWorkStart + workAmount), m_VirtualEnd);

        // Allow a 16-byte variance of scanning because of
        // integer rounding errors
        if(workCurrent >= 16)
            threadWorkStart -= 16;

        // Memory allocation optimization
        // TODO: Option to conserve memory
        threadBlocks[i].reserve(100000);

        // Execute
        localThreads[i] = std::thread(&LinearPass::AnalysisWorker, this, threadWorkStart, threadWorkStop, &threadBlocks[i]);

        // Increment the work counter
        workCurrent += workAmount;
    }

    // Wait for all threads to finish and combine vectors
    for(uint i = 0; i < m_MaximumThreads; i++)
    {
        localThreads[i].join();
        m_InitialBlocks.insert(m_InitialBlocks.end(), threadBlocks[i].begin(), threadBlocks[i].end());
    }

    // Sort and remove duplicates
    std::sort(m_InitialBlocks.begin(), m_InitialBlocks.end());
    m_InitialBlocks.erase(std::unique(m_InitialBlocks.begin(), m_InitialBlocks.end()), m_InitialBlocks.end());

    // Logging
    dprintf("Total basic blocks: %d\n", m_InitialBlocks.size());

    // Cleanup
    delete[] threadBlocks;

    return true;
}

void LinearPass::AnalysisWorker(uint Start, uint End, std::vector<BasicBlock>* Blocks)
{
    Capstone disasm;
    uint blockBegin = Start;

    int intSeriesSize = 0;
    int intSeriesCount = 0;

    for(uint i = Start; i < End;)
    {
        if(!disasm.Disassemble(i, TranslateAddress(i)))
        {
            // Skip instructions that can't be determined
            i++;
            continue;
        }

        // Increment counter
        i += disasm.Size();

        // The basic block ends here if it is a branch
        bool call = disasm.InGroup(CS_GRP_CALL);    // CALL
        bool jmp = disasm.InGroup(CS_GRP_JUMP);     // JUMP
        bool ret = disasm.InGroup(CS_GRP_RET);      // RETURN
        bool intr = disasm.InGroup(CS_GRP_INT);     // INTERRUPT

        if(call || jmp || ret || intr)
        {
            BasicBlock block;
            block.VirtualStart = blockBegin;
            block.VirtualEnd = i - 1;

            // Check for calls
            if(call)
                block.SetFlag(BASIC_BLOCK_FLAG_CALL);

            // Check for returns
            if(ret)
                block.SetFlag(BASIC_BLOCK_FLAG_RET);

            // Check for interrupts
            if(intr)
                block.SetFlag(BASIC_BLOCK_FLAG_INT3);

            // Check for indirects
            auto operand = disasm.x86().operands[0];

            if(operand.mem.base != X86_REG_INVALID ||
                    operand.mem.index != X86_REG_INVALID ||
                    operand.mem.scale != 0)
            {
                block.SetFlag(BASIC_BLOCK_FLAG_INDIRECT);
            }

            Blocks->push_back(block);

            // Reset the loop variables
            blockBegin = i;
        }
    }
}