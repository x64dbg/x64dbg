#include <thread>
#include "AnalysisPass.h"
#include "Int3CoagulatorPass.h"
#include "console.h"

Int3CoagulatorPass::Int3CoagulatorPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks)
    : AnalysisPass(VirtualStart, VirtualEnd, MainBlocks)
{
}

Int3CoagulatorPass::~Int3CoagulatorPass()
{
}

const char* Int3CoagulatorPass::GetName()
{
    return "INT3 Group Combiner - DEPRECATED";
}

bool Int3CoagulatorPass::Analyse()
{
    // Execute
    std::thread thread(&Int3CoagulatorPass::AnalysisWorker, this, 0, m_MainBlocks.size(), &m_MainBlocks);

    // Wait for thread to finish
    thread.join();

    dprintf("Total basic blocks: %d\n", m_MainBlocks.size());
    return true;
}

void Int3CoagulatorPass::AnalysisWorker(uint Start, uint End, std::vector<BasicBlock>* Blocks)
{
    uint counterIndex = 0;      // Loop counter

    uint intSeriesStart = 0;    // Block starting address
    uint intSeriesCount = 0;    // Number of blocks
    uint intSeriesSize = 0;     // Size of instructions

    for(auto itr = Blocks->begin(); counterIndex < End; itr++, counterIndex++)
    {
        if(!itr->GetFlag(BASIC_BLOCK_FLAG_PAD))
        {
            // Synchronize the vector if more than 1 instruction
            // is present. (Combine)
            if(intSeriesCount > 1)
            {
                // Removal of old blocks
                itr = Blocks->erase(itr - intSeriesCount, itr);

                // Build the new block and insert
                BasicBlock block;
                block.VirtualStart = intSeriesStart;
                block.VirtualEnd = intSeriesStart + intSeriesSize;
                block.SetFlag(BASIC_BLOCK_FLAG_PAD);

                itr = Blocks->insert(itr, block);

                // Adjust the integer counter manually
                End -= (intSeriesCount - 1);
            }

            // Counter is reset because the series is broken
            intSeriesCount = 0;
            intSeriesSize = 0;
            continue;
        }

        // Hit! An INT3 instruction block has been found.
        // Update the counter stats.
        if(intSeriesCount == 0)
            intSeriesStart = itr->VirtualStart;

        intSeriesCount += 1;
        intSeriesSize += (itr->VirtualEnd - itr->VirtualStart);
    }
}