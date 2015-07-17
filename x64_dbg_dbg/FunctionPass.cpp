#include "FunctionPass.h"
#include <ppl.h>
#include "memory.h"
#include "console.h"
#include "debugger.h"
#include "module.h"
#include "function.h"

FunctionPass::FunctionPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks)
    : AnalysisPass(VirtualStart, VirtualEnd, MainBlocks)
{
    // Zero values
    m_FunctionInfo = nullptr;
    m_FunctionInfoSize = 0;

    // This will only be valid if the address range is within a loaded module
    m_ModuleStart = ModBaseFromAddr(VirtualStart);

    if(m_ModuleStart != 0)
    {
        char modulePath[MAX_PATH];
        memset(modulePath, 0, sizeof(modulePath));

        ModPathFromAddr(m_ModuleStart, modulePath, ARRAYSIZE(modulePath));

        HANDLE fileHandle;
        DWORD fileSize;
        HANDLE fileMapHandle;
        ULONG_PTR fileMapVa;
        if(StaticFileLoadW(
                    StringUtils::Utf8ToUtf16(modulePath).c_str(),
                    UE_ACCESS_READ,
                    false,
                    &fileHandle,
                    &fileSize,
                    &fileMapHandle,
                    &fileMapVa))
        {
            // Find a pointer to IMAGE_DIRECTORY_ENTRY_EXCEPTION for later use
            ULONG_PTR virtualOffset = GetPE32DataFromMappedFile(fileMapVa, IMAGE_DIRECTORY_ENTRY_EXCEPTION, UE_SECTIONVIRTUALOFFSET);
            m_FunctionInfoSize = (ULONG)GetPE32DataFromMappedFile(fileMapVa, IMAGE_DIRECTORY_ENTRY_EXCEPTION, UE_SECTIONVIRTUALSIZE);

            // Unload the file
            StaticFileUnloadW(nullptr, false, fileHandle, fileSize, fileMapHandle, fileMapVa);

            // Get a copy of the function table
            if(virtualOffset)
            {
                // Read the table into a buffer
                m_FunctionInfo = BridgeAlloc(m_FunctionInfoSize);

                if(m_FunctionInfo)
                    MemRead(virtualOffset + m_ModuleStart, m_FunctionInfo, m_FunctionInfoSize);
            }
        }
    }
}

FunctionPass::~FunctionPass()
{
    if(m_FunctionInfo)
        BridgeFree(m_FunctionInfo);
}

const char* FunctionPass::GetName()
{
    return "Function Analysis";
}

bool FunctionPass::Analyse()
{
    // THREAD_WORK = ceil(TOTAL / # THREADS)
    uint workAmount = (m_MainBlocks.size() + (IdealThreadCount() - 1)) / IdealThreadCount();

    // Initialize thread vector
    auto threadFunctions = new std::vector<FunctionDef>[IdealThreadCount()];

    concurrency::parallel_for(uint(0), IdealThreadCount(), [&](uint i)
    {
        uint threadWorkStart = (workAmount * i);
        uint threadWorkStop = min((threadWorkStart + workAmount), m_MainBlocks.size());

        // Memory allocation optimization
        // TODO: Option to conserve memory
        threadFunctions[i].reserve(30000);

        // Execute
        AnalysisWorker(threadWorkStart, threadWorkStop, &threadFunctions[i]);
    });

    std::vector<FunctionDef> funcs;

    // Merge thread vectors into single local
    for(uint i = 0; i < IdealThreadCount(); i++)
        std::move(threadFunctions[i].begin(), threadFunctions[i].end(), std::back_inserter(funcs));

    // Sort and remove duplicates
    std::sort(funcs.begin(), funcs.end());
    funcs.erase(std::unique(funcs.begin(), funcs.end()), funcs.end());

    dprintf("%u functions\n", funcs.size());

    FunctionClear();
    for(auto & func : funcs)
    {
        FunctionAdd(func.VirtualStart, func.VirtualEnd - 1, true);
    }
    GuiUpdateAllViews();

    delete[] threadFunctions;
    return true;
}

void FunctionPass::AnalysisWorker(uint Start, uint End, std::vector<FunctionDef>* Blocks)
{
    // Step 1: Use any defined functions in the PE function table
    FindFunctionWorkerPrepass(Start, End, Blocks);

    // Step 2: for each block that contains a CALL flag,
    // add it to a local function start array
    //
    // NOTE: *Some* indirect calls are included
    auto blockItr = std::next(m_MainBlocks.begin(), Start);

    for(uint i = Start; i < End; i++, ++blockItr)
    {
        if(blockItr->GetFlag(BASIC_BLOCK_FLAG_CALL))
        {
            uint destination = blockItr->Target;

            // Was it a pointer?
            if(blockItr->GetFlag(BASIC_BLOCK_FLAG_INDIRPTR))
            {
                // Read it from memory
                if(!MemRead(destination, &destination, sizeof(uint)))
                    continue;

                // Validity check
                if(!MemIsValidReadPtr(destination))
                    continue;

                dprintf("Indirect pointer: 0x%p 0x%p\n", blockItr->Target, destination);
            }

            // Destination must be within analysis limits
            if(!ValidateAddress(destination))
                continue;

            FunctionDef def;
            def.VirtualStart = destination;
            def.VirtualEnd = 0;
            def.BBlockStart = 0;
            def.BBlockEnd = 0;
            Blocks->push_back(def);
        }
    }

    // Step 3: Sort and remove duplicates
    std::sort(Blocks->begin(), Blocks->end());
    Blocks->erase(std::unique(Blocks->begin(), Blocks->end()), Blocks->end());

    // Step 4: Find the end of functions
    FindFunctionWorker(Blocks);

    dprintf("Total detected functions: %d\n", Blocks->size());

    // Step 5: Find all orphaned blocks and repeat analysis process
    // TODO
}

void FunctionPass::FindFunctionWorkerPrepass(uint Start, uint End, std::vector<FunctionDef>* Blocks)
{
    const uint minFunc = std::next(m_MainBlocks.begin(), Start)->VirtualStart;
    const uint maxFunc = std::next(m_MainBlocks.begin(), End - 1)->VirtualEnd;

#ifdef _WIN64
    EnumerateFunctionRuntimeEntries64([&](PRUNTIME_FUNCTION Function)
    {
        const uint funcAddr = m_ModuleStart + Function->BeginAddress;
        const uint funcEnd = m_ModuleStart + Function->EndAddress;

        // If within limits...
        if(funcAddr >= minFunc && funcAddr < maxFunc)
        {
            // Add the descriptor
            FunctionDef def;
            def.VirtualStart = funcAddr;
            def.VirtualEnd = funcEnd;
            def.BBlockStart = 0;
            def.BBlockEnd = 0;
            Blocks->push_back(def);
        }

        return true;
    });
#endif // _WIN64
}

void FunctionPass::FindFunctionWorker(std::vector<FunctionDef>* Blocks)
{
    // Cached final block
    BasicBlock* finalBlock = &m_MainBlocks.back();

    // Enumerate all function entries for this thread
    for(auto & block : *Blocks)
    {
        // Sometimes the ending address is already supplied, so
        // check first
        if(block.VirtualEnd != 0)
        {
            if(ResolveKnownFunctionEnd(&block))
                continue;
        }

        // Now the function end must be determined by heuristics (find manually)
        ResolveFunctionEnd(&block, finalBlock);
    }
}

bool FunctionPass::ResolveKnownFunctionEnd(FunctionDef* Function)
{
    // Helper to link final blocks to function
    auto startBlock = FindBBlockInRange(Function->VirtualStart);
    auto endBlock = FindBBlockInRange(Function->VirtualEnd);

    if(!startBlock || !endBlock)
        return false;

    // Find block start/end indices
    Function->BBlockStart = FindBBlockIndex(startBlock);
    Function->BBlockEnd = FindBBlockIndex(endBlock);

    // Set the flag for blocks that have been scanned
    for(BasicBlock* block = startBlock; (uint)block <= (uint)endBlock; block++)
        block->SetFlag(BASIC_BLOCK_FLAG_FUNCTION);

    return true;
}

bool FunctionPass::ResolveFunctionEnd(FunctionDef* Function, BasicBlock* LastBlock)
{
    assert(Function->VirtualStart != 0);

    // Find the first basic block of the function
    BasicBlock* block = FindBBlockInRange(Function->VirtualStart);

    if(!block)
    {
        assert(false);
        return false;
    }

    // The maximum address is determined by any jump that extends past
    // a RET or other terminating basic block. A function may have multiple
    // return statements.
    uint maximumAddr = 0;

    // Loop forever until the end is found
    for(; (uint)block <= (uint)LastBlock; block++)
    {
        // Block is now in use
        block->SetFlag(BASIC_BLOCK_FLAG_FUNCTION);

        // Calculate max from just linear instructions
        maximumAddr = max(maximumAddr, block->VirtualEnd - 1);

        // Find maximum jump target
        if(!block->GetFlag(BASIC_BLOCK_FLAG_CALL) && !block->GetFlag(BASIC_BLOCK_FLAG_INDIRECT))
        {
            if(block->Target != 0)
            {
                // Here's a problem: Compilers add tail-call elimination with a jump.
                // Solve this by creating a maximum jump limit: +/- 512 bytes from the end.
                //
                // abs(block->VirtualEnd - block->Target) -- unsigned
                if(min(block->VirtualEnd - block->Target, block->Target - block->VirtualEnd) <= 512)
                    maximumAddr = max(maximumAddr, block->Target);
            }
        }

        // Sanity check
        assert(maximumAddr >= block->VirtualStart);

        // Does this node contain the maximum address?
        if(maximumAddr >= block->VirtualStart && maximumAddr < block->VirtualEnd)
        {
            // It does! There's 4 possibilities next:
            //
            // 1. Return
            // 2. Tail-call elimination
            // 3. Optimized loop
            // 4. Function continues to next block
            //
            // 1.
            if(block->GetFlag(BASIC_BLOCK_FLAG_RET))
                break;

            if(block->Target != 0)
            {
                // NOTE: Both must be an absolute jump
                if(block->GetFlag(BASIC_BLOCK_FLAG_ABSJMP))
                {
                    // 2.
                    //
                    // abs(block->VirtualEnd - block->Target) -- unsigned
                    if(min(block->VirtualEnd - block->Target, block->Target - block->VirtualEnd) > 128)
                        break;

                    // 3.
                    if(block->Target >= Function->VirtualStart && block->Target < block->VirtualEnd)
                        break;
                }
            }

            // 4. Continue
        }
    }

    // Loop is done. Set the information in the function structure.
    Function->VirtualEnd = block->VirtualEnd;
    Function->BBlockEnd = FindBBlockIndex(block);
    return true;
}

#ifdef _WIN64
void FunctionPass::EnumerateFunctionRuntimeEntries64(std::function<bool (PRUNTIME_FUNCTION)> Callback)
{
    if(!m_FunctionInfo)
        return;

    // Get the table pointer and size
    auto functionTable = (PRUNTIME_FUNCTION)m_FunctionInfo;
    ULONG totalCount = (m_FunctionInfoSize / sizeof(RUNTIME_FUNCTION));

    // Enumerate each entry
    for(ULONG i = 0; i < totalCount; i++)
    {
        if(!Callback(&functionTable[i]))
            break;
    }
}
#endif // _WIN64