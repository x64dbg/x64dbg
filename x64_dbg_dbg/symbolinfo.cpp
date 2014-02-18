#include "symbolinfo.h"
#include "debugger.h"

static struct INTERNALSYMMOD
{
    duint base;
    char name[MAX_MODULE_SIZE];
    std::vector<SYMBOLINFO> symbols;
};

static std::vector<INTERNALSYMMOD> modules;

static BOOL CALLBACK EnumSymProc(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
    SYMBOLINFO cur;
    cur.addr=pSymInfo->Address;
    int len=strlen(pSymInfo->Name);
    char* decorated=(char*)BridgeAlloc(len+1);
    strcpy(decorated, pSymInfo->Name);
    char* undecorated=(char*)BridgeAlloc(len+1);
    if(!UnDecorateSymbolName(decorated, undecorated, len, UNDNAME_COMPLETE))
    {
        BridgeFree(undecorated);
        undecorated=0;
    }
    cur.decoratedSymbol=decorated;
    cur.undecoratedSymbol=undecorated;
    modules.at((int)(uint)UserContext).symbols.push_back(cur);
    return TRUE;
}

#ifdef _WIN64
static BOOL CALLBACK EnumModules(PCTSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext)
#else
static BOOL CALLBACK EnumModules(PCTSTR ModuleName, ULONG BaseOfDll, PVOID UserContext)
#endif //_WIN64
{
    INTERNALSYMMOD cur;
    cur.base=BaseOfDll;
    strcpy(cur.name, ModuleName);
    modules.push_back(cur);
    return TRUE;
}

void symbolupdategui()
{
    if(!SymEnumerateModules(fdProcessInfo->hProcess, EnumModules, 0))
    {
        modules.clear();
        return;
    }
    int module_count=modules.size();
    if(!module_count)
        return;
    for(int i=0; i<module_count; i++)
    {
        char mask[2]="*";
        if(!SymEnumSymbols(fdProcessInfo->hProcess, modules.at(i).base, mask, EnumSymProc, (PVOID)(uint)i))
        {
            int module_count=0;
            for(int j=0; j<module_count; j++)
            {
                INTERNALSYMMOD cur=modules.at(j);
                int symbol_count=cur.symbols.size();
                for(int k=0; k<symbol_count; k++)
                {
                    if(cur.symbols.at(k).decoratedSymbol)
                        BridgeFree((void*)cur.symbols.at(k).decoratedSymbol);
                    if(cur.symbols.at(k).undecoratedSymbol)
                        BridgeFree((void*)cur.symbols.at(k).undecoratedSymbol);
                    memset(&cur.symbols.at(k), 0, sizeof(SYMBOLINFO));
                }
                cur.symbols.clear();
            }
            modules.clear();
            return;
        }
    }
    SYMBOLMODULEINFO* SymModInfo=(SYMBOLMODULEINFO*)BridgeAlloc(sizeof(SYMBOLMODULEINFO)*module_count);
    for(int i=0; i<module_count; i++)
    {
        INTERNALSYMMOD curModule=modules.at(i);
        SymModInfo[i].base=curModule.base;
        strcpy(SymModInfo[i].name, curModule.name);
        int symbol_count=curModule.symbols.size();
        SymModInfo[i].symbol_count=symbol_count;
        SYMBOLINFO* CurSymInfo=(SYMBOLINFO*)BridgeAlloc(sizeof(SYMBOLINFO)*symbol_count);
        SymModInfo[i].symbols=CurSymInfo;
        for(int j=0; j<symbol_count; j++)
            memcpy(&CurSymInfo[j], &curModule.symbols.at(j), sizeof(SYMBOLINFO));
    }
    GuiSymbolUpdateList(module_count, SymModInfo);
}
