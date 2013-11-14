#include "addrinfo.h"
#include "debugger.h"

bool modnamefromaddr(uint addr, char* modname)
{
    IMAGEHLP_MODULE64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct=sizeof(IMAGEHLP_MODULE64);
    if(!SymGetModuleInfo64(fdProcessInfo->hProcess, (DWORD64)addr, &modInfo) or !modname)
        return false;
    _strlwr(modInfo.ModuleName);
    strcpy(modname, modInfo.ModuleName);
    return true;
}
