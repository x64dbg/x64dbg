#include "FunctionPass.h"
#include "capstone_wrapper.h"
#include <ppl.h>
#include "memory.h"
#include "console.h"

FunctionPass::FunctionPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks)
    : AnalysisPass(VirtualStart, VirtualEnd, MainBlocks)
{
}

FunctionPass::~FunctionPass()
{
}

const char* FunctionPass::GetName()
{
    return "Function Analysis";
}

bool FunctionPass::Analyse()
{
    // THREAD_WORK = (TOTAL / # THREADS)
    float workDivision = (float)m_MainBlocks.size() / (float)IdealThreadCount();
    uint workAmount = (int)ceil(workDivision);

    // Initialize thread vector
    auto threadFunctions = new std::vector<FunctionDef>[IdealThreadCount()];

    concurrency::parallel_for(uint(0), IdealThreadCount(), [&](uint i)
    {
        uint threadWorkStart = (workAmount * i);
        uint threadWorkStop = min((threadWorkStart + workAmount), m_MainBlocks.size());

        // Memory allocation optimization
        // TODO: Option to conserve memory
        threadFunctions[i].reserve(10000);

        // Execute
        AnalysisWorker(threadWorkStart, threadWorkStop, &threadFunctions[i]);
    });

    delete[] threadFunctions;
    return true;
}

void FunctionPass::AnalysisWorker(uint Start, uint End, std::vector<FunctionDef>* Blocks)
{
    // Step 1: for each block that contains a CALL flag,
    // add it to a local function start array
    //
    // NOTE: *Some* indirect calls are included
    auto blockItr = std::next(m_MainBlocks.begin(), Start);

    for(uint i = Start; i < End; i++, blockItr++)
    {
        if(blockItr->GetFlag(BASIC_BLOCK_FLAG_CALL))
        {
            uint destination = blockItr->Target;

            // Was it a pointer?
            if(blockItr->GetFlag(BASIC_BLOCK_FLAG_INDIRPTR))
            {
                // Read it from memory
                if(!MemRead((PVOID)destination, &destination, sizeof(uint), nullptr))
                    continue;

                // Validity check
                if(!MemIsValidReadPtr(destination))
                    continue;
            }

            FunctionDef def;
            def.VirtualStart = destination;
            def.VirtualEnd = destination;
            Blocks->push_back(def);
        }
    }

    // Step 2: Sort and remove duplicates
    std::sort(Blocks->begin(), Blocks->end());
    Blocks->erase(std::unique(Blocks->begin(), Blocks->end()), Blocks->end());

    // Step 3: ?
    dprintf("Total call instructions: %d\n", Blocks->size());
}