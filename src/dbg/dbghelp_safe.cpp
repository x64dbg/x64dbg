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
    __in PCSTR name,
    __out_ecount(maxStringLength) PSTR outputString,
    __in DWORD maxStringLength,
    __in DWORD flags
)
{
    STRONG_ACQUIRE();
    return UnDecorateSymbolName(name, outputString, maxStringLength, flags);
}
BOOL
SafeSymUnloadModule64(
    __in HANDLE hProcess,
    __in DWORD64 BaseOfDll
)
{
    STRONG_ACQUIRE();
    return SymUnloadModule64(hProcess, BaseOfDll);
}
BOOL
SafeSymSetSearchPathW(
    __in HANDLE hProcess,
    __in_opt PCWSTR SearchPath
)
{
    STRONG_ACQUIRE();
    return SymSetSearchPathW(hProcess, SearchPath);
}
DWORD
SafeSymSetOptions(
    __in DWORD   SymOptions
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
/*BOOL
SafeSymInitializeW(
    __in HANDLE hProcess,
    __in_opt PCWSTR UserSearchPath,
    __in BOOL fInvadeProcess
)
{
    STRONG_ACQUIRE();
    return SymInitializeW(hProcess, UserSearchPath, fInvadeProcess);
}*/
/*BOOL
SafeSymRegisterCallbackW64(
    __in HANDLE hProcess,
    __in PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
    __in ULONG64 UserContext
)
{
    STRONG_ACQUIRE();
    return SymRegisterCallbackW64(hProcess, CallbackFunction, UserContext);
}*/
DWORD64
SafeSymLoadModuleExW(
    __in HANDLE hProcess,
    __in_opt HANDLE hFile,
    __in_opt PCWSTR ImageName,
    __in_opt PCWSTR ModuleName,
    __in DWORD64 BaseOfDll,
    __in DWORD DllSize,
    __in_opt PMODLOAD_DATA Data,
    __in_opt DWORD Flags
)
{
    STRONG_ACQUIRE();
    return SymLoadModuleExW(hProcess, hFile, ImageName, ModuleName, BaseOfDll, DllSize, Data, Flags);
}
BOOL
SafeSymGetModuleInfoW64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PIMAGEHLP_MODULEW64 ModuleInfo
)
{
    STRONG_ACQUIRE();
    return SymGetModuleInfoW64(hProcess, qwAddr, ModuleInfo);
}
BOOL
SafeSymGetSearchPathW(
    __in HANDLE hProcess,
    __out_ecount(SearchPathLength) PWSTR SearchPath,
    __in DWORD SearchPathLength
)
{
    STRONG_ACQUIRE();
    return SymGetSearchPathW(hProcess, SearchPath, SearchPathLength);
}
/*BOOL
SafeSymEnumSymbols(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt PCSTR Mask,
    __in PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    __in_opt PVOID UserContext
)
{
    STRONG_ACQUIRE();
    return SymEnumSymbols(hProcess, BaseOfDll, Mask, EnumSymbolsCallback, UserContext);
}*/
/*BOOL
SafeSymGetLineFromAddrW64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PDWORD pdwDisplacement,
    __out PIMAGEHLP_LINEW64 Line64
)
{
    STRONG_ACQUIRE();
    return SymGetLineFromAddrW64(hProcess, qwAddr, pdwDisplacement, Line64);
}*/
/*BOOL
SafeSymFromName(
    __in HANDLE hProcess,
    __in PCSTR Name,
    __inout PSYMBOL_INFO Symbol
)
{
    STRONG_ACQUIRE();
    return SymFromName(hProcess, Name, Symbol);
}*/
/*BOOL
SafeSymFromAddr(
    __in HANDLE hProcess,
    __in DWORD64 Address,
    __out_opt PDWORD64 Displacement,
    __inout PSYMBOL_INFO Symbol
)
{
    STRONG_ACQUIRE();
    return SymFromAddr(hProcess, Address, Displacement, Symbol);
}*/
/*BOOL
SafeSymCleanup(
    __in HANDLE hProcess
)
{
    STRONG_ACQUIRE();
    return SymCleanup(hProcess);
}*/
BOOL
SafeStackWalk64(
    __in DWORD MachineType,
    __in HANDLE hProcess,
    __in HANDLE hThread,
    __inout LPSTACKFRAME64 StackFrame,
    __inout PVOID ContextRecord,
    __in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    __in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    __in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
)
{
    STRONG_ACQUIRE();
    return StackWalk64(MachineType, hProcess, hThread, StackFrame, ContextRecord, ReadMemoryRoutine, FunctionTableAccessRoutine, GetModuleBaseRoutine, TranslateAddress);
}
