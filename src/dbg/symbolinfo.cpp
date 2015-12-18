/**
 @file symbolinfo.cpp

 @brief Implements the symbolinfo class.
 */

#include "symbolinfo.h"
#include "debugger.h"
#include "console.h"
#include "module.h"
#include "label.h"
#include "addrinfo.h"

struct SYMBOLCBDATA
{
    CBSYMBOLENUM cbSymbolEnum;
    void* user;
};

typedef std::vector<SYMBOLINFO> SYMBOLINFOVECTOR;
typedef std::map<ULONG64, SYMBOLINFOVECTOR> SYMBOLINFOMAP;
SYMBOLINFOMAP modulesCacheList;


BOOL CALLBACK EnumSymbols(PSYMBOL_INFO SymInfo, ULONG SymbolSize, PVOID UserContext)
{
    bool returnValue;
    SYMBOLINFO curSymbol;
    memset(&curSymbol, 0, sizeof(SYMBOLINFO));

    // Convert from SYMBOL_INFO to SYMBOLINFO
    returnValue = SymGetSymbolInfo(SymInfo, &curSymbol, false);

    if (!returnValue)
        return false;

    // Add to the cache
    modulesCacheList[SymInfo->ModBase].push_back(curSymbol);

    SYMBOLCBDATA* cbData = (SYMBOLCBDATA*)UserContext;
    cbData->cbSymbolEnum(&curSymbol, cbData->user);
    return TRUE;
}

void SymEnum(duint Base, CBSYMBOLENUM EnumCallback, void* UserData)
{
    SYMBOLCBDATA symbolCbData;
    symbolCbData.cbSymbolEnum = EnumCallback;
    symbolCbData.user = UserData;

    SymEnumImports(Base, &symbolCbData);

    // Enumerate every single symbol for the module in 'base'
    if(!SafeSymEnumSymbols(fdProcessInfo->hProcess, Base, "*", EnumSymbols, &symbolCbData))
        dputs("SymEnumSymbols failed!");
}

void SymEnumFromCache(duint Base, CBSYMBOLENUM EnumCallback, void* UserData)
{
    SYMBOLCBDATA symbolCbData;
    symbolCbData.cbSymbolEnum = EnumCallback;
    symbolCbData.user = UserData;

    // Check if this module is cached in the list
    if (modulesCacheList.find(Base) != modulesCacheList.end())
    {
        SymEnumImports(Base, &symbolCbData);

        // Callback
        for (duint i = 0; i < modulesCacheList[Base].size(); i++)
            symbolCbData.cbSymbolEnum(&modulesCacheList[Base].at(i), symbolCbData.user);
    }
    else
    {
        // Then get the symbols and cache them
        SymEnum(Base, EnumCallback, UserData);
    }
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
        dputs("SymEnumerateModules64 failed!");
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
        dputs("SymGetSearchPathW failed!");
        return;
    }

    // Use the custom server path and directory
    char customSearchPath[MAX_PATH * 2];
    sprintf_s(customSearchPath, "SRV*%s*%s", szSymbolCachePath, SymbolStore);

    if(!SafeSymSetSearchPathW(fdProcessInfo->hProcess, StringUtils::Utf8ToUtf16(customSearchPath).c_str()))
    {
        dputs("SymSetSearchPathW (1) failed!");
        return;
    }

    // Reload
    for(auto & module : modList)
    {
        dprintf("Downloading symbols for %s...\n", module.name);

        wchar_t modulePath[MAX_PATH];
        if(!GetModuleFileNameExW(fdProcessInfo->hProcess, (HMODULE)module.base, modulePath, MAX_PATH))
        {
            dprintf("GetModuleFileNameExW(" fhex ") failed!\n", module.base);
            continue;
        }

        if(!SafeSymUnloadModule64(fdProcessInfo->hProcess, (DWORD64)module.base))
        {
            dprintf("SymUnloadModule64(" fhex ") failed!\n", module.base);
            continue;
        }

        if(!SafeSymLoadModuleEx(fdProcessInfo->hProcess, 0, StringUtils::Utf16ToUtf8(modulePath).c_str(), 0, (DWORD64)module.base, 0, 0, 0))
        {
            dprintf("SymLoadModuleEx(" fhex ") failed!\n", module.base);
            continue;
        }
    }

    // Restore the old search path
    if(!SafeSymSetSearchPathW(fdProcessInfo->hProcess, oldSearchPath))
        dputs("SymSetSearchPathW (2) failed!");
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

const char* SymGetSymbolicName(duint Address)
{
    //
    // This resolves an address to a module and symbol:
    // [modname.]symbolname
    //
    char label[MAX_SYM_NAME];

    // User labels have priority, but if one wasn't found,
    // default to a symbol lookup
    if(!LabelGet(Address, label))
    {
        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)];

        PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_LABEL_SIZE;

        // Perform a symbol lookup
        DWORD64 displacement = 0;

        if(!SafeSymFromAddr(fdProcessInfo->hProcess, (DWORD64)Address, &displacement, symbol))
            return nullptr;

        // If the symbol wasn't at offset 0 (start from the beginning) ignore it
        if(displacement != 0)
            return nullptr;

        // Terminate the string for sanity
        symbol->Name[symbol->MaxNameLen - 1] = '\0';

        if(!bUndecorateSymbolNames || !SafeUnDecorateSymbolName(symbol->Name, label, MAX_SYM_NAME, UNDNAME_COMPLETE))
            strcpy_s(label, symbol->Name);
    }

    // TODO: FIXME: STATIC VARIABLE
    static char symbolicname[MAX_MODULE_SIZE + MAX_SYM_NAME];
    char modname[MAX_MODULE_SIZE];

    if(ModNameFromAddr(Address, modname, false))
        sprintf_s(symbolicname, "%s.%s", modname, label);
    else
        sprintf_s(symbolicname, "<%s>", label);

    return symbolicname;
}

bool SymGetSourceLine(duint Cip, char* FileName, int* Line)
{
    IMAGEHLP_LINEW64 lineInfo;
    memset(&lineInfo, 0, sizeof(IMAGEHLP_LINE64));

    lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    // Perform a symbol lookup from a specific address
    DWORD displacement;

    if(!SymGetLineFromAddrW64(fdProcessInfo->hProcess, Cip, &displacement, &lineInfo))
        return false;

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
        IMAGEHLP_MODULE64 modInfo;
        memset(&modInfo, 0, sizeof(IMAGEHLP_MODULE64));
        modInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

        if(!SafeSymGetModuleInfo64(fdProcessInfo->hProcess, Cip, &modInfo))
            return false;

        // Strip the name, leaving only the file directory
        char* pdbFileName = strrchr(modInfo.LoadedPdbName, '\\');

        if(pdbFileName)
            pdbFileName[1] = '\0';

        // Copy back to the caller's buffer
        strcpy_s(FileName, MAX_STRING_SIZE, modInfo.LoadedPdbName);
        strcat_s(FileName, MAX_STRING_SIZE, NewFile.c_str());
    }

    return true;
}

void SymClearMemoryCache()
{
    for (auto& itr : modulesCacheList)
    {
        SYMBOLINFOVECTOR* pModuleVector = &itr.second;

        // Free up previously allocated memory
        for (duint i = 0; i < pModuleVector->size(); i++)
        {
            BridgeFree(pModuleVector->at(i).decoratedSymbol);
            BridgeFree(pModuleVector->at(i).undecoratedSymbol);
        }
    }

    // Clear the whole map
    modulesCacheList.clear();
}

bool SymGetSymbolInfo(PSYMBOL_INFO SymInfo, SYMBOLINFO* curSymbol, bool isImported)
{
    // SYMBOL_INFO is a structure used by Sym* functions
    // SYMBOLINFO is the custom structure used by the debugger
    // This functions fills SYMBOLINFO fields from SYMBOL_INFO data

    curSymbol->addr = (duint)SymInfo->Address;
    curSymbol->decoratedSymbol = (char*)BridgeAlloc(strlen(SymInfo->Name) + 1);
    curSymbol->undecoratedSymbol = (char*)BridgeAlloc(MAX_SYM_NAME);
    strcpy_s(curSymbol->decoratedSymbol, strlen(SymInfo->Name) + 1, SymInfo->Name);

    // Skip bad ordinals
    if (strstr(SymInfo->Name, "Ordinal"))
    {
        // Does the symbol point to the module base?
        if (SymInfo->Address == SymInfo->ModBase)
            return FALSE;
    }

    // Convert a mangled/decorated C++ name to a readable format
    if (!SafeUnDecorateSymbolName(SymInfo->Name, curSymbol->undecoratedSymbol, MAX_SYM_NAME, UNDNAME_COMPLETE))
    {
        BridgeFree(curSymbol->undecoratedSymbol);
        curSymbol->undecoratedSymbol = nullptr;
    }
    else if (!strcmp(curSymbol->decoratedSymbol, curSymbol->undecoratedSymbol))
    {
        BridgeFree(curSymbol->undecoratedSymbol);
        curSymbol->undecoratedSymbol = nullptr;
    }

    // Symbol is exported
    curSymbol->isImported = isImported;

    return true;
}

void SymEnumImports(duint Base, SYMBOLCBDATA* pSymbolCbData)
{
    char modImportString[MAX_IMPORT_SIZE];
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]; // Reserve enough space for symbol name, see msdn for this
    SYMBOLINFO curSymbol;
    PSYMBOL_INFO pSymInfo;
    std::vector<MODIMPORTINFO> imports;

    // SizeOfStruct and MaxNameLen need to be set or SymFromAddr() returns INVALID_PARAMETER
    pSymInfo = (PSYMBOL_INFO)buffer;
    pSymInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymInfo->MaxNameLen = MAX_SYM_NAME;

    // Enum imports if none found
    if (ModImportsFromAddr(Base, &imports) && !imports.size())
    {
        // Enum imports from current module
        apienumimports(Base, [](duint Base, duint Address, char* name, char* moduleName)
        {
            MODIMPORTINFO importInfo;
            importInfo.addr = Address;
            strcpy_s(importInfo.name, MAX_IMPORT_SIZE, name);
            strcpy_s(importInfo.moduleName, MAX_MODULE_SIZE, moduleName);

            // Add import to the module structure
            ModAddImportToModule(Base, importInfo);
        });
    }

    // Get imports
    if (ModImportsFromAddr(Base, &imports) && imports.size())
    {
        for (duint i = 0; i < imports.size(); i++)
        {
            // Can we get symbol for the import address?
            if (SafeSymFromAddr(fdProcessInfo->hProcess, (duint)imports[i].addr, 0, pSymInfo))
            {
                // Does the symbol point to the module base?
                if (!SymGetSymbolInfo(pSymInfo, &curSymbol, true))
                    continue;
            }
            else
            {
                // Otherwise just use import info from module itself
                curSymbol.addr = imports[i].addr;
                curSymbol.isImported = true;
                curSymbol.undecoratedSymbol = nullptr;
                curSymbol.decoratedSymbol = imports[i].name;
            }

            // Format so that we get: moduleName.importSymbol
            strcpy_s(modImportString, imports[i].moduleName);

            // Trim the extension if present
            char *modExt = strrchr(modImportString, '.');

            if (modExt)
                *modExt = '\0';

            // Buffers to hold the decorated and undecorated strings. Must be declared
            // outside of the if() scope.
            char undecBuf[MAX_IMPORT_SIZE];
            char decBuf[MAX_IMPORT_SIZE];

            if (curSymbol.undecoratedSymbol)
            {
                // module.undecorated
                strcpy_s(undecBuf, modImportString);
                strncpy_s(undecBuf, curSymbol.undecoratedSymbol, _TRUNCATE);

                curSymbol.undecoratedSymbol = undecBuf;
            }

            if (curSymbol.decoratedSymbol)
            {
                // module.decorated
                strcpy_s(decBuf, modImportString);
                strncpy_s(decBuf, curSymbol.decoratedSymbol, _TRUNCATE);

                curSymbol.decoratedSymbol = decBuf;
            }

            // Callback
            pSymbolCbData->cbSymbolEnum(&curSymbol, pSymbolCbData->user);
        }
    }
}