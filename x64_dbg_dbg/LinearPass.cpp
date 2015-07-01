#include "AnalysisPass.h"
#include "LinearPass.h"
#include <thread>
#include <ppl.h>
#include "console.h"

LinearPass::LinearPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks)
    : AnalysisPass(VirtualStart, VirtualEnd, MainBlocks)
{
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
    uint workAmount = m_DataSize / IdealThreadCount();

    // Initialize thread vector
    auto threadBlocks = new std::vector<BasicBlock>[IdealThreadCount()];

    concurrency::parallel_for(uint(0), IdealThreadCount(), [&](uint i)
    {
        uint threadWorkStart = m_VirtualStart + (workAmount * i);
        uint threadWorkStop = min((threadWorkStart + workAmount), m_VirtualEnd);

        // Allow a 16-byte variance of scanning because of
        // integer rounding errors
        if(threadWorkStart > m_VirtualStart)
        {
            threadWorkStart -= 16;
            threadWorkStop += 16;
        }

        // Memory allocation optimization
        // TODO: Option to conserve memory
        threadBlocks[i].reserve(100000);

        // Execute
        AnalysisWorker(threadWorkStart, threadWorkStop, &threadBlocks[i]);
    });

    // Clear old data and combine vectors
    m_MainBlocks.clear();

    for(uint i = 0; i < IdealThreadCount(); i++)
    {
        std::move(threadBlocks[i].begin(), threadBlocks[i].end(), std::back_inserter(m_MainBlocks));

        // Free old elements to conserve memory further
        BBlockArray().swap(threadBlocks[i]);
    }

    // Sort and remove duplicates
    std::sort(m_MainBlocks.begin(), m_MainBlocks.end());
    m_MainBlocks.erase(std::unique(m_MainBlocks.begin(), m_MainBlocks.end()), m_MainBlocks.end());

    // Logging
    dprintf("Total basic blocks: %d\n", m_MainBlocks.size());

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

                // Figure out the operand type
                auto operand = disasm.x86().operands[0];

                if(operand.type == X86_OP_IMM)
                {
                    // Branch target immediate
                    block->Target = operand.imm;
                }
                else
                {
                    // Indirects
                    block->SetFlag(BASIC_BLOCK_FLAG_INDIRECT);

                    if(operand.type == X86_OP_MEM &&
                            operand.mem.base == X86_REG_INVALID &&
                            operand.mem.index == X86_REG_INVALID &&
                            operand.mem.scale == 0)
                    {
                        block->SetFlag(BASIC_BLOCK_FLAG_INDIRPTR);
                        block->Target = operand.mem.disp;
                    }
                }
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