#ifndef _DBGHELP_SAFE_H
#define _DBGHELP_SAFE_H

#ifdef __GNUC__
#include "dbghelp\dbghelp.h"
#else
#include <dbghelp.h>
#endif //__GNUC__

DWORD
SafeUnDecorateSymbolName(
    __in PCSTR name,
    __out_ecount(maxStringLength) PSTR outputString,
    __in DWORD maxStringLength,
    __in DWORD flags
);
BOOL
SafeSymUnloadModule64(
    __in HANDLE hProcess,
    __in DWORD64 BaseOfDll
);
BOOL
SafeSymSetSearchPathW(
    __in HANDLE hProcess,
    __in_opt PCWSTR SearchPath
);
DWORD
SafeSymSetOptions(
    __in DWORD   SymOptions
);
DWORD
SafeSymGetOptions(
);
BOOL
SafeSymInitializeW(
    __in HANDLE hProcess,
    __in_opt PCWSTR UserSearchPath,
    __in BOOL fInvadeProcess
);
BOOL
SafeSymRegisterCallbackW64(
    __in HANDLE hProcess,
    __in PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
    __in ULONG64 UserContext
);
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
);
BOOL
SafeSymGetModuleInfoW64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PIMAGEHLP_MODULEW64 ModuleInfo
);
BOOL
SafeSymGetSearchPathW(
    __in HANDLE hProcess,
    __out_ecount(SearchPathLength) PWSTR SearchPath,
    __in DWORD SearchPathLength
);
BOOL
SafeSymEnumSymbols(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt PCSTR Mask,
    __in PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    __in_opt PVOID UserContext
);
BOOL
SafeSymEnumerateModules64(
    __in HANDLE hProcess,
    __in PSYM_ENUMMODULES_CALLBACK64 EnumModulesCallback,
    __in_opt PVOID UserContext
);
BOOL
SafeSymGetLineFromAddrW64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PDWORD pdwDisplacement,
    __out PIMAGEHLP_LINEW64 Line64
);
BOOL
SafeSymFromName(
    __in HANDLE hProcess,
    __in PCSTR Name,
    __inout PSYMBOL_INFO Symbol
);
BOOL
SafeSymFromAddr(
    __in HANDLE hProcess,
    __in DWORD64 Address,
    __out_opt PDWORD64 Displacement,
    __inout PSYMBOL_INFO Symbol
);
BOOL
SafeSymCleanup(
    __in HANDLE hProcess
);

#endif //_DBGHELP_SAFE_H