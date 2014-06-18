#include "breakpoint.h"
#include "debugger.h"
#include "addrinfo.h"
#include "console.h"
#include "memory.h"
#include "threading.h"

static BreakpointsMap breakpoints;

int bpgetlist(std::vector<BREAKPOINT>* list)
{
    if(!DbgIsDebugging())
        return false;
    BREAKPOINT curBp;
    int count=0;
    for(BreakpointsMap::iterator i=breakpoints.begin(); i!=breakpoints.end(); ++i)
    {
        curBp=i->second;
        curBp.addr+=curBp.modbase;
        curBp.active=memisvalidreadptr(fdProcessInfo->hProcess, curBp.addr);
        count++;
        if(list)
            list->push_back(curBp);
    }
    return count;
}

bool bpnew(uint addr, bool enabled, bool singleshoot, short oldbytes, BP_TYPE type, DWORD titantype, const char* name)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or bpget(addr, type, name, 0))
        return false;
    BREAKPOINT bp;
    modnamefromaddr(addr, bp.mod, true);
    bp.modbase=modbasefromaddr(addr);
    bp.active=true;
    bp.addr=addr-bp.modbase;
    bp.enabled=enabled;
    if(name and *name)
        strcpy(bp.name, name);
    else
        *bp.name='\0';
    bp.oldbytes=oldbytes;
    bp.singleshoot=singleshoot;
    bp.titantype=titantype;
    bp.type=type;
    breakpoints.insert(std::make_pair(std::make_pair(addr, type), bp));
    return true;
}

bool bpget(uint addr, BP_TYPE type, const char* name, BREAKPOINT* bp)
{
    if(!DbgIsDebugging())
        return false;
    BREAKPOINT curBp;
    for(BreakpointsMap::iterator i=breakpoints.begin(); i!=breakpoints.end(); ++i)
    {
        curBp=i->second;
        curBp.addr+=curBp.modbase;
        curBp.active=memisvalidreadptr(fdProcessInfo->hProcess, curBp.addr);
        if(name and *name)
        {
            if(i->first==std::make_pair(addr, type) or !strcmp(name, curBp.name))
            {
                if(bp)
                    *bp=curBp;
                return true;
            }
        }
        else if(i->first==std::make_pair(addr, type))
        {
            if(bp)
                *bp=curBp;
            return true;
        }
    }
    return false;
}

bool bpdel(uint addr, BP_TYPE type)
{
    if(!DbgIsDebugging())
        return false;
    if(breakpoints.count(std::make_pair(addr, type)))
    {
        breakpoints.erase(std::make_pair(addr, type));
        return true;
    }
    return false;
}

bool bpenable(uint addr, BP_TYPE type, bool enable)
{
    if(!DbgIsDebugging())
        return false;
    if(breakpoints.count(std::make_pair(addr, type)))
    {
        breakpoints[std::make_pair(addr, type)].enabled=true;
        return true;
    }
    return false;
}

bool bpsetname(uint addr, BP_TYPE type, const char* name)
{
    if(!DbgIsDebugging() or !name or !*name)
        return false;
    if(breakpoints.count(std::make_pair(addr, type)))
    {
        strcpy(breakpoints[std::make_pair(addr, type)].name, name);
        return true;
    }
    return false;
}

bool bpenumall(BPENUMCALLBACK cbEnum, const char* module)
{
    if(!DbgIsDebugging())
        return false;
    bool retval=true;
    BREAKPOINT curBp;
    for(BreakpointsMap::iterator i=breakpoints.begin(); i!=breakpoints.end(); ++i)
    {
        curBp=i->second;
        curBp.addr+=curBp.modbase; //RVA to VA
        curBp.active=memisvalidreadptr(fdProcessInfo->hProcess, curBp.addr); //TODO: wtf am I doing?
        if(module and *module)
        {
            if(!strcmp(curBp.mod, module))
            {
                if(!cbEnum(&curBp))
                    retval=false;
            }
        }
        else
        {
            if(!cbEnum(&curBp))
                retval=false;
        }        
    }
    return retval;
}

bool bpenumall(BPENUMCALLBACK cbEnum)
{
    return bpenumall(cbEnum, 0);
}

int bpgetcount(BP_TYPE type)
{
    int count=0;
    for(BreakpointsMap::iterator i=breakpoints.begin(); i!=breakpoints.end(); ++i)
    {
        if(i->first.first==type)
            count++;
    }
    return count;
}

void bptobridge(const BREAKPOINT* bp, BRIDGEBP* bridge)
{
    if(!bp or !bridge)
        return;
    memset(bridge, 0, sizeof(BRIDGEBP));
    bridge->active=bp->active;
    bridge->addr=bp->addr;
    bridge->enabled=bp->enabled;
    strcpy(bridge->mod, bp->mod);
    strcpy(bridge->name, bp->name);
    bridge->singleshoot=bp->singleshoot;
    switch(bp->type)
    {
    case BPNORMAL:
        bridge->type=bp_normal;
        break;
    case BPHARDWARE:
        bridge->type=bp_hardware;
        break;
    case BPMEMORY:
        bridge->type=bp_memory;
    default:
        bridge->type=bp_none;
    }
}
