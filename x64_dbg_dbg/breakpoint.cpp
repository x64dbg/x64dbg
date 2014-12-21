#include "breakpoint.h"
#include "debugger.h"
#include "addrinfo.h"
#include "console.h"
#include "memory.h"
#include "threading.h"

static BreakpointsInfo breakpoints;

int bpgetlist(std::vector<BREAKPOINT>* list)
{
    if(!DbgIsDebugging())
        return false;
    BREAKPOINT curBp;
    int count = 0;
    CriticalSectionLocker locker(LockBreakpoints);
    for(BreakpointsInfo::iterator i = breakpoints.begin(); i != breakpoints.end(); ++i)
    {
        curBp = i->second;
        curBp.addr += modbasefromname(curBp.mod);
        curBp.active = memisvalidreadptr(fdProcessInfo->hProcess, curBp.addr);
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
    uint modbase = modbasefromaddr(addr);
    bp.active = true;
    bp.addr = addr - modbase;
    bp.enabled = enabled;
    if(name and * name)
        strcpy_s(bp.name, name);
    else
        *bp.name = '\0';
    bp.oldbytes = oldbytes;
    bp.singleshoot = singleshoot;
    bp.titantype = titantype;
    bp.type = type;
    CriticalSectionLocker locker(LockBreakpoints);
    breakpoints.insert(std::make_pair(BreakpointKey(type, modhashfromva(addr)), bp));
    return true;
}

bool bpget(uint addr, BP_TYPE type, const char* name, BREAKPOINT* bp)
{
    if(!DbgIsDebugging())
        return false;
    BREAKPOINT curBp;
    CriticalSectionLocker locker(LockBreakpoints);
    if(!name)
    {
        BreakpointsInfo::iterator found = breakpoints.find(BreakpointKey(type, modhashfromva(addr)));
        if(found == breakpoints.end()) //not found
            return false;
        if(!bp)
            return true;
        curBp = found->second;
        curBp.addr += modbasefromaddr(addr);
        curBp.active = memisvalidreadptr(fdProcessInfo->hProcess, curBp.addr);
        *bp = curBp;
        return true;
    }
    for(BreakpointsInfo::iterator i = breakpoints.begin(); i != breakpoints.end(); ++i)
    {
        curBp = i->second;
        if(name and * name)
        {
            if(!strcmp(name, curBp.name))
            {
                if(bp)
                {
                    curBp.addr += modbasefromname(curBp.mod);
                    curBp.active = memisvalidreadptr(fdProcessInfo->hProcess, curBp.addr);
                    *bp = curBp;
                }
                return true;
            }
        }
    }
    return false;
}

bool bpdel(uint addr, BP_TYPE type)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockBreakpoints);
    return (breakpoints.erase(BreakpointKey(type, modhashfromva(addr))) > 0);
}

bool bpenable(uint addr, BP_TYPE type, bool enable)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockBreakpoints);
    BreakpointsInfo::iterator found = breakpoints.find(BreakpointKey(type, modhashfromva(addr)));
    if(found == breakpoints.end()) //not found
        return false;
    breakpoints[found->first].enabled = enable;
    return true;
}

bool bpsetname(uint addr, BP_TYPE type, const char* name)
{
    if(!DbgIsDebugging() or !name or !*name)
        return false;
    CriticalSectionLocker locker(LockBreakpoints);
    BreakpointsInfo::iterator found = breakpoints.find(BreakpointKey(type, modhashfromva(addr)));
    if(found == breakpoints.end()) //not found
        return false;
    strcpy_s(breakpoints[found->first].name, name);
    return true;
}

bool bpsettitantype(uint addr, BP_TYPE type, int titantype)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockBreakpoints);
    BreakpointsInfo::iterator found = breakpoints.find(BreakpointKey(type, modhashfromva(addr)));
    if(found == breakpoints.end()) //not found
        return false;
    breakpoints[found->first].titantype = titantype;
    return true;
}

bool bpenumall(BPENUMCALLBACK cbEnum, const char* module)
{
    if(!DbgIsDebugging())
        return false;
    bool retval = true;
    BREAKPOINT curBp;
    CriticalSectionLocker locker(LockBreakpoints);
    BreakpointsInfo::iterator i = breakpoints.begin();
    while(i != breakpoints.end())
    {
        BreakpointsInfo::iterator j = i;
        ++i;
        curBp = j->second;
        curBp.addr += modbasefromname(curBp.mod); //RVA to VA
        curBp.active = memisvalidreadptr(fdProcessInfo->hProcess, curBp.addr); //TODO: wtf am I doing?
        if(module and * module)
        {
            if(!strcmp(curBp.mod, module))
            {
                if(!cbEnum(&curBp))
                    retval = false;
            }
        }
        else
        {
            if(!cbEnum(&curBp))
                retval = false;
        }
    }
    return retval;
}

bool bpenumall(BPENUMCALLBACK cbEnum)
{
    return bpenumall(cbEnum, 0);
}

int bpgetcount(BP_TYPE type, bool enabledonly)
{
    int count = 0;
    CriticalSectionLocker locker(LockBreakpoints);
    for(BreakpointsInfo::iterator i = breakpoints.begin(); i != breakpoints.end(); ++i)
    {
        if(i->first.first == type && (!enabledonly || i->second.enabled))
            count++;
    }
    return count;
}

void bptobridge(const BREAKPOINT* bp, BRIDGEBP* bridge)
{
    if(!bp or !bridge)
        return;
    memset(bridge, 0, sizeof(BRIDGEBP));
    bridge->active = bp->active;
    bridge->addr = bp->addr;
    bridge->enabled = bp->enabled;
    strcpy(bridge->mod, bp->mod);
    strcpy(bridge->name, bp->name);
    bridge->singleshoot = bp->singleshoot;
    switch(bp->type)
    {
    case BPNORMAL:
        bridge->type = bp_normal;
        break;
    case BPHARDWARE:
        bridge->type = bp_hardware;
        break;
    case BPMEMORY:
        bridge->type = bp_memory;
        break; //so that's why it didn't show in the gui.
    default:
        bridge->type = bp_none;
        break;
    }
}

void bpcachesave(JSON root)
{
    CriticalSectionLocker locker(LockBreakpoints);
    const JSON jsonbreakpoints = json_array();
    for(BreakpointsInfo::iterator i = breakpoints.begin(); i != breakpoints.end(); ++i)
    {
        const BREAKPOINT curBreakpoint = i->second;
        if(curBreakpoint.singleshoot)
            continue; //skip
        JSON curjsonbreakpoint = json_object();
        json_object_set_new(curjsonbreakpoint, "address", json_hex(curBreakpoint.addr));
        json_object_set_new(curjsonbreakpoint, "enabled", json_boolean(curBreakpoint.enabled));
        if(curBreakpoint.type == BPNORMAL)
            json_object_set_new(curjsonbreakpoint, "oldbytes", json_hex(curBreakpoint.oldbytes));
        json_object_set_new(curjsonbreakpoint, "type", json_integer(curBreakpoint.type));
        json_object_set_new(curjsonbreakpoint, "titantype", json_hex(curBreakpoint.titantype));
        json_object_set_new(curjsonbreakpoint, "name", json_string(curBreakpoint.name));
        json_object_set_new(curjsonbreakpoint, "module", json_string(curBreakpoint.mod));
        json_array_append_new(jsonbreakpoints, curjsonbreakpoint);
    }
    if(json_array_size(jsonbreakpoints))
        json_object_set(root, "breakpoints", jsonbreakpoints);
    json_decref(jsonbreakpoints);
}

void bpcacheload(JSON root)
{
    CriticalSectionLocker locker(LockBreakpoints);
    breakpoints.clear();
    const JSON jsonbreakpoints = json_object_get(root, "breakpoints");
    if(jsonbreakpoints)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonbreakpoints, i, value)
        {
            BREAKPOINT curBreakpoint;
            memset(&curBreakpoint, 0, sizeof(BREAKPOINT));
            curBreakpoint.type = (BP_TYPE)json_integer_value(json_object_get(value, "type"));
            if(curBreakpoint.type == BPNORMAL)
                curBreakpoint.oldbytes = (short)json_hex_value(json_object_get(value, "oldbytes"));
            curBreakpoint.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curBreakpoint.enabled = json_boolean_value(json_object_get(value, "enabled"));
            curBreakpoint.titantype = (DWORD)json_hex_value(json_object_get(value, "titantype"));
            const char* name = json_string_value(json_object_get(value, "name"));
            if(name)
                strcpy_s(curBreakpoint.name, name);
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(curBreakpoint.mod, mod);
            const uint key = modhashfromname(curBreakpoint.mod) + curBreakpoint.addr;
            breakpoints.insert(std::make_pair(BreakpointKey(curBreakpoint.type, key), curBreakpoint));
        }
    }
}

void bpclear()
{
    CriticalSectionLocker locker(LockBreakpoints);
    BreakpointsInfo().swap(breakpoints);
}