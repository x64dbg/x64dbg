#include "_global.h"
#include "dbghelp_safe.h"

static CRITICAL_SECTION criticalSection;

struct Lock
{
    explicit Lock(bool weak)
    {
        if(weak)
            success = !!TryEnterCriticalSection(&criticalSection);
        else
        {
            EnterCriticalSection(&criticalSection);
            success = true;
        }
    }

    ~Lock()
    {
        if(success)
            LeaveCriticalSection(&criticalSection);
    }

    bool success;
};

#define WEAK_ACQUIRE() Lock __lock(true); if(!__lock.success) return 0;
#define STRONG_ACQUIRE() Lock __lock(false);

void SafeDbghelpInitialize()
{
    InitializeCriticalSection(&criticalSection);
}

void SafeDbghelpDeinitialize()
{
    DeleteCriticalSection(&criticalSection);
}

DWORD
SafeUnDecorateSymbolName(
    _In_ PCSTR name,
    _Out_ PSTR outputString,
    _In_ DWORD maxStringLength,
    _In_ DWORD flags
)
{
    STRONG_ACQUIRE();
    return UnDecorateSymbolName(name, outputString, maxStringLength, flags);
}

BOOL
SafeSymUnloadModule64(
    _In_ HANDLE hProcess,
    _In_ DWORD64 BaseOfDll
)
{
    STRONG_ACQUIRE();
    return SymUnloadModule64(hProcess, BaseOfDll);
}

BOOL
SafeSymSetSearchPathW(
    _In_ HANDLE hProcess,
    __in_opt PCWSTR SearchPath
)
{
    STRONG_ACQUIRE();
    return SymSetSearchPathW(hProcess, SearchPath);
}

DWORD
SafeSymSetOptions(
    _In_ DWORD   SymOptions
)
{
    STRONG_ACQUIRE();
    return SymSetOptions(SymOptions);
}

DWORD
SafeSymGetOptions(
)
{
    STRONG_ACQUIRE();
    return SymGetOptions();
}

DWORD64
SafeSymLoadModuleExW(
    _In_ HANDLE hProcess,
    __in_opt HANDLE hFile,
    __in_opt PCWSTR ImageName,
    __in_opt PCWSTR ModuleName,
    _In_ DWORD64 BaseOfDll,
    _In_ DWORD DllSize,
    __in_opt PMODLOAD_DATA Data,
    __in_opt DWORD Flags
)
{
    STRONG_ACQUIRE();
    return SymLoadModuleExW(hProcess, hFile, ImageName, ModuleName, BaseOfDll, DllSize, Data, Flags);
}

BOOL
SafeSymGetModuleInfoW64(
    _In_ HANDLE hProcess,
    _In_ DWORD64 qwAddr,
    _Out_ PIMAGEHLP_MODULEW64 ModuleInfo
)
{
    STRONG_ACQUIRE();
    return SymGetModuleInfoW64(hProcess, qwAddr, ModuleInfo);
}

BOOL
SafeSymGetSearchPathW(
    _In_ HANDLE hProcess,
    __out_ecount(SearchPathLength) PWSTR SearchPath,
    _In_ DWORD SearchPathLength
)
{
    STRONG_ACQUIRE();
    return SymGetSearchPathW(hProcess, SearchPath, SearchPathLength);
}

BOOL
SafeStackWalk64(
    _In_ DWORD MachineType,
    _In_ HANDLE hProcess,
    _In_ HANDLE hThread,
    _Inout_ LPSTACKFRAME64 StackFrame,
    _Inout_ PVOID ContextRecord,
    __in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    __in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    __in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
)
{
    STRONG_ACQUIRE();
    return StackWalk64(MachineType, hProcess, hThread, StackFrame, ContextRecord, ReadMemoryRoutine, FunctionTableAccessRoutine, GetModuleBaseRoutine, TranslateAddress);
}
