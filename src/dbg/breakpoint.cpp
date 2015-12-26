/**
 @file breakpoint.cpp

 @brief Implements the breakpoint class.
 */

#include "breakpoint.h"
#include "memory.h"
#include "threading.h"
#include "module.h"

typedef std::pair<BP_TYPE, duint> BreakpointKey;
std::map<BreakpointKey, BREAKPOINT> breakpoints;

BREAKPOINT* BpInfoFromAddr(BP_TYPE Type, duint Address)
{
    //
    // NOTE: THIS DOES _NOT_ USE LOCKS
    //
    auto found = breakpoints.find(BreakpointKey(Type, ModHashFromAddr(Address)));

    // Was the module found with this address?
    if(found == breakpoints.end())
        return nullptr;

    return &found->second;
}

int BpGetList(std::vector<BREAKPOINT>* List)
{
    SHARED_ACQUIRE(LockBreakpoints);

    // Did the caller request an output?
    if(List)
    {
        // Enumerate all breakpoints in the global list, fixing the relative
        // offset to a virtual address
        for(auto & i : breakpoints)
        {
            BREAKPOINT currentBp = i.second;
            currentBp.addr += ModBaseFromName(currentBp.mod);
            currentBp.active = MemIsValidReadPtr(currentBp.addr);

            List->push_back(currentBp);
        }
    }

    return (int)breakpoints.size();
}

bool BpNew(duint Address, bool Enable, bool Singleshot, short OldBytes, BP_TYPE Type, DWORD TitanType, const char* Name)
{
    ASSERT_DEBUGGING("Export call");

    // Fail if the address is a bad memory region
    if(!MemIsValidReadPtr(Address))
        return false;

    // Fail if the breakpoint already exists
    if(BpGet(Address, Type, Name, nullptr))
        return false;

    // Default to an empty name if one wasn't supplied
    if(!Name)
        Name = "";

    BREAKPOINT bp;
    memset(&bp, 0, sizeof(BREAKPOINT));

    ModNameFromAddr(Address, bp.mod, true);
    strcpy_s(bp.name, Name);

    bp.active = true;
    bp.addr = Address - ModBaseFromAddr(Address);
    bp.enabled = Enable;
    bp.oldbytes = OldBytes;
    bp.singleshoot = Singleshot;
    bp.titantype = TitanType;
    bp.type = Type;

    // Insert new entry to the global list
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    breakpoints.insert(std::make_pair(BreakpointKey(Type, ModHashFromAddr(Address)), bp));
    return true;
}

bool BpGet(duint Address, BP_TYPE Type, const char* Name, BREAKPOINT* Bp)
{
    ASSERT_DEBUGGING("Export call");
    SHARED_ACQUIRE(LockBreakpoints);

    // Name is optional
    if(!Name || Name[0] == '\0')
    {
        // Perform a lookup by address only
        BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

        if(!bpInfo)
            return false;

        // Succeed even if the user didn't request anything
        if(!Bp)
            return true;

        *Bp = *bpInfo;
        Bp->addr += ModBaseFromAddr(Address);
        Bp->active = MemIsValidReadPtr(Bp->addr);
        return true;
    }

    // Do a lookup by breakpoint name
    for(auto & i : breakpoints)
    {
        // Do the names match?
        if(strcmp(Name, i.second.name) != 0)
            continue;

        // Fill out the optional user buffer
        if(Bp)
        {
            *Bp = i.second;
            Bp->addr += ModBaseFromAddr(Address);
            Bp->active = MemIsValidReadPtr(Bp->addr);
        }

        // Return true if the name was found at all
        return true;
    }

    return false;
}

bool BpDelete(duint Address, BP_TYPE Type)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Erase the index from the global list
    return (breakpoints.erase(BreakpointKey(Type, ModHashFromAddr(Address))) > 0);
}

bool BpEnable(duint Address, BP_TYPE Type, bool Enable)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Check if the breakpoint exists first
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    bpInfo->enabled = Enable;

    //Re-read oldbytes
    if(Enable && Type == BPNORMAL)
    {
        if(!MemRead(Address, &bpInfo->oldbytes, sizeof(bpInfo->oldbytes)))
            return false;
    }
    return true;
}

bool BpSetName(duint Address, BP_TYPE Type, const char* Name)
{
    ASSERT_DEBUGGING("Future(?): This is not used anywhere");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // If a name wasn't supplied, set to nothing
    if(!Name)
        Name = "";

    // Check if the breakpoint exists first
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    strcpy_s(bpInfo->name, Name);
    return true;
}

bool BpSetTitanType(duint Address, BP_TYPE Type, int TitanType)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Set the TitanEngine type, separate from BP_TYPE
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    bpInfo->titantype = TitanType;
    return true;
}

bool BpEnumAll(BPENUMCALLBACK EnumCallback, const char* Module)
{
    ASSERT_DEBUGGING("Export call");
    SHARED_ACQUIRE(LockBreakpoints);

    // Loop each entry, executing the user's callback
    bool callbackStatus = true;

    for(auto i = breakpoints.begin(); i != breakpoints.end();)
    {
        auto j = i;
        ++i; // Increment here, because the callback might remove the current entry

        // If a module name was sent, check it
        if(Module && Module[0] != '\0')
        {
            if(strcmp(j->second.mod, Module) != 0)
                continue;
        }

        BREAKPOINT bpInfo = j->second;
        bpInfo.addr += ModBaseFromName(bpInfo.mod);
        bpInfo.active = MemIsValidReadPtr(bpInfo.addr);

        // Lock must be released due to callback sub-locks
        SHARED_RELEASE();

        // Execute the callback
        if(!EnumCallback(&bpInfo))
            callbackStatus = false;

        // Restore the breakpoint map lock
        SHARED_REACQUIRE();
    }

    return callbackStatus;
}

bool BpEnumAll(BPENUMCALLBACK EnumCallback)
{
    return BpEnumAll(EnumCallback, nullptr);
}

int BpGetCount(BP_TYPE Type, bool EnabledOnly)
{
    SHARED_ACQUIRE(LockBreakpoints);

    // Count the number of enabled/disabled breakpoint types
    int count = 0;

    for(auto & i : breakpoints)
    {
        // Check if the type matches
        if(i.first.first != Type)
            continue;

        // If it's not enabled, skip it
        if(EnabledOnly && !i.second.enabled)
            continue;

        count++;
    }

    return count;
}

void BpToBridge(const BREAKPOINT* Bp, BRIDGEBP* BridgeBp)
{
    //
    // Convert a debugger breakpoint to an open/exported
    // bridge breakpoint
    //
    ASSERT_NONNULL(Bp);
    ASSERT_NONNULL(BridgeBp);

    memset(BridgeBp, 0, sizeof(BRIDGEBP));
    strcpy_s(BridgeBp->mod, Bp->mod);
    strcpy_s(BridgeBp->name, Bp->name);

    BridgeBp->active = Bp->active;
    BridgeBp->addr = Bp->addr;
    BridgeBp->enabled = Bp->enabled;
    BridgeBp->singleshoot = Bp->singleshoot;

    switch(Bp->type)
    {
    case BPNORMAL:
        BridgeBp->type = bp_normal;
        break;
    case BPHARDWARE:
        BridgeBp->type = bp_hardware;
        break;
    case BPMEMORY:
        BridgeBp->type = bp_memory;
        break;
    default:
        BridgeBp->type = bp_none;
        break;
    }
}

void BpCacheSave(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Create a JSON array to store each sub-object with a breakpoint
    const JSON jsonBreakpoints = json_array();

    // Loop all breakpoints
    for(auto & i : breakpoints)
    {
        auto & breakpoint = i.second;

        // Ignore single-shot breakpoints
        if(breakpoint.singleshoot)
            continue;

        JSON jsonObj = json_object();
        json_object_set_new(jsonObj, "address", json_hex(breakpoint.addr));
        json_object_set_new(jsonObj, "enabled", json_boolean(breakpoint.enabled));

        // "Normal" breakpoints save the old data
        if(breakpoint.type == BPNORMAL)
            json_object_set_new(jsonObj, "oldbytes", json_hex(breakpoint.oldbytes));

        json_object_set_new(jsonObj, "type", json_integer(breakpoint.type));
        json_object_set_new(jsonObj, "titantype", json_hex(breakpoint.titantype));
        json_object_set_new(jsonObj, "name", json_string(breakpoint.name));
        json_object_set_new(jsonObj, "module", json_string(breakpoint.mod));
        json_array_append_new(jsonBreakpoints, jsonObj);
    }

    if(json_array_size(jsonBreakpoints))
        json_object_set(Root, "breakpoints", jsonBreakpoints);

    // Notify garbage collector
    json_decref(jsonBreakpoints);
}

void BpCacheLoad(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Remove all existing elements
    breakpoints.clear();

    // Get a handle to the root object -> breakpoints subtree
    const JSON jsonBreakpoints = json_object_get(Root, "breakpoints");

    // Return if there was nothing to load
    if(!jsonBreakpoints)
        return;

    size_t i;
    JSON value;
    json_array_foreach(jsonBreakpoints, i, value)
    {
        BREAKPOINT breakpoint;
        memset(&breakpoint, 0, sizeof(BREAKPOINT));

        if(breakpoint.type == BPNORMAL)
            breakpoint.oldbytes = (unsigned short)(json_hex_value(json_object_get(value, "oldbytes")) & 0xFFFF);
        breakpoint.type = (BP_TYPE)json_integer_value(json_object_get(value, "type"));
        breakpoint.addr = (duint)json_hex_value(json_object_get(value, "address"));
        breakpoint.enabled = json_boolean_value(json_object_get(value, "enabled"));
        breakpoint.titantype = (DWORD)json_hex_value(json_object_get(value, "titantype"));

        // Name
        const char* name = json_string_value(json_object_get(value, "name"));

        if(name)
            strcpy_s(breakpoint.name, name);

        // Module
        const char* mod = json_string_value(json_object_get(value, "module"));

        if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
            strcpy_s(breakpoint.mod, mod);

        // Build the hash map key: MOD_HASH + ADDRESS
        const duint key = ModHashFromName(breakpoint.mod) + breakpoint.addr;
        breakpoints.insert(std::make_pair(BreakpointKey(breakpoint.type, key), breakpoint));
    }
}

void BpClear()
{
    EXCLUSIVE_ACQUIRE(LockBreakpoints);
    breakpoints.clear();
}