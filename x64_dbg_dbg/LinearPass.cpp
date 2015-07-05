#include "AnalysisPass.h"
#include "LinearPass.h"
#include <thread>
#include <ppl.h>
#include "console.h"
#include "function.h"

LinearPass::LinearPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks)
    : AnalysisPass(VirtualStart, VirtualEnd, MainBlocks)
{
    // This is a fix for when the total data analysis size is less
    // than what parallelization can support. The minimum size requirement
    // is ((# THREADS) * (512)) bytes. If the requirement isn't met,
    // scale to use a single thread.
    if((512 * IdealThreadCount()) >= m_DataSize)
        SetIdealThreadCount(1);
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

        // Allow a 256-byte variance of scanning because of
        // integer rounding errors and instruction overlap
        if(threadWorkStart > m_VirtualStart)
        {
            threadWorkStart = max((threadWorkStart - 256), m_VirtualStart);
            threadWorkStop = min((threadWorkStop + 256), m_VirtualEnd);
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

    // Free memory ASAP
    delete[] threadBlocks;

    // Sort and remove duplicates
    std::sort(m_MainBlocks.begin(), m_MainBlocks.end());
    m_MainBlocks.erase(std::unique(m_MainBlocks.begin(), m_MainBlocks.end()), m_MainBlocks.end());

    // Run overlap analysis sub-pass
    AnalyseOverlaps();
    return true;
}

void LinearPass::AnalyseOverlaps()
{
    // Goal of this function:
    //
    // Remove all overlapping
    // basic blocks because of threads not ending or
    // starting at absolutely defined points.
    // (Example: one thread starts in the middle of
    // an instruction)
    //
    // This also checks for basic block targets jumping into
    // the middle of other basic blocks.
    //
    // THREAD_WORK = ceil(TOTAL / # THREADS)
    uint workTotal = m_MainBlocks.size();
    uint workAmount = (workTotal + (IdealThreadCount() - 1)) / IdealThreadCount();

    // Initialize thread vectors
    auto threadInserts = new std::vector<BasicBlock>[IdealThreadCount()];

    concurrency::parallel_for(uint(0), IdealThreadCount(), [&](uint i)
    {
        uint threadWorkStart = (workAmount * i);
        uint threadWorkStop = min((threadWorkStart + workAmount), workTotal);

        // Again, allow an overlap of +/- 1 entry
        if(threadWorkStart > 0)
        {
            threadWorkStart = max((threadWorkStart - 1), 0);
            threadWorkStop = min((threadWorkStop + 1), workAmount);
        }

        // Execute
        AnalysisOverlapWorker(threadWorkStart, threadWorkStop, &threadInserts[i]);
    });

    // THREAD VECTOR
    std::vector<BasicBlock> overlapInserts;
    {
        for(uint i = 0; i < IdealThreadCount(); i++)
            std::move(threadInserts[i].begin(), threadInserts[i].end(), std::back_inserter(overlapInserts));

        // Sort and remove duplicates
        std::sort(overlapInserts.begin(), overlapInserts.end());
        overlapInserts.erase(std::unique(overlapInserts.begin(), overlapInserts.end()), overlapInserts.end());

        delete[] threadInserts;
    }

    // GLOBAL VECTOR
    {
        // Erase blocks marked for deletion
        m_MainBlocks.erase(std::remove_if(m_MainBlocks.begin(), m_MainBlocks.end(), [](BasicBlock & Elem)
        {
            return Elem.GetFlag(BASIC_BLOCK_FLAG_DELETE);
        }));

        // Insert
        std::move(overlapInserts.begin(), overlapInserts.end(), std::back_inserter(m_MainBlocks));

        // Final sort
        std::sort(m_MainBlocks.begin(), m_MainBlocks.end());
    }
}

void LinearPass::AnalysisWorker(uint Start, uint End, BBlockArray* Blocks)
{
    Capstone disasm;

    uint blockBegin = Start;        // BBlock starting virtual address
    uint blockEnd = End;            // BBlock ending virtual address
    bool blockPrevInt = false;      // Indicator if the last instruction was INT
    BasicBlock* lastBlock = nullptr;// Avoid an expensive call to std::vector::back()

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
                lastBlock = CreateBlockWorker(Blocks, blockBegin, realBlockEnd, false, false, false, false);
                lastBlock->SetFlag(BASIC_BLOCK_FLAG_PREINT3);

                blockBegin = realBlockEnd;
            }
        }

        if(call || jmp || ret || intr)
        {
            // Was this an INT3?
            if(intr && blockPrevInt)
            {
                // Append it to the previous block
                lastBlock->VirtualEnd = blockEnd;
            }
            else
            {
                // Otherwise use the default route: create a new entry
                auto block = lastBlock = CreateBlockWorker(Blocks, blockBegin, blockEnd, call, jmp, ret, intr);

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
                            operand.mem.scale == 1)
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

void LinearPass::AnalysisOverlapWorker(uint Start, uint End, BBlockArray* Insertions)
{
    // Comparison function to see if two blocks overlap
    auto BlockOverlapsRemove = [](BasicBlock * A, BasicBlock * B) -> BasicBlock*
    {
        // Do the blocks overlap?
        if(max(A->VirtualStart, B->VirtualStart) <= min((A->VirtualEnd - 1), (B->VirtualEnd - 1)))
        {
            // Return the block that should be removed
            if(A->Size() > B->Size())
                return B;

            return A;
        }

        return nullptr;
    };

    // Get a pointer to pure data
    const auto blocks = m_MainBlocks.data();

    for(uint i = Start; i < End; i++)
    {
        const auto curr = &blocks[i];
        const auto next = &blocks[i + 1];

        // Current versus next (overlap -> delete)
        BasicBlock* removal = BlockOverlapsRemove(curr, next);

        if(removal)
            removal->SetFlag(BASIC_BLOCK_FLAG_DELETE);

        // Find blocks that need to be split in two because
        // of CALL/JMP targets
        //
        // Find targets in the virtual range
        if(ValidateAddress(curr->Target))
        {
            removal = FindBBlockInRange(curr->Target);

            // If the target does not equal the block start...
            if(removal && curr->Target != removal->VirtualStart)
            {
                // Mark for deletion
                removal->SetFlag(BASIC_BLOCK_FLAG_DELETE);

                // Block part 1
                BasicBlock block1;
                block1.VirtualStart = removal->VirtualStart;
                block1.VirtualEnd = curr->Target;
                block1.Target = 0;
                block1.Flags = BASIC_BLOCK_FLAG_CUTOFF;

                // Block part 2
                BasicBlock block2;
                block2.VirtualStart = curr->Target;
                block2.VirtualEnd = removal->VirtualEnd;
                block2.Target = removal->Target;
                block2.Flags = removal->Flags;

                Insertions->push_back(block1);
                Insertions->push_back(block2);
            }
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

BasicBlock* LinearPass::FindBBlockInRange(uint Address)
{
    // NOTE: This function assumes that the main block
    // vector is sorted
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