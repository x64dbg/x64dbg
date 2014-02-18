#include "symbolinfo.h"
#include "debugger.h"
#include "console.h"

static struct INTERNALSYMBOLMODULEINFO
{
    uint base;
    char name[MAX_MODULE_SIZE];
    std::vector<SYMBOLINFO> symbols;
};

static std::vector<INTERNALSYMBOLMODULEINFO> modList;

static BOOL CALLBACK EnumSymbols(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
    return TRUE;
}

void symbolloadmodule(MODINFO* modinfo)
{
    INTERNALSYMBOLMODULEINFO curModule;
    memset(&curModule, 0, sizeof(curModule));
    curModule.base=modinfo->base;
    sprintf(curModule.name, "%s%s", modinfo->name, modinfo->extension);
    modList.push_back(curModule);
}

void symbolunloadmodule(uint base)
{
}

void symbolclear()
{
    std::vector<INTERNALSYMBOLMODULEINFO>().swap(modList);
}

void symbolupdategui()
{
}
