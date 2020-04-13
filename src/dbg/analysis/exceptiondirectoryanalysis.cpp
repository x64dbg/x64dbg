#include "exceptiondirectoryanalysis.h"
#include "module.h"
#include "TitanEngine/TitanEngine.h"
#include "memory.h"
#include "console.h"
#include "function.h"

ExceptionDirectoryAnalysis::ExceptionDirectoryAnalysis(duint base, duint size)
    : Analysis(base, size),
      mFunctionInfoSize(0),
      mFunctionInfoData(nullptr)
{
#ifdef _WIN64
    // This will only be valid if the address range is within a loaded module
    mModuleBase = ModBaseFromAddr(base);

    if(mModuleBase != 0)
    {
        char modulePath[MAX_PATH];
        memset(modulePath, 0, sizeof(modulePath));

        ModPathFromAddr(mModuleBase, modulePath, ARRAYSIZE(modulePath));

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
            auto virtualOffset = GetPE32DataFromMappedFile(fileMapVa, IMAGE_DIRECTORY_ENTRY_EXCEPTION, UE_SECTIONVIRTUALOFFSET);
            mFunctionInfoSize = duint(GetPE32DataFromMappedFile(fileMapVa, IMAGE_DIRECTORY_ENTRY_EXCEPTION, UE_SECTIONVIRTUALSIZE));

            // Unload the file
            StaticFileUnloadW(nullptr, false, fileHandle, fileSize, fileMapHandle, fileMapVa);

            // Get a copy of the function table
            if(virtualOffset)
            {
                // Read the table into a buffer
                mFunctionInfoData = emalloc(mFunctionInfoSize);

                if(mFunctionInfoData)
                    MemRead(virtualOffset + mModuleBase, mFunctionInfoData, mFunctionInfoSize);
            }
        }
    }
#endif //_WIN64
}

ExceptionDirectoryAnalysis::~ExceptionDirectoryAnalysis()
{
    if(mFunctionInfoData)
        efree(mFunctionInfoData);
}

void ExceptionDirectoryAnalysis::Analyse()
{
#ifdef _WIN64
    EnumerateFunctionRuntimeEntries64([&](PRUNTIME_FUNCTION Function)
    {
        auto funcAddr = mModuleBase + Function->BeginAddress;
        auto funcEnd = mModuleBase + Function->EndAddress;

        // If within limits...
        if(inRange(funcAddr) && inRange(funcEnd))
            mFunctions.push_back({ funcAddr, funcEnd });

        return true;
    });
    dprintf(QT_TRANSLATE_NOOP("DBG", "%u functions discovered!\n"), DWORD(mFunctions.size()));
#else //x86
    dputs(QT_TRANSLATE_NOOP("DBG", "This kind of analysis doesn't work on x86 executables...\n"));
#endif // _WIN64
}

void ExceptionDirectoryAnalysis::SetMarkers()
{
    FunctionDelRange(mBase, mBase + mSize - 1, false);
    for(const auto & function : mFunctions)
        FunctionAdd(function.first, function.second, false);
}

#ifdef _WIN64
void ExceptionDirectoryAnalysis::EnumerateFunctionRuntimeEntries64(const std::function<bool(PRUNTIME_FUNCTION)> & Callback) const
{
    if(!mFunctionInfoData)
        return;

    // Get the table pointer and size
    auto functionTable = PRUNTIME_FUNCTION(mFunctionInfoData);
    auto totalCount = mFunctionInfoSize / sizeof(RUNTIME_FUNCTION);

    // Enumerate each entry
    for(duint i = 0; i < totalCount; i++)
    {
        if(!Callback(&functionTable[i]))
            break;
    }
}
#endif // _WIN64