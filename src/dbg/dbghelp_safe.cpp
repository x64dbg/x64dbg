#include "_global.h"
#include "dbghelp_safe.h"
#include "threading.h"

DWORD
SafeUnDecorateSymbolName(
    __in PCSTR name,
    __out_ecount(maxStringLength) PSTR outputString,
    __in DWORD maxStringLength,
    __in DWORD flags
)
{
    // NOTE: Disabled because of potential recursive deadlocks
    EXCLUSIVE_ACQUIRE(LockSym);
    return UnDecorateSymbolName(name, outputString, maxStringLength, flags);
}
BOOL
SafeSymUnloadModule64(
    __in HANDLE hProcess,
    __in DWORD64 BaseOfDll
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymUnloadModule64(hProcess, BaseOfDll);
}
BOOL
SafeSymSetSearchPathW(
    __in HANDLE hProcess,
    __in_opt PCWSTR SearchPath
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymSetSearchPathW(hProcess, SearchPath);
}
DWORD
SafeSymSetOptions(
    __in DWORD   SymOptions
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymSetOptions(SymOptions);
}
DWORD
SafeSymGetOptions(
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymGetOptions();
}
BOOL
SafeSymInitializeW(
    __in HANDLE hProcess,
    __in_opt PCWSTR UserSearchPath,
    __in BOOL fInvadeProcess
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymInitializeW(hProcess, UserSearchPath, fInvadeProcess);
}
BOOL
SafeSymRegisterCallbackW64(
    __in HANDLE hProcess,
    __in PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
    __in ULONG64 UserContext
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymRegisterCallbackW64(hProcess, CallbackFunction, UserContext);
}
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
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymLoadModuleExW(hProcess, hFile, ImageName, ModuleName, BaseOfDll, DllSize, Data, Flags);
}
BOOL
SafeSymGetModuleInfoW64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PIMAGEHLP_MODULEW64 ModuleInfo
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymGetModuleInfoW64(hProcess, qwAddr, ModuleInfo);
}
BOOL
SafeSymGetSearchPathW(
    __in HANDLE hProcess,
    __out_ecount(SearchPathLength) PWSTR SearchPath,
    __in DWORD SearchPathLength
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymGetSearchPathW(hProcess, SearchPath, SearchPathLength);
}
BOOL
SafeSymEnumSymbols(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt PCSTR Mask,
    __in PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    __in_opt PVOID UserContext
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymEnumSymbols(hProcess, BaseOfDll, Mask, EnumSymbolsCallback, UserContext);
}
BOOL
SafeSymEnumerateModules64(
    __in HANDLE hProcess,
    __in PSYM_ENUMMODULES_CALLBACK64 EnumModulesCallback,
    __in_opt PVOID UserContext
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymEnumerateModules64(hProcess, EnumModulesCallback, UserContext);
}
BOOL
SafeSymGetLineFromAddrW64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PDWORD pdwDisplacement,
    __out PIMAGEHLP_LINEW64 Line64
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymGetLineFromAddrW64(hProcess, qwAddr, pdwDisplacement, Line64);
}
BOOL
SafeSymFromName(
    __in HANDLE hProcess,
    __in PCSTR Name,
    __inout PSYMBOL_INFO Symbol
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymFromName(hProcess, Name, Symbol);
}
BOOL
SafeSymFromAddr(
    __in HANDLE hProcess,
    __in DWORD64 Address,
    __out_opt PDWORD64 Displacement,
    __inout PSYMBOL_INFO Symbol
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymFromAddr(hProcess, Address, Displacement, Symbol);
}
BOOL
SafeSymCleanup(
    __in HANDLE hProcess
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymCleanup(hProcess);
}