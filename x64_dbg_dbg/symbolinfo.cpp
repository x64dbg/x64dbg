/**
 @file symbolinfo.cpp

 @brief Implements the symbolinfo class.
 */

#include "symbolinfo.h"
#include "debugger.h"
#include "addrinfo.h"
#include "console.h"

/**
 @struct SYMBOLCBDATA

 @brief A symbolcbdata.
 */

struct SYMBOLCBDATA
{
    /**
     @brief The symbol enum.
     */

    CBSYMBOLENUM cbSymbolEnum;

    /**
     @brief The user.
     */

    void* user;
};

/**
 @fn static BOOL CALLBACK EnumSymbols(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)

 @brief Enum symbols.

 @param pSymInfo    Information describing the symbol.
 @param SymbolSize  Size of the symbol.
 @param UserContext Context for the user.

 @return A CALLBACK.
 */

static BOOL CALLBACK EnumSymbols(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
    int len = (int)strlen(pSymInfo->Name);
    SYMBOLINFO curSymbol;
    memset(&curSymbol, 0, sizeof(SYMBOLINFO));
    curSymbol.addr = (duint)pSymInfo->Address;
    curSymbol.decoratedSymbol = (char*)BridgeAlloc(len + 1);
    strcpy(curSymbol.decoratedSymbol, pSymInfo->Name);
    curSymbol.undecoratedSymbol = (char*)BridgeAlloc(MAX_SYM_NAME);
    if(strstr(pSymInfo->Name, "Ordinal"))
    {
        //skip bad ordinals
        if(pSymInfo->Address == pSymInfo->ModBase)
            return TRUE;
    }
    if(!UnDecorateSymbolName(pSymInfo->Name, curSymbol.undecoratedSymbol, MAX_SYM_NAME, UNDNAME_COMPLETE))
    {
        BridgeFree(curSymbol.undecoratedSymbol);
        curSymbol.undecoratedSymbol = 0;
    }
    else if(!strcmp(curSymbol.decoratedSymbol, curSymbol.undecoratedSymbol))
    {
        BridgeFree(curSymbol.undecoratedSymbol);
        curSymbol.undecoratedSymbol = 0;
    }
    SYMBOLCBDATA* cbData = (SYMBOLCBDATA*)UserContext;
    cbData->cbSymbolEnum(&curSymbol, cbData->user);
    return TRUE;
}

/**
 @fn void symenum(uint base, CBSYMBOLENUM cbSymbolEnum, void* user)

 @brief Symenums.

 @param base          The base.
 @param cbSymbolEnum  The symbol enum.
 @param [in,out] user If non-null, the user.
 */

void symenum(uint base, CBSYMBOLENUM cbSymbolEnum, void* user)
{
    SYMBOLCBDATA symbolCbData;
    symbolCbData.cbSymbolEnum = cbSymbolEnum;
    symbolCbData.user = user;
    char mask[] = "*";
    SymEnumSymbols(fdProcessInfo->hProcess, base, mask, EnumSymbols, &symbolCbData);
}

#ifdef _WIN64

/**
 @fn static BOOL CALLBACK EnumModules(LPCTSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext) #else static BOOL CALLBACK EnumModules(LPCTSTR ModuleName, ULONG BaseOfDll, PVOID UserContext) #endif

 @brief Enum modules.

 @param ModuleName  Name of the module.
 @param BaseOfDll   The base of DLL.
 @param UserContext Context for the user.

 @return A CALLBACK.
 */

static BOOL CALLBACK EnumModules(LPCTSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext)
#else

/**
 @fn static BOOL CALLBACK EnumModules(LPCTSTR ModuleName, ULONG BaseOfDll, PVOID UserContext) #endif

 @brief Enum modules.

 @param ModuleName  Name of the module.
 @param BaseOfDll   The base of DLL.
 @param UserContext Context for the user.

 @return A CALLBACK.
 */

static BOOL CALLBACK EnumModules(LPCTSTR ModuleName, ULONG BaseOfDll, PVOID UserContext)
#endif //_WIN64
{
    SYMBOLMODULEINFO curModule;
    memset(&curModule, 0, sizeof(SYMBOLMODULEINFO));
    curModule.base = BaseOfDll;
    modnamefromaddr(BaseOfDll, curModule.name, true);
    ((std::vector<SYMBOLMODULEINFO>*)UserContext)->push_back(curModule);
    return TRUE;
}

/**
 @fn void symupdatemodulelist()

 @brief Symupdatemodulelists this object.
 */

void symupdatemodulelist()
{
    std::vector<SYMBOLMODULEINFO> modList;
    modList.clear();
    SymEnumerateModules(fdProcessInfo->hProcess, EnumModules, &modList);
    int modcount = (int)modList.size();
    SYMBOLMODULEINFO* modListBridge = (SYMBOLMODULEINFO*)BridgeAlloc(sizeof(SYMBOLMODULEINFO) * modcount);
    for(int i = 0; i < modcount; i++)
        memcpy(&modListBridge[i], &modList.at(i), sizeof(SYMBOLMODULEINFO));
    GuiSymbolUpdateModuleList(modcount, modListBridge);
}

/**
 @fn void symdownloadallsymbols(const char* szSymbolStore)

 @brief Symdownloadallsymbols the given size symbol store.

 @param szSymbolStore The symbol store.
 */

void symdownloadallsymbols(const char* szSymbolStore)
{
    if(!szSymbolStore)
        szSymbolStore = "http://msdl.microsoft.com/download/symbols";
    std::vector<SYMBOLMODULEINFO> modList;
    modList.clear();
    SymEnumerateModules(fdProcessInfo->hProcess, EnumModules, &modList);
    int modcount = (int)modList.size();
    if(!modcount)
        return;
    char szOldSearchPath[MAX_PATH] = "";
    if(!SymGetSearchPath(fdProcessInfo->hProcess, szOldSearchPath, MAX_PATH)) //backup current path
    {
        dputs("SymGetSearchPath failed!");
        return;
    }
    char szServerSearchPath[MAX_PATH * 2] = "";
    sprintf_s(szServerSearchPath, "SRV*%s*%s", szSymbolCachePath, szSymbolStore);
    if(!SymSetSearchPath(fdProcessInfo->hProcess, szServerSearchPath)) //update search path
    {
        dputs("SymSetSearchPath (1) failed!");
        return;
    }
    for(int i = 0; i < modcount; i++) //reload all modules
    {
        dprintf("downloading symbols for %s...\n", modList.at(i).name);
        uint modbase = modList.at(i).base;
        char szModulePath[MAX_PATH] = "";
        if(!GetModuleFileNameExA(fdProcessInfo->hProcess, (HMODULE)modbase, szModulePath, MAX_PATH))
        {
            dprintf("GetModuleFileNameExA("fhex") failed!\n", modbase);
            continue;
        }
        if(!SymUnloadModule64(fdProcessInfo->hProcess, (DWORD64)modbase))
        {
            dprintf("SymUnloadModule64("fhex") failed!\n", modbase);
            continue;
        }
        if(!SymLoadModuleEx(fdProcessInfo->hProcess, 0, szModulePath, 0, (DWORD64)modbase, 0, 0, 0))
        {
            dprintf("SymLoadModuleEx("fhex") failed!\n", modbase);
            continue;
        }
    }
    if(!SymSetSearchPath(fdProcessInfo->hProcess, szOldSearchPath)) //restore search path
    {
        dputs("SymSetSearchPath (2) failed!");
    }
}

/**
 @fn bool symfromname(const char* name, uint* addr)

 @brief Symfromnames.

 @param name          The name.
 @param [in,out] addr If non-null, the address.

 @return true if it succeeds, false if it fails.
 */

bool symfromname(const char* name, uint* addr)
{
    if(!name or !strlen(name) or !addr or !_strnicmp(name, "ordinal", 7)) //skip 'OrdinalXXX'
        return false;
    char buffer[sizeof(SYMBOL_INFO) + MAX_LABEL_SIZE * sizeof(char)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_LABEL_SIZE;
    if(!SymFromName(fdProcessInfo->hProcess, name, pSymbol))
        return false;
    *addr = (uint)pSymbol->Address;
    return true;
}

/**
 @fn const char* symgetsymbolicname(uint addr)

 @brief Symgetsymbolicnames the given address.

 @param addr The address.

 @return null if it fails, else a char*.
 */

const char* symgetsymbolicname(uint addr)
{
    //[modname.]symbolname
    static char symbolicname[MAX_MODULE_SIZE + MAX_SYM_NAME] = "";
    char label[MAX_SYM_NAME] = "";
    bool retval = false;
    if(labelget(addr, label)) //user labels have priority
        retval = true;
    else //no user labels
    {
        DWORD64 displacement = 0;
        char buffer[sizeof(SYMBOL_INFO) + MAX_LABEL_SIZE * sizeof(char)];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_LABEL_SIZE;
        if(SymFromAddr(fdProcessInfo->hProcess, (DWORD64)addr, &displacement, pSymbol) and !displacement)
        {
            if(!settingboolget("Engine", "UndecorateSymbolNames") or !UnDecorateSymbolName(pSymbol->Name, label, MAX_SYM_NAME, UNDNAME_COMPLETE))
                strcpy_s(label, pSymbol->Name);
            retval = true;
        }
    }
    if(retval)
    {
        char modname[MAX_MODULE_SIZE] = "";
        if(modnamefromaddr(addr, modname, false))
            sprintf(symbolicname, "%s.%s", modname, label);
        else
            sprintf(symbolicname, "<%s>", label);
        return symbolicname;
    }
    return 0;
}
