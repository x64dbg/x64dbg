/**
@file symbolinfo.cpp

@brief Implements the symbolinfo class.
*/

#include "symbolinfo.h"
#include "debugger.h"
#include "console.h"
#include "module.h"
#include "addrinfo.h"

struct SYMBOLCBDATA
{
    CBSYMBOLENUM cbSymbolEnum;
    void* user;
    std::vector<char> decoratedSymbol;
    std::vector<char> undecoratedSymbol;
};

BOOL CALLBACK EnumSymbols(PSYMBOL_INFO SymInfo, ULONG SymbolSize, PVOID UserContext)
{
    SYMBOLCBDATA* cbData = (SYMBOLCBDATA*)UserContext;
    cbData->decoratedSymbol[0] = '\0';
    cbData->undecoratedSymbol[0] = '\0';

    SYMBOLINFO curSymbol;
    memset(&curSymbol, 0, sizeof(SYMBOLINFO));

    curSymbol.addr = (duint)SymInfo->Address;
    curSymbol.decoratedSymbol = cbData->decoratedSymbol.data();
    curSymbol.undecoratedSymbol = cbData->undecoratedSymbol.data();
    strncpy_s(curSymbol.decoratedSymbol, MAX_SYM_NAME, SymInfo->Name, _TRUNCATE);

    // Skip bad ordinals
    if(strstr(SymInfo->Name, "Ordinal"))
    {
        // Does the symbol point to the module base?
        if(SymInfo->Address == SymInfo->ModBase)
            return TRUE;
    }

    // Convert a mangled/decorated C++ name to a readable format
    if(!SafeUnDecorateSymbolName(SymInfo->Name, curSymbol.undecoratedSymbol, MAX_SYM_NAME, UNDNAME_COMPLETE))
        curSymbol.undecoratedSymbol = nullptr;
    else if(!strcmp(curSymbol.decoratedSymbol, curSymbol.undecoratedSymbol))
        curSymbol.undecoratedSymbol = nullptr;

    cbData->cbSymbolEnum(&curSymbol, cbData->user);
    return TRUE;
}

void SymEnumImports(duint Base, CBSYMBOLENUM EnumCallback, void* UserData)
{
    SYMBOLINFO symbol;
    memset(&symbol, 0, sizeof(SYMBOLINFO));
    symbol.isImported = true;
    apienumimports(Base, [&](duint base, duint addr, char* name, char* moduleName)
    {
        symbol.addr = addr;
        symbol.decoratedSymbol = name;
        EnumCallback(&symbol, UserData);
    });
}

void SymEnum(duint Base, CBSYMBOLENUM EnumCallback, void* UserData)
{
    SYMBOLCBDATA symbolCbData;
    symbolCbData.cbSymbolEnum = EnumCallback;
    symbolCbData.user = UserData;
    symbolCbData.decoratedSymbol.resize(MAX_SYM_NAME + 1);
    symbolCbData.undecoratedSymbol.resize(MAX_SYM_NAME + 1);

    // Enumerate every single symbol for the module in 'base'
    if(!SafeSymEnumSymbols(fdProcessInfo->hProcess, Base, "*", EnumSymbols, &symbolCbData))
        dputs(QT_TRANSLATE_NOOP("DBG", "SymEnumSymbols failed!"));

    // Emit pseudo entry point symbol
    SYMBOLINFO symbol;
    memset(&symbol, 0, sizeof(SYMBOLINFO));
    symbol.decoratedSymbol = "OptionalHeader.AddressOfEntryPoint";
    symbol.addr = ModEntryFromAddr(Base);
    if(symbol.addr)
        EnumCallback(&symbol, UserData);

    SymEnumImports(Base, EnumCallback, UserData);
}

void SymEnumFromCache(duint Base, CBSYMBOLENUM EnumCallback, void* UserData)
{
    SymEnum(Base, EnumCallback, UserData);
}

bool SymGetModuleList(std::vector<SYMBOLMODULEINFO>* List)
{
    //
    // Inline lambda enum
    //
    auto EnumModules = [](LPCTSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext) -> BOOL
    {
        SYMBOLMODULEINFO curModule;
        curModule.base = (duint)BaseOfDll;

        // Terminate module name if one isn't found
        if(!ModNameFromAddr(curModule.base, curModule.name, true))
            curModule.name[0] = '\0';

        ((std::vector<SYMBOLMODULEINFO>*)UserContext)->push_back(curModule);
        return TRUE;
    };

    // Execute the symbol enumerator (Force cast to STDCALL)
    if(!SafeSymEnumerateModules64(fdProcessInfo->hProcess, EnumModules, List))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "SymEnumerateModules64 failed!"));
        return false;
    }

    return true;
}

void SymUpdateModuleList()
{
    // Build the vector of modules
    std::vector<SYMBOLMODULEINFO> modList;

    if(!SymGetModuleList(&modList))
    {
        GuiSymbolUpdateModuleList(0, nullptr);
        return;
    }

    // Create a new array to be sent to the GUI thread
    size_t moduleCount = modList.size();
    SYMBOLMODULEINFO* data = (SYMBOLMODULEINFO*)BridgeAlloc(moduleCount * sizeof(SYMBOLMODULEINFO));

    // Direct copy from std::vector data
    memcpy(data, modList.data(), moduleCount * sizeof(SYMBOLMODULEINFO));

    // Send the module data to the GUI for updating
    GuiSymbolUpdateModuleList((int)moduleCount, data);
}

void SymDownloadAllSymbols(const char* SymbolStore)
{
    // Default to Microsoft's symbol server
    if(!SymbolStore)
        SymbolStore = "http://msdl.microsoft.com/download/symbols";

    // Build the vector of modules
    std::vector<SYMBOLMODULEINFO> modList;

    if(!SymGetModuleList(&modList))
        return;

    // Skip loading if there aren't any found modules
    if(modList.size() <= 0)
        return;

    // Backup the current symbol search path
    wchar_t oldSearchPath[MAX_PATH];

    if(!SafeSymGetSearchPathW(fdProcessInfo->hProcess, oldSearchPath, MAX_PATH))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "SymGetSearchPathW failed!"));
        return;
    }

    // Use the custom server path and directory
    char customSearchPath[MAX_PATH * 2];
    sprintf_s(customSearchPath, "SRV*%s*%s", szSymbolCachePath, SymbolStore);

    if(!SafeSymSetSearchPathW(fdProcessInfo->hProcess, StringUtils::Utf8ToUtf16(customSearchPath).c_str()))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "SymSetSearchPathW (1) failed!"));
        return;
    }

    auto symOptions = SafeSymGetOptions();
    SafeSymSetOptions(symOptions & ~SYMOPT_IGNORE_CVREC);

    // Reload
    for(auto & module : modList)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Downloading symbols for %s...\n"), module.name);

        wchar_t modulePath[MAX_PATH];
        if(!GetModuleFileNameExW(fdProcessInfo->hProcess, (HMODULE)module.base, modulePath, MAX_PATH))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "GetModuleFileNameExW(%p) failed!\n"), module.base);
            continue;
        }

        if(!SafeSymUnloadModule64(fdProcessInfo->hProcess, (DWORD64)module.base))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "SymUnloadModule64(%p) failed!\n"), module.base);
            continue;
        }

        if(!SymLoadModuleExW(fdProcessInfo->hProcess, 0, modulePath, 0, (DWORD64)module.base, 0, 0, 0))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "SymLoadModuleEx(%p) failed!\n"), module.base);
            continue;
        }
    }

    SafeSymSetOptions(symOptions);

    // Restore the old search path
    if(!SafeSymSetSearchPathW(fdProcessInfo->hProcess, oldSearchPath))
        dputs(QT_TRANSLATE_NOOP("DBG", "SymSetSearchPathW (2) failed!"));
}

bool SymAddrFromName(const char* Name, duint* Address)
{
    if(!Name || Name[0] == '\0')
        return false;

    if(!Address)
        return false;

    // Skip 'OrdinalXXX'
    if(!_strnicmp(Name, "Ordinal", 7))
        return false;

    // According to MSDN:
    // Note that the total size of the data is the SizeOfStruct + (MaxNameLen - 1) * sizeof(TCHAR)
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)];

    PSYMBOL_INFO symbol = (PSYMBOL_INFO)&buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_LABEL_SIZE;

    if(!SafeSymFromName(fdProcessInfo->hProcess, Name, symbol))
        return false;

    *Address = (duint)symbol->Address;
    return true;
}

String SymGetSymbolicName(duint Address)
{
    //
    // This resolves an address to a module and symbol:
    // [modname.]symbolname
    //
    char label[MAX_SYM_NAME];
    char modname[MAX_MODULE_SIZE];
    auto hasModule = ModNameFromAddr(Address, modname, false);

    // User labels have priority, but if one wasn't found,
    // default to a symbol lookup
    if(!DbgGetLabelAt(Address, SEG_DEFAULT, label))
    {
        if(hasModule)
            return StringUtils::sprintf("%s.%p", modname, Address);
        return "";
    }

    if(hasModule)
        return StringUtils::sprintf("<%s.%s>", modname, label);
    return StringUtils::sprintf("<%s>", label);
}

bool SymGetSourceLine(duint Cip, char* FileName, int* Line, DWORD* disp)
{
    IMAGEHLP_LINEW64 lineInfo;
    memset(&lineInfo, 0, sizeof(IMAGEHLP_LINE64));

    lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    // Perform a symbol lookup from a specific address
    DWORD displacement;

    if(!SymGetLineFromAddrW64(fdProcessInfo->hProcess, Cip, &displacement, &lineInfo))
        return false;

    if(disp)
        *disp = displacement;

    String NewFile = StringUtils::Utf16ToUtf8(lineInfo.FileName);

    // Copy line number if requested
    if(Line)
        *Line = lineInfo.LineNumber;

    // Copy file name if requested
    if(FileName)
    {
        // Check if it was a full path
        if(NewFile[1] == ':' && NewFile[2] == '\\')
        {
            // Success: no more parsing
            strcpy_s(FileName, MAX_STRING_SIZE, NewFile.c_str());
            return true;
        }

        // Construct full path from pdb path
        IMAGEHLP_MODULEW64 modInfo;
        memset(&modInfo, 0, sizeof(modInfo));
        modInfo.SizeOfStruct = sizeof(modInfo);

        if(!SafeSymGetModuleInfoW64(fdProcessInfo->hProcess, Cip, &modInfo))
            return false;

        // Strip the name, leaving only the file directory
        wchar_t* pdbFileName = wcsrchr(modInfo.LoadedPdbName, L'\\');

        if(pdbFileName)
            pdbFileName[1] = L'\0';

        // Copy back to the caller's buffer
        strcpy_s(FileName, MAX_STRING_SIZE, StringUtils::Utf16ToUtf8(modInfo.LoadedPdbName).c_str());
        strcat_s(FileName, MAX_STRING_SIZE, NewFile.c_str());
    }

    return true;
}