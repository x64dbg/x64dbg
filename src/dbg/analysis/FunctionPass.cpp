#include "FunctionPass.h"
#include "memory.h"
#include "console.h"
#include "debugger.h"
#include "module.h"
#include "function.h"

FunctionPass::FunctionPass(duint VirtualStart, duint VirtualEnd, BBlockArray & MainBlocks)
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
    duint workAmount = (m_MainBlocks.size() + (IdealThreadCount() - 1)) / IdealThreadCount();

    // Initialize thread vector
    auto threadFunctions = new std::vector<FunctionDef>[IdealThreadCount()];

    ParallelFor(IdealThreadCount(), [&](duint i)
    {
        // Memory allocation optimization
        // TODO: Option to conserve memory
        threadFunctions[i].reserve(30000);

        // Execute
        duint threadWorkStart = (workAmount * i);
        duint threadWorkStop = std::min<duint>((threadWorkStart + workAmount), m_MainBlocks.size());

        AnalysisWorker(threadWorkStart, threadWorkStop, &threadFunctions[i]);
    });

    // Merge thread vectors into single local
    std::vector<FunctionDef> funcs;

    for(duint i = 0; i < IdealThreadCount(); i++)
        std::move(threadFunctions[i].begin(), threadFunctions[i].end(), std::back_inserter(funcs));

    // Sort and remove duplicates
    std::sort(funcs.begin(), funcs.end());
    funcs.erase(std::unique(funcs.begin(), funcs.end()), funcs.end());

    dprintf(QT_TRANSLATE_NOOP("DBG", "%u functions\n"), DWORD(funcs.size()));

    FunctionDelRange(m_VirtualStart, m_VirtualEnd - 1, false);
    for(auto & func : funcs)
    {
        FunctionAdd(func.VirtualStart, func.VirtualEnd, false, func.InstrCount);
    }
    GuiUpdateAllViews();

    delete[] threadFunctions;
    return true;
}

void FunctionPass::AnalysisWorker(duint Start, duint End, std::vector<FunctionDef>* Blocks)
{
    //
    // Step 1: Use any defined functions in the PE function table
    //
    FindFunctionWorkerPrepass(Start, End, Blocks);

    //
    // Step 2: for each block that contains a CALL flag,
    // add it to a local function start array
    //
    // NOTE: *Some* indirect calls are included
    auto blockItr = std::next(m_MainBlocks.begin(), Start);

    for(duint i = Start; i < End; i++, ++blockItr)
    {
        if(blockItr->GetFlag(BASIC_BLOCK_FLAG_CALL))
        {
            duint destination = blockItr->Target;

            // Was it a pointer?
            if(blockItr->GetFlag(BASIC_BLOCK_FLAG_INDIRPTR))
            {
                // Read it from memory
                if(!MemRead(destination, &destination, sizeof(duint)))
                    continue;

                // Validity check
                if(!MemIsValidReadPtr(destination))
                    continue;

                dprintf(QT_TRANSLATE_NOOP("DBG", "Indirect pointer: 0x%p 0x%p\n"), blockItr->Target, destination);
            }

            // Destination must be within analysis limits
            if(!ValidateAddress(destination))
                continue;

            Blocks->push_back({ destination, 0, 0, 0, 0 });
        }
    }

    //
    // Step 3: Sort and remove duplicates
    //
    std::sort(Blocks->begin(), Blocks->end());
    Blocks->erase(std::unique(Blocks->begin(), Blocks->end()), Blocks->end());

    //
    // Step 4: Find function ends
    //
    FindFunctionWorker(Blocks);

    //
    // Step 5: Find all orphaned blocks and repeat analysis process
    //
    // Starting from the first global block, scan until an "untouched" block is found
    blockItr = std::next(m_MainBlocks.begin(), Start);

    // Cached final block
    BasicBlock* finalBlock = &m_MainBlocks.back();

    duint virtEnd = 0;
    for(duint i = Start; i < End; i++, ++blockItr)
    {
        if(blockItr->VirtualStart < virtEnd)
            continue;

        // Skip padding
        if(blockItr->GetFlag(BASIC_BLOCK_FLAG_PAD))
            continue;

        // Is the block untouched?
        if(blockItr->GetFlag(BASIC_BLOCK_FLAG_FUNCTION))
            continue;

        // Try to define a function
        FunctionDef def { blockItr->VirtualStart, 0, 0, 0, 0 };

        if(ResolveFunctionEnd(&def, finalBlock))
        {
            Blocks->push_back(def);
            virtEnd = def.VirtualEnd;
        }
    }
}

void FunctionPass::FindFunctionWorkerPrepass(duint Start, duint End, std::vector<FunctionDef>* Blocks)
{
    return;
    const duint minFunc = std::next(m_MainBlocks.begin(), Start)->VirtualStart;
    const duint maxFunc = std::next(m_MainBlocks.begin(), End - 1)->VirtualEnd;

#ifdef _WIN64
    // RUNTIME_FUNCTION exception information
    EnumerateFunctionRuntimeEntries64([&](PRUNTIME_FUNCTION Function)
    {
        const duint funcAddr = m_ModuleStart + Function->BeginAddress;
        const duint funcEnd = m_ModuleStart + Function->EndAddress;

        // If within limits...
        if(funcAddr >= minFunc && funcAddr < maxFunc)
        {
            // Add the descriptor (virtual start/end)
            Blocks->push_back({ funcAddr, funcEnd, 0, 0, 0 });
        }

        return true;
    });
#endif // _WIN64

    // Module exports (return value ignored)
    apienumexports(m_ModuleStart, [&](duint Base, const char* Module, const char* Name, duint Address)
    {
        // If within limits...
        if(Address >= minFunc && Address < maxFunc)
        {
            // Add the descriptor (virtual start)
            Blocks->push_back({ Address, 0, 0, 0, 0 });
        }
    });
}

void FunctionPass::FindFunctionWorker(std::vector<FunctionDef>* Blocks)
{
    // Cached final block
    BasicBlock* finalBlock = &m_MainBlocks.back();

    // Enumerate all function entries for this thread
    for(auto & block : *Blocks)
    {
        // Sometimes the ending address is already supplied, so check first
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
    for(BasicBlock* block = startBlock; (duint)block <= (duint)endBlock; block++)
    {
        // Block now in use
        block->SetFlag(BASIC_BLOCK_FLAG_FUNCTION);

        // Counter
        Function->InstrCount += block->InstrCount;
    }

    return true;
}

bool FunctionPass::ResolveFunctionEnd(FunctionDef* Function, BasicBlock* LastBlock)
{
    ASSERT_TRUE(Function->VirtualStart != 0);

    // Find the first basic block of the function
    BasicBlock* block = FindBBlockInRange(Function->VirtualStart);

    if(!block)
    {
        ASSERT_ALWAYS("Block should exist at this point");
        return false;
    }

    // The maximum address is determined by any jump that extends past
    // a RET or other terminating basic block. A function may have multiple
    // return statements.
    duint maximumAddr = 0;

    // Loop forever until the end is found
    for(; (duint)block <= (duint)LastBlock; block++)
    {
        if(block->GetFlag(BASIC_BLOCK_FLAG_CALL_TARGET) && block->VirtualStart != Function->VirtualStart)
        {
            block--;
            break;
        }

        // Block is now in use
        block->SetFlag(BASIC_BLOCK_FLAG_FUNCTION);

        // Increment instruction count
        Function->InstrCount += block->InstrCount;

        // Calculate max from just linear instructions
        maximumAddr = (std::max)(maximumAddr, block->VirtualEnd);

        // Find maximum jump target
        if(!block->GetFlag(BASIC_BLOCK_FLAG_CALL) && !block->GetFlag(BASIC_BLOCK_FLAG_INDIRECT))
        {
            if(block->Target != 0 && block->Target >= maximumAddr)
            {
                // Here's a problem: Compilers add tail-call elimination with a jump.
                // Solve this by creating a maximum jump limit.
                auto targetBlock = FindBBlockInRange(block->Target);

                // If (target block found) and (target block is not called)
                if(targetBlock && !targetBlock->GetFlag(BASIC_BLOCK_FLAG_CALL_TARGET))
                {
                    duint blockEnd = targetBlock->VirtualEnd;

                    //
                    // Edge case when a compiler emits:
                    //
                    // pop ebp
                    // jmp some_func
                    // int3
                    // int3
                    //                  some_func:
                    //  push ebp
                    //
                    // Where INT3 will align "some_func" to 4, 8, 12, or 16.
                    // INT3 padding is also optional (if the jump fits perfectly).
                    //
                    if(true/*block->GetFlag(BASIC_BLOCK_FLAG_ABSJMP)*/)
                    {

                        {
                            // Check if padding is aligned to 4
                            auto nextBlock = block + 1;

                            if((duint)nextBlock <= (duint)LastBlock)
                            {
                                if(nextBlock->GetFlag(BASIC_BLOCK_FLAG_PAD))
                                {
                                    // If this block is aligned to 4 bytes at the end
                                    if((nextBlock->VirtualEnd + 1) % 4 == 0)
                                        blockEnd = block->VirtualEnd;
                                }
                            }
                        }
                    }

                    // Now calculate the maximum end address, taking into account the jump destination
                    maximumAddr = (std::max)(maximumAddr, blockEnd);
                }
            }
        }

        // Coherence check
        ASSERT_TRUE(maximumAddr >= block->VirtualStart);

        // Does this node contain the maximum address?
        if(maximumAddr >= block->VirtualStart && maximumAddr <= block->VirtualEnd)
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
                    if(block->VirtualEnd == maximumAddr)
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
void FunctionPass::EnumerateFunctionRuntimeEntries64(const std::function<bool(PRUNTIME_FUNCTION)> & Callback)
{
    if(!m_FunctionInfo)
        return;

    // Get the table pointer and size
    auto functionTable = (PRUNTIME_FUNCTION)m_FunctionInfo;
    size_t totalCount = (m_FunctionInfoSize / sizeof(RUNTIME_FUNCTION));

    // Enumerate each entry
    for(size_t i = 0; i < totalCount; i++)
    {
        if(!Callback(&functionTable[i]))
            break;
    }
}
#endif // _WIN64