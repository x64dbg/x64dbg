#include "cmd-breakpoint-control.h"
#include "console.h"
#include "memory.h"
#include "debugger.h"
#include "exception.h"
#include "value.h"

// breakpoint enumeration callbacks
static bool cbDeleteAllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPNORMAL)
        return true;
    if(!BpDelete(bp->addr, BPNORMAL))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete breakpoint failed (BpDelete): %p\n"), bp->addr);
        return false;
    }
    if(bp->enabled && !DeleteBPX(bp->addr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete breakpoint failed (DeleteBPX): %p\n"), bp->addr);
        return false;
    }
    return true;
}

static bool cbEnableAllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPNORMAL || bp->enabled)
        return true;

    if(!SetBPX(bp->addr, bp->titantype, (void*)cbUserBreakpoint))
    {
        if(!MemIsValidReadPtr(bp->addr))
            return true;
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable breakpoint %p (SetBPX)\n"), bp->addr);
        return false;
    }
    if(!BpEnable(bp->addr, BPNORMAL, true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable breakpoint %p (BpEnable)\n"), bp->addr);
        return false;
    }
    return true;
}

static bool cbDisableAllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPNORMAL || !bp->enabled)
        return true;

    if(!BpEnable(bp->addr, BPNORMAL, false))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable breakpoint %p (BpEnable)\n"), bp->addr);
        return false;
    }
    if(!DeleteBPX(bp->addr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable breakpoint %p (DeleteBPX)\n"), bp->addr);
        return false;
    }
    return true;
}

bool cbDebugSetBPX(int argc, char* argv[]) //bp addr [,name [,type]]
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
    int type = 0;
    bool singleshoot = false;
    if(strstr(argtype, "ss"))
    {
        type |= UE_SINGLESHOOT;
        singleshoot = true;
    }
    else
        type |= UE_BREAKPOINT;
    if(strstr(argtype, "long"))
        type |= UE_BREAKPOINT_TYPE_LONG_INT3;
    else if(strstr(argtype, "ud2"))
        type |= UE_BREAKPOINT_TYPE_UD2;
    else if(strstr(argtype, "short"))
        type |= UE_BREAKPOINT_TYPE_INT3;
    short oldbytes;
    const char* bpname = 0;
    if(*argname)
        bpname = argname;
    BREAKPOINT bp;
    if(BpGet(addr, BPNORMAL, bpname, &bp))
    {
        if(!bp.enabled)
            return DbgCmdExecDirect(StringUtils::sprintf("bpe %p", bp.addr).c_str());
        dputs(QT_TRANSLATE_NOOP("DBG", "Breakpoint already set!"));
        return true;
    }
    if(IsBPXEnabled(addr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting breakpoint at %p! (IsBPXEnabled)\n"), addr);
        return false;
    }
    if(!MemRead(addr, &oldbytes, sizeof(short)))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting breakpoint at %p! (memread)\n"), addr);
        return false;
    }
    if(!BpNew(addr, true, singleshoot, oldbytes, BPNORMAL, type, bpname))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting breakpoint at %p! (bpnew)\n"), addr);
        return false;
    }
    if(!SetBPX(addr, type, (void*)cbUserBreakpoint))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting breakpoint at %p! (SetBPX)\n"), addr);
        if(!BpDelete(addr, BPNORMAL))
            dprintf(QT_TRANSLATE_NOOP("DBG", "Error handling invalid breakpoint at %p! (bpdel)\n"), addr);
        return false;
    }
    GuiUpdateAllViews();
    if(bpname)
        dprintf(QT_TRANSLATE_NOOP("DBG", "Breakpoint at %p (%s) set!\n"), addr, bpname);
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "Breakpoint at %p set!\n"), addr);
    return true;
}

bool cbDebugDeleteBPX(int argc, char* argv[])
{
    if(argc < 2) //delete all breakpoints
    {
        if(!BpGetCount(BPNORMAL))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No breakpoints to delete!"));
            return true;
        }
        if(!BpEnumAll(cbDeleteAllBreakpoints)) //at least one deletion failed
        {
            GuiUpdateAllViews();
            return false;
        }
        dputs(QT_TRANSLATE_NOOP("DBG", "All breakpoints deleted!"));
        GuiUpdateAllViews();
        return true;
    }
    BREAKPOINT found;
    if(BpGet(0, BPNORMAL, argv[1], &found)) //found a breakpoint with name
    {
        if(!BpDelete(found.addr, BPNORMAL))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Delete breakpoint failed (bpdel): %p\n"), found.addr);
            return false;
        }
        if(found.enabled && !DeleteBPX(found.addr))
        {
            GuiUpdateAllViews();
            if(!MemIsValidReadPtr(found.addr))
                return true;
            dprintf(QT_TRANSLATE_NOOP("DBG", "Delete breakpoint failed (DeleteBPX): %p\n"), found.addr);
            return false;
        }
        return true;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPNORMAL, 0, &found)) //invalid breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!BpDelete(found.addr, BPNORMAL))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete breakpoint failed (bpdel): %p\n"), found.addr);
        return false;
    }
    if(found.enabled && !DeleteBPX(found.addr))
    {
        GuiUpdateAllViews();
        if(!MemIsValidReadPtr(found.addr))
            return true;
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete breakpoint failed (DeleteBPX): %p\n"), found.addr);
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Breakpoint deleted!"));
    GuiUpdateAllViews();
    return true;
}

bool cbDebugEnableBPX(int argc, char* argv[])
{
    if(argc < 2) //enable all breakpoints
    {
        if(!BpGetCount(BPNORMAL))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No breakpoints to enable!"));
            return true;
        }
        if(!BpEnumAll(cbEnableAllBreakpoints)) //at least one enable failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All breakpoints enabled!"));
        GuiUpdateAllViews();
        return true;
    }
    BREAKPOINT found;
    if(BpGet(0, BPNORMAL, argv[1], &found)) //found a breakpoint with name
    {
        if(!SetBPX(found.addr, found.titantype, (void*)cbUserBreakpoint))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable breakpoint %p (SetBPX)\n"), found.addr);
            return false;
        }
        if(!BpEnable(found.addr, BPNORMAL, true))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable breakpoint %p (BpEnable)\n"), found.addr);
            return false;
        }
        GuiUpdateAllViews();
        return true;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPNORMAL, 0, &found)) //invalid breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(found.enabled)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Breakpoint already enabled!"));
        GuiUpdateAllViews();
        return true;
    }
    if(!SetBPX(found.addr, found.titantype, (void*)cbUserBreakpoint))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable breakpoint %p (SetBPX)\n"), found.addr);
        return false;
    }
    if(!BpEnable(found.addr, BPNORMAL, true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable breakpoint %p (BpEnable)\n"), found.addr);
        return false;
    }
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "Breakpoint enabled!"));
    return true;
}

bool cbDebugDisableBPX(int argc, char* argv[])
{
    if(argc < 2) //delete all breakpoints
    {
        if(!BpGetCount(BPNORMAL))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No breakpoints to disable!"));
            return true;
        }
        if(!BpEnumAll(cbDisableAllBreakpoints)) //at least one deletion failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All breakpoints disabled!"));
        GuiUpdateAllViews();
        return true;
    }
    BREAKPOINT found;
    if(BpGet(0, BPNORMAL, argv[1], &found)) //found a breakpoint with name
    {
        if(!BpEnable(found.addr, BPNORMAL, false))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable breakpoint %p (BpEnable)\n"), found.addr);
            return false;
        }
        if(!DeleteBPX(found.addr))
        {
            GuiUpdateAllViews();
            if(!MemIsValidReadPtr(found.addr))
                return true;
            dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable breakpoint %p (DeleteBPX)\n"), found.addr);
            return false;
        }
        GuiUpdateAllViews();
        return true;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPNORMAL, 0, &found)) //invalid breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!found.enabled)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Breakpoint already disabled!"));
        return true;
    }
    if(!BpEnable(found.addr, BPNORMAL, false))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable breakpoint %p (BpEnable)\n"), found.addr);
        return false;
    }
    if(!DeleteBPX(found.addr))
    {
        GuiUpdateAllViews();
        if(!MemIsValidReadPtr(found.addr))
            return true;
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable breakpoint %p (DeleteBPX)\n"), found.addr);
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Breakpoint disabled!"));
    GuiUpdateAllViews();
    return true;
}

static bool cbDeleteAllHardwareBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPHARDWARE)
        return true;
    if(!BpDelete(bp->addr, BPHARDWARE))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete hardware breakpoint failed (BpDelete): %p\n"), bp->addr);
        return false;
    }
    if(bp->enabled && !DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype)))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete hardware breakpoint failed (DeleteHardwareBreakPoint): %p\n"), bp->addr);
        return false;
    }
    return true;
}

static bool cbEnableAllHardwareBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPHARDWARE || bp->enabled)
        return true;
    DWORD drx = 0;
    if(!GetUnusedHardwareBreakPointRegister(&drx))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Did not enable hardware breakpoint %p (all slots full)\n"), bp->addr);
        return true;
    }
    int titantype = bp->titantype;
    TITANSETDRX(titantype, drx);
    BpSetTitanType(bp->addr, BPHARDWARE, titantype);
    if(!BpEnable(bp->addr, BPHARDWARE, true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable hardware breakpoint %p (BpEnable)\n"), bp->addr);
        return false;
    }
    if(!SetHardwareBreakPoint(bp->addr, drx, TITANGETTYPE(bp->titantype), TITANGETSIZE(bp->titantype), (void*)cbHardwareBreakpoint))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable hardware breakpoint %p (SetHardwareBreakPoint)\n"), bp->addr);
        return false;
    }
    return true;
}

static bool cbDisableAllHardwareBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPHARDWARE)
        return true;
    if(!BpEnable(bp->addr, BPHARDWARE, false))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable hardware breakpoint %p (BpEnable)\n"), bp->addr);
        return false;
    }
    if(bp->enabled && !DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype)))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable hardware breakpoint %p (DeleteHardwareBreakPoint)\n"), bp->addr);
        return false;
    }
    return true;
}

bool cbDebugSetHardwareBreakpoint(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr;
    if(!valfromstring(argv[1], &addr))
        return false;
    DWORD type = UE_HARDWARE_EXECUTE;
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
    DWORD titsize = UE_HARDWARE_SIZE_1;
    if(argc > 3)
    {
        duint size;
        if(!valfromstring(argv[3], &size))
            return false;
        switch(size)
        {
        case 1:
            titsize = UE_HARDWARE_SIZE_1;
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
#endif // _WIN64
        default:
            titsize = UE_HARDWARE_SIZE_1;
            dputs(QT_TRANSLATE_NOOP("DBG", "Invalid size, using 1"));
            break;
        }
        if((addr % size) != 0)
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Address not aligned to %d\n"), int(size));
            return false;
        }
    }
    DWORD drx = 0;
    if(!GetUnusedHardwareBreakPointRegister(&drx))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "You can only set 4 hardware breakpoints"));
        return false;
    }
    int titantype = 0;
    TITANSETDRX(titantype, drx);
    TITANSETTYPE(titantype, type);
    TITANSETSIZE(titantype, titsize);
    //TODO: hwbp in multiple threads TEST
    BREAKPOINT bp;
    if(BpGet(addr, BPHARDWARE, 0, &bp))
    {
        if(!bp.enabled)
            return DbgCmdExecDirect(StringUtils::sprintf("bphwe %p", bp.addr).c_str());
        dputs(QT_TRANSLATE_NOOP("DBG", "Hardware breakpoint already set!"));
        return true;
    }
    if(!BpNew(addr, true, false, 0, BPHARDWARE, titantype, 0))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error setting hardware breakpoint (bpnew)!"));
        return false;
    }
    if(!SetHardwareBreakPoint(addr, drx, type, titsize, (void*)cbHardwareBreakpoint))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error setting hardware breakpoint (TitanEngine)!"));
        return false;
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "Hardware breakpoint at %p set!\n"), addr);
    GuiUpdateAllViews();
    return true;
}

bool cbDebugDeleteHardwareBreakpoint(int argc, char* argv[])
{
    if(argc < 2) //delete all breakpoints
    {
        if(!BpGetCount(BPHARDWARE))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No hardware breakpoints to delete!"));
            return true;
        }
        if(!BpEnumAll(cbDeleteAllHardwareBreakpoints)) //at least one deletion failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All hardware breakpoints deleted!"));
        GuiUpdateAllViews();
        return true;
    }
    BREAKPOINT found;
    if(BpGet(0, BPHARDWARE, argv[1], &found)) //found a breakpoint with name
    {
        if(!BpDelete(found.addr, BPHARDWARE))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Delete hardware breakpoint failed: %p (BpDelete)\n"), found.addr);
            return false;
        }
        if(!DeleteHardwareBreakPoint(TITANGETDRX(found.titantype)))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Delete hardware breakpoint failed: %p (DeleteHardwareBreakPoint)\n"), found.addr);
            return false;
        }
        return true;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPHARDWARE, 0, &found)) //invalid breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such hardware breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!BpDelete(found.addr, BPHARDWARE))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete hardware breakpoint failed: %p (BpDelete)\n"), found.addr);
        return false;
    }
    if(!DeleteHardwareBreakPoint(TITANGETDRX(found.titantype)))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete hardware breakpoint failed: %p (DeleteHardwareBreakPoint)\n"), found.addr);
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Hardware breakpoint deleted!"));
    GuiUpdateAllViews();
    return true;
}

bool cbDebugEnableHardwareBreakpoint(int argc, char* argv[])
{
    DWORD drx = 0;
    if(!GetUnusedHardwareBreakPointRegister(&drx))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "You can only set 4 hardware breakpoints"));
        return false;
    }
    if(argc < 2) //enable all hardware breakpoints
    {
        if(!BpGetCount(BPHARDWARE))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No hardware breakpoints to enable!"));
            return true;
        }
        if(!BpEnumAll(cbEnableAllHardwareBreakpoints)) //at least one enable failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All hardware breakpoints enabled!"));
        GuiUpdateAllViews();
        return true;
    }
    BREAKPOINT found;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPHARDWARE, 0, &found)) //invalid hardware breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such hardware breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(found.enabled)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Hardware breakpoint already enabled!"));
        GuiUpdateAllViews();
        return true;
    }
    TITANSETDRX(found.titantype, drx);
    BpSetTitanType(found.addr, BPHARDWARE, found.titantype);
    if(!SetHardwareBreakPoint(found.addr, drx, TITANGETTYPE(found.titantype), TITANGETSIZE(found.titantype), (void*)cbHardwareBreakpoint))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable hardware breakpoint %p (SetHardwareBreakpoint)\n"), found.addr);
        return false;
    }
    if(!BpEnable(found.addr, BPHARDWARE, true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable hardware breakpoint %p (BpEnable)\n"), found.addr);
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Hardware breakpoint enabled!"));
    GuiUpdateAllViews();
    return true;
}

bool cbDebugDisableHardwareBreakpoint(int argc, char* argv[])
{
    if(argc < 2) //delete all hardware breakpoints
    {
        if(!BpGetCount(BPHARDWARE))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No hardware breakpoints to disable!"));
            return true;
        }
        if(!BpEnumAll(cbDisableAllHardwareBreakpoints)) //at least one deletion failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All hardware breakpoints disabled!"));
        GuiUpdateAllViews();
        return true;
    }
    BREAKPOINT found;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPHARDWARE, 0, &found)) //invalid hardware breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such hardware breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!found.enabled)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Hardware breakpoint already disabled!"));
        return true;
    }
    if(!BpEnable(found.addr, BPHARDWARE, false))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable hardware breakpoint %p (BpEnable)\n"), found.addr);
        return false;
    }
    if(!DeleteHardwareBreakPoint(TITANGETDRX(found.titantype)))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable hardware breakpoint %p (DeleteHardwareBreakpoint)\n"), found.addr);
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Hardware breakpoint disabled!"));
    GuiUpdateAllViews();
    return true;
}

static bool cbDeleteAllMemoryBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPMEMORY)
        return true;
    duint size;
    MemFindBaseAddr(bp->addr, &size);
    if(!BpDelete(bp->addr, BPMEMORY))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete memory breakpoint failed (BpDelete): %p\n"), bp->addr);
        return false;
    }
    if(bp->enabled && !RemoveMemoryBPX(bp->addr, size))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete memory breakpoint failed (RemoveMemoryBPX): %p\n"), bp->addr);
        return false;
    }
    return true;
}

static bool cbEnableAllMemoryBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPMEMORY || bp->enabled)
        return true;
    duint size = 0;
    MemFindBaseAddr(bp->addr, &size);
    if(!BpEnable(bp->addr, BPMEMORY, true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable memory breakpoint %p (BpEnable)\n"), bp->addr);
        return false;
    }
    if(!SetMemoryBPXEx(bp->addr, size, bp->titantype, !bp->singleshoot, (void*)cbMemoryBreakpoint))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable memory breakpoint %p (SetMemoryBPXEx)\n"), bp->addr);
        return false;
    }
    return true;
}

static bool cbDisableAllMemoryBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPMEMORY || !bp->enabled)
        return true;
    if(!BpEnable(bp->addr, BPMEMORY, false))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable memory breakpoint %p (BpEnable)\n"), bp->addr);
        return false;
    }
    if(!RemoveMemoryBPX(bp->addr, 0))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable memory breakpoint %p (RemoveMemoryBPX)\n"), bp->addr);
        return false;
    }
    return true;
}

bool cbDebugSetMemoryBpx(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr;
    if(!valfromstring(argv[1], &addr))
        return false;
    bool restore = false;
    char arg3[deflen] = "";
    if(argc > 3)
        strcpy_s(arg3, argv[3]);
    if(argc > 2)
    {
        if(*argv[2] == '1')
            restore = true;
        else if(*argv[2] == '0')
            restore = false;
        else
            strcpy_s(arg3, argv[2]);
    }
    DWORD type = UE_MEMORY;
    if(*arg3)
    {
        switch(*arg3)
        {
        case 'a': //read+write+execute
            type = UE_MEMORY;
            break;
        case 'r': //read
            type = UE_MEMORY_READ;
            break;
        case 'w': //write
            type = UE_MEMORY_WRITE;
            break;
        case 'x': //execute
            type = UE_MEMORY_EXECUTE;
            break;
        default:
            dputs(QT_TRANSLATE_NOOP("DBG", "Invalid type (argument ignored)"));
            break;
        }
    }
    duint size = 0;
    duint base = MemFindBaseAddr(addr, &size, true);
    bool singleshoot = false;
    if(!restore)
        singleshoot = true;
    BREAKPOINT bp;
    if(BpGet(base, BPMEMORY, 0, &bp))
    {
        if(!bp.enabled)
            return BpEnable(base, BPMEMORY, true);
        dputs(QT_TRANSLATE_NOOP("DBG", "Memory breakpoint already set!"));
        return true;
    }
    if(!BpNew(base, true, singleshoot, 0, BPMEMORY, type, 0, size))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error setting memory breakpoint! (BpNew)"));
        return false;
    }
    if(!SetMemoryBPXEx(base, size, type, restore, (void*)cbMemoryBreakpoint))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error setting memory breakpoint! (SetMemoryBPXEx)"));
        return false;
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "Memory breakpoint at %p set!\n"), addr);
    GuiUpdateAllViews();
    return true;
}

bool cbDebugDeleteMemoryBreakpoint(int argc, char* argv[])
{
    if(argc < 2) //delete all breakpoints
    {
        if(!BpGetCount(BPMEMORY))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No memory breakpoints to delete!"));
            return true;
        }
        if(!BpEnumAll(cbDeleteAllMemoryBreakpoints)) //at least one deletion failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All memory breakpoints deleted!"));
        GuiUpdateAllViews();
        return true;
    }
    BREAKPOINT found;
    if(BpGet(0, BPMEMORY, argv[1], &found)) //found a breakpoint with name
    {
        duint size;
        MemFindBaseAddr(found.addr, &size);
        if(!BpDelete(found.addr, BPMEMORY))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Delete memory breakpoint failed: %p (BpDelete)\n"), found.addr);
            return false;
        }
        if(!RemoveMemoryBPX(found.addr, size))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Delete memory breakpoint failed: %p (RemoveMemoryBPX)\n"), found.addr);
            return false;
        }
        return true;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPMEMORY, 0, &found)) //invalid breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such memory breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    duint size;
    MemFindBaseAddr(found.addr, &size);
    if(!BpDelete(found.addr, BPMEMORY))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete memory breakpoint failed: %p (BpDelete)\n"), found.addr);
        return false;
    }
    if(!RemoveMemoryBPX(found.addr, size))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete memory breakpoint failed: %p (RemoveMemoryBPX)\n"), found.addr);
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Memory breakpoint deleted!"));
    GuiUpdateAllViews();
    return true;
}

bool cbDebugEnableMemoryBreakpoint(int argc, char* argv[])
{
    if(argc < 2) //enable all memory breakpoints
    {
        if(!BpGetCount(BPMEMORY))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No memory breakpoints to enable!"));
            return true;
        }
        if(!BpEnumAll(cbEnableAllMemoryBreakpoints)) //at least one enable failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All memory breakpoints enabled!"));
        GuiUpdateAllViews();
        return true;
    }
    BREAKPOINT found;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPMEMORY, 0, &found)) //invalid memory breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such memory breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(found.enabled)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Memory breakpoint already enabled!"));
        GuiUpdateAllViews();
        return true;
    }
    duint size = 0;
    MemFindBaseAddr(found.addr, &size);
    if(!SetMemoryBPXEx(found.addr, size, found.titantype, !found.singleshoot, (void*)cbMemoryBreakpoint))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable memory breakpoint %p (SetMemoryBPXEx)\n"), found.addr);
        return false;
    }
    if(!BpEnable(found.addr, BPMEMORY, true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable memory breakpoint %p (BpEnable)\n"), found.addr);
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Memory breakpoint enabled!"));
    GuiUpdateAllViews();
    return true;
}

bool cbDebugDisableMemoryBreakpoint(int argc, char* argv[])
{
    if(argc < 2) //disable all memory breakpoints
    {
        if(!BpGetCount(BPMEMORY))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No memory breakpoints to disable!"));
            return true;
        }
        if(!BpEnumAll(cbDisableAllMemoryBreakpoints)) //at least one deletion failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All memory breakpoints disabled!"));
        GuiUpdateAllViews();
        return true;
    }
    BREAKPOINT found;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPMEMORY, 0, &found)) //invalid memory breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such memory breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!found.enabled)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Memory breakpoint already disabled!"));
        return true;
    }
    duint size = 0;
    MemFindBaseAddr(found.addr, &size);
    if(!RemoveMemoryBPX(found.addr, size))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable memory breakpoint %p (RemoveMemoryBPX)\n"), found.addr);
        return false;
    }
    if(!BpEnable(found.addr, BPMEMORY, false))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable memory breakpoint %p (BpEnable)\n"), found.addr);
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Memory breakpoint disabled!"));
    GuiUpdateAllViews();
    return true;
}

static bool cbDeleteAllDllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPDLL || !bp->enabled)
        return true;
    if(!BpDelete(bp->addr, BPDLL))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not delete DLL breakpoint %s (BpDelete)\n"), bp->mod);
        return false;
    }
    if(!dbgdeletedllbreakpoint(bp->mod, bp->titantype))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not delete DLL breakpoint %s (LibrarianRemoveBreakPoint)\n"), bp->mod);
        return false;
    }
    return true;
}

static bool cbEnableAllDllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPDLL || bp->enabled)
        return true;

    if(!BpEnable(bp->addr, BPDLL, true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable DLL breakpoint %s (BpEnable)\n"), bp->mod);
        return false;
    }
    if(!dbgsetdllbreakpoint(bp->mod, bp->titantype, bp->singleshoot))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable DLL breakpoint %s (LibrarianSetBreakPoint)\n"), bp->mod);
        return false;
    }
    return true;
}

static bool cbDisableAllDllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPDLL || !bp->enabled)
        return true;

    if(!BpEnable(bp->addr, BPDLL, false))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable DLL breakpoint %s (BpEnable)\n"), bp->mod);
        return false;
    }
    if(!dbgdeletedllbreakpoint(bp->mod, bp->titantype))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable DLL breakpoint %s (LibrarianRemoveBreakPoint)\n"), bp->mod);
        return false;
    }
    return true;
}

bool cbDebugBpDll(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    _strlwr_s(argv[1], strlen(argv[1]) + 1); //NOTE: does not really work on unicode strings
    DWORD type = UE_ON_LIB_ALL;
    if(argc > 2)
    {
        switch(*argv[2])
        {
        case 'a':
            type = UE_ON_LIB_ALL;
            break;
        case 'l':
            type = UE_ON_LIB_LOAD;
            break;
        case 'u':
            type = UE_ON_LIB_UNLOAD;
            break;
        }
    }
    bool singleshoot = false;
    if(argc > 3)
        singleshoot = true;
    if(!BpNewDll(argv[1], true, singleshoot, type, ""))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error creating Dll breakpoint! (BpNewDll)"));
        return false;
    }
    if(!dbgsetdllbreakpoint(argv[1], type, singleshoot))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error creating Dll breakpoint! (LibrarianSetBreakPoint)"));
        return false;
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "Dll breakpoint set on \"%s\"!\n"), argv[1]);
    DebugUpdateBreakpointsViewAsync();
    return true;
}

bool cbDebugBcDll(int argc, char* argv[])
{
    if(argc < 2)
    {
        // delete all Dll breakpoints
        if(!BpGetCount(BPDLL))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No DLL breakpoints to delete!"));
            return true;
        }
        if(!BpEnumAll(cbDeleteAllDllBreakpoints)) //at least one deletion failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All DLL breakpoints deleted!"));
        DebugUpdateBreakpointsViewAsync();
        return true;
    }
    _strlwr_s(argv[1], strlen(argv[1]) + 1); //NOTE: does not really work on unicode strings
    BREAKPOINT bp;
    if(!BpGetAny(BPDLL, argv[1], &bp))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to find DLL breakpoint '%s'...\n"), argv[1]);
        return false;
    }
    if(!BpDelete(bp.addr, BPDLL))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to remove DLL breakpoint (BpDelete)..."));
        return false;
    }
    DebugUpdateBreakpointsViewAsync();
    if(!dbgdeletedllbreakpoint(bp.mod, bp.titantype))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to remove DLL breakpoint (dbgdeletedllbreakpoint)..."));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "DLL breakpoint removed!"));
    return true;
}

bool cbDebugBpDllEnable(int argc, char* argv[])
{
    if(argc < 2) //enable all DLL breakpoints
    {
        if(!BpGetCount(BPDLL))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No DLL breakpoints to enable!"));
            return true;
        }
        if(!BpEnumAll(cbEnableAllDllBreakpoints)) //at least one deletion failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All DLL breakpoints enabled!"));
        GuiUpdateAllViews();
        return true;
    }
    _strlwr_s(argv[1], strlen(argv[1]) + 1); //NOTE: does not really work on unicode strings
    BREAKPOINT found;
    duint addr = 0;
    if(!BpGetAny(BPDLL, argv[1], &found)) //invalid DLL breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such DLL breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(found.enabled)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "DLL breakpoint already enabled!"));
        return true;
    }
    if(!BpEnable(found.addr, BPDLL, true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable DLL breakpoint %s (BpEnable)\n"), found.mod);
        return false;
    }
    if(!dbgsetdllbreakpoint(found.mod, found.titantype, found.singleshoot))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable DLL breakpoint %s (LibrarianSetBreakPoint)\n"), found.mod);
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "DLL breakpoint enable!"));
    GuiUpdateAllViews();
    return true;
}

bool cbDebugBpDllDisable(int argc, char* argv[])
{
    if(argc < 2) //disable all DLL breakpoints
    {
        if(!BpGetCount(BPDLL))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No DLL breakpoints to disable!"));
            return true;
        }
        if(!BpEnumAll(cbDisableAllDllBreakpoints)) //at least one deletion failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All DLL breakpoints disabled!"));
        GuiUpdateAllViews();
        return true;
    }
    _strlwr_s(argv[1], strlen(argv[1]) + 1); //NOTE: does not really work on unicode strings
    BREAKPOINT found;
    duint addr = 0;
    if(!BpGetAny(BPDLL, argv[1], &found)) //invalid DLL breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such DLL breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!found.enabled)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "DLL breakpoint already disabled!"));
        return true;
    }
    if(!BpEnable(found.addr, BPDLL, false))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable DLL breakpoint %s (BpEnable)\n"), found.mod);
        return false;
    }
    if(!dbgdeletedllbreakpoint(found.mod, found.titantype))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable DLL breakpoint %s (LibrarianRemoveBreakPoint)\n"), found.mod);
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "DLL breakpoint disabled!"));
    GuiUpdateAllViews();
    return true;
}

static bool cbDeleteAllExceptionBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPEXCEPTION)
        return true;

    if(!BpDelete(bp->addr, BPEXCEPTION))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not delete exception breakpoint %p (BpEnable)\n"), bp->addr);
        return false;
    }
    return true;
}

static bool cbEnableAllExceptionBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPEXCEPTION || bp->enabled)
        return true;

    if(!BpEnable(bp->addr, BPEXCEPTION, true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable exception breakpoint %p (BpEnable)\n"), bp->addr);
        return false;
    }
    return true;
}

static bool cbDisableAllExceptionBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPEXCEPTION || !bp->enabled)
        return true;

    if(!BpEnable(bp->addr, BPEXCEPTION, false))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable exception breakpoint %p (BpEnable)\n"), bp->addr);
        return false;
    }
    return true;
}

bool cbDebugSetExceptionBPX(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint ExceptionCode;
    if(!valfromstring(argv[1], &ExceptionCode))
    {
        ExceptionCode = 0;
        if(!ExceptionNameToCode(argv[1], reinterpret_cast<unsigned int*>(&ExceptionCode)))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid exception code: %s.\n"), argv[1]);
            return false;
        }
    }
    const String & ExceptionName = ExceptionCodeToName((unsigned int)ExceptionCode);
    if(BpGet(ExceptionCode, BPEXCEPTION, nullptr, nullptr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Exception breakpoint %X (%s) already exists!\n"), DWORD(ExceptionCode), ExceptionName.c_str());
        return false;
    }
    auto extype = ex_firstchance;
    if(argc > 2)
    {
        duint chance;
        if(scmp(argv[2], "first"))
            extype = ex_firstchance;
        else if(scmp(argv[2], "second"))
            extype = ex_secondchance;
        else if(scmp(argv[2], "all"))
            extype = ex_all;
        else if(valfromstring(argv[2], &chance))
        {
            switch(chance)
            {
            case 1:
                extype = ex_firstchance;
                break;
            case 2:
                extype = ex_secondchance;
                break;
            case 3:
                extype = ex_all;
                break;
            default:
                _plugin_logprintf(QT_TRANSLATE_NOOP("DBG", "Invalid exception type!"));
                return false;
            }
        }
        else
        {
            _plugin_logprintf(QT_TRANSLATE_NOOP("DBG", "Invalid exception type!"));
            return false;
        }
    }
    if(!BpNew(ExceptionCode, true, false, 0, BPEXCEPTION, extype, ""))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to set exception breakpoint! (BpNew)"));
        return false;
    }
    DebugUpdateBreakpointsViewAsync();
    return true;
}

bool cbDebugDeleteExceptionBPX(int argc, char* argv[])
{
    if(argc < 2)
    {
        // delete all exception breakpoints
        if(!BpGetCount(BPEXCEPTION))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No exception breakpoints to delete!"));
            return true;
        }
        if(!BpEnumAll(cbDeleteAllExceptionBreakpoints)) //at least one enable failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All exception breakpoints deleted!"));
        DebugUpdateBreakpointsViewAsync();
        return true;
    }
    BREAKPOINT found;
    if(BpGet(0, BPEXCEPTION, argv[1], &found)) //found a breakpoint with name
    {
        if(!BpDelete(found.addr, BPEXCEPTION))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Delete exception breakpoint failed (bpdel): %p\n"), found.addr);
            return false;
        }
        return true;
    }
    duint addr = 0;
    if((!ExceptionNameToCode(argv[1], reinterpret_cast<unsigned int*>(&addr)) && !valfromstring(argv[1], &addr)) || !BpGet(addr, BPEXCEPTION, 0, &found)) //invalid breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such exception breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!BpDelete(found.addr, BPEXCEPTION))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Delete exception breakpoint failed (bpdel): %p\n"), found.addr);
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Exception breakpoint deleted!"));
    DebugUpdateBreakpointsViewAsync();
    return true;
}

bool cbDebugEnableExceptionBPX(int argc, char* argv[])
{
    if(argc < 2) //enable all breakpoints
    {
        if(!BpGetCount(BPEXCEPTION))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No exception breakpoints to enable!"));
            return true;
        }
        if(!BpEnumAll(cbEnableAllExceptionBreakpoints)) //at least one enable failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All exception breakpoints enabled!"));
        DebugUpdateBreakpointsViewAsync();
        return true;
    }
    BREAKPOINT found;
    if(BpGet(0, BPEXCEPTION, argv[1], &found)) //found a breakpoint with name
    {
        if(!BpEnable(found.addr, BPEXCEPTION, true))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable exception breakpoint %p (BpEnable)\n"), found.addr);
            return false;
        }
        DebugUpdateBreakpointsViewAsync();
        return true;
    }
    duint addr = 0;
    if((!ExceptionNameToCode(argv[1], reinterpret_cast<unsigned int*>(&addr)) && !valfromstring(argv[1], &addr)) || !BpGet(addr, BPEXCEPTION, 0, &found)) //invalid breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such exception breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(found.enabled)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Exception breakpoint already enabled!"));
        DebugUpdateBreakpointsViewAsync();
        return true;
    }
    if(!BpEnable(found.addr, BPEXCEPTION, true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not enable exception breakpoint %p (BpEnable)\n"), found.addr);
        return false;
    }
    DebugUpdateBreakpointsViewAsync();
    dputs(QT_TRANSLATE_NOOP("DBG", "Exception breakpoint enabled!"));
    return true;
}

bool cbDebugDisableExceptionBPX(int argc, char* argv[])
{
    if(argc < 2) //disable all breakpoints
    {
        if(!BpGetCount(BPEXCEPTION))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No exception breakpoints to disable!"));
            return true;
        }
        if(!BpEnumAll(cbDisableAllExceptionBreakpoints)) //at least one deletion failed
            return false;
        dputs(QT_TRANSLATE_NOOP("DBG", "All exception breakpoints disabled!"));
        GuiUpdateAllViews();
        return true;
    }
    BREAKPOINT found;
    if(BpGet(0, BPEXCEPTION, argv[1], &found)) //found a breakpoint with name
    {
        if(!BpEnable(found.addr, BPEXCEPTION, false))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable exception breakpoint %p (BpEnable)\n"), found.addr);
            return false;
        }
        GuiUpdateAllViews();
        return true;
    }
    duint addr = 0;
    if((!ExceptionNameToCode(argv[1], reinterpret_cast<unsigned int*>(&addr)) && !valfromstring(argv[1], &addr)) || !BpGet(addr, BPEXCEPTION, 0, &found)) //invalid breakpoint
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such exception breakpoint \"%s\"\n"), argv[1]);
        return false;
    }
    if(!found.enabled)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Exception breakpoint already disabled!"));
        return true;
    }
    if(!BpEnable(found.addr, BPEXCEPTION, false))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not disable exception breakpoint %p (BpEnable)\n"), found.addr);
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Exception breakpoint disabled!"));
    GuiUpdateAllViews();
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
        if(*bp->name)
            dprintf_untranslated("%d:%s:\"%s\":\"%s\"\n", enabled, type, bp->mod, bp->name);
        else
            dprintf_untranslated("%d:%s:\"%s\"\n", enabled, type, bp->mod);
    }
    else if(*bp->name)
        dprintf_untranslated("%d:%s:%p:\"%s\"\n", enabled, type, bp->addr, bp->name);
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
    DWORD type = 0;
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
