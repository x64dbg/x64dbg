#include "AnalysisPass.h"
#include "LinearPass.h"
#include "console.h"

LinearPass::LinearPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks)
    : AnalysisPass(VirtualStart, VirtualEnd, MainBlocks)
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

const char* LinearPass::GetName()
{
    return "Linear Scandown";
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
    m_MainBlocks.clear();

    for(uint i = 0; i < m_MaximumThreads; i++)
    {
        localThreads[i].join();
        m_MainBlocks.insert(m_MainBlocks.end(), threadBlocks[i].begin(), threadBlocks[i].end());
    }

    // Sort and remove duplicates
    std::sort(m_MainBlocks.begin(), m_MainBlocks.end());
    m_MainBlocks.erase(std::unique(m_MainBlocks.begin(), m_MainBlocks.end()), m_MainBlocks.end());

    // Logging
    dprintf("Total basic blocks: %d\n", m_MainBlocks.size());

    FILE* f = fopen("C:\\test.txt", "w");

    for(auto & block : m_MainBlocks)
    {
        char buf[256];
        size_t size = sprintf_s(buf, "Start: 0x%p End: 0x%p\n", block.VirtualStart, block.VirtualEnd);

        fwrite(buf, size, 1, f);
    }

    fclose(f);

    // Cleanup
    delete[] threadBlocks;

    return true;
}

void LinearPass::AnalysisWorker(uint Start, uint End, std::vector<BasicBlock>* Blocks)
{
    Capstone disasm;

    uint blockBegin = Start;    // BBlock starting virtual address
    uint blockEnd = End;        // BBlock ending virtual address
    bool blockPrevInt = false;  // Indicator if the last instruction was INT

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
        blockEnd = i;

        // The basic block ends here if it is a branch
        bool call = disasm.InGroup(CS_GRP_CALL);    // CALL
        bool jmp = disasm.InGroup(CS_GRP_JUMP);     // JUMP
        bool ret = disasm.InGroup(CS_GRP_RET);      // RETURN
        bool intr = disasm.InGroup(CS_GRP_INT);     // INTERRUPT

        if(intr)
        {
            // INT3s are treated differently. They are all created as their
            // own separate block for more analysis later.
            uint realBlockEnd = blockEnd - disasm.Size();

            if((realBlockEnd - blockBegin) > 0)
            {
                // The next line terminates the BBlock before the INT instruction.
                // Early termination, faked as an indirect JMP. Rare case.
                CreateBlockWorker(Blocks, blockBegin, realBlockEnd, false, false, false, false)->SetFlag(BASIC_BLOCK_FLAG_PREINT3);

                blockBegin = realBlockEnd;
            }
        }

        if(call || jmp || ret || intr)
        {
            // Was this an INT3?
            if(intr && blockPrevInt)
            {
                // Append it to the previous block
                Blocks->back().VirtualEnd = blockEnd;
            }
            else
            {
                // Otherwise use the default route: create a new entry
                auto block = CreateBlockWorker(Blocks, blockBegin, blockEnd, call, jmp, ret, intr);

                // Indirect branching
                auto operand = disasm.x86().operands[0];

                if(operand.mem.base != X86_REG_INVALID ||
                        operand.mem.index != X86_REG_INVALID ||
                        operand.mem.scale != 0)
                    block->SetFlag(BASIC_BLOCK_FLAG_INDIRECT);

                // Branch target
                block->Target = operand.imm;
            }

            // Reset the loop variables
            blockBegin = i;
            blockPrevInt = intr;
        }
    }
}

BasicBlock* LinearPass::CreateBlockWorker(std::vector<BasicBlock>* Blocks, uint Start, uint End, bool Call, bool Jmp, bool Ret, bool Intr)
{
    BasicBlock block;
    block.VirtualStart = Start;
    block.VirtualEnd = End;
    block.Flags = 0;
    block.Target = 0;

    // Check for calls
    if(Call)
        block.SetFlag(BASIC_BLOCK_FLAG_CALL);

    // Check for returns
    if(Ret)
        block.SetFlag(BASIC_BLOCK_FLAG_RET);

    // Check for interrupts
    if(Intr)
        block.SetFlag(BASIC_BLOCK_FLAG_INT3);

    Blocks->push_back(block);
    return &Blocks->back();
}