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
    // EXCLUSIVE_ACQUIRE(LockSym);
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
SafeSymRegisterCallback64(
    __in HANDLE hProcess,
    __in PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
    __in ULONG64 UserContext
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
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
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymLoadModuleExW(hProcess, hFile, StringUtils::Utf8ToUtf16(ImageName).c_str(), StringUtils::Utf8ToUtf16(ModuleName).c_str(), BaseOfDll, DllSize, Data, Flags);
}
BOOL
SafeSymGetModuleInfo64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PIMAGEHLP_MODULE64 ModuleInfo
)
{
    EXCLUSIVE_ACQUIRE(LockSym);

    IMAGEHLP_MODULEW64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct = sizeof(modInfo);
    BOOL ret = SymGetModuleInfoW64(hProcess, qwAddr, &modInfo);

    ModuleInfo->SizeOfStruct = modInfo.SizeOfStruct;
    ModuleInfo->BaseOfImage = modInfo.BaseOfImage;
    ModuleInfo->ImageSize = modInfo.ImageSize;
    ModuleInfo->TimeDateStamp = modInfo.TimeDateStamp;
    ModuleInfo->CheckSum = modInfo.CheckSum;
    ModuleInfo->NumSyms = modInfo.NumSyms;
    ModuleInfo->SymType = modInfo.SymType;
    strcpy_s(ModuleInfo->ModuleName, 32, StringUtils::Utf16ToUtf8(modInfo.ModuleName).c_str());
    strcpy_s(ModuleInfo->ImageName, 256, StringUtils::Utf16ToUtf8(modInfo.ImageName).c_str());
    strcpy_s(ModuleInfo->LoadedImageName, 256, StringUtils::Utf16ToUtf8(modInfo.LoadedImageName).c_str());
    strcpy_s(ModuleInfo->LoadedPdbName, 256, StringUtils::Utf16ToUtf8(modInfo.LoadedPdbName).c_str());
    ModuleInfo->CVSig = modInfo.CVSig;
    strcpy_s(ModuleInfo->CVData, MAX_PATH * 3, StringUtils::Utf16ToUtf8(modInfo.CVData).c_str());
    ModuleInfo->PdbSig = modInfo.PdbSig;
    ModuleInfo->PdbSig70 = modInfo.PdbSig70;
    ModuleInfo->PdbAge = modInfo.PdbAge;
    ModuleInfo->PdbUnmatched = modInfo.PdbUnmatched;
    ModuleInfo->DbgUnmatched = modInfo.DbgUnmatched;
    ModuleInfo->LineNumbers = modInfo.LineNumbers;
    ModuleInfo->GlobalSymbols = modInfo.GlobalSymbols;
    ModuleInfo->TypeInfo = modInfo.TypeInfo;
    ModuleInfo->SourceIndexed = modInfo.SourceIndexed;
    ModuleInfo->Publics = modInfo.Publics;

    return ret;
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
SafeSymGetLineFromAddr64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PDWORD pdwDisplacement,
    __out PIMAGEHLP_LINE64 Line64
)
{
    EXCLUSIVE_ACQUIRE(LockSym);
    return SymGetLineFromAddr64(hProcess, qwAddr, pdwDisplacement, Line64);
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