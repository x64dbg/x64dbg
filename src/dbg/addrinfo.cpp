/**
 @file addrinfo.cpp

 @brief Implements the addrinfo class.
 */

#include "addrinfo.h"
#include "debugger.h"
#include "memory.h"
#include "module.h"
#include "value.h"
#include "threading.h"

///api functions
bool apienumexports(duint base, const EXPORTENUMCALLBACK & cbEnum)
{
    SHARED_ACQUIRE(LockModules);
    auto modinfo = ModInfoFromAddr(base);
    if(!modinfo)
        return false;

    char modname[MAX_MODULE_SIZE] = "";
    sprintf_s(modname, "%s%s", modinfo->name, modinfo->extension);

    for(auto & modExport : modinfo->exports)
    {
        if(modExport.forwarded)
        {
            duint remote_addr;
            if(valfromstring(modExport.forwardName.c_str(), &remote_addr))
                cbEnum(base, modname, modExport.name.c_str(), remote_addr);
        }
        else
        {
            cbEnum(base, modname, modExport.name.c_str(), modExport.rva + base);
        }
    }
    return true;
}

bool apienumimports(duint base, const IMPORTENUMCALLBACK & cbEnum)
{
    SHARED_ACQUIRE(LockModules);
    auto modinfo = ModInfoFromAddr(base);
    if(!modinfo)
        return false;

    char modname[MAX_MODULE_SIZE] = "";
    sprintf_s(modname, "%s%s", modinfo->name, modinfo->extension);

    for(auto & modImport : modinfo->imports)
    {
        cbEnum(base, modImport.iatRva + base, modImport.name.c_str(), modinfo->importModules.at(modImport.moduleIndex).c_str());
    }
    return true;
}