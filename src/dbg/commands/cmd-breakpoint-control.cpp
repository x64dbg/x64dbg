#include "cmd-breakpoint-control.h"
#include "console.h"
#include "memory.h"
#include "debugger.h"
#include "exception.h"
#include "value.h"

enum class BreakpointOperation
{
    Remove,
    Enable,
    Disable
};

// Commands only resolve arguments and report results. The breakpoint API owns
// both the database entry and its engine registration, including bulk operations.
static bool updateBreakpoint(const BREAKPOINT & bp, BreakpointOperation operation)
{
    if(operation == BreakpointOperation::Remove)
        return BpDelete(bp);
    return BpEnable(bp, operation == BreakpointOperation::Enable);
}

static bool breakpointCommand(BP_TYPE type, BreakpointOperation operation, int argc, char* argv[])
{
    bool success = true;
    if(argc < 2)
    {
        std::vector<BREAKPOINT> list;
        BpGetList(&list);
        for(const auto & bp : list)
        {
            if(bp.type == type && !updateBreakpoint(bp, operation))
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Could not update breakpoint %p!\n"), bp.addr);
                success = false;
            }
        }
    }
    else
    {
        BREAKPOINT bp;
        if(!BpGetAny(type, argv[1], &bp))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
            return false;
        }
        success = updateBreakpoint(bp, operation);
        if(!success)
            dprintf(QT_TRANSLATE_NOOP("DBG", "Could not update breakpoint %p!\n"), bp.addr);
    }
    GuiUpdateAllViews();
    return success;
}

bool cbDebugDeleteBPX(int argc, char* argv[])
{
    return breakpointCommand(BPNORMAL, BreakpointOperation::Remove, argc, argv);
}

bool cbDebugEnableBPX(int argc, char* argv[])
{
    return breakpointCommand(BPNORMAL, BreakpointOperation::Enable, argc, argv);
}

bool cbDebugDisableBPX(int argc, char* argv[])
{
    return breakpointCommand(BPNORMAL, BreakpointOperation::Disable, argc, argv);
}

bool cbDebugDeleteHardwareBreakpoint(int argc, char* argv[])
{
    return breakpointCommand(BPHARDWARE, BreakpointOperation::Remove, argc, argv);
}

bool cbDebugEnableHardwareBreakpoint(int argc, char* argv[])
{
    return breakpointCommand(BPHARDWARE, BreakpointOperation::Enable, argc, argv);
}

bool cbDebugDisableHardwareBreakpoint(int argc, char* argv[])
{
    return breakpointCommand(BPHARDWARE, BreakpointOperation::Disable, argc, argv);
}

bool cbDebugDeleteMemoryBreakpoint(int argc, char* argv[])
{
    return breakpointCommand(BPMEMORY, BreakpointOperation::Remove, argc, argv);
}

bool cbDebugEnableMemoryBreakpoint(int argc, char* argv[])
{
    return breakpointCommand(BPMEMORY, BreakpointOperation::Enable, argc, argv);
}

bool cbDebugDisableMemoryBreakpoint(int argc, char* argv[])
{
    return breakpointCommand(BPMEMORY, BreakpointOperation::Disable, argc, argv);
}

bool cbDebugBcDll(int argc, char* argv[])
{
    return breakpointCommand(BPDLL, BreakpointOperation::Remove, argc, argv);
}

bool cbDebugBpDllEnable(int argc, char* argv[])
{
    return breakpointCommand(BPDLL, BreakpointOperation::Enable, argc, argv);
}

bool cbDebugBpDllDisable(int argc, char* argv[])
{
    return breakpointCommand(BPDLL, BreakpointOperation::Disable, argc, argv);
}

bool cbDebugDeleteExceptionBPX(int argc, char* argv[])
{
    return breakpointCommand(BPEXCEPTION, BreakpointOperation::Remove, argc, argv);
}

bool cbDebugEnableExceptionBPX(int argc, char* argv[])
{
    return breakpointCommand(BPEXCEPTION, BreakpointOperation::Enable, argc, argv);
}

bool cbDebugDisableExceptionBPX(int argc, char* argv[])
{
    return breakpointCommand(BPEXCEPTION, BreakpointOperation::Disable, argc, argv);
}

// Software breakpoints
bool cbDebugSetBPX(int argc, char* argv[]) // bp addr [,name [,type]]
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    char argaddr[deflen] = "";
    strcpy_s(argaddr, argv[1]);
    char argname[deflen] = "";
    if(argc > 2)
        strcpy_s(argname, argv[2]);
    char argtype[deflen] = "";
    bool has_arg2 = argc > 3;
    if(has_arg2)
        strcpy_s(argtype, argv[3]);
    if(!has_arg2 && (scmp(argname, "ss") || scmp(argname, "long") || scmp(argname, "ud2")))
    {
        strcpy_s(argtype, argname);
        *argname = 0;
    }
    _strlwr_s(argtype);
    duint addr = 0;
    if(!valfromstring(argaddr, &addr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid addr: \"%s\"\n"), argaddr);
        return false;
    }
    bool singleshoot = strstr(argtype, "ss") != nullptr;
    int type = singleshoot ? UE_SINGLESHOOT : UE_BREAKPOINT;
    if(strstr(argtype, "long"))
        type |= UE_BREAKPOINT_TYPE_LONG_INT3;
    else if(strstr(argtype, "ud2"))
        type |= UE_BREAKPOINT_TYPE_UD2;
    else if(strstr(argtype, "short"))
        type |= UE_BREAKPOINT_TYPE_INT3;
    const char* bpname = *argname ? argname : nullptr;
    BREAKPOINT bp;
    bool success;
    if(BpGet(addr, BPNORMAL, bpname, &bp))
        success = BpEnable(bp, true);
    else
        success = BpNew(addr, true, singleshoot, 0, BPNORMAL, type, bpname);
    if(!success)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting breakpoint at %p!\n"), addr);
        return false;
    }
    GuiUpdateAllViews();
    dprintf(QT_TRANSLATE_NOOP("DBG", "Breakpoint at %p set!\n"), addr);
    return true;
}

// Hardware breakpoints
bool cbDebugSetHardwareBreakpoint(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr;
    if(!valfromstring(argv[1], &addr))
        return false;
    TitanHardwareBreakpointType type = UE_HARDWARE_EXECUTE;
    if(argc > 2)
    {
        switch(*argv[2])
        {
        case 'r':
            type = UE_HARDWARE_READWRITE;
            break;
        case 'w':
            type = UE_HARDWARE_WRITE;
            break;
        case 'x':
            break;
        default:
            dputs(QT_TRANSLATE_NOOP("DBG", "Invalid type, assuming 'x'"));
            break;
        }
    }
    TitanHardwareBreakpointSize titsize = UE_HARDWARE_SIZE_1;
    if(argc > 3)
    {
        duint size;
        if(!valfromstring(argv[3], &size))
            return false;
        switch(size)
        {
        case 1:
            break;
        case 2:
            titsize = UE_HARDWARE_SIZE_2;
            break;
        case 4:
            titsize = UE_HARDWARE_SIZE_4;
            break;
#ifdef _WIN64
        case 8:
            titsize = UE_HARDWARE_SIZE_8;
            break;
#endif
        default:
            dputs(QT_TRANSLATE_NOOP("DBG", "Invalid size, using 1"));
            size = 1;
            break;
        }
        if((addr % size) != 0)
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Address not aligned to %d\n"), int(size));
            return false;
        }
    }
    int titantype = 0;
    TITANSETDRX(titantype, UE_DR7);
    TITANSETTYPE(titantype, type);
    TITANSETSIZE(titantype, titsize);
    BREAKPOINT bp;
    bool success = BpGet(addr, BPHARDWARE, nullptr, &bp) ? BpEnable(bp, true) :
                   BpNew(addr, true, false, 0, BPHARDWARE, titantype, nullptr);
    if(!success)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error setting hardware breakpoint!"));
        return false;
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "Hardware breakpoint at %p set!\n"), addr);
    GuiUpdateAllViews();
    return true;
}

// Memory breakpoints
static bool memoryBreakpointType(const char* text, TitanMemoryBreakpointType & type)
{
    switch(*text)
    {
    case '\0':
    case 'a':
        type = UE_MEMORY;
        return true;
    case 'r':
        type = UE_MEMORY_READ;
        return true;
    case 'w':
        type = UE_MEMORY_WRITE;
        return true;
    case 'x':
        type = UE_MEMORY_EXECUTE;
        return true;
    default:
        return false;
    }
}

static bool setMemoryBreakpoint(duint address, duint size, TitanMemoryBreakpointType type, bool singleshoot)
{
    BREAKPOINT bp;
    bool success = BpGet(address, BPMEMORY, nullptr, &bp) ? BpEnable(bp, true) :
                   BpNew(address, true, singleshoot, 0, BPMEMORY, type, nullptr, size);
    if(!success)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error setting memory breakpoint!"));
        return false;
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "Memory breakpoint at %p[%p] set!\n"), address, size);
    GuiUpdateAllViews();
    return true;
}

bool cbDebugSetMemoryBpx(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint address;
    if(!valfromstring(argv[1], &address))
        return false;
    bool restore = false;
    const char* argtype = argc > 3 ? argv[3] : "";
    if(argc > 2)
    {
        if(*argv[2] == '1')
            restore = true;
        else if(*argv[2] != '0')
            argtype = argv[2];
    }
    TitanMemoryBreakpointType type = UE_MEMORY;
    if(!memoryBreakpointType(argtype, type))
        dputs(QT_TRANSLATE_NOOP("DBG", "Invalid type (argument ignored)"));
    duint size = 0;
    auto base = MemFindBaseAddr(address, &size, true);
    return setMemoryBreakpoint(base, size, type, !restore);
}

bool cbDebugSetMemoryRangeBpx(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint address, size;
    if(!valfromstring(argv[1], &address, false) || !valfromstring(argv[2], &size, false))
        return false;
    TitanMemoryBreakpointType type = UE_MEMORY;
    bool singleshoot = false;
    if(argc > 3)
    {
        if(!memoryBreakpointType(argv[3], type))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Invalid type specified!"));
            return false;
        }
        singleshoot = strstr(argv[3], "ss") != nullptr;
    }
    return setMemoryBreakpoint(address, size, type, singleshoot);
}

// DLL breakpoints
bool cbDebugBpDll(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    DWORD type = UE_ON_LIB_ALL;
    if(argc > 2)
    {
        if(*argv[2] == 'l')
            type = UE_ON_LIB_LOAD;
        else if(*argv[2] == 'u')
            type = UE_ON_LIB_UNLOAD;
    }
    if(!BpNewDll(argv[1], true, argc > 3, type, ""))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error creating Dll breakpoint! (BpNewDll)"));
        return false;
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "Dll breakpoint set on \"%s\"!\n"), argv[1]);
    DebugUpdateBreakpointsViewAsync();
    return true;
}

// Exception breakpoints
bool cbDebugSetExceptionBPX(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint code = 0;
    if(!valfromstring(argv[1], &code) && !ExceptionNameToCode(argv[1], reinterpret_cast<unsigned int*>(&code)))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid exception code: %s.\n"), argv[1]);
        return false;
    }
    auto type = ex_firstchance;
    if(argc > 2)
    {
        duint chance;
        if(scmp(argv[2], "first"))
            type = ex_firstchance;
        else if(scmp(argv[2], "second"))
            type = ex_secondchance;
        else if(scmp(argv[2], "all"))
            type = ex_all;
        else if(valfromstring(argv[2], &chance) && chance >= ex_firstchance && chance <= ex_all)
            type = static_cast<BPEXTYPE>(chance);
        else
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Invalid exception type!"));
            return false;
        }
    }
    if(!BpNew(code, true, false, 0, BPEXCEPTION, type, ""))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to set exception breakpoint! (BpNew)"));
        return false;
    }
    DebugUpdateBreakpointsViewAsync();
    return true;
}

bool cbDebugSetBPGoto(int argc, char* argv[])
{
    if(argc != 3)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "argument count mismatch!\n"));
        return false;
    }
    char cmd[deflen];
    sprintf_s(cmd, "SetBreakpointCondition %s, 0", argv[1]);
    if(!cmddirectexec(cmd))
        return false;
    sprintf_s(cmd, "SetBreakpointCommand %s, \"bpgoto(%s)\"", argv[1], argv[2]);
    if(!cmddirectexec(cmd))
        return false;
    sprintf_s(cmd, "SetBreakpointCommandCondition %s, 1", argv[1]);
    if(!cmddirectexec(cmd))
        return false;
    sprintf_s(cmd, "SetBreakpointFastResume %s, 0", argv[1]);
    if(!cmddirectexec(cmd))
        return false;
    return true;
}

static bool cbBreakpointList(const BREAKPOINT* bp)
{
    const char* type = 0;
    if(bp->type == BPNORMAL)
    {
        if(bp->singleshoot)
            type = "SS";
        else
            type = "BP";
    }
    else if(bp->type == BPHARDWARE)
        type = "HW";
    else if(bp->type == BPMEMORY)
        type = "GP";
    else if(bp->type == BPDLL)
        type = "DLL";
    else if(bp->type == BPEXCEPTION)
        type = "EX";
    bool enabled = bp->enabled;
    if(bp->type == BPDLL)
    {
        if(!bp->name.empty())
            dprintf_untranslated("%d:%s:\"%s\":\"%s\"\n", enabled, type, bp->module.c_str(), bp->name.c_str());
        else
            dprintf_untranslated("%d:%s:\"%s\"\n", enabled, type, bp->module.c_str());
    }
    else if(!bp->name.empty())
        dprintf_untranslated("%d:%s:%p:\"%s\"\n", enabled, type, bp->addr, bp->name.c_str());
    else
        dprintf_untranslated("%d:%s:%p\n", enabled, type, bp->addr);
    return true;
}

bool cbDebugBplist(int argc, char* argv[])
{
    if(!BpEnumAll(cbBreakpointList))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Something went wrong..."));
        return false;
    }
    return true;
}

bool cbDebugSetBPXOptions(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    TitanBreakpointType type;
    const char* strType = 0;
    duint setting_type;
    if(strstr(argv[1], "long"))
    {
        setting_type = 1; //break_int3long
        strType = "TYPE_LONG_INT3";
        type = UE_BREAKPOINT_LONG_INT3;
    }
    else if(strstr(argv[1], "ud2"))
    {
        setting_type = 2; //break_ud2
        strType = "TYPE_UD2";
        type = UE_BREAKPOINT_UD2;
    }
    else if(strstr(argv[1], "short"))
    {
        setting_type = 0; //break_int3short
        strType = "TYPE_INT3";
        type = UE_BREAKPOINT_INT3;
    }
    else
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Invalid type specified!"));
        return false;
    }
    SetBPXOptions(type);
    BridgeSettingSetUint("Engine", "BreakpointType", setting_type);
    dprintf(QT_TRANSLATE_NOOP("DBG", "Default breakpoint type set to: %s\n"), strType);
    return true;
}
