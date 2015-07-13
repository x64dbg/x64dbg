#include "exceptiondirectoryanalysis.h"
#include "module.h"
#include "TitanEngine/TitanEngine.h"
#include "memory.h"
#include "console.h"
#include "function.h"

ExceptionDirectoryAnalysis::ExceptionDirectoryAnalysis(uint base, uint size) : Analysis(base, size)
{
    _functionInfoData = nullptr;
#ifdef _WIN64
    // This will only be valid if the address range is within a loaded module
    _moduleBase = ModBaseFromAddr(base);

    if(_moduleBase != 0)
    {
        char modulePath[MAX_PATH];
        memset(modulePath, 0, sizeof(modulePath));

        ModPathFromAddr(_moduleBase, modulePath, ARRAYSIZE(modulePath));

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
            _functionInfoSize = (uint)GetPE32DataFromMappedFile(fileMapVa, IMAGE_DIRECTORY_ENTRY_EXCEPTION, UE_SECTIONVIRTUALSIZE);

            // Unload the file
            StaticFileUnloadW(nullptr, false, fileHandle, fileSize, fileMapHandle, fileMapVa);

            // Get a copy of the function table
            if(virtualOffset)
            {
                // Read the table into a buffer
                _functionInfoData = emalloc(_functionInfoSize);

                if(_functionInfoData)
                    MemRead(virtualOffset + _moduleBase, _functionInfoData, _functionInfoSize);
            }
        }
    }
#endif //_WIN64
}

ExceptionDirectoryAnalysis::~ExceptionDirectoryAnalysis()
{
    if(_functionInfoData)
        efree(_functionInfoData);
}

void ExceptionDirectoryAnalysis::Analyse()
{
#ifdef _WIN64
    EnumerateFunctionRuntimeEntries64([&](PRUNTIME_FUNCTION Function)
    {
        const uint funcAddr = _moduleBase + Function->BeginAddress;
        const uint funcEnd = _moduleBase + Function->EndAddress;

        // If within limits...
        if(funcAddr >= _base && funcAddr < _base + _size)
            _functions.push_back({ funcAddr, funcEnd });

        return true;
    });
    dprintf("%u functions discovered!\n", _functions.size());
#else //x32
    dprintf("This kind of analysis doesn't work on x32 executables...\n");
#endif // _WIN64
}

void ExceptionDirectoryAnalysis::SetMarkers()
{
    FunctionDelRange(_base, _base + _size);
    for(const auto & function : _functions)
        FunctionAdd(function.first, function.second, false);
}

#ifdef _WIN64
void ExceptionDirectoryAnalysis::EnumerateFunctionRuntimeEntries64(std::function<bool(PRUNTIME_FUNCTION)> Callback)
{
    if(!_functionInfoData)
        return;

    // Get the table pointer and size
    auto functionTable = (PRUNTIME_FUNCTION)_functionInfoData;
    uint totalCount = (_functionInfoSize / sizeof(RUNTIME_FUNCTION));

    // Enumerate each entry
    for(uint i = 0; i < totalCount; i++)
    {
        if(!Callback(&functionTable[i]))
            break;
    }
}
#endif // _WIN64