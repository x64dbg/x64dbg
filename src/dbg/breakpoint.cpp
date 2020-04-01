/**
 @file breakpoint.cpp

 @brief Implements the breakpoint class.
 */

#include "breakpoint.h"
#include "memory.h"
#include "threading.h"
#include "module.h"
#include "value.h"
#include "debugger.h"
#include "exception.h"
#include <algorithm>

typedef std::pair<BP_TYPE, duint> BreakpointKey;
std::map<BreakpointKey, BREAKPOINT> breakpoints;

static void setBpActive(BREAKPOINT & bp)
{
    if(bp.type == BPHARDWARE) //TODO: properly implement this (check debug registers)
        bp.active = true;
    else if(bp.type == BPDLL || bp.type == BPEXCEPTION)
        bp.active = true;
    else
        bp.active = MemIsValidReadPtr(bp.addr);
}

BREAKPOINT* BpInfoFromAddr(BP_TYPE Type, duint Address)
{
    //
    // NOTE: THIS DOES _NOT_ USE LOCKS
    //
    std::map<BreakpointKey, BREAKPOINT>::iterator found;
    if(Type != BPDLL && Type != BPEXCEPTION)
        found = breakpoints.find(BreakpointKey(Type, ModHashFromAddr(Address)));
    else
        found = breakpoints.find(BreakpointKey(Type, Address)); // Address = ModHashFromName(ModuleName)

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
            if(currentBp.type != BPDLL && currentBp.type != BPEXCEPTION)
                currentBp.addr += ModBaseFromName(currentBp.mod);
            setBpActive(currentBp);

            List->push_back(currentBp);
        }
        std::sort(List->begin(), List->end(), [](const BREAKPOINT & a, const BREAKPOINT & b)
        {
            return std::make_pair(a.type, a.addr) < std::make_pair(b.type, b.addr);
        });
    }

    return (int)breakpoints.size();
}

bool BpNew(duint Address, bool Enable, bool Singleshot, short OldBytes, BP_TYPE Type, DWORD TitanType, const char* Name, duint memsize)
{
    ASSERT_DEBUGGING("Export call");

    // Fail if the address is a bad memory region
    if(Type != BPDLL && Type != BPEXCEPTION)
    {
        if(!MemIsValidReadPtr(Address))
            return false;
    }

    // Fail if the breakpoint already exists
    if(BpGet(Address, Type, Name, nullptr))
        return false;

    // Default to an empty name if one wasn't supplied
    if(!Name)
        Name = "";

    BREAKPOINT bp;
    memset(&bp, 0, sizeof(BREAKPOINT));

    if(Type != BPDLL && Type != BPEXCEPTION)
    {
        ModNameFromAddr(Address, bp.mod, true);
    }
    strncpy_s(bp.name, Name, _TRUNCATE);

    bp.active = true;
    if(Type != BPDLL && Type != BPEXCEPTION)
        bp.addr = Address - ModBaseFromAddr(Address);
    else
        bp.addr = Address;
    bp.enabled = Enable;
    bp.oldbytes = OldBytes;
    bp.singleshoot = Singleshot;
    bp.titantype = TitanType;
    bp.type = Type;
    bp.memsize = memsize;

    // Insert new entry to the global list
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    if(Type != BPDLL && Type != BPEXCEPTION)
    {
        return breakpoints.insert(std::make_pair(BreakpointKey(Type, ModHashFromAddr(Address)), bp)).second;
    }
    else
    {
        return breakpoints.insert(std::make_pair(BreakpointKey(Type, Address), bp)).second;
    }
}

bool BpNewDll(const char* module, bool Enable, bool Singleshot, DWORD TitanType, const char* Name)
{
    // Default to an empty name if one wasn't supplied
    if(!Name)
        Name = "";

    BREAKPOINT bp;
    memset(&bp, 0, sizeof(BREAKPOINT));
    strcpy_s(bp.mod, module);
    strcpy_s(bp.name, Name);
    bp.active = true;
    bp.enabled = Enable;
    bp.singleshoot = Singleshot;
    bp.titantype = TitanType;
    bp.type = BPDLL;
    bp.addr = BpGetDLLBpAddr(module);

    // Insert new entry to the global list
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    return breakpoints.insert(std::make_pair(BreakpointKey(BPDLL, bp.addr), bp)).second;
}

bool BpGet(duint Address, BP_TYPE Type, const char* Name, BREAKPOINT* Bp)
{
    if(!DbgIsDebugging())
        return false;
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
        if(bpInfo->type != BPDLL && bpInfo->type != BPEXCEPTION)
            Bp->addr += ModBaseFromAddr(Address);
        setBpActive(*Bp);
        return true;
    }

    // If name in a special format "libwinpthread-1.dll":$7792, find the breakpoint even if the DLL might not be loaded yet.
    const char* separatorPos;
    separatorPos = strstr(Name, ":$"); //DLL file names cannot contain ":" char anyway, so ignoring the quotes is fine. The following part of RVA expression might contain ":$"?
    if(separatorPos && Type != BPDLL && Type != BPEXCEPTION)
    {
        char* DLLName = _strdup(Name);
        char* RVAPos = DLLName + (separatorPos - Name);
        RVAPos[0] = RVAPos[1] = '\0';
        RVAPos = RVAPos + 2; //Now 2 strings separated by NULs
        if(valfromstring(RVAPos, &Address)) //"Address" reused here. No usage of original "Address" argument.
        {
            if(separatorPos != Name)   //Check if DLL name is surrounded by quotes. Don't be out of bounds!
            {
                if(DLLName[0] == '"' && RVAPos[-3] == '"')
                {
                    RVAPos[-3] = '\0';
                    DLLName[0] = '\0';
                }
            }
            if(DLLName[0] != '\0')
            {
                duint base = ModBaseFromName(DLLName); //Is the DLL actually loaded?
                Address += base ? base : ModHashFromName(DLLName);
            }
            else
            {
                duint base = ModBaseFromName(DLLName + 1);
                Address += base ? base : ModHashFromName(DLLName + 1);
            }

            // Perform a lookup by address only
            BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

            if(!bpInfo)
                return false;

            // Succeed even if the user didn't request anything
            if(!Bp)
                return true;

            *Bp = *bpInfo;
            Bp->addr = Address;
            setBpActive(*Bp);
            return true;
        }
        free(DLLName);
    }

    // Do a lookup by breakpoint name
    for(auto & i : breakpoints)
    {
        // Do the names match?
        if(_stricmp(Name, i.second.name) != 0)
            continue;

        // Fill out the optional user buffer
        if(Bp)
        {
            *Bp = i.second;
            if(i.second.type != BPDLL && i.second.type != BPEXCEPTION)
                Bp->addr += ModBaseFromAddr(Address);
            setBpActive(*Bp);
        }

        // Return true if the name was found at all
        return true;
    }

    return false;
}

bool BpGetAny(BP_TYPE Type, const char* Name, BREAKPOINT* Bp)
{
    if(BpGet(0, Type, Name, Bp))
        return true;
    if(Type != BPDLL)
    {
        duint addr;
        if(valfromstring(Name, &addr))
            if(BpGet(addr, Type, 0, Bp))
                return true;
        if(Type == BPEXCEPTION)
        {
            addr = 0;
            if(ExceptionNameToCode(Name, reinterpret_cast<unsigned int*>(&addr)))
                if(BpGet(addr, BPEXCEPTION, 0, Bp))
                    return true;
        }
    }
    else
    {
        if(BpGet(BpGetDLLBpAddr(Name), Type, 0, Bp))
            return true;
    }
    return false;
}

bool BpUpdateDllPath(const char* module1, BREAKPOINT** newBpInfo)
{
    const char* dashPos1 = max(strrchr(module1, '\\'), strrchr(module1, '/'));
    EXCLUSIVE_ACQUIRE(LockBreakpoints);
    for(auto & i : breakpoints)
    {
        BREAKPOINT & bpRef = i.second;
        if(bpRef.type == BPDLL && bpRef.enabled)
        {
            if(_stricmp(bpRef.mod, module1) == 0)
            {
                BREAKPOINT temp;
                temp = bpRef;
                strcpy_s(temp.mod, module1);
                temp.addr = ModHashFromName(module1);
                breakpoints.erase(i.first);
                auto newItem = breakpoints.insert(std::make_pair(BreakpointKey(BPDLL, temp.addr), temp));
                *newBpInfo = &newItem.first->second;
                return true;
            }
            const char* dashPos = max(strrchr(bpRef.mod, '\\'), strrchr(bpRef.mod, '/'));
            if(dashPos == nullptr)
                dashPos = bpRef.mod;
            else
                dashPos += 1;
            if(dashPos1 != nullptr && _stricmp(dashPos, dashPos1 + 1) == 0) // filename matches
            {
                BREAKPOINT temp;
                temp = bpRef;
                strcpy_s(temp.mod, dashPos1 + 1);
                temp.addr = ModHashFromName(dashPos1 + 1);
                breakpoints.erase(i.first);
                auto newItem = breakpoints.insert(std::make_pair(BreakpointKey(BPDLL, temp.addr), temp));
                *newBpInfo = &newItem.first->second;
                return true;
            }
        }
    }
    *newBpInfo = nullptr;
    return false;
}

duint BpGetDLLBpAddr(const char* fileName)
{
    const char* dashPos1 = max(strrchr(fileName, '\\'), strrchr(fileName, '/'));
    if(dashPos1 == nullptr)
        dashPos1 = fileName;
    else
        dashPos1++;
    return ModHashFromName(dashPos1);
}

bool BpDelete(duint Address, BP_TYPE Type)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Erase the index from the global list
    if(Type != BPDLL)
        return (breakpoints.erase(BreakpointKey(Type, ModHashFromAddr(Address))) > 0);
    else
        return (breakpoints.erase(BreakpointKey(BPDLL, Address)) > 0);
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

    strncpy_s(bpInfo->name, Name, _TRUNCATE);
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

bool BpSetBreakCondition(duint Address, BP_TYPE Type, const char* Condition)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Set breakpoint breakCondition
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    strncpy_s(bpInfo->breakCondition, Condition, _TRUNCATE);
    return true;
}

bool BpSetLogText(duint Address, BP_TYPE Type, const char* Log)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Set breakpoint logText
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    strncpy_s(bpInfo->logText, Log, _TRUNCATE);

    // Make log breakpoints silent (meaning they don't output the default log).
    bpInfo->silent = *Log != '\0';
    return true;
}

bool BpSetLogCondition(duint Address, BP_TYPE Type, const char* Condition)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Set breakpoint logText
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    strncpy_s(bpInfo->logCondition, Condition, _TRUNCATE);
    return true;
}

bool BpSetCommandText(duint Address, BP_TYPE Type, const char* Cmd)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Set breakpoint hit command
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    strncpy_s(bpInfo->commandText, Cmd, _TRUNCATE);
    return true;
}

bool BpSetCommandCondition(duint Address, BP_TYPE Type, const char* Condition)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Set breakpoint hit command
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    strncpy_s(bpInfo->commandCondition, Condition, _TRUNCATE);
    return true;
}

bool BpSetFastResume(duint Address, BP_TYPE Type, bool fastResume)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Set breakpoint fast resume
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    bpInfo->fastResume = fastResume;
    return true;
}

bool BpSetSingleshoot(duint Address, BP_TYPE Type, bool singleshoot)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Set breakpoint fast resume
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    bpInfo->singleshoot = singleshoot;
    return true;
}

bool BpSetSilent(duint Address, BP_TYPE Type, bool silent)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Set breakpoint fast resume
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    bpInfo->silent = silent;
    return true;
}

bool BpEnumAll(BPENUMCALLBACK EnumCallback, const char* Module, duint base)
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
        if(Module)
        {
            if(strcmp(j->second.mod, Module) != 0)
                continue;
        }

        BREAKPOINT bpInfo = j->second;
        if(bpInfo.type != BPDLL && bpInfo.type != BPEXCEPTION)
        {
            if(base) //workaround for some Windows bullshit with compatibility mode
                bpInfo.addr += base;
            else
                bpInfo.addr += ModBaseFromName(bpInfo.mod);
        }
        setBpActive(bpInfo);

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


uint32 BpGetHitCount(duint Address, BP_TYPE Type)
{
    SHARED_ACQUIRE(LockBreakpoints);

    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return 0;

    return bpInfo->hitcount;
}

bool BpResetHitCount(duint Address, BP_TYPE Type, uint32 newHitCount)
{
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    bpInfo->hitcount = newHitCount;
    return true;
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
    strncpy_s(BridgeBp->mod, Bp->mod, _TRUNCATE);
    strncpy_s(BridgeBp->name, Bp->name, _TRUNCATE);
    strncpy_s(BridgeBp->breakCondition, Bp->breakCondition, _TRUNCATE);
    strncpy_s(BridgeBp->logText, Bp->logText, _TRUNCATE);
    strncpy_s(BridgeBp->logCondition, Bp->logCondition, _TRUNCATE);
    strncpy_s(BridgeBp->commandText, Bp->commandText, _TRUNCATE);
    strncpy_s(BridgeBp->commandCondition, Bp->commandCondition, _TRUNCATE);

    BridgeBp->active = Bp->active;
    BridgeBp->addr = Bp->addr;
    BridgeBp->enabled = Bp->enabled;
    BridgeBp->singleshoot = Bp->singleshoot;
    BridgeBp->fastResume = Bp->fastResume;
    BridgeBp->silent = Bp->silent;
    BridgeBp->hitCount = Bp->hitcount;

    switch(Bp->type)
    {
    case BPNORMAL:
        BridgeBp->type = bp_normal;
        break;
    case BPHARDWARE:
        BridgeBp->type = bp_hardware;
        switch(TITANGETDRX(Bp->titantype))
        {
        case UE_DR0:
            BridgeBp->slot = 0;
            break;
        case UE_DR1:
            BridgeBp->slot = 1;
            break;
        case UE_DR2:
            BridgeBp->slot = 2;
            break;
        case UE_DR3:
            BridgeBp->slot = 3;
            break;
        }
        switch(TITANGETSIZE(Bp->titantype))
        {
        case UE_HARDWARE_SIZE_1:
            BridgeBp->hwSize = hw_byte;
            break;
        case UE_HARDWARE_SIZE_2:
            BridgeBp->hwSize = hw_word;
            break;
        case UE_HARDWARE_SIZE_4:
            BridgeBp->hwSize = hw_dword;
            break;
        case UE_HARDWARE_SIZE_8:
            BridgeBp->hwSize = hw_qword;
            break;
        }
        switch(TITANGETTYPE(Bp->titantype))
        {
        case UE_HARDWARE_READWRITE:
            BridgeBp->typeEx = hw_access;
            break;
        case UE_HARDWARE_WRITE:
            BridgeBp->typeEx = hw_write;
            break;
        case UE_HARDWARE_EXECUTE:
            BridgeBp->typeEx = hw_execute;
            break;
        }
        break;
    case BPMEMORY:
        BridgeBp->type = bp_memory;
        break;
    case BPDLL:
        BridgeBp->type = bp_dll;
        switch(Bp->titantype)
        {
        case UE_ON_LIB_LOAD:
            BridgeBp->typeEx = dll_load;
            break;
        case UE_ON_LIB_UNLOAD:
            BridgeBp->typeEx = dll_unload;
            break;
        case UE_ON_LIB_ALL:
            BridgeBp->typeEx = dll_all;
            break;
        }
        break;
    case BPEXCEPTION:
        BridgeBp->type = bp_exception;
        switch(Bp->titantype) //1:First-chance, 2:Second-chance, 3:Both
        {
        case 1:
            BridgeBp->typeEx = ex_firstchance;
            break;
        case 2:
            BridgeBp->typeEx = ex_secondchance;
            break;
        case 3:
            BridgeBp->typeEx = ex_all;
            break;
        default:
            __debugbreak();
        }
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
    for(const auto & i : breakpoints)
    {
        const auto & breakpoint = i.second;

        // Ignore single-shot breakpoints
        if(breakpoint.singleshoot)
            continue;

        JSON jsonObj = json_object();
        json_object_set_new(jsonObj, "address", json_hex(breakpoint.addr));
        json_object_set_new(jsonObj, "enabled", json_boolean(breakpoint.enabled));

        if(breakpoint.type == BPNORMAL) // "Normal" breakpoints save the old data
            json_object_set_new(jsonObj, "oldbytes", json_hex(breakpoint.oldbytes));
        else if(breakpoint.type == BPMEMORY) // Memory breakpoints save the memory size
            json_object_set_new(jsonObj, "memsize", json_hex(breakpoint.memsize));

        json_object_set_new(jsonObj, "type", json_integer(breakpoint.type));
        json_object_set_new(jsonObj, "titantype", json_hex(breakpoint.titantype));
        json_object_set_new(jsonObj, "name", json_string(breakpoint.name));
        if(breakpoint.type != BPEXCEPTION)
            json_object_set_new(jsonObj, "module", json_string(breakpoint.mod));
        json_object_set_new(jsonObj, "breakCondition", json_string(breakpoint.breakCondition));
        json_object_set_new(jsonObj, "logText", json_string(breakpoint.logText));
        json_object_set_new(jsonObj, "logCondition", json_string(breakpoint.logCondition));
        json_object_set_new(jsonObj, "commandText", json_string(breakpoint.commandText));
        json_object_set_new(jsonObj, "commandCondition", json_string(breakpoint.commandCondition));
        json_object_set_new(jsonObj, "fastResume", json_boolean(breakpoint.fastResume));
        json_object_set_new(jsonObj, "silent", json_boolean(breakpoint.silent));
        json_array_append_new(jsonBreakpoints, jsonObj);
    }

    if(json_array_size(jsonBreakpoints))
        json_object_set(Root, "breakpoints", jsonBreakpoints);

    // Notify garbage collector
    json_decref(jsonBreakpoints);
}

template<typename T>
static void loadStringValue(JSON value, T & dest, const char* key)
{
    auto text = json_string_value(json_object_get(value, key));
    if(text)
        strncpy_s(dest, text, _TRUNCATE);
}

void BpCacheLoad(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

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

        breakpoint.type = (BP_TYPE)json_integer_value(json_object_get(value, "type"));
        if(breakpoint.type == BPNORMAL)
            breakpoint.oldbytes = (unsigned short)(json_hex_value(json_object_get(value, "oldbytes")) & 0xFFFF);
        else if(breakpoint.type == BPMEMORY)
            breakpoint.memsize = (duint)json_hex_value(json_object_get(value, "memsize"));
        breakpoint.addr = (duint)json_hex_value(json_object_get(value, "address"));
        breakpoint.enabled = json_boolean_value(json_object_get(value, "enabled"));
        breakpoint.titantype = (DWORD)json_hex_value(json_object_get(value, "titantype"));

        // String values
        loadStringValue(value, breakpoint.name, "name");
        if(breakpoint.type != BPEXCEPTION)
            loadStringValue(value, breakpoint.mod, "module");
        loadStringValue(value, breakpoint.breakCondition, "breakCondition");
        loadStringValue(value, breakpoint.logText, "logText");
        loadStringValue(value, breakpoint.logCondition, "logCondition");
        loadStringValue(value, breakpoint.commandText, "commandText");
        loadStringValue(value, breakpoint.commandCondition, "commandCondition");

        // Fast resume
        breakpoint.fastResume = json_boolean_value(json_object_get(value, "fastResume"));
        breakpoint.silent = json_boolean_value(json_object_get(value, "silent"));

        // Build the hash map key: MOD_HASH + ADDRESS
        duint key;
        if(breakpoint.type != BPDLL)
        {
            key = ModHashFromName(breakpoint.mod) + breakpoint.addr;
        }
        else
        {
            key = BpGetDLLBpAddr(breakpoint.mod);
        }
        breakpoints[BreakpointKey(breakpoint.type, key)] = breakpoint;
    }
}

void BpClear()
{
    EXCLUSIVE_ACQUIRE(LockBreakpoints);
    breakpoints.clear();
}
