#include "FunctionPass.h"
#include "capstone_wrapper.h"
#include <ppl.h>
#include "memory.h"
#include "console.h"
#include "debugger.h"
#include "module.h"

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

            dprintf("VA - 0x%p\n", virtualOffset);

            // Get a copy of the function table
            if(virtualOffset)
            {
                // Read the table into a buffer
                m_FunctionInfo = BridgeAlloc(m_FunctionInfoSize);

                if(m_FunctionInfo)
                    MemRead((PVOID)(virtualOffset + m_ModuleStart), m_FunctionInfo, m_FunctionInfoSize, nullptr);
            }
        }
    }

    dprintf("Function info: 0x%p - 0x%p\n", m_FunctionInfo, m_FunctionInfoSize);
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
    // Step 1: Use any defined functions in the PE function table
    FindFunctionWorkerPrepass(Start, End, Blocks);

    // Step 2: for each block that contains a CALL flag,
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

                dprintf("Indirect pointer: 0x%p 0x%p\n", blockItr->Target, destination);
            }

            // Destination must be within analysis limits
            if(!ValidateAddress(destination))
                continue;

            FunctionDef def;
            def.VirtualStart = destination;
            def.VirtualEnd = destination;
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
    uint minFunc = std::next(m_MainBlocks.begin(), Start)->VirtualStart;
    uint maxFunc = std::next(m_MainBlocks.begin(), End - 1)->VirtualEnd;

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
}

void FunctionPass::FindFunctionWorker(std::vector<FunctionDef>* Blocks)
{
}

void FunctionPass::EnumerateFunctionRuntimeEntries64(std::function<bool (PRUNTIME_FUNCTION)> Callback)
{
    if(!m_FunctionInfo)
        return;

    // Get the table pointer
    PRUNTIME_FUNCTION functionTable = (PRUNTIME_FUNCTION)m_FunctionInfo;

    // Enumerate each entry
    ULONG totalCount = (m_FunctionInfoSize / sizeof(RUNTIME_FUNCTION));

    for(ULONG i = 0; i < totalCount; i++)
    {
        if(!Callback(&functionTable[i]))
            break;
    }
}