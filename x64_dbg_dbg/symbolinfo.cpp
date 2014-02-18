#include "symbolinfo.h"
#include "debugger.h"
#include "addrinfo.h"

static struct SYMBOLCBDATA
{
    CBSYMBOLENUM cbSymbolEnum;
    void* user;
};

static BOOL CALLBACK EnumSymbols(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
    int len=strlen(pSymInfo->Name);
    SYMBOLINFO curSymbol;
    memset(&curSymbol, 0, sizeof(SYMBOLINFO));
    curSymbol.addr=pSymInfo->Address;
    curSymbol.decoratedSymbol=(char*)BridgeAlloc(len+1);
    strcpy(curSymbol.decoratedSymbol, pSymInfo->Name);
    curSymbol.undecoratedSymbol=(char*)BridgeAlloc(MAX_SYM_NAME);
    if(!UnDecorateSymbolName(pSymInfo->Name, curSymbol.undecoratedSymbol, MAX_SYM_NAME, UNDNAME_COMPLETE))
    {
        BridgeFree(curSymbol.undecoratedSymbol);
        curSymbol.undecoratedSymbol=0;
    }
    else if(!strcmp(curSymbol.decoratedSymbol, curSymbol.undecoratedSymbol))
    {
        BridgeFree(curSymbol.undecoratedSymbol);
        curSymbol.undecoratedSymbol=0;
    }
    SYMBOLCBDATA* cbData=(SYMBOLCBDATA*)UserContext;
    cbData->cbSymbolEnum(&curSymbol, cbData->user);
    return TRUE;
}

void symbolenum(uint base, CBSYMBOLENUM cbSymbolEnum, void* user)
{
    SYMBOLCBDATA symbolCbData;
    symbolCbData.cbSymbolEnum=cbSymbolEnum;
    symbolCbData.user=user;
    char mask[]="*";
    SymEnumSymbols(fdProcessInfo->hProcess, base, mask, EnumSymbols, &symbolCbData);
}

#ifdef _WIN64
static BOOL CALLBACK EnumModules(PCTSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext)
#else
static BOOL CALLBACK EnumModules(PCTSTR ModuleName, ULONG BaseOfDll, PVOID UserContext)
#endif //_WIN64
{
    SYMBOLMODULEINFO curModule;
    memset(&curModule, 0, sizeof(SYMBOLMODULEINFO));
    curModule.base=BaseOfDll;
    modnamefromaddr(BaseOfDll, curModule.name, true);
    ((std::vector<SYMBOLMODULEINFO>*)UserContext)->push_back(curModule);
    return TRUE;
}

void symbolupdatemodulelist()
{
    std::vector<SYMBOLMODULEINFO> modList;
    modList.clear();
    SymEnumerateModules(fdProcessInfo->hProcess, EnumModules, &modList);
    int modcount=modList.size();
    SYMBOLMODULEINFO* modListBridge=(SYMBOLMODULEINFO*)BridgeAlloc(sizeof(SYMBOLMODULEINFO)*modcount);
    for(int i=0; i<modcount; i++)
        memcpy(&modListBridge[i], &modList.at(i), sizeof(SYMBOLMODULEINFO));
    GuiSymbolUpdateModuleList(modcount, modListBridge);
}
