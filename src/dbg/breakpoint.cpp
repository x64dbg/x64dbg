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
static std::map<BreakpointKey, BREAKPOINT> breakpoints;

struct BreakpointLogFile
{
    int refCount = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    bool needsFlush = false;
};

static std::unordered_map<std::string,
       BreakpointLogFile,
       StringUtils::CaseInsensitiveHash,
       StringUtils::CaseInsensitiveEqual> breakpointLogFiles;
bool bTruncateBreakpointLogs = false;

static void setBpActive(BREAKPOINT & bp, duint addrAdjust = 0)
{
    // DLL/Exception breakpoints are always enabled
    if(bp.type == BPDLL || bp.type == BPEXCEPTION)
    {
        bp.active = true;
        return;
    }

    // Breakpoints without modules need a valid address
    if(bp.module.empty())
    {
        bp.active = MemIsValidReadPtr(bp.addr);
        return;
    }
    else
    {
        auto modLoaded = ModBaseFromName(bp.module.c_str()) != 0;
        if(bp.type == BPHARDWARE)
            bp.active = modLoaded;
        else
            bp.active = modLoaded && MemIsValidReadPtr(bp.addr + addrAdjust);
    }
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
                currentBp.addr += ModBaseFromName(currentBp.module.c_str());
            setBpActive(currentBp); // address adjusted

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

    // Use BpNewDll for DLL breakpoints
    if(Type == BPDLL)
        __debugbreak();

    // Fail if the address is a bad memory region
    if(Type != BPEXCEPTION)
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

    if(Type != BPEXCEPTION)
    {
        char mod[MAX_MODULE_SIZE] = "";
        ModNameFromAddr(Address, mod, true);
        bp.module = mod;
    }
    bp.name = Name;

    bp.active = true;
    if(Type != BPEXCEPTION)
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

    if(Type != BPEXCEPTION)
    {
        return breakpoints.emplace(BreakpointKey(Type, ModHashFromAddr(Address)), bp).second;
    }
    else
    {
        return breakpoints.emplace(BreakpointKey(Type, Address), bp).second;
    }
}

bool BpNewDll(const char* module, bool Enable, bool Singleshot, DWORD TitanType, const char* Name)
{
    // Default to an empty name if one wasn't supplied
    if(!Name)
        Name = "";

    BREAKPOINT bp;
    bp.module = module;
    bp.name = Name;
    bp.active = true;
    bp.enabled = Enable;
    bp.singleshoot = Singleshot;
    bp.titantype = TitanType;
    bp.type = BPDLL;
    bp.addr = BpGetDLLBpAddr(module);

    // Insert new entry to the global list
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    return breakpoints.emplace(BreakpointKey(BPDLL, bp.addr), bp).second;
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
        setBpActive(*Bp); // address adjusted
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
        duint Rva;
        if(valfromstring(RVAPos, &Rva))
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
                Rva += base ? base : ModHashFromName(DLLName);
            }
            else
            {
                duint base = ModBaseFromName(DLLName + 1);
                Rva += base ? base : ModHashFromName(DLLName + 1);
            }

            free(DLLName);

            // Perform a lookup by address only
            BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Rva);

            if(!bpInfo)
                return false;

            // Succeed even if the user didn't request anything
            if(!Bp)
                return true;

            *Bp = *bpInfo;
            Bp->addr = Rva;
            setBpActive(*Bp); // address is modhash
            return true;
        }
        else
        {
            free(DLLName);
        }
    }

    // Do a lookup by breakpoint name
    for(auto & i : breakpoints)
    {
        // Breakpoint name match
        if(_stricmp(Name, i.second.name.c_str()) != 0)
            // Module name match in case of DLL Breakpoints
            if(i.second.type != BPDLL || _stricmp(Name, i.second.module.c_str()) != 0)
                continue;

        // Fill out the optional user buffer
        if(Bp)
        {
            *Bp = i.second;
            if(i.second.type != BPDLL && i.second.type != BPEXCEPTION)
                Bp->addr += ModBaseFromAddr(Address);
            setBpActive(*Bp); // address adjusted
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

duint BpGetDLLBpAddr(const char* fileName)
{
    const char* dashPos1 = max(strrchr(fileName, '\\'), strrchr(fileName, '/'));
    if(dashPos1 == nullptr)
        dashPos1 = fileName;
    else
        dashPos1++;
    return ModHashFromName(dashPos1);
}

static bool safeDelete(BP_TYPE Type, duint AddressHash)
{
    auto itr = breakpoints.find(BreakpointKey(Type, AddressHash));
    if(itr == breakpoints.end())
    {
        return false;
    }

    BpLogFileRelease(itr->second.logFile);
    breakpoints.erase(itr);
    return true;
}

bool BpDelete(duint Address, BP_TYPE Type)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Erase the index from the global list
    if(Type != BPDLL && Type != BPEXCEPTION)
        return safeDelete(Type, ModHashFromAddr(Address));
    else
        return safeDelete(Type, Address);
}

bool BpDelete(const BREAKPOINT & Bp)
{
    // Breakpoints without a module can be deleted without special logic
    if(Bp.type == BPDLL || Bp.type == BPEXCEPTION || Bp.module.empty())
        return safeDelete(Bp.type, Bp.addr);

    // Extract the RVA from the breakpoint
    auto rva = Bp.addr;
    auto loadedBase = ModBaseFromName(Bp.module.c_str());
    if(loadedBase != 0 && Bp.addr > loadedBase)
        rva -= loadedBase;

    // Calculate the breakpoint key with the module hash and rva
    auto modHash = ModHashFromName(Bp.module.c_str());
    return safeDelete(Bp.type, modHash + rva);
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

    bpInfo->name = Name;
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

    bpInfo->breakCondition = Condition;
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

    bpInfo->logText = Log;

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

    bpInfo->logCondition = Condition;
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

    bpInfo->commandText = Cmd;
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

    bpInfo->commandCondition = Condition;
    return true;
}

bool BpSetLogFile(duint Address, BP_TYPE Type, const char* LogFile)
{
    ASSERT_DEBUGGING("Command function call");
    EXCLUSIVE_ACQUIRE(LockBreakpoints);

    // Set breakpoint hit command
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    std::string newLogFile = LogFile;
    BpLogFileAcquire(newLogFile);
    BpLogFileRelease(bpInfo->logFile);
    bpInfo->logFile = std::move(newLogFile);
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

    // Set breakpoint singleshoot
    BREAKPOINT* bpInfo = BpInfoFromAddr(Type, Address);

    if(!bpInfo)
        return false;

    bpInfo->singleshoot = singleshoot;
    // Update singleshoot information in TitanEngine
    switch(Type)
    {
    case BPNORMAL:
        bpInfo->titantype = (bpInfo->titantype & ~UE_SINGLESHOOT) | (singleshoot ? UE_SINGLESHOOT : 0);
        if(IsBPXEnabled(Address) && bpInfo->enabled)
        {
            if(!DeleteBPX(Address))
                dprintf(QT_TRANSLATE_NOOP("DBG", "Delete breakpoint failed (DeleteBPX): %p\n"), Address);
            if(!SetBPX(Address, bpInfo->titantype, cbUserBreakpoint))
                dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting breakpoint at %p! (SetBPX)\n"), Address);
        }
        break;
    case BPMEMORY:
        if(bpInfo->enabled)
        {
            if(!RemoveMemoryBPX(Address, bpInfo->memsize))
                dprintf(QT_TRANSLATE_NOOP("DBG", "Delete memory breakpoint failed (RemoveMemoryBPX): %p\n"), Address);
            if(!SetMemoryBPXEx(Address, bpInfo->memsize, bpInfo->titantype, !singleshoot, cbMemoryBreakpoint))
                dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable memory breakpoint %p (SetMemoryBPXEx)\n"), Address);
        }
        break;
    }
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
            if(strcmp(j->second.module.c_str(), Module) != 0)
                continue;
        }

        BREAKPOINT bpInfo = j->second;
        if(bpInfo.type != BPDLL && bpInfo.type != BPEXCEPTION)
        {
            if(base) //workaround for some Windows bullshit with compatibility mode
                bpInfo.addr += base;
            else
                bpInfo.addr += ModBaseFromName(bpInfo.module.c_str());
        }
        setBpActive(bpInfo); // address adjusted

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

static BPXTYPE BpTypeToBridge(BP_TYPE type)
{
    switch(type)
    {
    case BPNORMAL:
        return bp_normal;
    case BPHARDWARE:
        return bp_hardware;
    case BPMEMORY:
        return bp_memory;
    case BPDLL:
        return bp_dll;
    case BPEXCEPTION:
        return bp_exception;
    default:
        return bp_none;
    }
}

static BP_TYPE BpTypeFromBridge(BPXTYPE type)
{
    switch(type)
    {
    case bp_normal:
        return BPNORMAL;
    case bp_hardware:
        return BPHARDWARE;
    case bp_memory:
        return BPMEMORY;
    case bp_dll:
        return BPDLL;
    case bp_exception:
        return BPEXCEPTION;
    default:
        return BPNORMAL;
    }
}

static duint BpToBridgeTypeEx(const BREAKPOINT & Bp)
{
    switch(Bp.type)
    {
    case BPHARDWARE:
        switch(TITANGETTYPE(Bp.titantype))
        {
        case UE_HARDWARE_READWRITE:
            return hw_access;
        case UE_HARDWARE_WRITE:
            return hw_write;
        case UE_HARDWARE_EXECUTE:
            return hw_execute;
        }
        break;
    case BPMEMORY:
        switch(Bp.titantype)
        {
        case UE_MEMORY_READ:
            return mem_read;
        case UE_MEMORY_WRITE:
            return mem_write;
        case UE_MEMORY_EXECUTE:
            return mem_execute;
        case UE_MEMORY:
            return mem_access;
        }
        break;
    case BPDLL:
        switch(Bp.titantype)
        {
        case UE_ON_LIB_LOAD:
            return dll_load;
        case UE_ON_LIB_UNLOAD:
            return dll_unload;
        case UE_ON_LIB_ALL:
            return dll_all;
        }
        break;
    case BPEXCEPTION:
        switch(Bp.titantype)  //1:First-chance, 2:Second-chance, 3:Both
        {
        case 1:
            return ex_firstchance;
        case 2:
            return ex_secondchance;
        case 3:
            return ex_all;
        }
        break;
    default:
        break;
    }
    return 0;
}

static duint BpToBridgeHwSize(const BREAKPOINT & Bp)
{
    switch(Bp.type)
    {
    case BPHARDWARE:
        switch(TITANGETSIZE(Bp.titantype))
        {
        case UE_HARDWARE_SIZE_1:
            return 1;
        case UE_HARDWARE_SIZE_2:
            return 2;
        case UE_HARDWARE_SIZE_4:
            return 4;
        case UE_HARDWARE_SIZE_8:
            return 8;
        }
        break;
    default:
        break;
    }
    return 0;
}

static duint BpToBridgeHwSlot(const BREAKPOINT & Bp)
{
    switch(Bp.type)
    {
    case BPHARDWARE:
        switch(TITANGETDRX(Bp.titantype))
        {
        case UE_DR0:
            return 0;
        case UE_DR1:
            return 1;
        case UE_DR2:
            return 2;
        case UE_DR3:
            return 3;
        }
        break;
    default:
        break;
    }
    return 0;
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
    strncpy_s(BridgeBp->mod, Bp->module.c_str(), _TRUNCATE);
    strncpy_s(BridgeBp->name, Bp->name.c_str(), _TRUNCATE);
    strncpy_s(BridgeBp->breakCondition, Bp->breakCondition.c_str(), _TRUNCATE);
    strncpy_s(BridgeBp->logText, Bp->logText.c_str(), _TRUNCATE);
    strncpy_s(BridgeBp->logCondition, Bp->logCondition.c_str(), _TRUNCATE);
    strncpy_s(BridgeBp->commandText, Bp->commandText.c_str(), _TRUNCATE);
    strncpy_s(BridgeBp->commandCondition, Bp->commandCondition.c_str(), _TRUNCATE);

    BridgeBp->active = Bp->active;
    BridgeBp->addr = Bp->addr;
    BridgeBp->enabled = Bp->enabled;
    BridgeBp->singleshoot = Bp->singleshoot;
    BridgeBp->fastResume = Bp->fastResume;
    BridgeBp->silent = Bp->silent;
    BridgeBp->hitCount = Bp->hitcount;

    BridgeBp->type = BpTypeToBridge(Bp->type);
    BridgeBp->slot = (unsigned short)BpToBridgeHwSlot(*Bp);
    BridgeBp->hwSize = (unsigned char)BpToBridgeHwSize(*Bp);
    BridgeBp->typeEx = (unsigned char)BpToBridgeTypeEx(*Bp);
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
            json_object_set_new(jsonObj, "module", json_string(breakpoint.module));
        json_object_set_new(jsonObj, "breakCondition", json_string(breakpoint.breakCondition));
        json_object_set_new(jsonObj, "logText", json_string(breakpoint.logText));
        json_object_set_new(jsonObj, "logCondition", json_string(breakpoint.logCondition));
        json_object_set_new(jsonObj, "commandText", json_string(breakpoint.commandText));
        json_object_set_new(jsonObj, "commandCondition", json_string(breakpoint.commandCondition));
        json_object_set_new(jsonObj, "logFile", json_string(breakpoint.logFile));
        json_object_set_new(jsonObj, "fastResume", json_boolean(breakpoint.fastResume));
        json_object_set_new(jsonObj, "silent", json_boolean(breakpoint.silent));
        json_array_append_new(jsonBreakpoints, jsonObj);
    }

    if(json_array_size(jsonBreakpoints))
        json_object_set(Root, "breakpoints", jsonBreakpoints);

    // Notify garbage collector
    json_decref(jsonBreakpoints);
}

template<size_t Count>
static void loadStringValue(JSON value, char(& dest)[Count], const char* key)
{
    auto text = json_string_value(json_object_get(value, key));
    if(text)
        strncpy_s(dest, text, _TRUNCATE);
}

static void loadStringValue(JSON value, std::string & dest, const char* key)
{
    auto text = json_string_value(json_object_get(value, key));
    if(text)
        dest = text;
}

void BpCacheLoad(JSON Root, bool migrateCommandCondition)
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

        breakpoint.type = (BP_TYPE)json_integer_value(json_object_get(value, "type"));
        if(breakpoint.type == BPNORMAL)
            breakpoint.oldbytes = (unsigned short)(json_hex_value(json_object_get(value, "oldbytes")) & 0xFFFF);
        else if(breakpoint.type == BPMEMORY)
            breakpoint.memsize = (duint)json_hex_value(json_object_get(value, "memsize"));
        breakpoint.addr = (duint)json_hex_value(json_object_get(value, "address"));
        breakpoint.enabled = json_boolean_value(json_object_get(value, "enabled"));
        breakpoint.titantype = (DWORD)json_hex_value(json_object_get(value, "titantype"));
        if(breakpoint.type == BPHARDWARE)
            TITANSETDRX(breakpoint.titantype, UE_DR7); // DR7 is used as a sentinel value to prevent wrongful deletion

        // String values
        loadStringValue(value, breakpoint.name, "name");
        if(breakpoint.type != BPEXCEPTION)
            loadStringValue(value, breakpoint.module, "module");
        loadStringValue(value, breakpoint.breakCondition, "breakCondition");
        loadStringValue(value, breakpoint.logText, "logText");
        loadStringValue(value, breakpoint.logCondition, "logCondition");
        loadStringValue(value, breakpoint.commandText, "commandText");
        loadStringValue(value, breakpoint.commandCondition, "commandCondition");
        loadStringValue(value, breakpoint.logFile, "logFile");
        BpLogFileAcquire(breakpoint.logFile);

        // On 2023-06-10 the default of the command condition was changed from $breakpointcondition to 1
        // If we detect an older database, try to preserve the old behavior.
        if(migrateCommandCondition && !breakpoint.commandText.empty() && !breakpoint.commandCondition.empty())
        {
            breakpoint.commandCondition = "$breakpointcondition";
        }

        // Fast resume
        breakpoint.fastResume = json_boolean_value(json_object_get(value, "fastResume"));
        breakpoint.silent = json_boolean_value(json_object_get(value, "silent"));

        // Build the hash map key: MOD_HASH + ADDRESS
        duint key;
        if(breakpoint.type != BPDLL)
        {
            key = ModHashFromName(breakpoint.module.c_str()) + breakpoint.addr;
        }
        else
        {
            // NOTE: full paths in DLL breakpoints are not supported
            auto slashIdx = breakpoint.module.rfind('\\');
            if(slashIdx != String::npos)
                breakpoint.module = breakpoint.module.substr(slashIdx + 1);
            key = BpGetDLLBpAddr(breakpoint.module.c_str());
            breakpoint.addr = key;
        }
        breakpoints[BreakpointKey(breakpoint.type, key)] = breakpoint;
    }
}

void BpClear()
{
    EXCLUSIVE_ACQUIRE(LockBreakpoints);
    breakpoints.clear();

    // Close breakpoint logs
    for(const auto & itr : breakpointLogFiles)
    {
        if(itr.second.hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(itr.second.hFile);
        }
    }
    breakpointLogFiles.clear();
}

void BpLogFileAcquire(const std::string & logFile)
{
    if(!logFile.empty())
    {
        breakpointLogFiles[logFile].refCount++;
    }
}

void BpLogFileRelease(const std::string & logFile)
{
    if(logFile.empty())
    {
        return;
    }

    auto itr = breakpointLogFiles.find(logFile);
    if(itr == breakpointLogFiles.end())
    {
        // Trying to release a non-existing log file
        return;
    }

    if(--itr->second.refCount <= 0)
    {
        auto hFile = itr->second.hFile;
        if(hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
        }
        breakpointLogFiles.erase(itr);
    }
}

HANDLE BpLogFileOpen(const std::string & logFile)
{
    SHARED_ACQUIRE(LockBreakpoints);

    auto itr = breakpointLogFiles.find(logFile);
    if(itr == breakpointLogFiles.end())
    {
        // NOTE: This can only happen when there is a programming error
        SetLastError(ERROR_HANDLE_EOF);
        return INVALID_HANDLE_VALUE;
    }

    if(itr->second.hFile != INVALID_HANDLE_VALUE)
    {
        itr->second.needsFlush = true;
        return itr->second.hFile;
    }

    auto hFile = CreateFileW(
                     StringUtils::Utf8ToUtf16(logFile).c_str(),
                     GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                     nullptr,
                     bTruncateBreakpointLogs ? CREATE_ALWAYS : OPEN_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     nullptr
                 );
    if(hFile != INVALID_HANDLE_VALUE)
    {
        if(!bTruncateBreakpointLogs)
        {
            SetFilePointer(hFile, 0, nullptr, FILE_END);
        }

        itr->second.hFile = hFile;
        itr->second.needsFlush = true;
    }
    return hFile;
}

void BpLogFileFlush()
{
    SHARED_ACQUIRE(LockBreakpoints);
    for(auto & itr : breakpointLogFiles)
    {
        if(itr.second.needsFlush)
        {
            FlushFileBuffers(itr.second.hFile);
            itr.second.needsFlush = false;
        }
    }
}

// New breakpoint API

static __forceinline BREAKPOINT* BpFromRef(const BP_REF & Ref)
{
    if(Ref.type == bp_none)
        return nullptr;

    auto found = breakpoints.find(BreakpointKey(BpTypeFromBridge(Ref.type), Ref.module + Ref.offset));
    if(found == breakpoints.end())
        return nullptr;
    return &found->second;
}

template<bool Exclusive = false, typename Func>
static __forceinline bool BpRefOperation(const BP_REF & Ref, Func && func)
{
    SectionLocker < LockBreakpoints, !Exclusive > __Lock;
    auto bp = BpFromRef(Ref);
    return bp == nullptr ? false : func(*bp);
}

std::vector<BP_REF> BpRefList()
{
    SHARED_ACQUIRE(LockBreakpoints);
    std::vector<BP_REF> result;
    result.reserve(breakpoints.size());
    for(auto & itr : breakpoints)
    {
        const auto & bp = itr.second;
        BP_REF ref = {};
        switch(bp.type)
        {
        case BPDLL:
            BpRefDll(ref, bp.module.c_str());
            break;
        case BPEXCEPTION:
            BpRefException(ref, (unsigned int)bp.addr);
            break;
        default:
            ref.type = BpTypeToBridge(bp.type);
            ref.module = ModHashFromName(bp.module.c_str());
            ref.offset = bp.addr;
            break;
        }
        // This is the invariant for the breakpoint key.
        assert(bp.type == itr.first.first && ref.module + ref.offset == itr.first.second);
        result.push_back(ref);
    }
    std::sort(result.begin(), result.end(), [](const BP_REF & a, const BP_REF & b)
    {
        auto bpA = BpFromRef(a);
        auto bpB = BpFromRef(b);
        return std::make_tuple(bpA->type, bpA->module, bpA->addr) < std::make_tuple(bpB->type, bpB->module, bpB->addr);
    });
    return result;
}

bool BpRefVa(BP_REF & Ref, BPXTYPE Type, duint Va)
{
    if(Type == bp_none || Type == bp_dll || Type == bp_exception)
    {
        Ref = { bp_none };
        return false;
    }

    Ref.type = Type;

    SHARED_ACQUIRE(LockModules);
    auto info = ModInfoFromAddr(Va);
    if(info != nullptr)
    {
        Ref.module = info->hash;
        Ref.offset = Va - info->base;
    }
    else
    {
        Ref.module = 0;
        Ref.offset = Va;
    }
    return true;
}

bool BpRefRva(BP_REF & Ref, BPXTYPE Type, const char* Module, duint Rva)
{
    if(Type == bp_none || Type == bp_dll || Type == bp_exception)
    {
        Ref = { bp_none };
        return false;
    }

    Ref.type = Type;
    Ref.module = BpGetDLLBpAddr(Module);
    Ref.offset = Rva;
    return false;
}

void BpRefDll(BP_REF & Ref, const char* Module)
{
    Ref.type = bp_dll;
    Ref.module = BpGetDLLBpAddr(Module);
    Ref.offset = 0;
}

void BpRefException(BP_REF & Ref, unsigned int ExceptionCode)
{
    Ref.type = bp_exception;
    Ref.module = 0;
    Ref.offset = ExceptionCode;
}

bool BpRefExists(const BP_REF & Ref)
{
    SHARED_ACQUIRE(LockBreakpoints);
    return BpFromRef(Ref) != nullptr;
}

bool BpGetFieldNumber(const BP_REF & Ref, BP_FIELD Field, duint & Value)
{
    return BpRefOperation(Ref, [&](BREAKPOINT & bp)
    {
        switch(Field)
        {
        case bpf_type:
            Value = BpTypeToBridge(bp.type);
            return true;
        case bpf_offset:
            Value = bp.addr;
            return true;
        case bpf_address:
            Value = bp.addr;
            // Add the module base when applicable
            if(bp.type != BPDLL && bp.type != BPEXCEPTION)
                Value += ModBaseFromName(bp.module.c_str());
            return true;
        case bpf_enabled:
            Value = bp.enabled;
            return true;
        case bpf_singleshoot:
            Value = bp.singleshoot;
            return true;
        case bpf_active:
            if(bp.type == BPDLL || bp.type == BPEXCEPTION)
            {
                Value = true;
            }
            else
            {
                // HACK: we modify the structure here
                setBpActive(bp, ModBaseFromName(bp.module.c_str()));
                Value = bp.active;
            }
            return true;
        case bpf_silent:
            Value = bp.silent;
            return true;
        case bpf_typeex:
            Value = BpToBridgeTypeEx(bp);
            return true;
        case bpf_hwsize:
            Value = BpToBridgeHwSize(bp);
            return true;
        case bpf_hwslot:
            Value = BpToBridgeHwSlot(bp);
            return true;
        case bpf_oldbytes:
            Value = bp.oldbytes;
            return true;
        case bpf_fastresume:
            Value = bp.fastResume;
            return true;
        case bpf_hitcount:
            Value = bp.hitcount;
            return true;
        default:
            __debugbreak();
            return false;
        }
    });
}

bool BpSetFieldNumber(const BP_REF & Ref, BP_FIELD Field, duint Value)
{
    return BpRefOperation<true>(Ref, [&](BREAKPOINT & bp)
    {
        switch(Field)
        {
        case bpf_enabled:
            // TODO: actually enable/disable the breakpoint (requires further refactoring)
            //bp.enabled = !!Value;
            return false;
        case bpf_singleshoot:
            bp.singleshoot = !!Value;
            return true;
        case bpf_silent:
            bp.silent = !!Value;
            return true;
        case bpf_fastresume:
            bp.fastResume = !!Value;
            return true;
        case bpf_hitcount:
            bp.hitcount = (uint32_t)Value;
            return true;
        default:
            __debugbreak();
            return false;
        }
    });
}

bool BpGetFieldText(const BP_REF & Ref, BP_FIELD Field, std::string & Value)
{
    return BpGetFieldText(Ref, Field, [](const char* str, void* userdata)
    {
        *(std::string*)userdata = str;
    }, &Value);
}

bool BpGetFieldText(const BP_REF & Ref, BP_FIELD Field, CBSTRING Callback, void* Userdata)
{
    return BpRefOperation(Ref, [&](const BREAKPOINT & bp)
    {
        switch(Field)
        {
        case bpf_module:
            Callback(bp.module.c_str(), Userdata);
            return true;
        case bpf_name:
            Callback(bp.name.c_str(), Userdata);
            return true;
        case bpf_breakcondition:
            Callback(bp.breakCondition.c_str(), Userdata);
            return true;
        case bpf_logtext:
            Callback(bp.logText.c_str(), Userdata);
            return true;
        case bpf_logcondition:
            Callback(bp.logCondition.c_str(), Userdata);
            return true;
        case bpf_commandtext:
            Callback(bp.commandText.c_str(), Userdata);
            return true;
        case bpf_commandcondition:
            Callback(bp.commandCondition.c_str(), Userdata);
            return true;
        case bpf_logfile:
            Callback(bp.logFile.c_str(), Userdata);
            return true;
        default:
            __debugbreak();
            return false;
        }
    });
}

bool BpSetFieldText(const BP_REF & Ref, BP_FIELD Field, const char* Value)
{
    return BpRefOperation<true>(Ref, [&](BREAKPOINT & bp)
    {
        switch(Field)
        {
        case bpf_name:
            bp.name = Value;
            return true;
        case bpf_breakcondition:
            bp.breakCondition = Value;
            return true;
        case bpf_logtext:
            bp.logText = Value;
            return true;
        case bpf_logcondition:
            bp.logCondition = Value;
            return true;
        case bpf_commandtext:
            bp.commandText = Value;
            return true;
        case bpf_commandcondition:
            bp.commandCondition = Value;
            return true;
        case bpf_logfile:
        {
            std::string newLogFile = Value;
            BpLogFileAcquire(newLogFile);
            BpLogFileRelease(bp.logFile);
            bp.logFile = std::move(newLogFile);
            return true;
        }
        default:
            __debugbreak();
            return false;
        }
    });
}
