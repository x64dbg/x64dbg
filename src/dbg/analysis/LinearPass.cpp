#include <thread>
#include <ppl.h>
#include "AnalysisPass.h"
#include "LinearPass.h"
#include <zydis_wrapper.h>

LinearPass::LinearPass(duint VirtualStart, duint VirtualEnd, BBlockArray & MainBlocks)
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
    duint workAmount = m_DataSize / IdealThreadCount();

    // Initialize thread vector
    auto threadBlocks = new std::vector<BasicBlock>[IdealThreadCount()];

    concurrency::parallel_for(duint(0), IdealThreadCount(), [&](duint i)
    {
        duint threadWorkStart = m_VirtualStart + (workAmount * i);
        duint threadWorkStop = min((threadWorkStart + workAmount), m_VirtualEnd);

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

    for(duint i = 0; i < IdealThreadCount(); i++)
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
    // Remove all overlapping basic blocks because of threads
    // not ending or starting at absolutely defined points.
    // (Example: one thread starts in the middle of an instruction)
    //
    // This also checks for basic block targets jumping into
    // the middle of other basic blocks.
    //
    // THREAD_WORK = ceil(TOTAL / # THREADS)
    duint workTotal = m_MainBlocks.size();
    duint workAmount = (workTotal + (IdealThreadCount() - 1)) / IdealThreadCount();

    // Initialize thread vectors
    auto threadInserts = new std::vector<BasicBlock>[IdealThreadCount()];

    concurrency::parallel_for(duint(0), IdealThreadCount(), [&](duint i)
    {
        duint threadWorkStart = (workAmount * i);
        duint threadWorkStop = min((threadWorkStart + workAmount), workTotal);

        // Again, allow an overlap of +/- 1 entry
        if(threadWorkStart > 0)
        {
            threadWorkStart = max((threadWorkStart - 1), 0);
            threadWorkStop = min((threadWorkStop + 1), workTotal);
        }

        // Execute
        AnalysisOverlapWorker(threadWorkStart, threadWorkStop, &threadInserts[i]);
    });

    // THREAD VECTOR
    std::vector<BasicBlock> overlapInserts;
    {
        for(duint i = 0; i < IdealThreadCount(); i++)
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

void LinearPass::AnalysisWorker(duint Start, duint End, BBlockArray* Blocks)
{
    Zydis disasm;

    duint blockBegin = Start;        // BBlock starting virtual address
    duint blockEnd = 0;              // BBlock ending virtual address

    bool blockPrevPad = false;       // Indicator if the last instruction was padding
    BasicBlock* lastBlock = nullptr; // Avoid an expensive call to std::vector::back()

    int insnCount = 0;               // Temporary number of instructions counted for a block

    for(duint i = Start; i < End;)
    {
        if(!disasm.Disassemble(i, TranslateAddress(i), int(End - i)))
        {
            // Skip instructions that can't be determined
            i++;
            continue;
        }

        // Increment counters
        i += disasm.Size();
        blockEnd = i;
        insnCount++;

        // The basic block ends here if it is a branch
        bool call = disasm.IsCall();        // CALL
        bool jmp = disasm.IsJump();         // JUMP
        bool ret = disasm.IsRet();          // RETURN
        bool padding = disasm.IsFilling();  // INSTRUCTION PADDING

        if(padding)
        {
            // PADDING is treated differently. They are all created as their
            // own separate block for more analysis later.
            duint realBlockEnd = blockEnd - disasm.Size();

            if((realBlockEnd - blockBegin) > 0)
            {
                // The next line terminates the BBlock before the INT instruction.
                // Early termination, faked as an indirect JMP. Rare case.
                lastBlock = CreateBlockWorker(Blocks, blockBegin, realBlockEnd, false, false, false, false);
                lastBlock->SetFlag(BASIC_BLOCK_FLAG_PREPAD);

                blockBegin = realBlockEnd;
                lastBlock->InstrCount = insnCount;
                insnCount = 0;
            }
        }

        if(call || jmp || ret || padding)
        {
            // Was this a padding instruction?
            if(padding && blockPrevPad)
            {
                // Append it to the previous block
                lastBlock->VirtualEnd = blockEnd;
            }
            else
            {
                // Otherwise use the default route: create a new entry
                auto block = lastBlock = CreateBlockWorker(Blocks, blockBegin, blockEnd, call, jmp, ret, padding);

                // Counters
                lastBlock->InstrCount = insnCount;
                insnCount = 0;

                if(!padding)
                {
                    // Check if absolute jump, regardless of operand
                    if(disasm.GetId() == ZYDIS_MNEMONIC_JMP)
                        block->SetFlag(BASIC_BLOCK_FLAG_ABSJMP);

                    // Figure out the operand type(s)
                    if(disasm.OpCount() > 0)
                    {
                        const auto & operand = disasm[0];

                        if(operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
                        {
                            // Branch target immediate
                            block->Target = (duint)operand.imm.value.u;
                        }
                        else
                        {
                            // Indirects (no operand, register, or memory)
                            block->SetFlag(BASIC_BLOCK_FLAG_INDIRECT);

                            if(operand.type == ZYDIS_OPERAND_TYPE_MEMORY &&
                                    operand.mem.base == ZYDIS_REGISTER_RIP &&
                                    operand.mem.index == ZYDIS_REGISTER_NONE &&
                                    operand.mem.scale == 1)
                            {
                                /*
                                block->SetFlag(BASIC_BLOCK_FLAG_INDIRPTR);
                                block->Target = (duint)operand.mem.disp;
                                */
                            }
                        }
                    }
                }
            }

            // Reset the loop variables
            blockBegin = i;
            blockPrevPad = padding;
        }
    }
}

void LinearPass::AnalysisOverlapWorker(duint Start, duint End, BBlockArray* Insertions)
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

    for(duint i = Start; i < End; i++)
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

            if(removal)
            {
                if(curr->GetFlag(BASIC_BLOCK_FLAG_CALL))
                    removal->SetFlag(BASIC_BLOCK_FLAG_CALL_TARGET);

                // If the target does not equal the block start...
                if(curr->Target != removal->VirtualStart)
                {
                    // Mark for deletion
                    removal->SetFlag(BASIC_BLOCK_FLAG_DELETE);

                    // Block part 1
                    BasicBlock block1;
                    block1.VirtualStart = removal->VirtualStart;
                    block1.VirtualEnd = curr->Target;
                    block1.Target = 0;
                    block1.Flags = BASIC_BLOCK_FLAG_CUTOFF; // Attributes of the top half
                    block1.InstrCount = removal->InstrCount;

                    // Block part 2
                    BasicBlock block2;
                    block2.VirtualStart = curr->Target;
                    block2.VirtualEnd = removal->VirtualEnd;
                    block2.Target = removal->Target;
                    block2.Flags = removal->Flags;          // Attributes of the bottom half (taken from original block)
                    block2.InstrCount = removal->InstrCount;

                    Insertions->push_back(block1);
                    Insertions->push_back(block2);
                }
            }
        }
    }
}

BasicBlock* LinearPass::CreateBlockWorker(std::vector<BasicBlock>* Blocks, duint Start, duint End, bool Call, bool Jmp, bool Ret, bool Pad)
{
    BasicBlock block { Start, End - 1, 0, 0, 0 };

    // Check for calls
    if(Call)
        block.SetFlag(BASIC_BLOCK_FLAG_CALL);

    // Check for returns
    if(Ret)
        block.SetFlag(BASIC_BLOCK_FLAG_RET);

    // Check for interrupts
    if(Pad)
        block.SetFlag(BASIC_BLOCK_FLAG_PAD);

    Blocks->push_back(block);

    // std::vector::back() incurs a very large performance overhead (30% slower) when
    // used in debug mode. This code eliminates it from showing up in the profiler.
#ifdef _DEBUG
    return &Blocks->data()[Blocks->size() - 1];
#else
    return &Blocks->back();
#endif // _DEBUG
}