/**
 @file debugger_commands.cpp

 @brief Implements the debugger commands class.
 */

#include "debugger_commands.h"
#include "jit.h"
#include "console.h"
#include "value.h"
#include "thread.h"
#include "memory.h"
#include "threading.h"
#include "variable.h"
#include "plugin_loader.h"
#include "simplescript.h"
#include "symbolinfo.h"
#include "assemble.h"
#include "disasm_fast.h"
#include "module.h"
#include "comment.h"
#include "label.h"
#include "bookmark.h"
#include "function.h"

static bool bScyllaLoaded = false;
duint LoadLibThreadID;
duint DLLNameMem;
duint ASMAddr;
TITAN_ENGINE_CONTEXT_t backupctx = { 0 };
extern std::vector<std::pair<duint, duint>> RunToUserCodeBreakpoints;

CMDRESULT cbDebugInit(int argc, char* argv[])
{
    cbDebugStop(argc, argv);

    static char arg1[deflen] = "";
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    strcpy_s(arg1, argv[1]);
    char szResolvedPath[MAX_PATH] = "";
    if(ResolveShortcut(GuiGetWindowHandle(), StringUtils::Utf8ToUtf16(arg1).c_str(), szResolvedPath, _countof(szResolvedPath)))
    {
        dprintf("Resolved shortcut \"%s\"->\"%s\"\n", arg1, szResolvedPath);
        strcpy_s(arg1, szResolvedPath);
    }
    if(!FileExists(arg1))
    {
        dputs("File does not exist!");
        return STATUS_ERROR;
    }
    Handle hFile = CreateFileW(StringUtils::Utf8ToUtf16(arg1).c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        dputs("Could not open file!");
        return STATUS_ERROR;
    }
    GetFileNameFromHandle(hFile, arg1); //get full path of the file
    hFile.Close();

    //do some basic checks
    switch(GetFileArchitecture(arg1))
    {
    case invalid:
        dputs("Invalid PE file!");
        return STATUS_ERROR;
#ifdef _WIN64
    case x32:
        dputs("Use x32dbg to debug this file!");
#else //x86
    case x64:
        dputs("Use x64dbg to debug this file!");
#endif //_WIN64
        return STATUS_ERROR;
    default:
        break;
    }

    static char arg2[deflen] = "";
    if(argc > 2)
        strcpy_s(arg2, argv[2]);
    char* commandline = 0;
    if(strlen(arg2))
        commandline = arg2;

    char arg3[deflen] = "";
    if(argc > 3)
        strcpy_s(arg3, argv[3]);

    static char currentfolder[deflen] = "";
    strcpy_s(currentfolder, arg1);
    int len = (int)strlen(currentfolder);
    while(currentfolder[len] != '\\' && len != 0)
        len--;
    currentfolder[len] = 0;

    if(DirExists(arg3))
        strcpy_s(currentfolder, arg3);

    static INIT_STRUCT init;
    memset(&init, 0, sizeof(INIT_STRUCT));
    init.exe = arg1;
    init.commandline = commandline;
    if(*currentfolder)
        init.currentfolder = currentfolder;
    CloseHandle(CreateThread(0, 0, threadDebugLoop, &init, 0, 0));
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugStop(int argc, char* argv[])
{
    // HACK: TODO: Don't kill script on debugger ending a process
    //scriptreset(); //reset the currently-loaded script
    StopDebug();
    while(waitislocked(WAITID_STOP))  //custom waiting
    {
        unlock(WAITID_RUN);
        Sleep(1);
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugRun(int argc, char* argv[])
{
    // Don't "run" twice if the program is already running
    if(dbgisrunning())
        return STATUS_ERROR;

    dbgsetispausedbyuser(false);
    GuiSetDebugState(running);
    unlock(WAITID_RUN);
    PLUG_CB_RESUMEDEBUG callbackInfo;
    callbackInfo.reserved = 0;
    plugincbcall(CB_RESUMEDEBUG, &callbackInfo);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugErun(int argc, char* argv[])
{
    if(!dbgisrunning())
        dbgsetskipexceptions(true);
    return cbDebugRun(argc, argv);
}

CMDRESULT cbDebugSetBPXOptions(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments!");
        return STATUS_ERROR;
    }
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
        dputs("Invalid type specified!");
        return STATUS_ERROR;
    }
    SetBPXOptions(type);
    BridgeSettingSetUint("Engine", "BreakpointType", setting_type);
    dprintf("Default breakpoint type set to: %s\n", strType);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetBPX(int argc, char* argv[]) //bp addr [,name [,type]]
{
    if(argc < 2)
    {
        dputs("Not enough arguments!");
        return STATUS_ERROR;
    }
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
    _strlwr(argtype);
    duint addr = 0;
    if(!valfromstring(argaddr, &addr))
    {
        dprintf("Invalid addr: \"%s\"\n", argaddr);
        return STATUS_ERROR;
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
            return DbgCmdExecDirect(StringUtils::sprintf("bpe " fhex, bp.addr).c_str()) ? STATUS_CONTINUE : STATUS_ERROR;
        dputs("Breakpoint already set!");
        return STATUS_CONTINUE;
    }
    if(IsBPXEnabled(addr))
    {
        dprintf("Error setting breakpoint at " fhex "! (IsBPXEnabled)\n", addr);
        return STATUS_ERROR;
    }
    if(!MemRead(addr, &oldbytes, sizeof(short)))
    {
        dprintf("Error setting breakpoint at " fhex "! (memread)\n", addr);
        return STATUS_ERROR;
    }
    if(!BpNew(addr, true, singleshoot, oldbytes, BPNORMAL, type, bpname))
    {
        dprintf("Error setting breakpoint at " fhex "! (bpnew)\n", addr);
        return STATUS_ERROR;
    }
    GuiUpdateAllViews();
    if(!SetBPX(addr, type, (void*)cbUserBreakpoint))
    {
        if(!MemIsValidReadPtr(addr))
            return STATUS_CONTINUE;
        dprintf("Error setting breakpoint at " fhex "! (SetBPX)\n", addr);
        return STATUS_ERROR;
    }
    dprintf("Breakpoint at " fhex " set!\n", addr);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDeleteBPX(int argc, char* argv[])
{
    if(argc < 2) //delete all breakpoints
    {
        if(!BpGetCount(BPNORMAL))
        {
            dputs("No breakpoints to delete!");
            return STATUS_CONTINUE;
        }
        if(!BpEnumAll(cbDeleteAllBreakpoints))  //at least one deletion failed
        {
            GuiUpdateAllViews();
            return STATUS_ERROR;
        }
        dputs("All breakpoints deleted!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    if(BpGet(0, BPNORMAL, argv[1], &found)) //found a breakpoint with name
    {
        if(!BpDelete(found.addr, BPNORMAL))
        {
            dprintf("Delete breakpoint failed (bpdel): " fhex "\n", found.addr);
            return STATUS_ERROR;
        }
        if(found.enabled && !DeleteBPX(found.addr))
        {
            GuiUpdateAllViews();
            if(!MemIsValidReadPtr(found.addr))
                return STATUS_CONTINUE;
            dprintf("Delete breakpoint failed (DeleteBPX): " fhex "\n", found.addr);
            return STATUS_ERROR;
        }
        return STATUS_CONTINUE;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPNORMAL, 0, &found)) //invalid breakpoint
    {
        dprintf("No such breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!BpDelete(found.addr, BPNORMAL))
    {
        dprintf("Delete breakpoint failed (bpdel): " fhex "\n", found.addr);
        return STATUS_ERROR;
    }
    if(found.enabled && !DeleteBPX(found.addr))
    {
        GuiUpdateAllViews();
        if(!MemIsValidReadPtr(found.addr))
            return STATUS_CONTINUE;
        dprintf("Delete breakpoint failed (DeleteBPX): " fhex "\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("Breakpoint deleted!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugEnableBPX(int argc, char* argv[])
{
    if(argc < 2) //enable all breakpoints
    {
        if(!BpGetCount(BPNORMAL))
        {
            dputs("No breakpoints to enable!");
            return STATUS_CONTINUE;
        }
        if(!BpEnumAll(cbEnableAllBreakpoints)) //at least one enable failed
            return STATUS_ERROR;
        dputs("All breakpoints enabled!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    if(BpGet(0, BPNORMAL, argv[1], &found)) //found a breakpoint with name
    {
        if(!SetBPX(found.addr, found.titantype, (void*)cbUserBreakpoint))
        {
            dprintf("Could not enable breakpoint " fhex " (SetBPX)\n", found.addr);
            return STATUS_ERROR;
        }
        if(!BpEnable(found.addr, BPNORMAL, true))
        {
            dprintf("Could not enable breakpoint " fhex " (BpEnable)\n", found.addr);
            return STATUS_ERROR;
        }
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPNORMAL, 0, &found)) //invalid breakpoint
    {
        dprintf("No such breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(found.enabled)
    {
        dputs("Breakpoint already enabled!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    if(!SetBPX(found.addr, found.titantype, (void*)cbUserBreakpoint))
    {
        dprintf("Could not enable breakpoint " fhex " (SetBPX)\n", found.addr);
        return STATUS_ERROR;
    }
    if(!BpEnable(found.addr, BPNORMAL, true))
    {
        dprintf("Could not enable breakpoint " fhex " (BpEnable)\n", found.addr);
        return STATUS_ERROR;
    }
    GuiUpdateAllViews();
    dputs("Breakpoint enabled!");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDisableBPX(int argc, char* argv[])
{
    if(argc < 2) //delete all breakpoints
    {
        if(!BpGetCount(BPNORMAL))
        {
            dputs("No breakpoints to disable!");
            return STATUS_CONTINUE;
        }
        if(!BpEnumAll(cbDisableAllBreakpoints)) //at least one deletion failed
            return STATUS_ERROR;
        dputs("All breakpoints disabled!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    if(BpGet(0, BPNORMAL, argv[1], &found)) //found a breakpoint with name
    {
        if(!BpEnable(found.addr, BPNORMAL, false))
        {
            dprintf("Could not disable breakpoint " fhex " (BpEnable)\n", found.addr);
            return STATUS_ERROR;
        }
        if(!DeleteBPX(found.addr))
        {
            GuiUpdateAllViews();
            if(!MemIsValidReadPtr(found.addr))
                return STATUS_CONTINUE;
            dprintf("Could not disable breakpoint " fhex " (DeleteBPX)\n", found.addr);
            return STATUS_ERROR;
        }
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPNORMAL, 0, &found)) //invalid breakpoint
    {
        dprintf("No such breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!found.enabled)
    {
        dputs("Breakpoint already disabled!");
        return STATUS_CONTINUE;
    }
    if(!BpEnable(found.addr, BPNORMAL, false))
    {
        dprintf("Could not disable breakpoint " fhex " (BpEnable)\n", found.addr);
        return STATUS_ERROR;
    }
    if(!DeleteBPX(found.addr))
    {
        GuiUpdateAllViews();
        if(!MemIsValidReadPtr(found.addr))
            return STATUS_CONTINUE;
        dprintf("Could not disable breakpoint " fhex " (DeleteBPX)\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("Breakpoint disabled!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

static CMDRESULT cbDebugSetBPXTextCommon(BP_TYPE Type, int argc, char* argv[], const char* description, std::function<bool(duint, BP_TYPE, const char*)> setFunction)
{
    BREAKPOINT bp;
    if(argc < 2)
    {
        dprintf("not enough arguments!\n");
        return STATUS_ERROR;
    }
    auto value = "";
    if(argc > 2)
        value = argv[2];

    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf("No such breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!setFunction(bp.addr, Type, value))
    {
        dprintf("Can't set %s on breakpoint \"%s\"\n", description, argv[1]);
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

static CMDRESULT cbDebugSetBPXNameCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, "name", BpSetName);
}

static CMDRESULT cbDebugSetBPXConditionCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, "break condition", BpSetBreakCondition);
}

static CMDRESULT cbDebugSetBPXLogCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, "logging text", BpSetLogText);
}

static CMDRESULT cbDebugSetBPXLogConditionCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, "logging condition", BpSetLogCondition);
}

static CMDRESULT cbDebugSetBPXCommandCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, "command on hit", BpSetCommandText);
}

static CMDRESULT cbDebugSetBPXCommandConditionCommon(BP_TYPE Type, int argc, char* argv[])
{
    return cbDebugSetBPXTextCommon(Type, argc, argv, "command condition", BpSetCommandCondition);
}

static CMDRESULT cbDebugGetBPXHitCountCommon(BP_TYPE Type, int argc, char* argv[])
{
    if(argc < 2)
    {
        dprintf("not enough arguments!\n");
        return STATUS_ERROR;
    }
    BREAKPOINT bp;
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf("No such breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    varset("$result", bp.hitcount, false);
    return STATUS_CONTINUE;

}

static CMDRESULT cbDebugResetBPXHitCountCommon(BP_TYPE Type, int argc, char* argv[])
{
    if(argc < 2)
    {
        dprintf("not enough arguments!\n");
        return STATUS_ERROR;
    }
    duint value = 0;
    if(argc > 2)
        if(!valfromstring(argv[2], &value, false))
            return STATUS_ERROR;
    BREAKPOINT bp;
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf("No such breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!BpResetHitCount(bp.addr, Type, (uint32)value))
    {
        dprintf("Can't set hit count on breakpoint \"%s\"", argv[1]);
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;

}

static CMDRESULT cbDebugSetBPXFastResumeCommon(BP_TYPE Type, int argc, char* argv[])
{
    BREAKPOINT bp;
    if(argc < 2)
    {
        dprintf("not enough arguments!\n");
        return STATUS_ERROR;
    }
    auto fastResume = true;
    if(argc > 2)
    {
        duint value;
        if(!valfromstring(argv[2], &value, false))
            return STATUS_ERROR;
        fastResume = value != 0;
    }
    if(!BpGetAny(Type, argv[1], &bp))
    {
        dprintf("No such breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!BpSetFastResume(bp.addr, Type, fastResume))
    {
        dprintf("Can't set fast resume on breakpoint \"%1\"", argv[1]);
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetBPXName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXFastResume(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugResetBPXHitCount(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugGetBPXHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPNORMAL, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXHardwareFastResume(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugResetBPXHardwareHitCount(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugGetBPXHardwareHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryCondition(int argc, char* argv[])
{
    return cbDebugSetBPXConditionCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryLog(int argc, char* argv[])
{
    return cbDebugSetBPXLogCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryLogCondition(int argc, char* argv[])
{
    return cbDebugSetBPXLogConditionCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryCommand(int argc, char* argv[])
{
    return cbDebugSetBPXCommandCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryCommandCondition(int argc, char* argv[])
{
    return cbDebugSetBPXCommandConditionCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugResetBPXMemoryHitCount(int argc, char* argv[])
{
    return cbDebugSetBPXFastResumeCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPXMemoryFastResume(int argc, char* argv[])
{
    return cbDebugResetBPXHitCountCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugGetBPXMemoryHitCount(int argc, char* argv[])
{
    return cbDebugGetBPXHitCountCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugSetBPGoto(int argc, char* argv[])
{
    if(argc != 3)
    {
        dputs("argument count mismatch!\n");
        return STATUS_ERROR;
    }
    char cmd[deflen];
    _snprintf(cmd, sizeof(cmd), "SetBreakpointCondition %s, 0", argv[1]);
    if(!DbgCmdExecDirect(cmd))
        return STATUS_ERROR;
    _snprintf(cmd, sizeof(cmd), "SetBreakpointCommand %s, \"CIP=%s\"", argv[1], argv[2]);
    if(!DbgCmdExecDirect(cmd))
        return STATUS_ERROR;
    _snprintf(cmd, sizeof(cmd), "SetBreakpointCommandCondition %s, 1", argv[1]);
    if(!DbgCmdExecDirect(cmd))
        return STATUS_ERROR;
    _snprintf(cmd, sizeof(cmd), "SetBreakpointFastResume %s, 0", argv[1]);
    if(!DbgCmdExecDirect(cmd))
        return STATUS_ERROR;
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetHardwareBreakpoint(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr;
    if(!valfromstring(argv[1], &addr))
        return STATUS_ERROR;
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
            dputs("Invalid type, assuming 'x'");
            break;
        }
    }
    DWORD titsize = UE_HARDWARE_SIZE_1;
    if(argc > 3)
    {
        duint size;
        if(!valfromstring(argv[3], &size))
            return STATUS_ERROR;
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
            dputs("Invalid size, using 1");
            break;
        }
        if((addr % size) != 0)
        {
            dprintf("Address not aligned to %d\n", size);
            return STATUS_ERROR;
        }
    }
    DWORD drx = 0;
    if(!GetUnusedHardwareBreakPointRegister(&drx))
    {
        dputs("You can only set 4 hardware breakpoints");
        return STATUS_ERROR;
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
            return DbgCmdExecDirect(StringUtils::sprintf("bphwe " fhex, bp.addr).c_str()) ? STATUS_CONTINUE : STATUS_ERROR;
        dputs("Hardware breakpoint already set!");
        return STATUS_CONTINUE;
    }
    if(!BpNew(addr, true, false, 0, BPHARDWARE, titantype, 0))
    {
        dputs("Error setting hardware breakpoint (bpnew)!");
        return STATUS_ERROR;
    }
    if(!SetHardwareBreakPoint(addr, drx, type, titsize, (void*)cbHardwareBreakpoint))
    {
        dputs("Error setting hardware breakpoint (TitanEngine)!");
        return STATUS_ERROR;
    }
    dprintf("Hardware breakpoint at " fhex " set!\n", addr);
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDeleteHardwareBreakpoint(int argc, char* argv[])
{
    if(argc < 2)  //delete all breakpoints
    {
        if(!BpGetCount(BPHARDWARE))
        {
            dputs("No hardware breakpoints to delete!");
            return STATUS_CONTINUE;
        }
        if(!BpEnumAll(cbDeleteAllHardwareBreakpoints))  //at least one deletion failed
            return STATUS_ERROR;
        dputs("All hardware breakpoints deleted!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    if(BpGet(0, BPHARDWARE, argv[1], &found))  //found a breakpoint with name
    {
        if(!BpDelete(found.addr, BPHARDWARE))
        {
            dprintf("Delete hardware breakpoint failed: " fhex " (BpDelete)\n", found.addr);
            return STATUS_ERROR;
        }
        if(!DeleteHardwareBreakPoint(TITANGETDRX(found.titantype)))
        {
            dprintf("Delete hardware breakpoint failed: " fhex " (DeleteHardwareBreakPoint)\n", found.addr);
            return STATUS_ERROR;
        }
        return STATUS_CONTINUE;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPHARDWARE, 0, &found))  //invalid breakpoint
    {
        dprintf("No such hardware breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!BpDelete(found.addr, BPHARDWARE))
    {
        dprintf("Delete hardware breakpoint failed: " fhex " (BpDelete)\n", found.addr);
        return STATUS_ERROR;
    }
    if(!DeleteHardwareBreakPoint(TITANGETDRX(found.titantype)))
    {
        dprintf("Delete hardware breakpoint failed: " fhex " (DeleteHardwareBreakPoint)\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("Hardware breakpoint deleted!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugEnableHardwareBreakpoint(int argc, char* argv[])
{
    DWORD drx = 0;
    if(!GetUnusedHardwareBreakPointRegister(&drx))
    {
        dputs("You can only set 4 hardware breakpoints");
        return STATUS_ERROR;
    }
    if(argc < 2)  //enable all hardware breakpoints
    {
        if(!BpGetCount(BPHARDWARE))
        {
            dputs("No hardware breakpoints to enable!");
            return STATUS_CONTINUE;
        }
        if(!BpEnumAll(cbEnableAllHardwareBreakpoints))  //at least one enable failed
            return STATUS_ERROR;
        dputs("All hardware breakpoints enabled!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPHARDWARE, 0, &found))  //invalid hardware breakpoint
    {
        dprintf("No such hardware breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(found.enabled)
    {
        dputs("Hardware breakpoint already enabled!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    TITANSETDRX(found.titantype, drx);
    BpSetTitanType(found.addr, BPHARDWARE, found.titantype);
    if(!SetHardwareBreakPoint(found.addr, drx, TITANGETTYPE(found.titantype), TITANGETSIZE(found.titantype), (void*)cbHardwareBreakpoint))
    {
        dprintf("Could not enable hardware breakpoint " fhex " (SetHardwareBreakpoint)\n", found.addr);
        return STATUS_ERROR;
    }
    if(!BpEnable(found.addr, BPHARDWARE, true))
    {
        dprintf("Could not enable hardware breakpoint " fhex " (BpEnable)\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("Hardware breakpoint enabled!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDisableHardwareBreakpoint(int argc, char* argv[])
{
    if(argc < 2)  //delete all hardware breakpoints
    {
        if(!BpGetCount(BPHARDWARE))
        {
            dputs("No hardware breakpoints to disable!");
            return STATUS_CONTINUE;
        }
        if(!BpEnumAll(cbDisableAllHardwareBreakpoints))  //at least one deletion failed
            return STATUS_ERROR;
        dputs("All hardware breakpoints disabled!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPHARDWARE, 0, &found))  //invalid hardware breakpoint
    {
        dprintf("No such hardware breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!found.enabled)
    {
        dputs("Hardware breakpoint already disabled!");
        return STATUS_CONTINUE;
    }
    if(!BpEnable(found.addr, BPHARDWARE, false))
    {
        dprintf("Could not disable hardware breakpoint " fhex " (BpEnable)\n", found.addr);
        return STATUS_ERROR;
    }
    if(!DeleteHardwareBreakPoint(TITANGETDRX(found.titantype)))
    {
        dprintf("Could not disable hardware breakpoint " fhex " (DeleteHardwareBreakpoint)\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("Hardware breakpoint disabled!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetBPXHardwareName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPHARDWARE, argc, argv);
}

CMDRESULT cbDebugSetMemoryBpx(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr;
    if(!valfromstring(argv[1], &addr))
        return STATUS_ERROR;
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
        case 'r':
            type = UE_MEMORY_READ;
            break;
        case 'w':
            type = UE_MEMORY_WRITE;
            break;
        case 'x':
            type = UE_MEMORY_EXECUTE; //EXECUTE
            break;
        default:
            dputs("Invalid type (argument ignored)");
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
            return BpEnable(base, BPMEMORY, true) ? STATUS_CONTINUE : STATUS_ERROR;
        dputs("Memory breakpoint already set!");
        return STATUS_CONTINUE;
    }
    if(!BpNew(base, true, singleshoot, 0, BPMEMORY, type, 0))
    {
        dputs("Error setting memory breakpoint! (BpNew)");
        return STATUS_ERROR;
    }
    if(!SetMemoryBPXEx(base, size, type, restore, (void*)cbMemoryBreakpoint))
    {
        dputs("Error setting memory breakpoint! (SetMemoryBPXEx)");
        return STATUS_ERROR;
    }
    dprintf("Memory breakpoint at " fhex " set!\n", addr);
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDeleteMemoryBreakpoint(int argc, char* argv[])
{
    if(argc < 2)  //delete all breakpoints
    {
        if(!BpGetCount(BPMEMORY))
        {
            dputs("no memory breakpoints to delete!");
            return STATUS_CONTINUE;
        }
        if(!BpEnumAll(cbDeleteAllMemoryBreakpoints))  //at least one deletion failed
            return STATUS_ERROR;
        dputs("All memory breakpoints deleted!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    if(BpGet(0, BPMEMORY, argv[1], &found))  //found a breakpoint with name
    {
        duint size;
        MemFindBaseAddr(found.addr, &size);
        if(!BpDelete(found.addr, BPMEMORY))
        {
            dprintf("Delete memory breakpoint failed: " fhex " (BpDelete)\n", found.addr);
            return STATUS_ERROR;
        }
        if(!RemoveMemoryBPX(found.addr, size))
        {
            dprintf("Delete memory breakpoint failed: " fhex " (RemoveMemoryBPX)\n", found.addr);
            return STATUS_ERROR;
        }
        return STATUS_CONTINUE;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPMEMORY, 0, &found))  //invalid breakpoint
    {
        dprintf("No such memory breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    duint size;
    MemFindBaseAddr(found.addr, &size);
    if(!BpDelete(found.addr, BPMEMORY))
    {
        dprintf("Delete memory breakpoint failed: " fhex " (BpDelete)\n", found.addr);
        return STATUS_ERROR;
    }
    if(!RemoveMemoryBPX(found.addr, size))
    {
        dprintf("Delete memory breakpoint failed: " fhex " (RemoveMemoryBPX)\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("Memory breakpoint deleted!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugEnableMemoryBreakpoint(int argc, char* argv[])
{
    if(argc < 2)  //enable all memory breakpoints
    {
        if(!BpGetCount(BPMEMORY))
        {
            dputs("No memory breakpoints to enable!");
            return STATUS_CONTINUE;
        }
        if(!BpEnumAll(cbEnableAllMemoryBreakpoints))  //at least one enable failed
            return STATUS_ERROR;
        dputs("All memory breakpoints enabled!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPMEMORY, 0, &found))  //invalid memory breakpoint
    {
        dprintf("No such memory breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(found.enabled)
    {
        dputs("Memory memory already enabled!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    duint size = 0;
    MemFindBaseAddr(found.addr, &size);
    if(!SetMemoryBPXEx(found.addr, size, found.titantype, !found.singleshoot, (void*)cbMemoryBreakpoint))
    {
        dprintf("Could not enable memory breakpoint " fhex " (SetMemoryBPXEx)\n", found.addr);
        return STATUS_ERROR;
    }
    if(!BpEnable(found.addr, BPMEMORY, true))
    {
        dprintf("Could not enable memory breakpoint " fhex " (BpEnable)\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("Memory breakpoint enabled!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDisableMemoryBreakpoint(int argc, char* argv[])
{
    if(argc < 2)  //delete all memory breakpoints
    {
        if(!BpGetCount(BPMEMORY))
        {
            dputs("No memory breakpoints to disable!");
            return STATUS_CONTINUE;
        }
        if(!BpEnumAll(cbDisableAllMemoryBreakpoints))  //at least one deletion failed
            return STATUS_ERROR;
        dputs("All memory breakpoints disabled!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !BpGet(addr, BPMEMORY, 0, &found))  //invalid memory breakpoint
    {
        dprintf("No such memory breakpoint \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!found.enabled)
    {
        dputs("Memory breakpoint already disabled!");
        return STATUS_CONTINUE;
    }
    duint size = 0;
    MemFindBaseAddr(found.addr, &size);
    if(!RemoveMemoryBPX(found.addr, size))
    {
        dprintf("Could not disable memory breakpoint " fhex " (RemoveMemoryBPX)\n", found.addr);
        return STATUS_ERROR;
    }
    if(!BpEnable(found.addr, BPMEMORY, false))
    {
        dprintf("Could not disable memory breakpoint " fhex " (BpEnable)\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("Memory breakpoint disabled!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetBPXMemoryName(int argc, char* argv[])
{
    return cbDebugSetBPXNameCommon(BPMEMORY, argc, argv);
}

CMDRESULT cbDebugBplist(int argc, char* argv[])
{
    if(!BpEnumAll(cbBreakpointList))
    {
        dputs("Something went wrong...");
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugStepInto(int argc, char* argv[])
{
    StepInto((void*)cbStep);
    dbgsetstepping(true);
    return cbDebugRun(argc, argv);
}

CMDRESULT cbDebugeStepInto(int argc, char* argv[])
{
    dbgsetskipexceptions(true);
    return cbDebugStepInto(argc, argv);
}

CMDRESULT cbDebugStepOver(int argc, char* argv[])
{
    StepOver((void*)cbStep);
    dbgsetstepping(true);
    return cbDebugRun(argc, argv);
}

CMDRESULT cbDebugeStepOver(int argc, char* argv[])
{
    dbgsetskipexceptions(true);
    return cbDebugStepOver(argc, argv);
}

CMDRESULT cbDebugSingleStep(int argc, char* argv[])
{
    duint stepcount = 1;
    if(argc > 1)
        if(!valfromstring(argv[1], &stepcount))
            stepcount = 1;
    SingleStep((DWORD)stepcount, (void*)cbStep);
    dbgsetstepping(true);
    return cbDebugRun(argc, argv);
}

CMDRESULT cbDebugeSingleStep(int argc, char* argv[])
{
    dbgsetskipexceptions(true);
    return cbDebugSingleStep(argc, argv);
}

CMDRESULT cbDebugHide(int argc, char* argv[])
{
    if(HideDebugger(fdProcessInfo->hProcess, UE_HIDE_PEBONLY))
        dputs("Debugger hidden");
    else
        dputs("Something went wrong");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDisasm(int argc, char* argv[])
{
    duint addr = 0;
    if(argc > 1)
    {
        if(!valfromstring(argv[1], &addr))
            addr = GetContextDataEx(hActiveThread, UE_CIP);
    }
    else
    {
        addr = GetContextDataEx(hActiveThread, UE_CIP);
    }
    if(!MemIsValidReadPtr(addr))
        return STATUS_CONTINUE;
    DebugUpdateGui(addr, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugRtr(int argc, char* argv[])
{
    StepOver((void*)cbRtrStep);
    cbDebugRun(argc, argv);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugeRtr(int argc, char* argv[])
{
    dbgsetskipexceptions(true);
    return cbDebugRtr(argc, argv);
}

CMDRESULT cbDebugRunToParty(int argc, char* argv[])
{
    EXCLUSIVE_ACQUIRE(LockRunToUserCode);
    std::vector<MODINFO> AllModules;
    ModGetList(AllModules);
    if(!RunToUserCodeBreakpoints.empty())
    {
        dputs("Run to party is busy.\n");
        return STATUS_ERROR;
    }
    int party = atoi(argv[1]); // party is a signed integer
    for(auto i : AllModules)
    {
        if(i.party == party)
        {
            for(auto j : i.sections)
            {
                BREAKPOINT bp;
                if(!BpGet(j.addr, BPMEMORY, nullptr, &bp))
                {
                    RunToUserCodeBreakpoints.push_back(std::make_pair(j.addr, j.size));
                    SetMemoryBPXEx(j.addr, j.size, UE_MEMORY_EXECUTE, false, (void*)cbRunToUserCodeBreakpoint);
                }
            }
        }
    }
    cbDebugRun(argc, argv);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugRtu(int argc, char* argv[])
{
    char* newargv[] = { "RunToParty", "0" };
    return cbDebugRunToParty(argc, newargv);
}

static CMDRESULT cbDebugConditionalTrace(void* callBack, bool stepOver, int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments");
        return STATUS_ERROR;
    }
    if(dbgtraceactive())
    {
        dputs("Trace already active");
        return STATUS_ERROR;
    }
    duint maxCount = 50000;
    if(argc > 2 && !valfromstring(argv[2], &maxCount, false))
        return STATUS_ERROR;
    if(!dbgsettracecondition(argv[1], maxCount))
    {
        dprintf("Invalid expression \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(stepOver)
        StepOver(callBack);
    else
        StepInto(callBack);
    cbDebugRun(argc, argv);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugTocnd(int argc, char* argv[])
{
    return cbDebugConditionalTrace((void*)cbTOCNDStep, true, argc, argv);
}

CMDRESULT cbDebugTicnd(int argc, char* argv[])
{
    return cbDebugConditionalTrace((void*)cbTICNDStep, false, argc, argv);
}

CMDRESULT cbDebugTibt(int argc, char* argv[])
{
    if(argc == 1)
    {
        char* new_argv[] = { "tibt", "0" };
        return cbDebugConditionalTrace((void*)cbTIBTStep, false, 2, new_argv);
    }
    else
        return cbDebugConditionalTrace((void*)cbTIBTStep, false, argc, argv);
}

CMDRESULT cbDebugTobt(int argc, char* argv[])
{
    if(argc == 1)
    {
        char* new_argv[] = { "tobt", "0" };
        return cbDebugConditionalTrace((void*)cbTOBTStep, true, 2, new_argv);
    }
    else
        return cbDebugConditionalTrace((void*)cbTOBTStep, true, argc, argv);
}

CMDRESULT cbDebugTiit(int argc, char* argv[])
{
    if(argc == 1)
    {
        char* new_argv[] = { "tiit", "0" };
        return cbDebugConditionalTrace((void*)cbTIITStep, false, 2, new_argv);
    }
    else
        return cbDebugConditionalTrace((void*)cbTIITStep, false, argc, argv);
}

CMDRESULT cbDebugToit(int argc, char* argv[])
{
    if(argc == 1)
    {
        char* new_argv[] = { "toit", "0" };
        return cbDebugConditionalTrace((void*)cbTOITStep, true, 2, new_argv);
    }
    else
        return cbDebugConditionalTrace((void*)cbTOITStep, true, argc, argv);
}

CMDRESULT cbDebugAlloc(int argc, char* argv[])
{
    duint size = 0x1000;
    if(argc > 1)
        if(!valfromstring(argv[1], &size, false))
            return STATUS_ERROR;
    duint mem = (duint)MemAllocRemote(0, size);
    if(!mem)
        dputs("VirtualAllocEx failed");
    else
        dprintf(fhex"\n", mem);
    if(mem)
        varset("$lastalloc", mem, true);
    //update memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    varset("$res", mem, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugFree(int argc, char* argv[])
{
    duint lastalloc;
    varget("$lastalloc", &lastalloc, 0, 0);
    duint addr = lastalloc;
    if(argc > 1)
    {
        if(!valfromstring(argv[1], &addr, false))
            return STATUS_ERROR;
    }
    else if(!lastalloc)
    {
        dputs("$lastalloc is zero, provide a page address");
        return STATUS_ERROR;
    }
    if(addr == lastalloc)
        varset("$lastalloc", (duint)0, true);
    bool ok = !!VirtualFreeEx(fdProcessInfo->hProcess, (void*)addr, 0, MEM_RELEASE);
    if(!ok)
        dputs("VirtualFreeEx failed");
    //update memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    varset("$res", ok, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugMemset(int argc, char* argv[])
{
    duint addr;
    duint value;
    duint size;
    if(argc < 3)
    {
        dputs("Not enough arguments");
        return STATUS_ERROR;
    }
    if(!valfromstring(argv[1], &addr, false) || !valfromstring(argv[2], &value, false))
        return STATUS_ERROR;
    if(argc > 3)
    {
        if(!valfromstring(argv[3], &size, false))
            return STATUS_ERROR;
    }
    else
    {
        duint base = MemFindBaseAddr(addr, &size, true);
        if(!base)
        {
            dputs("Invalid address specified");
            return STATUS_ERROR;
        }
        duint diff = addr - base;
        addr = base + diff;
        size -= diff;
    }
    BYTE fi = value & 0xFF;
    if(!Fill((void*)addr, size & 0xFFFFFFFF, &fi))
        dputs("Memset failed");
    else
        dprintf("Memory " fhex " (size: %.8X) set to %.2X\n", addr, size & 0xFFFFFFFF, value & 0xFF);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugBenchmark(int argc, char* argv[])
{
    duint addr = MemFindBaseAddr(GetContextDataEx(hActiveThread, UE_CIP), 0);
    DWORD ticks = GetTickCount();
    for(duint i = addr; i < addr + 100000; i++)
    {
        CommentSet(i, "test", false);
        LabelSet(i, "test", false);
        BookmarkSet(i, false);
        FunctionAdd(i, i, false);
    }
    dprintf("%ums\n", GetTickCount() - ticks);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugPause(int argc, char* argv[])
{
    if(!dbgisrunning())
    {
        dputs("Program is not running");
        return STATUS_ERROR;
    }
    if(SuspendThread(hActiveThread) == -1)
    {
        dputs("Error suspending thread");
        return STATUS_ERROR;
    }
    duint CIP = GetContextDataEx(hActiveThread, UE_CIP);
    if(!SetBPX(CIP, UE_BREAKPOINT, (void*)cbPauseBreakpoint))
    {
        dprintf("Error setting breakpoint at " fhex "! (SetBPX)\n", CIP);
        if(ResumeThread(hActiveThread) == -1)
        {
            dputs("Error resuming thread");
            return STATUS_ERROR;
        }
        return STATUS_ERROR;
    }
    dbgsetispausedbyuser(true);
    if(ResumeThread(hActiveThread) == -1)
    {
        dputs("Error resuming thread");
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

static DWORD WINAPI scyllaThread(void* lpParam)
{
    typedef INT (WINAPI * SCYLLASTARTGUI)(DWORD pid, HINSTANCE mod, DWORD_PTR entrypoint);
    SCYLLASTARTGUI ScyllaStartGui = 0;
    HINSTANCE hScylla = LoadLibraryW(L"Scylla.dll");
    if(!hScylla)
    {
        dputs("Error loading Scylla.dll!");
        bScyllaLoaded = false;
        FreeLibrary(hScylla);
        return 0;
    }
    ScyllaStartGui = (SCYLLASTARTGUI)GetProcAddress(hScylla, "ScyllaStartGui");
    if(!ScyllaStartGui)
    {
        dputs("Could not find export 'ScyllaStartGui' inside Scylla.dll");
        bScyllaLoaded = false;
        FreeLibrary(hScylla);
        return 0;
    }
    auto cip = GetContextDataEx(fdProcessInfo->hThread, UE_CIP);
    auto cipModBase = ModBaseFromAddr(cip);
    ScyllaStartGui(fdProcessInfo->dwProcessId, (HINSTANCE)cipModBase, cip);
    FreeLibrary(hScylla);
    bScyllaLoaded = false;
    return 0;
}

CMDRESULT cbDebugStartScylla(int argc, char* argv[])
{
    if(bScyllaLoaded)
    {
        dputs("Scylla is already loaded");
        return STATUS_ERROR;
    }
    bScyllaLoaded = true;
    CloseHandle(CreateThread(0, 0, scyllaThread, 0, 0, 0));
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugAttach(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments!");
        return STATUS_ERROR;
    }
    duint pid = 0;
    if(!valfromstring(argv[1], &pid, false))
        return STATUS_ERROR;
    if(argc > 2)
    {
        duint eventHandle = 0;
        if(!valfromstring(argv[2], &eventHandle, false))
            return STATUS_ERROR;
        dbgsetattachevent((HANDLE)eventHandle);
    }
    if(DbgIsDebugging())
        DbgCmdExecDirect("stop");
    Handle hProcess = TitanOpenProcess(PROCESS_ALL_ACCESS, false, (DWORD)pid);
    if(!hProcess)
    {
        dprintf("Could not open process %X!\n", pid);
        return STATUS_ERROR;
    }
    BOOL wow64 = false, mewow64 = false;
    if(!IsWow64Process(hProcess, &wow64) || !IsWow64Process(GetCurrentProcess(), &mewow64))
    {
        dputs("IsWow64Process failed!");
        return STATUS_ERROR;
    }
    if((mewow64 && !wow64) || (!mewow64 && wow64))
    {
#ifdef _WIN64
        dputs("Use x32dbg to debug this process!");
#else
        dputs("Use x64dbg to debug this process!");
#endif // _WIN64
        return STATUS_ERROR;
    }
    wchar_t wszFileName[MAX_PATH] = L"";
    if(!GetModuleFileNameExW(hProcess, 0, wszFileName, MAX_PATH))
    {
        dprintf("Could not get module filename %X!\n", pid);
        return STATUS_ERROR;
    }
    strcpy_s(szFileName, StringUtils::Utf16ToUtf8(wszFileName).c_str());
    CloseHandle(CreateThread(0, 0, threadAttachLoop, (void*)pid, 0, 0));
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDetach(int argc, char* argv[])
{
    unlock(WAITID_RUN); //run
    dbgsetisdetachedbyuser(true); //detach when paused
    StepInto((void*)cbDetach);
    DebugBreakProcess(fdProcessInfo->hProcess);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDump(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr))
    {
        dprintf("Invalid address \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    if(argc > 2)
    {
        duint index = 0;
        if(!valfromstring(argv[2], &index))
        {
            dprintf("Invalid address \"%s\"!\n", argv[2]);
            return STATUS_ERROR;
        }
        GuiDumpAtN(addr, int(index));
    }
    else
        GuiDumpAt(addr);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugStackDump(int argc, char* argv[])
{
    duint addr = 0;
    if(argc < 2)
        addr = GetContextDataEx(hActiveThread, UE_CSP);
    else if(!valfromstring(argv[1], &addr))
    {
        dprintf("Invalid address \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    duint csp = GetContextDataEx(hActiveThread, UE_CSP);
    duint size = 0;
    duint base = MemFindBaseAddr(csp, &size);
    if(base && addr >= base && addr < (base + size))
        DebugUpdateStack(addr, csp, true);
    else
        dputs("Invalid stack address!");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugContinue(int argc, char* argv[])
{
    if(argc < 2)
    {
        SetNextDbgContinueStatus(DBG_CONTINUE);
        dputs("Exception will be swallowed");
    }
    else
    {
        SetNextDbgContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
        dputs("Exception will be thrown in the program");
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugBpDll(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments!");
        return STATUS_ERROR;
    }
    DWORD type = UE_ON_LIB_ALL;
    if(argc > 2)
    {
        switch(*argv[2])
        {
        case 'l':
            type = UE_ON_LIB_LOAD;
            break;
        case 'u':
            type = UE_ON_LIB_UNLOAD;
            break;
        }
    }
    bool singleshoot = true;
    if(argc > 3)
        singleshoot = false;
    LibrarianSetBreakPoint(argv[1], type, singleshoot, (void*)cbLibrarianBreakpoint);
    dprintf("Dll breakpoint set on \"%s\"!\n", argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugBcDll(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments");
        return STATUS_ERROR;
    }
    if(!LibrarianRemoveBreakPoint(argv[1], UE_ON_LIB_ALL))
    {
        dputs("Failed to remove DLL breakpoint...");
        return STATUS_ERROR;
    }
    dputs("DLL breakpoint removed!");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSwitchthread(int argc, char* argv[])
{
    duint threadid = fdProcessInfo->dwThreadId; //main thread
    if(argc > 1)
        if(!valfromstring(argv[1], &threadid, false))
            return STATUS_ERROR;
    if(!ThreadIsValid((DWORD)threadid)) //check if the thread is valid
    {
        dprintf("Invalid thread %X\n", threadid);
        return STATUS_ERROR;
    }
    //switch thread
    hActiveThread = ThreadGetHandle((DWORD)threadid);
    DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), true);
    dputs("Thread switched!");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSuspendthread(int argc, char* argv[])
{
    duint threadid = fdProcessInfo->dwThreadId;
    if(argc > 1)
        if(!valfromstring(argv[1], &threadid, false))
            return STATUS_ERROR;
    if(!ThreadIsValid((DWORD)threadid)) //check if the thread is valid
    {
        dprintf("Invalid thread %X\n", threadid);
        return STATUS_ERROR;
    }
    //suspend thread
    if(SuspendThread(ThreadGetHandle((DWORD)threadid)) == -1)
    {
        dputs("Error suspending thread");
        return STATUS_ERROR;
    }
    dputs("Thread suspended");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugResumethread(int argc, char* argv[])
{
    duint threadid = fdProcessInfo->dwThreadId;
    if(argc > 1)
        if(!valfromstring(argv[1], &threadid, false))
            return STATUS_ERROR;
    if(!ThreadIsValid((DWORD)threadid)) //check if the thread is valid
    {
        dprintf("Invalid thread %X\n", threadid);
        return STATUS_ERROR;
    }
    //resume thread
    if(ResumeThread(ThreadGetHandle((DWORD)threadid)) == -1)
    {
        dputs("Error resuming thread");
        return STATUS_ERROR;
    }
    dputs("Thread resumed!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugKillthread(int argc, char* argv[])
{
    duint threadid = fdProcessInfo->dwThreadId;
    if(argc > 1)
        if(!valfromstring(argv[1], &threadid, false))
            return STATUS_ERROR;
    duint exitcode = 0;
    if(argc > 2)
        if(!valfromstring(argv[2], &exitcode, false))
            return STATUS_ERROR;
    if(!ThreadIsValid((DWORD)threadid)) //check if the thread is valid
    {
        dprintf("Invalid thread %X\n", threadid);
        return STATUS_ERROR;
    }
    //terminate thread
    if(TerminateThread(ThreadGetHandle((DWORD)threadid), (DWORD)exitcode) != 0)
    {
        GuiUpdateAllViews();
        dputs("Thread terminated");
        return STATUS_CONTINUE;
    }
    dputs("Error terminating thread!");
    return STATUS_ERROR;
}

CMDRESULT cbDebugSuspendAllThreads(int argc, char* argv[])
{
    dprintf("%d/%d thread(s) suspended\n", ThreadSuspendAll(), ThreadGetCount());

    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugResumeAllThreads(int argc, char* argv[])
{
    dprintf("%d/%d thread(s) resumed\n", ThreadResumeAll(), ThreadGetCount());

    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetPriority(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("Not enough arguments!");
        return STATUS_ERROR;
    }
    duint threadid;
    if(!valfromstring(argv[1], &threadid, false))
        return STATUS_ERROR;
    duint priority;
    if(!valfromstring(argv[2], &priority))
    {
        if(_strcmpi(argv[2], "Normal") == 0)
            priority = THREAD_PRIORITY_NORMAL;
        else if(_strcmpi(argv[2], "AboveNormal") == 0)
            priority = THREAD_PRIORITY_ABOVE_NORMAL;
        else if(_strcmpi(argv[2], "TimeCritical") == 0)
            priority = THREAD_PRIORITY_TIME_CRITICAL;
        else if(_strcmpi(argv[2], "Idle") == 0)
            priority = THREAD_PRIORITY_IDLE;
        else if(_strcmpi(argv[2], "BelowNormal") == 0)
            priority = THREAD_PRIORITY_BELOW_NORMAL;
        else if(_strcmpi(argv[2], "Highest") == 0)
            priority = THREAD_PRIORITY_HIGHEST;
        else if(_strcmpi(argv[2], "Lowest") == 0)
            priority = THREAD_PRIORITY_LOWEST;
        else
        {
            dputs("Unknown priority value, read the help!");
            return STATUS_ERROR;
        }
    }
    else
    {
        switch(priority) //check if the priority value is valid
        {
        case THREAD_PRIORITY_NORMAL:
        case THREAD_PRIORITY_ABOVE_NORMAL:
        case THREAD_PRIORITY_TIME_CRITICAL:
        case THREAD_PRIORITY_IDLE:
        case THREAD_PRIORITY_BELOW_NORMAL:
        case THREAD_PRIORITY_HIGHEST:
        case THREAD_PRIORITY_LOWEST:
            break;
        default:
            dputs("Unknown priority value, read the help!");
            return STATUS_ERROR;
        }
    }
    if(!ThreadIsValid((DWORD)threadid)) //check if the thread is valid
    {
        dprintf("Invalid thread %X\n", threadid);
        return STATUS_ERROR;
    }
    //set thread priority
    if(SetThreadPriority(ThreadGetHandle((DWORD)threadid), (int)priority) == 0)
    {
        dputs("Error setting thread priority");
        return STATUS_ERROR;
    }
    dputs("Thread priority changed!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDownloadSymbol(int argc, char* argv[])
{
    dputs("This may take very long, depending on your network connection and data in the debug directory...");
    char szDefaultStore[MAX_SETTING_SIZE] = "";
    const char* szSymbolStore = szDefaultStore;
    if(!BridgeSettingGet("Symbols", "DefaultStore", szDefaultStore)) //get default symbol store from settings
    {
        strcpy_s(szDefaultStore, "http://msdl.microsoft.com/download/symbols");
        BridgeSettingSet("Symbols", "DefaultStore", szDefaultStore);
    }
    if(argc < 2) //no arguments
    {
        SymDownloadAllSymbols(szSymbolStore); //download symbols for all modules
        GuiSymbolRefreshCurrent();
        dputs("Done! See symbol log for more information");
        return STATUS_CONTINUE;
    }
    //get some module information
    duint modbase = ModBaseFromName(argv[1]);
    if(!modbase)
    {
        dprintf("Invalid module \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    wchar_t wszModulePath[MAX_PATH] = L"";
    if(!GetModuleFileNameExW(fdProcessInfo->hProcess, (HMODULE)modbase, wszModulePath, MAX_PATH))
    {
        dputs("GetModuleFileNameExA failed!");
        return STATUS_ERROR;
    }
    wchar_t szOldSearchPath[MAX_PATH] = L"";
    if(!SafeSymGetSearchPathW(fdProcessInfo->hProcess, szOldSearchPath, MAX_PATH)) //backup current search path
    {
        dputs("SymGetSearchPath failed!");
        return STATUS_ERROR;
    }
    char szServerSearchPath[MAX_PATH * 2] = "";
    if(argc > 2)
        szSymbolStore = argv[2];
    sprintf_s(szServerSearchPath, "SRV*%s*%s", szSymbolCachePath, szSymbolStore);
    if(!SafeSymSetSearchPathW(fdProcessInfo->hProcess, StringUtils::Utf8ToUtf16(szServerSearchPath).c_str())) //set new search path
    {
        dputs("SymSetSearchPath (1) failed!");
        return STATUS_ERROR;
    }
    if(!SafeSymUnloadModule64(fdProcessInfo->hProcess, (DWORD64)modbase)) //unload module
    {
        SafeSymSetSearchPathW(fdProcessInfo->hProcess, szOldSearchPath);
        dputs("SymUnloadModule64 failed!");
        return STATUS_ERROR;
    }
    auto symOptions = SafeSymGetOptions();
    SafeSymSetOptions(symOptions & ~SYMOPT_IGNORE_CVREC);
    if(!SafeSymLoadModuleExW(fdProcessInfo->hProcess, 0, wszModulePath, 0, (DWORD64)modbase, 0, 0, 0)) //load module
    {
        dputs("SymLoadModuleEx failed!");
        SafeSymSetOptions(symOptions);
        SafeSymSetSearchPathW(fdProcessInfo->hProcess, szOldSearchPath);
        return STATUS_ERROR;
    }
    SafeSymSetOptions(symOptions);
    if(!SafeSymSetSearchPathW(fdProcessInfo->hProcess, szOldSearchPath))
    {
        dputs("SymSetSearchPathW (2) failed!");
        return STATUS_ERROR;
    }
    GuiSymbolRefreshCurrent();
    dputs("Done! See symbol log for more information");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugGetJITAuto(int argc, char* argv[])
{
    bool jit_auto = false;
    arch actual_arch = invalid;

    if(argc == 1)
    {
        if(!dbggetjitauto(&jit_auto, notfound, & actual_arch, NULL))
        {
            dprintf("Error getting JIT auto %s\n", (actual_arch == x64) ? "x64" : "x32");
            return STATUS_ERROR;
        }
    }
    else if(argc == 2)
    {
        readwritejitkey_error_t rw_error;
        if(_strcmpi(argv[1], "x64") == 0)
            actual_arch = x64;
        else if(_strcmpi(argv[1], "x32") == 0)
            actual_arch = x32;
        else
        {
            dputs("Unknown JIT auto entry type. Use x64 or x32 as parameter.");
            return STATUS_ERROR;
        }

        if(!dbggetjitauto(& jit_auto, actual_arch, NULL, & rw_error))
        {
            if(rw_error == ERROR_RW_NOTWOW64)
                dprintf("Error using x64 arg the debugger is not a WOW64 process\n");
            else
                dprintf("Error getting JIT auto %s\n", argv[1]);
            return STATUS_ERROR;
        }
    }
    else
    {
        dputs("Unknown JIT auto entry type. Use x64 or x32 as parameter");
    }

    dprintf("JIT auto %s: %s\n", (actual_arch == x64) ? "x64" : "x32", jit_auto ? "ON" : "OFF");

    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetJITAuto(int argc, char* argv[])
{
    arch actual_arch;
    bool set_jit_auto;
    if(!IsProcessElevated())
    {
        dprintf("Error run the debugger as Admin to setjitauto\n");
        return STATUS_ERROR;
    }
    if(argc < 2)
    {
        dprintf("Error setting JIT Auto. Use ON:1 or OFF:0 arg or x64/x32, ON:1 or OFF:0.\n");
        return STATUS_ERROR;
    }
    else if(argc == 2)
    {
        if(_strcmpi(argv[1], "1") == 0 || _strcmpi(argv[1], "ON") == 0)
            set_jit_auto = true;
        else if(_strcmpi(argv[1], "0") == 0 || _strcmpi(argv[1], "OFF") == 0)
            set_jit_auto = false;
        else
        {
            dputs("Error unknown parameters. Use ON:1 or OFF:0");
            return STATUS_ERROR;
        }

        if(!dbgsetjitauto(set_jit_auto, notfound, & actual_arch, NULL))
        {
            dprintf("Error setting JIT auto %s\n", (actual_arch == x64) ? "x64" : "x32");
            return STATUS_ERROR;
        }
    }
    else if(argc == 3)
    {
        readwritejitkey_error_t rw_error;
        actual_arch = x64;

        if(_strcmpi(argv[1], "x64") == 0)
            actual_arch = x64;
        else if(_strcmpi(argv[1], "x32") == 0)
            actual_arch = x32;
        else
        {
            dputs("Unknown JIT auto entry type. Use x64 or x32 as parameter");
            return STATUS_ERROR;
        }

        if(_strcmpi(argv[2], "1") == 0 || _strcmpi(argv[2], "ON") == 0)
            set_jit_auto = true;
        else if(_strcmpi(argv[2], "0") == 0 || _strcmpi(argv[2], "OFF") == 0)
            set_jit_auto = false;
        else
        {
            dputs("Error unknown parameters. Use x86 or x64 and ON:1 or OFF:0\n");
            return STATUS_ERROR;
        }

        if(!dbgsetjitauto(set_jit_auto, actual_arch, NULL, & rw_error))
        {
            if(rw_error == ERROR_RW_NOTWOW64)
                dprintf("Error using x64 arg the debugger is not a WOW64 process\n");
            else

                dprintf("Error getting JIT auto %s\n", (actual_arch == x64) ? "x64" : "x32");
            return STATUS_ERROR;
        }
    }
    else
    {
        dputs("Error unknown parameters use x86 or x64, ON/1 or OFF/0\n");
        return STATUS_ERROR;
    }

    dprintf("New JIT auto %s: %s\n", (actual_arch == x64) ? "x64" : "x32", set_jit_auto ? "ON" : "OFF");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetJIT(int argc, char* argv[])
{
    arch actual_arch = invalid;
    char* jit_debugger_cmd = "";
    char oldjit[MAX_SETTING_SIZE] = "";
    char path[JIT_ENTRY_DEF_SIZE];
    if(!IsProcessElevated())
    {
        dprintf("Error run the debugger as Admin to setjit\n");
        return STATUS_ERROR;
    }
    if(argc < 2)
    {
        dbggetdefjit(path);

        jit_debugger_cmd = path;
        if(!dbgsetjit(jit_debugger_cmd, notfound, & actual_arch, NULL))
        {
            dprintf("Error setting JIT %s\n", (actual_arch == x64) ? "x64" : "x32");
            return STATUS_ERROR;
        }
    }
    else if(argc == 2)
    {
        if(!_strcmpi(argv[1], "old"))
        {
            jit_debugger_cmd = oldjit;
            if(!BridgeSettingGet("JIT", "Old", jit_debugger_cmd))
            {
                dputs("Error there is no old JIT entry stored.");
                return STATUS_ERROR;
            }

            if(!dbgsetjit(jit_debugger_cmd, notfound, & actual_arch, NULL))
            {
                dprintf("Error setting JIT %s\n", (actual_arch == x64) ? "x64" : "x32");
                return STATUS_ERROR;
            }
        }
        else if(!_strcmpi(argv[1], "oldsave"))
        {
            char path[JIT_ENTRY_DEF_SIZE];
            dbggetdefjit(path);
            char get_entry[JIT_ENTRY_MAX_SIZE] = "";
            bool get_last_jit = true;

            if(!dbggetjit(get_entry, notfound, & actual_arch, NULL))
                get_last_jit = false;
            else
                strcpy_s(oldjit, get_entry);

            jit_debugger_cmd = path;
            if(!dbgsetjit(jit_debugger_cmd, notfound, & actual_arch, NULL))
            {
                dprintf("Error setting JIT %s\n", (actual_arch == x64) ? "x64" : "x32");
                return STATUS_ERROR;
            }
            if(get_last_jit)
            {
                if(_stricmp(oldjit, path))
                    BridgeSettingSet("JIT", "Old", oldjit);
            }
        }
        else if(!_strcmpi(argv[1], "restore"))
        {
            jit_debugger_cmd = oldjit;

            if(!BridgeSettingGet("JIT", "Old", jit_debugger_cmd))
            {
                dputs("Error there is no old JIT entry stored.");
                return STATUS_ERROR;
            }

            if(!dbgsetjit(jit_debugger_cmd, notfound, & actual_arch, NULL))
            {
                dprintf("Error setting JIT %s\n", (actual_arch == x64) ? "x64" : "x32");
                return STATUS_ERROR;
            }
            BridgeSettingSet("JIT", 0, 0);
        }
        else
        {
            jit_debugger_cmd = argv[1];
            if(!dbgsetjit(jit_debugger_cmd, notfound, & actual_arch, NULL))
            {
                dprintf("Error setting JIT %s\n", (actual_arch == x64) ? "x64" : "x32");
                return STATUS_ERROR;
            }
        }
    }
    else if(argc == 3)
    {
        readwritejitkey_error_t rw_error;

        if(!_strcmpi(argv[1], "old"))
        {
            BridgeSettingSet("JIT", "Old", argv[2]);

            dprintf("New OLD JIT stored: %s\n", argv[2]);

            return STATUS_CONTINUE;
        }

        else if(_strcmpi(argv[1], "x64") == 0)
            actual_arch = x64;
        else if(_strcmpi(argv[1], "x32") == 0)
            actual_arch = x32;
        else
        {
            dputs("Unknown JIT entry type. Use OLD, x64 or x32 as parameter.");
            return STATUS_ERROR;
        }

        jit_debugger_cmd = argv[2];
        if(!dbgsetjit(jit_debugger_cmd, actual_arch, NULL, & rw_error))
        {
            if(rw_error == ERROR_RW_NOTWOW64)
                dprintf("Error using x64 arg. The debugger is not a WOW64 process\n");
            else
                dprintf("Error setting JIT %s\n", (actual_arch == x64) ? "x64" : "x32");
            return STATUS_ERROR;
        }
    }
    else
    {
        dputs("Error unknown parameters. Use old, oldsave, restore, x86 or x64 as parameter.");
        return STATUS_ERROR;
    }

    dprintf("New JIT %s: %s\n", (actual_arch == x64) ? "x64" : "x32", jit_debugger_cmd);

    return STATUS_CONTINUE;
}

CMDRESULT cbDebugGetJIT(int argc, char* argv[])
{
    char get_entry[JIT_ENTRY_MAX_SIZE] = "";
    arch actual_arch;

    if(argc < 2)
    {
        if(!dbggetjit(get_entry, notfound, & actual_arch, NULL))
        {
            dprintf("Error getting JIT %s\n", (actual_arch == x64) ? "x64" : "x32");
            return STATUS_ERROR;
        }
    }
    else
    {
        readwritejitkey_error_t rw_error;
        char oldjit[MAX_SETTING_SIZE] = "";
        if(_strcmpi(argv[1], "OLD") == 0)
        {
            if(!BridgeSettingGet("JIT", "Old", (char*) & oldjit))
            {
                dputs("Error: there is not an OLD JIT entry stored yet.");
                return STATUS_ERROR;
            }
            else
            {
                dprintf("OLD JIT entry stored: %s\n", oldjit);
                return STATUS_CONTINUE;
            }
        }
        else if(_strcmpi(argv[1], "x64") == 0)
            actual_arch = x64;
        else if(_strcmpi(argv[1], "x32") == 0)
            actual_arch = x32;
        else
        {
            dputs("Unknown JIT entry type. Use OLD, x64 or x32 as parameter.");
            return STATUS_ERROR;
        }

        if(!dbggetjit(get_entry, actual_arch, NULL, & rw_error))
        {
            if(rw_error == ERROR_RW_NOTWOW64)
                dprintf("Error using x64 arg. The debugger is not a WOW64 process\n");
            else
                dprintf("Error getting JIT %s\n", argv[1]);
            return STATUS_ERROR;
        }
    }

    dprintf("JIT %s: %s\n", (actual_arch == x64) ? "x64" : "x32", get_entry);

    return STATUS_CONTINUE;
}

CMDRESULT cbDebugGetPageRights(int argc, char* argv[])
{
    duint addr = 0;
    char rights[RIGHTS_STRING_SIZE];

    if(argc != 2 || !valfromstring(argv[1], &addr))
    {
        dprintf("Error: using an address as arg1\n");
        return STATUS_ERROR;
    }

    if(!MemGetPageRights(addr, rights))
    {
        dprintf("Error getting rights of page: %s\n", argv[1]);
        return STATUS_ERROR;
    }

    dprintf("Page: " fhex ", Rights: %s\n", addr, rights);

    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetPageRights(int argc, char* argv[])
{
    duint addr = 0;
    char rights[RIGHTS_STRING_SIZE];

    if(argc < 3 || !valfromstring(argv[1], &addr))
    {
        dprintf("Error: Using an address as arg1 and as arg2: Execute, ExecuteRead, ExecuteReadWrite, ExecuteWriteCopy, NoAccess, ReadOnly, ReadWrite, WriteCopy. You can add a G at first for add PAGE GUARD, example: GReadOnly\n");
        return STATUS_ERROR;
    }

    if(!MemSetPageRights(addr, argv[2]))
    {
        dprintf("Error: Set rights of " fhex " with Rights: %s\n", addr, argv[2]);
        return STATUS_ERROR;
    }

    if(!MemGetPageRights(addr, rights))
    {
        dprintf("Error getting rights of page: %s\n", argv[1]);
        return STATUS_ERROR;
    }

    //update the memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    dprintf("New rights of " fhex ": %s\n", addr, rights);

    return STATUS_CONTINUE;
}

CMDRESULT cbDebugLoadLib(int argc, char* argv[])
{
    if(argc < 2)
    {
        dprintf("Error: you must specify the name of the DLL to load\n");
        return STATUS_ERROR;
    }

    LoadLibThreadID = fdProcessInfo->dwThreadId;
    HANDLE LoadLibThread = ThreadGetHandle((DWORD)LoadLibThreadID);

    DLLNameMem = MemAllocRemote(0, strlen(argv[1]) + 1);
    ASMAddr = MemAllocRemote(0, 0x1000);

    if(!DLLNameMem || !ASMAddr)
    {
        dprintf("Error: couldn't allocate memory in debuggee");
        return STATUS_ERROR;
    }

    if(!MemWrite(DLLNameMem, argv[1],  strlen(argv[1])))
    {
        dprintf("Error: couldn't write process memory");
        return STATUS_ERROR;
    }

    int size = 0;
    int counter = 0;
    duint LoadLibraryA = 0;
    char command[50] = "";
    char error[MAX_ERROR_SIZE] = "";

    GetFullContextDataEx(LoadLibThread, &backupctx);

    if(!valfromstring("kernel32:LoadLibraryA", &LoadLibraryA, false))
    {
        dprintf("Error: couldn't get kernel32:LoadLibraryA");
        return STATUS_ERROR;
    }

    // Arch specific asm code
#ifdef _WIN64
    sprintf(command, "mov rcx, " fhex, (duint)DLLNameMem);
#else
    sprintf(command, "push " fhex, DLLNameMem);
#endif // _WIN64

    assembleat((duint)ASMAddr, command, &size, error, true);
    counter += size;

#ifdef _WIN64
    sprintf(command, "mov rax, " fhex, LoadLibraryA);
    assembleat((duint)ASMAddr + counter, command, &size, error, true);
    counter += size;
    sprintf(command, "call rax");
#else
    sprintf(command, "call " fhex, LoadLibraryA);
#endif // _WIN64

    assembleat((duint)ASMAddr + counter, command, &size, error, true);
    counter += size;

    SetContextDataEx(LoadLibThread, UE_CIP, (duint)ASMAddr);
    auto ok = SetBPX((duint)ASMAddr + counter, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, (void*)cbDebugLoadLibBPX);

    ThreadSuspendAll();
    ResumeThread(LoadLibThread);

    unlock(WAITID_RUN);

    return ok ? STATUS_CONTINUE : STATUS_ERROR;
}

void cbDebugLoadLibBPX()
{
    HANDLE LoadLibThread = ThreadGetHandle((DWORD)LoadLibThreadID);
#ifdef _WIN64
    duint LibAddr = GetContextDataEx(LoadLibThread, UE_RAX);
#else
    duint LibAddr = GetContextDataEx(LoadLibThread, UE_EAX);
#endif //_WIN64
    varset("$result", LibAddr, false);
    backupctx.eflags &= ~0x100;
    SetFullContextDataEx(LoadLibThread, &backupctx);
    MemFreeRemote(DLLNameMem);
    MemFreeRemote(ASMAddr);
    ThreadResumeAll();
    //update GUI
    GuiSetDebugState(paused);
    DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), true);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved = 0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    wait(WAITID_RUN);
}

void showcommandlineerror(cmdline_error_t* cmdline_error)
{
    bool unkown = false;

    switch(cmdline_error->type)
    {
    case CMDL_ERR_ALLOC:
        dprintf("Error allocating memory for cmdline");
        break;
    case CMDL_ERR_CONVERTUNICODE:
        dprintf("Error converting UNICODE cmdline");
        break;
    case CMDL_ERR_READ_PEBBASE:
        dprintf("Error reading PEB base addres");
        break;
    case CMDL_ERR_READ_PROCPARM_CMDLINE:
        dprintf("Error reading PEB -> ProcessParameters -> CommandLine UNICODE_STRING");
        break;
    case CMDL_ERR_READ_PROCPARM_PTR:
        dprintf("Error reading PEB -> ProcessParameters pointer address");
        break;
    case CMDL_ERR_GET_PEB:
        dprintf("Error Getting remote PEB address");
        break;
    case CMDL_ERR_READ_GETCOMMANDLINEBASE:
        dprintf("Error Getting command line base address");
        break;
    case CMDL_ERR_CHECK_GETCOMMANDLINESTORED:
        dprintf("Error checking the pattern of the commandline stored");
        break;
    case CMDL_ERR_WRITE_GETCOMMANDLINESTORED:
        dprintf("Error writing the new command line stored");
        break;
    case CMDL_ERR_GET_GETCOMMANDLINE:
        dprintf("Error getting getcommandline");
        break;
    case CMDL_ERR_ALLOC_UNICODEANSI_COMMANDLINE:
        dprintf("Error allocating the page with UNICODE and ANSI command lines");
        break;
    case CMDL_ERR_WRITE_ANSI_COMMANDLINE:
        dprintf("Error writing the ANSI command line in the page");
        break;
    case CMDL_ERR_WRITE_UNICODE_COMMANDLINE:
        dprintf("Error writing the UNICODE command line in the page");
        break;
    case CMDL_ERR_WRITE_PEBUNICODE_COMMANDLINE:
        dprintf("Error writing command line UNICODE in PEB");
        break;
    default:
        unkown = true;
        dputs("Error getting cmdline");
        break;
    }

    if(!unkown)
    {
        if(cmdline_error->addr != 0)
            dprintf(" (Address: " fhex ")", cmdline_error->addr);
        dputs("");
    }
}

CMDRESULT cbDebugGetCmdline(int argc, char* argv[])
{
    char* cmd_line;
    cmdline_error_t cmdline_error = {(cmdline_error_type_t) 0, 0};

    if(!dbggetcmdline(& cmd_line, & cmdline_error))
    {
        showcommandlineerror(& cmdline_error);
        return STATUS_ERROR;
    }

    dprintf("Command line: %s\n", cmd_line);

    efree(cmd_line);

    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetCmdline(int argc, char* argv[])
{
    cmdline_error_t cmdline_error = {(cmdline_error_type_t) 0, 0};

    if(argc != 2)
    {
        dputs("Error: write the arg1 with the new command line of the process debugged");
        return STATUS_ERROR;
    }

    if(!dbgsetcmdline(argv[1], &cmdline_error))
    {
        showcommandlineerror(&cmdline_error);
        return STATUS_ERROR;
    }

    //update the memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    dprintf("New command line: %s\n", argv[1]);

    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSkip(int argc, char* argv[])
{
    SetNextDbgContinueStatus(DBG_CONTINUE); //swallow the exception
    duint cip = GetContextDataEx(hActiveThread, UE_CIP);
    BASIC_INSTRUCTION_INFO basicinfo;
    memset(&basicinfo, 0, sizeof(basicinfo));
    disasmfast(cip, &basicinfo);
    cip += basicinfo.size;
    SetContextDataEx(hActiveThread, UE_CIP, cip);
    DebugUpdateGui(cip, false); //update GUI
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetfreezestack(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments!");
        return STATUS_ERROR;
    }
    bool freeze = *argv[1] != '0';
    dbgsetfreezestack(freeze);
    dprintf("Stack is now %s\n", freeze ? "freezed" : "unfreezed");
    return STATUS_CONTINUE;
}
