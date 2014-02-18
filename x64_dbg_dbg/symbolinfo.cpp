#include "symbolinfo.h"
#include "debugger.h"
#include "console.h"

static BOOL CALLBACK EnumSymProc(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
    return TRUE;
}

#ifdef _WIN64
static BOOL CALLBACK EnumModules(PCTSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext)
#else
static BOOL CALLBACK EnumModules(PCTSTR ModuleName, ULONG BaseOfDll, PVOID UserContext)
#endif //_WIN64
{
    return TRUE;
}

void symbolupdategui()
{
}
