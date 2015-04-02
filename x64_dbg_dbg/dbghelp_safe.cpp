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
    CriticalSectionLocker locker(LockSym);
    return UnDecorateSymbolName(name, outputString, maxStringLength, flags);
}
BOOL
SafeSymUnloadModule64(
    __in HANDLE hProcess,
    __in DWORD64 BaseOfDll
)
{
    CriticalSectionLocker locker(LockSym);
    return SymUnloadModule64(hProcess, BaseOfDll);
}
BOOL
SafeSymSetSearchPath(
    __in HANDLE hProcess,
    __in_opt PCSTR SearchPath
)
{
    CriticalSectionLocker locker(LockSym);
    return SymSetSearchPath(hProcess, SearchPath);
}
DWORD
SafeSymSetOptions(
    __in DWORD   SymOptions
)
{
    CriticalSectionLocker locker(LockSym);
    return SymSetOptions(SymOptions);
}
BOOL
SafeSymInitialize(
    __in HANDLE hProcess,
    __in_opt PCSTR UserSearchPath,
    __in BOOL fInvadeProcess
)
{
    CriticalSectionLocker locker(LockSym);
    return SymInitialize(hProcess, UserSearchPath, fInvadeProcess);
}
BOOL
SafeSymRegisterCallback64(
    __in HANDLE hProcess,
    __in PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
    __in ULONG64 UserContext
)
{
    CriticalSectionLocker locker(LockSym);
    return SymRegisterCallback64(hProcess, CallbackFunction, UserContext);
}
DWORD64
SafeSymLoadModuleEx(
    __in HANDLE hProcess,
    __in_opt HANDLE hFile,
    __in_opt PCSTR ImageName,
    __in_opt PCSTR ModuleName,
    __in DWORD64 BaseOfDll,
    __in DWORD DllSize,
    __in_opt PMODLOAD_DATA Data,
    __in_opt DWORD Flags
)
{
    CriticalSectionLocker locker(LockSym);
    return SymLoadModuleEx(hProcess, hFile, ImageName, ModuleName, BaseOfDll, DllSize, Data, Flags);
}
BOOL
SafeSymGetModuleInfo64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PIMAGEHLP_MODULE64 ModuleInfo
)
{
    CriticalSectionLocker locker(LockSym);
    return SymGetModuleInfo64(hProcess, qwAddr, ModuleInfo);
}
BOOL
SafeSymGetSearchPath(
    __in HANDLE hProcess,
    __out_ecount(SearchPathLength) PSTR SearchPath,
    __in DWORD SearchPathLength
)
{
    CriticalSectionLocker locker(LockSym);
    return SymGetSearchPath(hProcess, SearchPath, SearchPathLength);
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
    CriticalSectionLocker locker(LockSym);
    return SymEnumSymbols(hProcess, BaseOfDll, Mask, EnumSymbolsCallback, UserContext);
}
BOOL
SafeSymEnumerateModules(
    __in HANDLE hProcess,
    __in PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,
    __in_opt PVOID UserContext
)
{
    CriticalSectionLocker locker(LockSym);
    return SymEnumerateModules(hProcess, EnumModulesCallback, UserContext);
}
BOOL
SafeSymGetLineFromAddr64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PDWORD pdwDisplacement,
    __out PIMAGEHLP_LINE64 Line64
)
{
    CriticalSectionLocker locker(LockSym);
    return SymGetLineFromAddr64(hProcess, qwAddr, pdwDisplacement, Line64);
}
BOOL
SafeSymFromName(
    __in HANDLE hProcess,
    __in PCSTR Name,
    __inout PSYMBOL_INFO Symbol
)
{
    CriticalSectionLocker locker(LockSym);
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
    CriticalSectionLocker locker(LockSym);
    return SymFromAddr(hProcess, Address, Displacement, Symbol);
}
BOOL
SafeSymCleanup(
    __in HANDLE hProcess
)
{
    CriticalSectionLocker locker(LockSym);
    return SymCleanup(hProcess);
}