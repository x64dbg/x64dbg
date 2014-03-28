#include "symbolinfo.h"
#include "debugger.h"
#include "addrinfo.h"

struct SYMBOLCBDATA
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

void symenum(uint base, CBSYMBOLENUM cbSymbolEnum, void* user)
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

void symupdatemodulelist()
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

bool symfromname(const char* name, uint* addr)
{
    if(!name or !strlen(name) or !addr)
        return false;
    char buffer[sizeof(SYMBOL_INFO) + MAX_LABEL_SIZE * sizeof(char)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_LABEL_SIZE;
    if(!SymFromName(fdProcessInfo->hProcess, name, pSymbol))
        return false;
    *addr=(uint)pSymbol->Address;
    return true;
}

const char* symgetsymbolicname(uint addr)
{
    //[modname.]symbolname
    static char symbolicname[MAX_MODULE_SIZE+MAX_SYM_NAME]="";
    char label[MAX_SYM_NAME]="";
    bool retval=false;
    if(labelget(addr, label)) //user labels have priority
        retval=true;
    else //no user labels
    {
        DWORD64 displacement=0;
        char buffer[sizeof(SYMBOL_INFO) + MAX_LABEL_SIZE * sizeof(char)];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_LABEL_SIZE;
        if(SymFromAddr(fdProcessInfo->hProcess, (DWORD64)addr, &displacement, pSymbol) and !displacement)
        {
            if(!settingboolget("Engine", "UndecorateSymbolNames") or !UnDecorateSymbolName(pSymbol->Name, label, MAX_SYM_NAME, UNDNAME_COMPLETE))
                strcpy(label, pSymbol->Name);
            retval=true;
        }
    }
    if(retval)
    {
        char modname[MAX_MODULE_SIZE]="";
        if(modnamefromaddr(addr, modname, false))
            sprintf(symbolicname, "%s.%s", modname, label);
        else
            sprintf(symbolicname, "<%s>", label);
        return symbolicname;
    }
    return 0;
}
