#include "debugger.h"
#include "console.h"
#include "memory.h"
#include "threading.h"
#include "command.h"
#include "addrinfo.h"
#include "thread.h"
#include "plugin_loader.h"
#include "breakpoint.h"
#include "symbolinfo.h"
#include "variable.h"
#include "x64_dbg.h"

static PROCESS_INFORMATION g_pi = {0, 0, 0, 0};
static char szBaseFileName[MAX_PATH] = "";
static bool bFileIsDll = false;
static uint pDebuggedBase = 0;
static uint pDebuggedEntry = 0;
static bool isStepping = false;
static bool isPausedByUser = false;
static bool isDetachedByUser = false;
static bool bIsAttached = false;
static bool bSkipExceptions = false;
static bool bBreakOnNextDll = false;
static int ecount = 0;
static std::vector<ExceptionRange> ignoredExceptionRange;
static std::map<unsigned int, const char*> exceptionNames;
static SIZE_T cachePrivateUsage = 0;
static HANDLE hEvent = 0;

//Superglobal variables
char szFileName[MAX_PATH] = "";
char szSymbolCachePath[MAX_PATH] = "";
char sqlitedb[deflen] = "";
PROCESS_INFORMATION* fdProcessInfo = &g_pi;
HANDLE hActiveThread;

static DWORD WINAPI memMapThread(void* ptr)
{
    while(true)
    {
        while(!DbgIsDebugging())
            Sleep(1);
        const SIZE_T PrivateUsage = dbggetprivateusage(fdProcessInfo->hProcess);
        if(cachePrivateUsage != PrivateUsage && !dbgisrunning()) //update the memory map when
        {
            cachePrivateUsage = PrivateUsage;
            memupdatemap(fdProcessInfo->hProcess);
        }
        Sleep(1000);
    }
    return 0;
}

void dbginit()
{
    exceptionNames.insert(std::make_pair(0x40000005, "STATUS_SEGMENT_NOTIFICATION"));
    exceptionNames.insert(std::make_pair(0x4000001C, "STATUS_WX86_UNSIMULATE"));
    exceptionNames.insert(std::make_pair(0x4000001D, "STATUS_WX86_CONTINUE"));
    exceptionNames.insert(std::make_pair(0x4000001E, "STATUS_WX86_SINGLE_STEP"));
    exceptionNames.insert(std::make_pair(0x4000001F, "STATUS_WX86_BREAKPOINT"));
    exceptionNames.insert(std::make_pair(0x40000020, "STATUS_WX86_EXCEPTION_CONTINUE"));
    exceptionNames.insert(std::make_pair(0x40000021, "STATUS_WX86_EXCEPTION_LASTCHANCE"));
    exceptionNames.insert(std::make_pair(0x40000022, "STATUS_WX86_EXCEPTION_CHAIN"));
    exceptionNames.insert(std::make_pair(0x40000028, "STATUS_WX86_CREATEWX86TIB"));
    exceptionNames.insert(std::make_pair(0x40010003, "DBG_TERMINATE_THREAD"));
    exceptionNames.insert(std::make_pair(0x40010004, "DBG_TERMINATE_PROCESS"));
    exceptionNames.insert(std::make_pair(0x40010005, "DBG_CONTROL_C"));
    exceptionNames.insert(std::make_pair(0x40010006, "DBG_PRINTEXCEPTION_C"));
    exceptionNames.insert(std::make_pair(0x40010007, "DBG_RIPEXCEPTION"));
    exceptionNames.insert(std::make_pair(0x40010008, "DBG_CONTROL_BREAK"));
    exceptionNames.insert(std::make_pair(0x40010009, "DBG_COMMAND_EXCEPTION"));
    exceptionNames.insert(std::make_pair(0x80000001, "EXCEPTION_GUARD_PAGE"));
    exceptionNames.insert(std::make_pair(0x80000002, "EXCEPTION_DATATYPE_MISALIGNMENT"));
    exceptionNames.insert(std::make_pair(0x80000003, "EXCEPTION_BREAKPOINT"));
    exceptionNames.insert(std::make_pair(0x80000004, "EXCEPTION_SINGLE_STEP"));
    exceptionNames.insert(std::make_pair(0x80000026, "STATUS_LONGJUMP"));
    exceptionNames.insert(std::make_pair(0x80000029, "STATUS_UNWIND_CONSOLIDATE"));
    exceptionNames.insert(std::make_pair(0x80010001, "DBG_EXCEPTION_NOT_HANDLED"));
    exceptionNames.insert(std::make_pair(0xC0000005, "EXCEPTION_ACCESS_VIOLATION"));
    exceptionNames.insert(std::make_pair(0xC0000006, "EXCEPTION_IN_PAGE_ERROR"));
    exceptionNames.insert(std::make_pair(0xC0000008, "EXCEPTION_INVALID_HANDLE"));
    exceptionNames.insert(std::make_pair(0xC000000D, "STATUS_INVALID_PARAMETER"));
    exceptionNames.insert(std::make_pair(0xC0000017, "STATUS_NO_MEMORY"));
    exceptionNames.insert(std::make_pair(0xC000001D, "EXCEPTION_ILLEGAL_INSTRUCTION"));
    exceptionNames.insert(std::make_pair(0xC0000025, "EXCEPTION_NONCONTINUABLE_EXCEPTION"));
    exceptionNames.insert(std::make_pair(0xC0000026, "EXCEPTION_INVALID_DISPOSITION"));
    exceptionNames.insert(std::make_pair(0xC000008C, "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"));
    exceptionNames.insert(std::make_pair(0xC000008D, "EXCEPTION_FLT_DENORMAL_OPERAND"));
    exceptionNames.insert(std::make_pair(0xC000008E, "EXCEPTION_FLT_DIVIDE_BY_ZERO"));
    exceptionNames.insert(std::make_pair(0xC000008F, "EXCEPTION_FLT_INEXACT_RESULT"));
    exceptionNames.insert(std::make_pair(0xC0000090, "EXCEPTION_FLT_INVALID_OPERATION"));
    exceptionNames.insert(std::make_pair(0xC0000091, "EXCEPTION_FLT_OVERFLOW"));
    exceptionNames.insert(std::make_pair(0xC0000092, "EXCEPTION_FLT_STACK_CHECK"));
    exceptionNames.insert(std::make_pair(0xC0000093, "EXCEPTION_FLT_UNDERFLOW"));
    exceptionNames.insert(std::make_pair(0xC0000094, "EXCEPTION_INT_DIVIDE_BY_ZERO"));
    exceptionNames.insert(std::make_pair(0xC0000095, "EXCEPTION_INT_OVERFLOW"));
    exceptionNames.insert(std::make_pair(0xC0000096, "EXCEPTION_PRIV_INSTRUCTION"));
    exceptionNames.insert(std::make_pair(0xC00000FD, "EXCEPTION_STACK_OVERFLOW"));
    exceptionNames.insert(std::make_pair(0xC0000135, "STATUS_DLL_NOT_FOUND"));
    exceptionNames.insert(std::make_pair(0xC0000138, "STATUS_ORDINAL_NOT_FOUND"));
    exceptionNames.insert(std::make_pair(0xC0000139, "STATUS_ENTRYPOINT_NOT_FOUND"));
    exceptionNames.insert(std::make_pair(0xC000013A, "STATUS_CONTROL_C_EXIT"));
    exceptionNames.insert(std::make_pair(0xC0000142, "STATUS_DLL_INIT_FAILED"));
    exceptionNames.insert(std::make_pair(0xC000014A, "STATUS_ILLEGAL_FLOAT_CONTEXT"));
    exceptionNames.insert(std::make_pair(0xC0000194, "EXCEPTION_POSSIBLE_DEADLOCK"));
    exceptionNames.insert(std::make_pair(0xC00002B4, "STATUS_FLOAT_MULTIPLE_FAULTS"));
    exceptionNames.insert(std::make_pair(0xC00002B5, "STATUS_FLOAT_MULTIPLE_TRAPS"));
    exceptionNames.insert(std::make_pair(0xC00002C5, "STATUS_DATATYPE_MISALIGNMENT_ERROR"));
    exceptionNames.insert(std::make_pair(0xC00002C9, "STATUS_REG_NAT_CONSUMPTION"));
    exceptionNames.insert(std::make_pair(0xC0000409, "STATUS_STACK_BUFFER_OVERRUN"));
    exceptionNames.insert(std::make_pair(0xC0000417, "STATUS_INVALID_CRUNTIME_PARAMETER"));
    exceptionNames.insert(std::make_pair(0xC0000420, "STATUS_ASSERTION_FAILURE"));
    exceptionNames.insert(std::make_pair(0x04242420, "CLRDBG_NOTIFICATION_EXCEPTION_CODE"));
    exceptionNames.insert(std::make_pair(0xE0434352, "CLR_EXCEPTION"));
    exceptionNames.insert(std::make_pair(0xE06D7363, "CPP_EH_EXCEPTION"));
    CloseHandle(CreateThread(0, 0, memMapThread, 0, 0, 0));
}

SIZE_T dbggetprivateusage(HANDLE hProcess, bool update)
{
    PROCESS_MEMORY_COUNTERS_EX memoryCounters;
    memoryCounters.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);
    if(!GetProcessMemoryInfo(fdProcessInfo->hProcess, (PPROCESS_MEMORY_COUNTERS)&memoryCounters, sizeof(PROCESS_MEMORY_COUNTERS_EX)))
        return 0;
    if(update)
        cachePrivateUsage = memoryCounters.PrivateUsage;
    return memoryCounters.PrivateUsage;
}

uint dbgdebuggedbase()
{
    return pDebuggedBase;
}

void dbgdisablebpx()
{
    std::vector<BREAKPOINT> list;
    int bpcount = bpgetlist(&list);
    for(int i = 0; i < bpcount; i++)
    {
        if(list[i].type == BPNORMAL and IsBPXEnabled(list[i].addr))
            DeleteBPX(list[i].addr);
    }
}

void dbgenablebpx()
{
    std::vector<BREAKPOINT> list;
    int bpcount = bpgetlist(&list);
    for(int i = 0; i < bpcount; i++)
    {
        if(list[i].type == BPNORMAL and !IsBPXEnabled(list[i].addr) and list[i].enabled)
            SetBPX(list[i].addr, list[i].titantype, (void*)cbUserBreakpoint);
    }
}

bool dbgisrunning()
{
    if(!waitislocked(WAITID_RUN))
        return true;
    return false;
}

bool dbgisdll()
{
    return bFileIsDll;
}

void dbgsetattachevent(HANDLE handle)
{
    hEvent = handle;
}

void dbgsetskipexceptions(bool skip)
{
    bSkipExceptions = skip;
}

void dbgsetstepping(bool stepping)
{
    isStepping = stepping;
}

void dbgsetispausedbyuser(bool b)
{
    isPausedByUser = b;
}

void dbgsetisdetachedbyuser(bool b)
{
    isDetachedByUser = b;
}

void dbgclearignoredexceptions()
{
    std::vector<ExceptionRange>().swap(ignoredExceptionRange);
}

void dbgaddignoredexception(ExceptionRange range)
{
    ignoredExceptionRange.push_back(range);
}

bool dbgisignoredexception(unsigned int exception)
{
    for(unsigned int i = 0; i < ignoredExceptionRange.size(); i++)
    {
        unsigned int curStart = ignoredExceptionRange.at(i).start;
        unsigned int curEnd = ignoredExceptionRange.at(i).end;
        if(exception >= curStart && exception <= curEnd)
            return true;
    }
    return false;
}

bool dbgcmdnew(const char* name, CBCOMMAND cbCommand, bool debugonly)
{
    if(!cmdnew(dbggetcommandlist(), name, cbCommand, debugonly))
        return false;
    GuiAutoCompleteAddCmd(name);
    return true;
}

bool dbgcmddel(const char* name)
{
    if(!cmddel(dbggetcommandlist(), name))
        return false;
    GuiAutoCompleteDelCmd(name);
    return true;
}

DWORD WINAPI updateCallStackThread(void* ptr)
{
    GuiUpdateCallStack();
    return 0;
}

void DebugUpdateGui(uint disasm_addr, bool stack)
{
    uint cip = GetContextDataEx(hActiveThread, UE_CIP);
    if(memisvalidreadptr(fdProcessInfo->hProcess, disasm_addr))
        GuiDisasmAt(disasm_addr, cip);
    uint csp = GetContextDataEx(hActiveThread, UE_CSP);
    if(stack)
        GuiStackDumpAt(csp, csp);
    static uint cacheCsp = 0;
    if(csp != cacheCsp)
    {
        cacheCsp = csp;
        CloseHandle(CreateThread(0, 0, updateCallStackThread, 0, 0, 0));
    }
    char modname[MAX_MODULE_SIZE] = "";
    char modtext[MAX_MODULE_SIZE * 2] = "";
    if(!modnamefromaddr(disasm_addr, modname, true))
        *modname = 0;
    else
        sprintf(modtext, "Module: %s - ", modname);
    char title[1024] = "";
    sprintf(title, "File: %s - PID: %X - %sThread: %X", szBaseFileName, fdProcessInfo->dwProcessId, modtext, threadgetid(hActiveThread));
    GuiUpdateWindowTitle(title);
    GuiUpdateAllViews();
}

void cbUserBreakpoint()
{
    hActiveThread = threadgethandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    BREAKPOINT bp;
    BRIDGEBP pluginBp;
    PLUG_CB_BREAKPOINT bpInfo;
    bpInfo.breakpoint = 0;
    if(!bpget(GetContextDataEx(hActiveThread, UE_CIP), BPNORMAL, 0, &bp) and bp.enabled)
        dputs("breakpoint reached not in list!");
    else
    {
        const char* bptype = "INT3";
        int titantype = bp.titantype;
        if((titantype & UE_BREAKPOINT_TYPE_UD2) == UE_BREAKPOINT_TYPE_UD2)
            bptype = "UD2";
        else if((titantype & UE_BREAKPOINT_TYPE_LONG_INT3) == UE_BREAKPOINT_TYPE_LONG_INT3)
            bptype = "LONG INT3";
        const char* symbolicname = symgetsymbolicname(bp.addr);
        if(symbolicname)
        {
            if(*bp.name)
                dprintf("%s breakpoint \"%s\" at %s ("fhex")!\n", bptype, bp.name, symbolicname, bp.addr);
            else
                dprintf("%s breakpoint at %s ("fhex")!\n", bptype, symbolicname, bp.addr);
        }
        else
        {
            if(*bp.name)
                dprintf("%s breakpoint \"%s\" at "fhex"!\n", bptype, bp.name, bp.addr);
            else
                dprintf("%s breakpoint at "fhex"!\n", bptype, bp.addr);
        }
        if(bp.singleshoot)
            bpdel(bp.addr, BPNORMAL);
        bptobridge(&bp, &pluginBp);
        bpInfo.breakpoint = &pluginBp;
    }
    GuiSetDebugState(paused);
    DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), true);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions = false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved = 0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    plugincbcall(CB_BREAKPOINT, &bpInfo);
    wait(WAITID_RUN);
}

void cbHardwareBreakpoint(void* ExceptionAddress)
{
    hActiveThread = threadgethandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    uint cip = GetContextDataEx(hActiveThread, UE_CIP);
    BREAKPOINT bp;
    BRIDGEBP pluginBp;
    PLUG_CB_BREAKPOINT bpInfo;
    bpInfo.breakpoint = 0;
    if(!bpget((uint)ExceptionAddress, BPHARDWARE, 0, &bp))
        dputs("hardware breakpoint reached not in list!");
    else
    {
        const char* bpsize = "";
        switch(TITANGETSIZE(bp.titantype)) //size
        {
        case UE_HARDWARE_SIZE_1:
            bpsize = "byte, ";
            break;
        case UE_HARDWARE_SIZE_2:
            bpsize = "word, ";
            break;
        case UE_HARDWARE_SIZE_4:
            bpsize = "dword, ";
            break;
#ifdef _WIN64
        case UE_HARDWARE_SIZE_8:
            bpsize = "qword, ";
            break;
#endif //_WIN64
        }
        const char* bptype = "";
        switch(TITANGETTYPE(bp.titantype)) //type
        {
        case UE_HARDWARE_EXECUTE:
            bptype = "execute";
            bpsize = "";
            break;
        case UE_HARDWARE_READWRITE:
            bptype = "read/write";
            break;
        case UE_HARDWARE_WRITE:
            bptype = "write";
            break;
        }
        const char* symbolicname = symgetsymbolicname(bp.addr);
        if(symbolicname)
        {
            if(*bp.name)
                dprintf("hardware breakpoint (%s%s) \"%s\" at %s ("fhex")!\n", bpsize, bptype, bp.name, symbolicname, bp.addr);
            else
                dprintf("hardware breakpoint (%s%s) at %s ("fhex")!\n", bpsize, bptype, symbolicname, bp.addr);
        }
        else
        {
            if(*bp.name)
                dprintf("hardware breakpoint (%s%s) \"%s\" at "fhex"!\n", bpsize, bptype, bp.name, bp.addr);
            else
                dprintf("hardware breakpoint (%s%s) at "fhex"!\n", bpsize, bptype, bp.addr);
        }
        bptobridge(&bp, &pluginBp);
        bpInfo.breakpoint = &pluginBp;
    }
    GuiSetDebugState(paused);
    DebugUpdateGui(cip, true);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions = false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved = 0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    plugincbcall(CB_BREAKPOINT, &bpInfo);
    wait(WAITID_RUN);
}

void cbMemoryBreakpoint(void* ExceptionAddress)
{
    hActiveThread = threadgethandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    uint cip = GetContextDataEx(hActiveThread, UE_CIP);
    uint size;
    uint base = memfindbaseaddr((uint)ExceptionAddress, &size, true);
    BREAKPOINT bp;
    BRIDGEBP pluginBp;
    PLUG_CB_BREAKPOINT bpInfo;
    bpInfo.breakpoint = 0;
    if(!bpget(base, BPMEMORY, 0, &bp))
        dputs("memory breakpoint reached not in list!");
    else
    {
        const char* bptype = "";
        switch(bp.titantype)
        {
        case UE_MEMORY_READ:
            bptype = " (read)";
            break;
        case UE_MEMORY_WRITE:
            bptype = " (write)";
            break;
        case UE_MEMORY_EXECUTE:
            bptype = " (execute)";
            break;
        case UE_MEMORY:
            bptype = " (read/write/execute)";
            break;
        }
        const char* symbolicname = symgetsymbolicname(bp.addr);
        if(symbolicname)
        {
            if(*bp.name)
                dprintf("memory breakpoint%s \"%s\" at %s ("fhex", "fhex")!\n", bptype, bp.name, symbolicname, bp.addr, ExceptionAddress);
            else
                dprintf("memory breakpoint%s at %s ("fhex", "fhex")!\n", bptype, symbolicname, bp.addr, ExceptionAddress);
        }
        else
        {
            if(*bp.name)
                dprintf("memory breakpoint%s \"%s\" at "fhex" ("fhex")!\n", bptype, bp.name, bp.addr, ExceptionAddress);
            else
                dprintf("memory breakpoint%s at "fhex" ("fhex")!\n", bptype, bp.addr, ExceptionAddress);
        }
        bptobridge(&bp, &pluginBp);
        bpInfo.breakpoint = &pluginBp;
    }
    if(bp.singleshoot)
        bpdel(bp.addr, BPMEMORY); //delete from breakpoint list
    GuiSetDebugState(paused);
    DebugUpdateGui(cip, true);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions = false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved = 0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    plugincbcall(CB_BREAKPOINT, &bpInfo);
    wait(WAITID_RUN);
}

void cbLibrarianBreakpoint(void* lpData)
{
    bBreakOnNextDll = true;
}

static BOOL CALLBACK SymRegisterCallbackProc64(HANDLE hProcess, ULONG ActionCode, ULONG64 CallbackData, ULONG64 UserContext)
{
    //TODO: utf8
    UNREFERENCED_PARAMETER(hProcess);
    UNREFERENCED_PARAMETER(UserContext);
    PIMAGEHLP_CBA_EVENT evt;
    switch(ActionCode)
    {
    case CBA_EVENT:
    {
        evt = (PIMAGEHLP_CBA_EVENT)CallbackData;
        const char* text = (const char*)evt->desc;
        if(strstr(text, "Successfully received a response from the server."))
            break;
        if(strstr(text, "Waiting for the server to respond to a request."))
            break;
        int len = (int)strlen(text);
        bool suspress = false;
        for(int i = 0; i < len; i++)
            if(text[i] == 0x08)
            {
                suspress = true;
                break;
            }
        int percent = 0;
        static bool zerobar = false;
        if(zerobar)
        {
            zerobar = false;
            GuiSymbolSetProgress(0);
        }
        if(strstr(text, " bytes -  "))
        {
            Memory<char*> newtext(len + 1, "SymRegisterCallbackProc64:newtext");
            strcpy(newtext, text);
            strstr(newtext, " bytes -  ")[8] = 0;
            GuiSymbolLogAdd(newtext);
            suspress = true;
        }
        else if(strstr(text, " copied         "))
        {
            GuiSymbolSetProgress(100);
            GuiSymbolLogAdd(" downloaded!\n");
            suspress = true;
            zerobar = true;
        }
        else if(sscanf(text, "%*s %d percent", &percent) == 1 or sscanf(text, "%d percent", &percent) == 1)
        {
            GuiSymbolSetProgress(percent);
            suspress = true;
        }

        if(!suspress)
            GuiSymbolLogAdd(text);
    }
    break;

    case CBA_DEBUG_INFO:
    {
        GuiSymbolLogAdd((const char*)CallbackData);
    }
    break;

    default:
    {
        return FALSE;
    }
    }
    return TRUE;
}

static bool cbSetModuleBreakpoints(const BREAKPOINT* bp)
{
    if(!bp->enabled)
        return true;
    switch(bp->type)
    {
    case BPNORMAL:
    {
        if(!SetBPX(bp->addr, bp->titantype, (void*)cbUserBreakpoint))
            dprintf("could not set breakpoint "fhex"!\n", bp->addr);
    }
    break;

    case BPMEMORY:
    {
        uint size = 0;
        memfindbaseaddr(bp->addr, &size);
        if(!SetMemoryBPXEx(bp->addr, size, bp->titantype, !bp->singleshoot, (void*)cbMemoryBreakpoint))
            dprintf("could not set memory breakpoint "fhex"!\n", bp->addr);
    }
    break;

    case BPHARDWARE:
    {
        DWORD drx = 0;
        if(!GetUnusedHardwareBreakPointRegister(&drx))
        {
            dputs("you can only set 4 hardware breakpoints");
            return false;
        }
        int titantype = bp->titantype;
        TITANSETDRX(titantype, drx);
        bpsettitantype(bp->addr, BPHARDWARE, titantype);
        if(!SetHardwareBreakPoint(bp->addr, drx, TITANGETTYPE(bp->titantype), TITANGETSIZE(bp->titantype), (void*)cbHardwareBreakpoint))
            dprintf("could not set hardware breakpoint "fhex"!\n", bp->addr);
    }
    break;

    default:
        break;
    }
    return true;
}

static bool cbRemoveModuleBreakpoints(const BREAKPOINT* bp)
{
    //TODO: more breakpoint types
    switch(bp->type)
    {
    case BPNORMAL:
        if(IsBPXEnabled(bp->addr))
            DeleteBPX(bp->addr);
        break;
    case BPMEMORY:
        if(bp->enabled)
            RemoveMemoryBPX(bp->addr, 0);
        break;
    case BPHARDWARE:
        if(bp->enabled)
            DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype));
        break;
    default:
        break;
    }
    return true;
}

void cbStep()
{
    hActiveThread = threadgethandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    isStepping = false;
    GuiSetDebugState(paused);
    DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), true);
    PLUG_CB_STEPPED stepInfo;
    stepInfo.reserved = 0;
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions = false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved = 0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    plugincbcall(CB_STEPPED, &stepInfo);
    wait(WAITID_RUN);
}

static void cbRtrFinalStep()
{
    hActiveThread = threadgethandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    GuiSetDebugState(paused);
    DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), true);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions = false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved = 0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    wait(WAITID_RUN);
}

static unsigned char getCIPch()
{
    unsigned char ch = 0x90;
    uint cip = GetContextDataEx(hActiveThread, UE_CIP);
    memread(fdProcessInfo->hProcess, (void*)cip, &ch, 1, 0);
    return ch;
}

void cbRtrStep()
{
    unsigned int cipch = getCIPch();
    if(cipch == 0xC3 or cipch == 0xC2)
        cbRtrFinalStep();
    else
        StepOver((void*)cbRtrStep);
}

///custom handlers
static void cbCreateProcess(CREATE_PROCESS_DEBUG_INFO* CreateProcessInfo)
{
    //TODO: utf8
    void* base = CreateProcessInfo->lpBaseOfImage;
    char DebugFileName[deflen] = "";
    if(!GetFileNameFromHandle(CreateProcessInfo->hFile, DebugFileName))
    {
        if(!DevicePathFromFileHandleA(CreateProcessInfo->hFile, DebugFileName, deflen))
            strcpy(DebugFileName, "??? (GetFileNameFromHandle failed!)");
    }
    dprintf("Process Started: "fhex" %s\n", base, DebugFileName);

    //init program database
    int len = (int)strlen(szFileName);
    while(szFileName[len] != '\\' && len != 0)
        len--;
    if(len)
        len++;
    strcpy_s(sqlitedb, szFileName + len);
#ifdef _WIN64
    strcat_s(sqlitedb, ".dd64");
#else
    strcat_s(sqlitedb, ".dd32");
#endif // _WIN64
    sprintf(dbpath, "%s\\%s", dbbasepath, sqlitedb);
    dprintf("Database file: %s\n", dbpath);
    dbload();
    SymSetOptions(SYMOPT_DEBUG | SYMOPT_LOAD_LINES | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | SYMOPT_FAVOR_COMPRESSED | SYMOPT_IGNORE_NT_SYMPATH);
    GuiSymbolLogClear();
    char szServerSearchPath[MAX_PATH * 2] = "";
    sprintf_s(szServerSearchPath, "SRV*%s", szSymbolCachePath);
    SymInitialize(fdProcessInfo->hProcess, szServerSearchPath, false); //initialize symbols
    SymRegisterCallback64(fdProcessInfo->hProcess, SymRegisterCallbackProc64, 0);
    SymLoadModuleEx(fdProcessInfo->hProcess, CreateProcessInfo->hFile, DebugFileName, 0, (DWORD64)base, 0, 0, 0);
    IMAGEHLP_MODULE64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct = sizeof(modInfo);
    if(SymGetModuleInfo64(fdProcessInfo->hProcess, (DWORD64)base, &modInfo))
        modload((uint)base, modInfo.ImageSize, modInfo.ImageName);
    dbggetprivateusage(fdProcessInfo->hProcess, true);
    memupdatemap(fdProcessInfo->hProcess); //update memory map
    char modname[256] = "";
    if(modnamefromaddr((uint)base, modname, true))
        bpenumall(cbSetModuleBreakpoints, modname);
    GuiUpdateBreakpointsView();
    if(!bFileIsDll and !bIsAttached) //Set entry breakpoint
    {
        pDebuggedBase = (uint)CreateProcessInfo->lpBaseOfImage; //debugged base = executable
        char command[256] = "";

        if(settingboolget("Events", "TlsCallbacks"))
        {
            DWORD NumberOfCallBacks = 0;
            TLSGrabCallBackData(DebugFileName, 0, &NumberOfCallBacks);
            if(NumberOfCallBacks)
            {
                dprintf("TLS Callbacks: %d\n", NumberOfCallBacks);
                Memory<uint*> TLSCallBacks(NumberOfCallBacks * sizeof(uint), "cbCreateProcess:TLSCallBacks");
                if(!TLSGrabCallBackData(DebugFileName, TLSCallBacks, &NumberOfCallBacks))
                    dputs("failed to get TLS callback addresses!");
                else
                {
                    for(unsigned int i = 0; i < NumberOfCallBacks; i++)
                    {
                        sprintf(command, "bp "fhex",\"TLS Callback %d\",ss", TLSCallBacks[i], i + 1);
                        cmddirectexec(dbggetcommandlist(), command);
                    }
                }
            }
        }

        if(settingboolget("Events", "EntryBreakpoint"))
        {
            sprintf(command, "bp "fhex",\"entry breakpoint\",ss", CreateProcessInfo->lpStartAddress);
            cmddirectexec(dbggetcommandlist(), command);
        }
    }
    GuiUpdateBreakpointsView();

    //call plugin callback
    PLUG_CB_CREATEPROCESS callbackInfo;
    callbackInfo.CreateProcessInfo = CreateProcessInfo;
    callbackInfo.modInfo = &modInfo;
    callbackInfo.DebugFileName = DebugFileName;
    callbackInfo.fdProcessInfo = fdProcessInfo;
    plugincbcall(CB_CREATEPROCESS, &callbackInfo);

    //update thread list
    CREATE_THREAD_DEBUG_INFO threadInfo;
    threadInfo.hThread = CreateProcessInfo->hThread;
    threadInfo.lpStartAddress = CreateProcessInfo->lpStartAddress;
    threadInfo.lpThreadLocalBase = CreateProcessInfo->lpThreadLocalBase;
    threadcreate(&threadInfo);
}

static void cbExitProcess(EXIT_PROCESS_DEBUG_INFO* ExitProcess)
{
    PLUG_CB_EXITPROCESS callbackInfo;
    callbackInfo.ExitProcess = ExitProcess;
    plugincbcall(CB_EXITPROCESS, &callbackInfo);
    //Cleanup
    SymCleanup(fdProcessInfo->hProcess);
}

static void cbCreateThread(CREATE_THREAD_DEBUG_INFO* CreateThread)
{
    threadcreate(CreateThread); //update thread list
    DWORD dwThreadId = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    hActiveThread = threadgethandle(dwThreadId);

    if(settingboolget("Events", "ThreadEntry"))
    {
        char command[256] = "";
        sprintf(command, "bp "fhex",\"Thread %X\",ss", CreateThread->lpStartAddress, dwThreadId);
        cmddirectexec(dbggetcommandlist(), command);
    }

    PLUG_CB_CREATETHREAD callbackInfo;
    callbackInfo.CreateThread = CreateThread;
    callbackInfo.dwThreadId = dwThreadId;
    plugincbcall(CB_CREATETHREAD, &callbackInfo);

    dprintf("Thread %X created\n", dwThreadId);

    if(settingboolget("Events", "ThreadStart"))
    {
        dbggetprivateusage(fdProcessInfo->hProcess, true);
        memupdatemap(fdProcessInfo->hProcess); //update memory map
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
}

static void cbExitThread(EXIT_THREAD_DEBUG_INFO* ExitThread)
{
    hActiveThread = threadgethandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    DWORD dwThreadId = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    PLUG_CB_EXITTHREAD callbackInfo;
    callbackInfo.ExitThread = ExitThread;
    callbackInfo.dwThreadId = dwThreadId;
    plugincbcall(CB_EXITTHREAD, &callbackInfo);
    threadexit(dwThreadId);
    dprintf("Thread %X exit\n", dwThreadId);

    if(settingboolget("Events", "ThreadEnd"))
    {
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
}

static void cbSystemBreakpoint(void* ExceptionData)
{
    hActiveThread = threadgethandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    //log message
    if(bIsAttached)
        dputs("attach breakpoint reached!");
    else
        dputs("system breakpoint reached!");
    bSkipExceptions = false; //we are not skipping first-chance exceptions
    uint cip = GetContextDataEx(hActiveThread, UE_CIP);
    GuiDumpAt(memfindbaseaddr(cip, 0, true)); //dump somewhere

    //plugin callbacks
    PLUG_CB_SYSTEMBREAKPOINT callbackInfo;
    callbackInfo.reserved = 0;
    plugincbcall(CB_SYSTEMBREAKPOINT, &callbackInfo);

    if(bIsAttached ? settingboolget("Events", "AttachBreakpoint") : settingboolget("Events", "SystemBreakpoint"))
    {
        //update GUI
        GuiSetDebugState(paused);
        DebugUpdateGui(cip, true);
        //lock
        lock(WAITID_RUN);
        SetForegroundWindow(GuiGetWindowHandle());
        PLUG_CB_PAUSEDEBUG pauseInfo;
        pauseInfo.reserved = 0;
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        wait(WAITID_RUN);
    }
}

static void cbLoadDll(LOAD_DLL_DEBUG_INFO* LoadDll)
{
    hActiveThread = threadgethandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    void* base = LoadDll->lpBaseOfDll;
    char DLLDebugFileName[deflen] = "";
    if(!GetFileNameFromHandle(LoadDll->hFile, DLLDebugFileName))
    {
        if(!DevicePathFromFileHandleA(LoadDll->hFile, DLLDebugFileName, deflen))
            strcpy(DLLDebugFileName, "??? (GetFileNameFromHandle failed!)");
    }
    SymLoadModuleEx(fdProcessInfo->hProcess, LoadDll->hFile, DLLDebugFileName, 0, (DWORD64)base, 0, 0, 0);
    IMAGEHLP_MODULE64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
    if(SymGetModuleInfo64(fdProcessInfo->hProcess, (DWORD64)base, &modInfo))
        modload((uint)base, modInfo.ImageSize, modInfo.ImageName);
    dbggetprivateusage(fdProcessInfo->hProcess, true);
    memupdatemap(fdProcessInfo->hProcess); //update memory map
    char modname[256] = "";
    if(modnamefromaddr((uint)base, modname, true))
        bpenumall(cbSetModuleBreakpoints, modname);
    GuiUpdateBreakpointsView();
    bool bAlreadySetEntry = false;
    if(bFileIsDll and !_stricmp(DLLDebugFileName, szFileName) and !bIsAttached) //Set entry breakpoint
    {
        pDebuggedBase = (uint)base;
        char command[256] = "";
        if(settingboolget("Events", "EntryBreakpoint"))
        {
            bAlreadySetEntry = true;
            sprintf(command, "bp "fhex",\"entry breakpoint\",ss", pDebuggedBase + pDebuggedEntry);
            cmddirectexec(dbggetcommandlist(), command);
        }
    }
    GuiUpdateBreakpointsView();

    if((bBreakOnNextDll || settingboolget("Events", "DllEntry")) && !bAlreadySetEntry)
    {
        uint oep = GetPE32Data(DLLDebugFileName, 0, UE_OEP);
        if(oep)
        {
            char command[256] = "";
            sprintf(command, "bp "fhex",\"DllMain (%s)\",ss", oep + (uint)base, modname);
            cmddirectexec(dbggetcommandlist(), command);
        }
    }

    dprintf("DLL Loaded: "fhex" %s\n", base, DLLDebugFileName);

    //plugin callback
    PLUG_CB_LOADDLL callbackInfo;
    callbackInfo.LoadDll = LoadDll;
    callbackInfo.modInfo = &modInfo;
    callbackInfo.modname = modname;
    plugincbcall(CB_LOADDLL, &callbackInfo);

    if(bBreakOnNextDll || settingboolget("Events", "DllLoad"))
    {
        bBreakOnNextDll = false;
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
}

static void cbUnloadDll(UNLOAD_DLL_DEBUG_INFO* UnloadDll)
{
    hActiveThread = threadgethandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    PLUG_CB_UNLOADDLL callbackInfo;
    callbackInfo.UnloadDll = UnloadDll;
    plugincbcall(CB_UNLOADDLL, &callbackInfo);

    void* base = UnloadDll->lpBaseOfDll;
    char modname[256] = "???";
    if(modnamefromaddr((uint)base, modname, true))
        bpenumall(cbRemoveModuleBreakpoints, modname);
    GuiUpdateBreakpointsView();
    SymUnloadModule64(fdProcessInfo->hProcess, (DWORD64)base);
    dprintf("DLL Unloaded: "fhex" %s\n", base, modname);

    if(bBreakOnNextDll || settingboolget("Events", "DllUnload"))
    {
        bBreakOnNextDll = false;
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

    modunload((uint)base);
}

static void cbOutputDebugString(OUTPUT_DEBUG_STRING_INFO* DebugString)
{
    //TODO: utf8
    hActiveThread = threadgethandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    PLUG_CB_OUTPUTDEBUGSTRING callbackInfo;
    callbackInfo.DebugString = DebugString;
    plugincbcall(CB_OUTPUTDEBUGSTRING, &callbackInfo);

    if(!DebugString->fUnicode) //ASCII
    {
        Memory<char*> DebugText(DebugString->nDebugStringLength + 1, "cbOutputDebugString:DebugText");
        if(memread(fdProcessInfo->hProcess, DebugString->lpDebugStringData, DebugText, DebugString->nDebugStringLength, 0))
        {
            int len = (int)strlen(DebugText);
            int escape_count = 0;
            for(int i = 0; i < len; i++)
                if(DebugText[i] == '\\' or DebugText[i] == '\"' or !isprint(DebugText[i]))
                    escape_count++;
            Memory<char*> DebugTextEscaped(len + escape_count * 3 + 1, "cbOutputDebugString:DebugTextEscaped");
            for(int i = 0, j = 0; i < len; i++)
            {
                switch(DebugText[i])
                {
                case '\t':
                    j += sprintf(DebugTextEscaped + j, "\\t");
                    break;
                case '\f':
                    j += sprintf(DebugTextEscaped + j, "\\f");
                    break;
                case '\v':
                    j += sprintf(DebugTextEscaped + j, "\\v");
                    break;
                case '\n':
                    j += sprintf(DebugTextEscaped + j, "\\n");
                    break;
                case '\r':
                    j += sprintf(DebugTextEscaped + j, "\\r");
                    break;
                case '\\':
                    j += sprintf(DebugTextEscaped + j, "\\\\");
                    break;
                case '\"':
                    j += sprintf(DebugTextEscaped + j, "\\\"");
                    break;
                default:
                    if(!isprint(DebugText[i])) //unknown unprintable character
                        j += sprintf(DebugTextEscaped + j, "\\%.2x", DebugText[i]);
                    else
                        j += sprintf(DebugTextEscaped + j, "%c", DebugText[i]);
                    break;
                }
            }
            dprintf("DebugString: \"%s\"\n", DebugTextEscaped);
        }
    }

    if(settingboolget("Events", "DebugStrings"))
    {
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
}

static void cbException(EXCEPTION_DEBUG_INFO* ExceptionData)
{
    //TODO: utf8
    hActiveThread = threadgethandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    PLUG_CB_EXCEPTION callbackInfo;
    callbackInfo.Exception = ExceptionData;
    unsigned int ExceptionCode = ExceptionData->ExceptionRecord.ExceptionCode;
    GuiSetLastException(ExceptionCode);

    uint addr = (uint)ExceptionData->ExceptionRecord.ExceptionAddress;
    if(ExceptionData->ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        if(isDetachedByUser)
        {
            PLUG_CB_DETACH detachInfo;
            detachInfo.fdProcessInfo = fdProcessInfo;
            plugincbcall(CB_DETACH, &detachInfo);
            if(!DetachDebuggerEx(fdProcessInfo->dwProcessId))
                dputs("DetachDebuggerEx failed...");
            else
                dputs("detached!");
            isDetachedByUser = false;
            return;
        }
        else if(isPausedByUser)
        {
            dputs("paused!");
            SetNextDbgContinueStatus(DBG_CONTINUE);
            GuiSetDebugState(paused);
            DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), true);
            //lock
            lock(WAITID_RUN);
            SetForegroundWindow(GuiGetWindowHandle());
            bSkipExceptions = false;
            PLUG_CB_PAUSEDEBUG pauseInfo;
            pauseInfo.reserved = 0;
            plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
            plugincbcall(CB_EXCEPTION, &callbackInfo);
            wait(WAITID_RUN);
            return;
        }
        SetContextDataEx(hActiveThread, UE_CIP, (uint)ExceptionData->ExceptionRecord.ExceptionAddress);
    }
    else if(ExceptionData->ExceptionRecord.ExceptionCode == 0x406D1388) //SetThreadName exception
    {
        if(ExceptionData->ExceptionRecord.NumberParameters == sizeof(THREADNAME_INFO) / sizeof(uint))
        {
            THREADNAME_INFO nameInfo;
            memcpy(&nameInfo, ExceptionData->ExceptionRecord.ExceptionInformation, sizeof(THREADNAME_INFO));
            if(nameInfo.dwThreadID == -1) //current thread
                nameInfo.dwThreadID = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
            if(nameInfo.dwType == 0x1000 and nameInfo.dwFlags == 0 and threadisvalid(nameInfo.dwThreadID)) //passed basic checks
            {
                Memory<char*> ThreadName(MAX_THREAD_NAME_SIZE, "cbException:ThreadName");
                memset(ThreadName, 0, MAX_THREAD_NAME_SIZE);
                if(memread(fdProcessInfo->hProcess, nameInfo.szName, ThreadName, MAX_THREAD_NAME_SIZE - 1, 0))
                {
                    int len = (int)strlen(ThreadName);
                    int escape_count = 0;
                    for(int i = 0; i < len; i++)
                        if(ThreadName[i] == '\\' or ThreadName[i] == '\"' or !isprint(ThreadName[i]))
                            escape_count++;
                    Memory<char*> ThreadNameEscaped(len + escape_count * 3 + 1, "cbException:ThreadNameEscaped");
                    memset(ThreadNameEscaped, 0, len + escape_count * 3 + 1);
                    for(int i = 0, j = 0; i < len; i++)
                    {
                        switch(ThreadName[i])
                        {
                        case '\t':
                            j += sprintf(ThreadNameEscaped + j, "\\t");
                            break;
                        case '\f':
                            j += sprintf(ThreadNameEscaped + j, "\\f");
                            break;
                        case '\v':
                            j += sprintf(ThreadNameEscaped + j, "\\v");
                            break;
                        case '\n':
                            j += sprintf(ThreadNameEscaped + j, "\\n");
                            break;
                        case '\r':
                            j += sprintf(ThreadNameEscaped + j, "\\r");
                            break;
                        case '\\':
                            j += sprintf(ThreadNameEscaped + j, "\\\\");
                            break;
                        case '\"':
                            j += sprintf(ThreadNameEscaped + j, "\\\"");
                            break;
                        default:
                            if(!isprint(ThreadName[i])) //unknown unprintable character
                                j += sprintf(ThreadNameEscaped + j, "\\%.2x", ThreadName[i]);
                            else
                                j += sprintf(ThreadNameEscaped + j, "%c", ThreadName[i]);
                            break;
                        }
                    }
                    dprintf("SetThreadName(%X, \"%s\")\n", nameInfo.dwThreadID, ThreadNameEscaped);
                    threadsetname(nameInfo.dwThreadID, ThreadNameEscaped);
                }
            }
        }
    }
    const char* exceptionName = 0;
    if(exceptionNames.count(ExceptionCode))
        exceptionName = exceptionNames[ExceptionCode];
    if(ExceptionData->dwFirstChance) //first chance exception
    {
        if(exceptionName)
            dprintf("first chance exception on "fhex" (%.8X, %s)!\n", addr, ExceptionCode, exceptionName);
        else
            dprintf("first chance exception on "fhex" (%.8X)!\n", addr, ExceptionCode);
        SetNextDbgContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
        if(bSkipExceptions || dbgisignoredexception(ExceptionCode))
            return;
    }
    else //lock the exception
    {
        if(exceptionName)
            dprintf("last chance exception on "fhex" (%.8X, %s)!\n", addr, ExceptionCode, exceptionName);
        else
            dprintf("last chance exception on "fhex" (%.8X)!\n", addr, ExceptionCode);
        SetNextDbgContinueStatus(DBG_CONTINUE);
    }

    GuiSetDebugState(paused);
    DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), true);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions = false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved = 0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    plugincbcall(CB_EXCEPTION, &callbackInfo);
    wait(WAITID_RUN);
}

static void cbDebugEvent(DEBUG_EVENT* DebugEvent)
{
    PLUG_CB_DEBUGEVENT debugEventInfo;
    debugEventInfo.DebugEvent = DebugEvent;
    plugincbcall(CB_DEBUGEVENT, &debugEventInfo);
}

DWORD WINAPI threadDebugLoop(void* lpParameter)
{
    //TODO: utf8
    lock(WAITID_STOP); //we are running
    //initialize
    bIsAttached = false;
    bSkipExceptions = false;
    bBreakOnNextDll = false;
    INIT_STRUCT* init = (INIT_STRUCT*)lpParameter;
    bFileIsDll = IsFileDLL(init->exe, 0);
    pDebuggedEntry = GetPE32Data(init->exe, 0, UE_OEP);
    strcpy_s(szFileName, init->exe);
    if(bFileIsDll)
        fdProcessInfo = (PROCESS_INFORMATION*)InitDLLDebug(init->exe, false, init->commandline, init->currentfolder, 0);
    else
        fdProcessInfo = (PROCESS_INFORMATION*)InitDebug(init->exe, init->commandline, init->currentfolder);
    if(!fdProcessInfo)
    {
        fdProcessInfo = &g_pi;
        dputs("error starting process (invalid pe?)!");
        unlock(WAITID_STOP);
        return 0;
    }
    BOOL wow64 = false, mewow64 = false;
    if(!IsWow64Process(fdProcessInfo->hProcess, &wow64) or !IsWow64Process(GetCurrentProcess(), &mewow64))
    {
        dputs("IsWow64Process failed!");
        StopDebug();
        unlock(WAITID_STOP);
        return 0;
    }
    if((mewow64 and !wow64) or (!mewow64 and wow64))
    {
#ifdef _WIN64
        dputs("Use x32_dbg to debug this process!");
#else
        dputs("Use x64_dbg to debug this process!");
#endif // _WIN64
        unlock(WAITID_STOP);
        return 0;
    }
    GuiAddRecentFile(szFileName);
    varset("$hp", (uint)fdProcessInfo->hProcess, true);
    varset("$pid", fdProcessInfo->dwProcessId, true);
    ecount = 0;
    cachePrivateUsage = 0;
    //NOTE: set custom handlers
    SetCustomHandler(UE_CH_CREATEPROCESS, (void*)cbCreateProcess);
    SetCustomHandler(UE_CH_EXITPROCESS, (void*)cbExitProcess);
    SetCustomHandler(UE_CH_CREATETHREAD, (void*)cbCreateThread);
    SetCustomHandler(UE_CH_EXITTHREAD, (void*)cbExitThread);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (void*)cbSystemBreakpoint);
    SetCustomHandler(UE_CH_LOADDLL, (void*)cbLoadDll);
    SetCustomHandler(UE_CH_UNLOADDLL, (void*)cbUnloadDll);
    SetCustomHandler(UE_CH_OUTPUTDEBUGSTRING, (void*)cbOutputDebugString);
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (void*)cbException);
    SetCustomHandler(UE_CH_DEBUGEVENT, (void*)cbDebugEvent);
    //inform GUI we started without problems
    GuiSetDebugState(initialized);
    //set GUI title
    strcpy(szBaseFileName, szFileName);
    int len = (int)strlen(szBaseFileName);
    while(szBaseFileName[len] != '\\' and len)
        len--;
    if(len)
        strcpy(szBaseFileName, szBaseFileName + len + 1);
    GuiUpdateWindowTitle(szBaseFileName);
    //call plugin callback
    PLUG_CB_INITDEBUG initInfo;
    initInfo.szFileName = szFileName;
    plugincbcall(CB_INITDEBUG, &initInfo);
    //run debug loop (returns when process debugging is stopped)
    DebugLoop();
    isDetachedByUser = false;
    //call plugin callback
    PLUG_CB_STOPDEBUG stopInfo;
    stopInfo.reserved = 0;
    plugincbcall(CB_STOPDEBUG, &stopInfo);
    //message the user/do final stuff
    RemoveAllBreakPoints(UE_OPTION_REMOVEALL); //remove all breakpoints
    //cleanup
    dbclose();
    modclear();
    threadclear();
    GuiSetDebugState(stopped);
    dputs("debugging stopped!");
    varset("$hp", (uint)0, true);
    varset("$pid", (uint)0, true);
    unlock(WAITID_STOP); //we are done
    return 0;
}

bool cbDeleteAllBreakpoints(const BREAKPOINT* bp)
{
    if(bpdel(bp->addr, BPNORMAL) and (!bp->enabled or DeleteBPX(bp->addr)))
        return true;
    dprintf("delete breakpoint failed: "fhex"\n", bp->addr);
    return false;
}

bool cbEnableAllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPNORMAL or bp->enabled)
        return true;
    if(!bpenable(bp->addr, BPNORMAL, true) or !SetBPX(bp->addr, bp->titantype, (void*)cbUserBreakpoint))
    {
        dprintf("could not enable breakpoint "fhex"\n", bp->addr);
        return false;
    }
    return true;
}

bool cbDisableAllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPNORMAL or !bp->enabled)
        return true;
    if(!bpenable(bp->addr, BPNORMAL, false) or !DeleteBPX(bp->addr))
    {
        dprintf("could not disable breakpoint "fhex"\n", bp->addr);
        return false;
    }
    return true;
}

bool cbEnableAllHardwareBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPHARDWARE or bp->enabled)
        return true;
    DWORD drx = 0;
    if(!GetUnusedHardwareBreakPointRegister(&drx))
    {
        dprintf("did not enable hardware breakpoint "fhex" (all slots full)\n", bp->addr);
        return true;
    }
    int titantype = bp->titantype;
    TITANSETDRX(titantype, drx);
    bpsettitantype(bp->addr, BPHARDWARE, titantype);
    if(!bpenable(bp->addr, BPHARDWARE, true) or !SetHardwareBreakPoint(bp->addr, drx, TITANGETTYPE(bp->titantype), TITANGETSIZE(bp->titantype), (void*)cbHardwareBreakpoint))
    {
        dprintf("could not enable hardware breakpoint "fhex"\n", bp->addr);
        return false;
    }
    return true;
}

bool cbDisableAllHardwareBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPHARDWARE or !bp->enabled)
        return true;
    if(!bpenable(bp->addr, BPHARDWARE, false) or !DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype)))
    {
        dprintf("could not disable hardware breakpoint "fhex"\n", bp->addr);
        return false;
    }
    return true;
}

bool cbEnableAllMemoryBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPMEMORY or bp->enabled)
        return true;
    uint size = 0;
    memfindbaseaddr(bp->addr, &size);
    if(!bpenable(bp->addr, BPMEMORY, true) or !SetMemoryBPXEx(bp->addr, size, bp->titantype, !bp->singleshoot, (void*)cbMemoryBreakpoint))
    {
        dprintf("could not enable memory breakpoint "fhex"\n", bp->addr);
        return false;
    }
    return true;
}

bool cbDisableAllMemoryBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPMEMORY or !bp->enabled)
        return true;
    if(!bpenable(bp->addr, BPMEMORY, false) or !DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype)))
    {
        dprintf("could not disable memory breakpoint "fhex"\n", bp->addr);
        return false;
    }
    return true;
}

bool cbBreakpointList(const BREAKPOINT* bp)
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
    bool enabled = bp->enabled;
    if(*bp->name)
        dprintf("%d:%s:"fhex":\"%s\"\n", enabled, type, bp->addr, bp->name);
    else
        dprintf("%d:%s:"fhex"\n", enabled, type, bp->addr);
    return true;
}

bool cbDeleteAllMemoryBreakpoints(const BREAKPOINT* bp)
{
    if(!bp->enabled)
        return true;
    uint size;
    memfindbaseaddr(bp->addr, &size);
    if(!bpdel(bp->addr, BPMEMORY) or !RemoveMemoryBPX(bp->addr, size))
    {
        dprintf("delete memory breakpoint failed: "fhex"\n", bp->addr);
        return STATUS_ERROR;
    }
    return true;
}

bool cbDeleteAllHardwareBreakpoints(const BREAKPOINT* bp)
{
    if(!bp->enabled)
        return true;
    if(!bpdel(bp->addr, BPHARDWARE) or !DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype)))
    {
        dprintf("delete hardware breakpoint failed: "fhex"\n", bp->addr);
        return STATUS_ERROR;
    }
    return true;
}

static void cbAttachDebugger()
{
    if(hEvent) //Signal the AeDebug event
    {
        SetEvent(hEvent);
        hEvent = 0;
    }
    varset("$hp", (uint)fdProcessInfo->hProcess, true);
    varset("$pid", fdProcessInfo->dwProcessId, true);
}

DWORD WINAPI threadAttachLoop(void* lpParameter)
{
    //TODO: utf8
    lock(WAITID_STOP);
    bIsAttached = true;
    bSkipExceptions = false;
    DWORD pid = (DWORD)lpParameter;
    static PROCESS_INFORMATION pi_attached;
    fdProcessInfo = &pi_attached;
    //do some init stuff
    bFileIsDll = IsFileDLL(szFileName, 0);
    GuiAddRecentFile(szFileName);
    ecount = 0;
    cachePrivateUsage = 0;
    //NOTE: set custom handlers
    SetCustomHandler(UE_CH_CREATEPROCESS, (void*)cbCreateProcess);
    SetCustomHandler(UE_CH_EXITPROCESS, (void*)cbExitProcess);
    SetCustomHandler(UE_CH_CREATETHREAD, (void*)cbCreateThread);
    SetCustomHandler(UE_CH_EXITTHREAD, (void*)cbExitThread);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (void*)cbSystemBreakpoint);
    SetCustomHandler(UE_CH_LOADDLL, (void*)cbLoadDll);
    SetCustomHandler(UE_CH_UNLOADDLL, (void*)cbUnloadDll);
    SetCustomHandler(UE_CH_OUTPUTDEBUGSTRING, (void*)cbOutputDebugString);
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (void*)cbException);
    SetCustomHandler(UE_CH_DEBUGEVENT, (void*)cbDebugEvent);
    //inform GUI start we started without problems
    GuiSetDebugState(initialized);
    //set GUI title
    strcpy_s(szBaseFileName, szFileName);
    int len = (int)strlen(szBaseFileName);
    while(szBaseFileName[len] != '\\' and len)
        len--;
    if(len)
        strcpy_s(szBaseFileName, szBaseFileName + len + 1);
    GuiUpdateWindowTitle(szBaseFileName);
    //call plugin callback (init)
    PLUG_CB_INITDEBUG initInfo;
    initInfo.szFileName = szFileName;
    plugincbcall(CB_INITDEBUG, &initInfo);
    //call plugin callback (attach)
    PLUG_CB_ATTACH attachInfo;
    attachInfo.dwProcessId = (DWORD)pid;
    plugincbcall(CB_ATTACH, &attachInfo);
    //run debug loop (returns when process debugging is stopped)
    AttachDebugger(pid, true, fdProcessInfo, (void*)cbAttachDebugger);
    isDetachedByUser = false;
    //call plugin callback
    PLUG_CB_STOPDEBUG stopInfo;
    stopInfo.reserved = 0;
    plugincbcall(CB_STOPDEBUG, &stopInfo);
    //message the user/do final stuff
    RemoveAllBreakPoints(UE_OPTION_REMOVEALL); //remove all breakpoints
    //cleanup
    dbclose();
    modclear();
    threadclear();
    GuiSetDebugState(stopped);
    dputs("debugging stopped!");
    varset("$hp", (uint)0, true);
    varset("$pid", (uint)0, true);
    unlock(WAITID_STOP);
    waitclear();
    return 0;
}

void cbDetach()
{
    if(!isDetachedByUser)
        return;
    PLUG_CB_DETACH detachInfo;
    detachInfo.fdProcessInfo = fdProcessInfo;
    plugincbcall(CB_DETACH, &detachInfo);
    if(!DetachDebuggerEx(fdProcessInfo->dwProcessId))
        dputs("DetachDebuggerEx failed...");
    else
        dputs("detached!");
    return;
}


bool IsProcessElevated()
{
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID SecurityIdentifier;
    if(!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &SecurityIdentifier))
        return 0;

    BOOL IsAdminMember;
    if(!CheckTokenMembership(NULL, SecurityIdentifier, &IsAdminMember))
        IsAdminMember = FALSE;

    FreeSid(SecurityIdentifier);
    return IsAdminMember ? true : false;
}

bool _readwritejitkey(char* jit_key_value, DWORD* jit_key_vale_size, char* key, arch arch_in, arch* arch_out, readwritejitkey_error_t* error, bool write)
{
    DWORD key_flags;
    DWORD lRv;
    HKEY hKey;
    DWORD dwDisposition;

    if(error != NULL)
        * error = ERROR_RW;

    if(write)
    {
        if(!IsProcessElevated())
        {
            if(error != NULL)
                * error = ERROR_RW_NOTADMIN;
            return false;
        }
        key_flags = KEY_WRITE;
    }
    else
        key_flags = KEY_READ;

    if(arch_out != NULL)
    {
        if(arch_in != x64 && arch_in != x32)
        {
#ifdef _WIN32
            * arch_out = x32;
#endif
#ifdef _WIN64
            * arch_out = x64;
#endif
        }
        else
            * arch_out = arch_in;
    }

    if(arch_in == x64)
    {
#ifndef _WIN64
        if(!IsWow64())
        {
            if(error != NULL)
                * error = ERROR_RW_NOTWOW64;

            return false;
        }
#endif

#ifdef _WIN32
        key_flags |= KEY_WOW64_64KEY;
#endif
    }
    else if(arch_in == x32)
    {
#ifdef _WIN64
        key_flags |= KEY_WOW64_32KEY;
#endif
    }

    if(write)
    {
        lRv = RegCreateKeyEx(HKEY_LOCAL_MACHINE, JIT_REG_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, key_flags, NULL, &hKey, &dwDisposition);
        if(lRv != ERROR_SUCCESS)
            return false;

        lRv = RegSetValueExA(hKey, key, 0, REG_SZ, (BYTE*) jit_key_value, (DWORD)(* jit_key_vale_size) + 1);
        RegCloseKey(hKey);
    }
    else
    {
        lRv = RegOpenKeyEx(HKEY_LOCAL_MACHINE, JIT_REG_KEY, 0, key_flags, &hKey);
        if(lRv != ERROR_SUCCESS)
        {
            if(error != NULL)
                * error = ERROR_RW_FILE_NOT_FOUND;

            return false;
        }

        lRv = RegQueryValueExA(hKey, key, 0, NULL, (LPBYTE)jit_key_value, jit_key_vale_size);
    }

    if(lRv != ERROR_SUCCESS)
        return false;

    return true;
}

bool dbgpagerightstostring(DWORD protect, char* rights)
{
    memset(rights, 0, RIGHTS_STRING);

    switch(protect & 0xFF)
    {
    case PAGE_EXECUTE:
        strcpy(rights, "E---");
        break;
    case PAGE_EXECUTE_READ:
        strcpy(rights, "ER--");
        break;
    case PAGE_EXECUTE_READWRITE:
        strcpy(rights, "ERW-");
        break;
    case PAGE_EXECUTE_WRITECOPY:
        strcpy(rights, "ERWC");
        break;
    case PAGE_NOACCESS:
        strcpy(rights, "----");
        break;
    case PAGE_READONLY:
        strcpy(rights, "-R--");
        break;
    case PAGE_READWRITE:
        strcpy(rights, "-RW-");
        break;
    case PAGE_WRITECOPY:
        strcpy(rights, "-RWC");
        break;
    }

    if(protect & PAGE_GUARD)
        strcat(rights, "G");
    else
        strcat(rights, "-");

    return true;
}

void dbggetpageligned(uint* addr)
{
#ifdef _WIN64
    * addr &=  0xFFFFFFFFFFFFF000;
#else // _WIN32
    * addr &= 0xFFFFF000;
#endif // _WIN64
}


bool dbgpagerightsfromstring(DWORD* protect, char* rights_string)
{
    //TODO: utf8
    if(strlen(rights_string) < 2)
        return false;

    * protect = 0;
    if(rights_string[0] == 'G' || rights_string[0] == 'g')
    {
        * protect |= PAGE_GUARD;
        rights_string++;
    }

    if(_strcmpi(rights_string, "Execute") == 0)
        * protect |= PAGE_EXECUTE;
    else if(_strcmpi(rights_string, "ExecuteRead") == 0)
        * protect |= PAGE_EXECUTE_READ;
    else if(_strcmpi(rights_string, "ExecuteReadWrite") == 0)
        * protect |= PAGE_EXECUTE_READWRITE;
    else if(_strcmpi(rights_string, "ExecuteWriteCopy") == 0)
        * protect |= PAGE_EXECUTE_WRITECOPY;
    else if(_strcmpi(rights_string, "NoAccess") == 0)
        * protect |= PAGE_NOACCESS;
    else if(_strcmpi(rights_string, "ReadOnly") == 0)
        * protect |= PAGE_READONLY;
    else if(_strcmpi(rights_string, "ReadWrite") == 0)
        * protect |= PAGE_READWRITE;
    else if(_strcmpi(rights_string, "WriteCopy") == 0)
        * protect |= PAGE_WRITECOPY;

    if(* protect == 0)
        return false;

    return true;
}

bool dbgsetpagerights(uint* addr, char* rights_string)
{
    DWORD protect;
    DWORD old_protect;

    dbggetpageligned(addr);

    if(!dbgpagerightsfromstring(& protect, rights_string))
        return false;

    if(VirtualProtectEx(fdProcessInfo->hProcess, (void*)*addr, PAGE_SIZE, protect, & old_protect) == 0)
        return false;

    return true;
}

bool dbggetpagerights(uint* addr, char* rights)
{
    dbggetpageligned(addr);

    MEMORY_BASIC_INFORMATION mbi;
    if(VirtualQueryEx(fdProcessInfo->hProcess, (const void*)*addr, &mbi, sizeof(mbi)) == 0)
        return false;

    return dbgpagerightstostring(mbi.Protect, rights);
}

bool dbggetjitauto(bool* auto_on, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out)
{
    char jit_entry[4];
    DWORD jit_entry_size = sizeof(jit_entry) - 1;
    readwritejitkey_error_t rw_error;

    if(_readwritejitkey(jit_entry, & jit_entry_size, "Auto", arch_in, arch_out, & rw_error, false) == false)
    {
        if(rw_error == ERROR_RW_FILE_NOT_FOUND)
        {
            if(rw_error_out != NULL)
                * rw_error_out = rw_error;

            return true;
        }

        return false;
    }
    if(_strcmpi(jit_entry, "1") == 0)
        * auto_on = true;
    else if(_strcmpi(jit_entry, "0") == 0)
        * auto_on = false;
    else
        return false;

    return true;
}

bool dbgsetjitauto(bool auto_on, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out)
{
    DWORD auto_string_size = sizeof("1");
    readwritejitkey_error_t rw_error;

    if(auto_on == false)
    {
        char jit_entry[4];
        DWORD jit_entry_size = sizeof(jit_entry) - 1;

        if(_readwritejitkey(jit_entry, & jit_entry_size, "Auto", arch_in, arch_out, & rw_error, false) == false)
        {
            if(rw_error == ERROR_RW_FILE_NOT_FOUND)
                return true;
        }
    }

    if(_readwritejitkey(auto_on ? "1" : "0", & auto_string_size, "Auto", arch_in, arch_out, & rw_error, true) == false)
    {
        if(rw_error_out != NULL)
            * rw_error_out = rw_error;
        return false;
    }

    return true;
}

bool dbggetjit(char jit_entry[JIT_ENTRY_MAX_SIZE], arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out)
{
    DWORD jit_entry_size = JIT_ENTRY_MAX_SIZE;
    readwritejitkey_error_t rw_error;

    if(_readwritejitkey(jit_entry, & jit_entry_size, "Debugger", arch_in, arch_out, & rw_error, false) == false)
    {
        if(rw_error_out != NULL)
            * rw_error_out = rw_error;

        return false;
    }

    return true;
}

bool dbggetdefjit(char* jit_entry)
{
    char path[JIT_ENTRY_DEF_SIZE];
    path[0] = '"';
    GetModuleFileNameA(GetModuleHandleA(NULL), &path[1], MAX_PATH);
    strcat(path, ATTACH_CMD_LINE);
    strcpy(jit_entry, path);

    return true;
}

bool dbgsetjit(char* jit_cmd, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out)
{
    //TODO: utf8
    DWORD jit_cmd_size = (DWORD)strlen(jit_cmd);
    readwritejitkey_error_t rw_error;
    if(_readwritejitkey(jit_cmd, & jit_cmd_size, "Debugger", arch_in, arch_out, & rw_error, true) == false)
    {
        if(rw_error_out != NULL)
            * rw_error_out = rw_error;

        return false;
    }

    return true;
}

bool dbglistprocesses(std::vector<PROCESSENTRY32>* list)
{
    list->clear();
    Handle hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(!hProcessSnap)
        return false;
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if(!Process32First(hProcessSnap, &pe32))
        return false;
    do
    {
        if(pe32.th32ProcessID == GetCurrentProcessId())
            continue;
        if(!_stricmp(pe32.szExeFile, "System"))
            continue;
        if(!_stricmp(pe32.szExeFile, "[System Process]"))
            continue;
        Handle hProcess = TitanOpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pe32.th32ProcessID);
        if(!hProcess)
            continue;
        BOOL wow64 = false, mewow64 = false;
        if(!IsWow64Process(hProcess, &wow64) or !IsWow64Process(GetCurrentProcess(), &mewow64))
            continue;
        if((mewow64 and !wow64) or (!mewow64 and wow64))
            continue;
        char szExePath[MAX_PATH] = "";
        if(GetModuleFileNameExA(hProcess, 0, szExePath, sizeof(szExePath)))
            strcpy_s(pe32.szExeFile, szExePath);
        list->push_back(pe32);
    }
    while(Process32Next(hProcessSnap, &pe32));
    return true;
}