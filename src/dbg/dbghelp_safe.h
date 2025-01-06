#ifndef _DBGHELP_SAFE_H
#define _DBGHELP_SAFE_H

#pragma warning(push)
#pragma warning(disable:4091)
#include <dbghelp.h>
#pragma warning(pop)
#include <sal.h>

void SafeDbghelpInitialize();
void SafeDbghelpDeinitialize();

BOOL
SafeSymUnloadModule64(
    _In_ HANDLE hProcess,
    _In_ DWORD64 BaseOfDll
);

BOOL
SafeSymSetSearchPathW(
    _In_ HANDLE hProcess,
    _In_opt_ PCWSTR SearchPath
);

DWORD
SafeSymSetOptions(
    _In_ DWORD   SymOptions
);

DWORD
SafeSymGetOptions(
);

DWORD64
SafeSymLoadModuleExW(
    _In_ HANDLE hProcess,
    _In_opt_ HANDLE hFile,
    _In_opt_ PCWSTR ImageName,
    _In_opt_ PCWSTR ModuleName,
    _In_ DWORD64 BaseOfDll,
    _In_ DWORD DllSize,
    _In_opt_ PMODLOAD_DATA Data,
    _In_opt_ DWORD Flags
);

BOOL
SafeSymGetModuleInfoW64(
    _In_ HANDLE hProcess,
    _In_ DWORD64 qwAddr,
    _Out_ PIMAGEHLP_MODULEW64 ModuleInfo
);

BOOL
SafeSymGetSearchPathW(
    _In_ HANDLE hProcess,
    _Out_ PWSTR SearchPath,
    _In_ DWORD SearchPathLength
);

BOOL
SafeStackWalk64(
    _In_ DWORD MachineType,
    _In_ HANDLE hProcess,
    _In_ HANDLE hThread,
    _Inout_ LPSTACKFRAME64 StackFrame,
    _Inout_ PVOID ContextRecord,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
);

#endif //_DBGHELP_SAFE_H