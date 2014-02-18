#include "symbolinfo.h"
#include "debugger.h"

void symbolenum(uint base, CBSYMBOLENUM cbSymbolEnum, void* user)
{
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
    strcpy(curModule.name, ModuleName);
    ((std::vector<SYMBOLMODULEINFO>*)UserContext)->push_back(curModule);
    return TRUE;
}

void symbolupdatemodulelist()
{
    std::vector<SYMBOLMODULEINFO> modList;
    modList.clear();
    //SymEnumerateModules(fdProcessInfo->hProcess, EnumModules, &modList);
    int modcount=modList.size();
    SYMBOLMODULEINFO* modListBridge=(SYMBOLMODULEINFO*)BridgeAlloc(sizeof(SYMBOLMODULEINFO)*modcount);
    for(int i=0; i<modcount; i++)
        memcpy(&modListBridge[i], &modList.at(i), sizeof(SYMBOLMODULEINFO));
    GuiSymbolUpdateModuleList(modcount, modListBridge);
}
