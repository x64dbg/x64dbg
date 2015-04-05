/**
 @file debugger.cpp

 @brief Implements the debugger class.
 */

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
#include "exception.h"
#include "error.h"
#include "module.h"

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
static SIZE_T cachePrivateUsage = 0;
static HANDLE hEvent = 0;
static String lastDebugText;
char szFileName[MAX_PATH] = "";
char szSymbolCachePath[MAX_PATH] = "";
char sqlitedb[deflen] = "";
PROCESS_INFORMATION* fdProcessInfo = &g_pi;
HANDLE hActiveThread;
bool bUndecorateSymbolNames = true;
bool bEnableSourceDebugging = true;

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
            MemUpdateMap(fdProcessInfo->hProcess);
        }
        Sleep(1000);
    }
    return 0;
}

void dbginit()
{
    ExceptionCodeInit();
    ErrorCodeInit();
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
    int bpcount = BpGetList(&list);
    for(int i = 0; i < bpcount; i++)
    {
        if(list[i].type == BPNORMAL and IsBPXEnabled(list[i].addr))
            DeleteBPX(list[i].addr);
    }
}

void dbgenablebpx()
{
    std::vector<BREAKPOINT> list;
    int bpcount = BpGetList(&list);
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
    ignoredExceptionRange.clear();
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
    if (MemIsValidReadPtr(disasm_addr))
    {
        if (bEnableSourceDebugging)
        {
            char szSourceFile[MAX_STRING_SIZE] = "";
            int line = 0;
            if (SymGetSourceLine(cip, szSourceFile, &line))
                GuiLoadSourceFile(szSourceFile, line);
        }
        GuiDisasmAt(disasm_addr, cip);
    }
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
    if(!ModNameFromAddr(disasm_addr, modname, true))
        *modname = 0;
    else
        sprintf(modtext, "Module: %s - ", modname);
    char title[1024] = "";
    sprintf(title, "File: %s - PID: %X - %sThread: %X", szBaseFileName, fdProcessInfo->dwProcessId, modtext, ThreadGetId(hActiveThread));
    GuiUpdateWindowTitle(title);
    GuiUpdateAllViews();
}

void cbUserBreakpoint()
{
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    BREAKPOINT bp;
    BRIDGEBP pluginBp;
    PLUG_CB_BREAKPOINT bpInfo;
    bpInfo.breakpoint = 0;
    if(!BpGet(GetContextDataEx(hActiveThread, UE_CIP), BPNORMAL, 0, &bp) and bp.enabled)
        dputs("Breakpoint reached not in list!");
    else
    {
        const char* bptype = "INT3";
        int titantype = bp.titantype;
        if((titantype & UE_BREAKPOINT_TYPE_UD2) == UE_BREAKPOINT_TYPE_UD2)
            bptype = "UD2";
        else if((titantype & UE_BREAKPOINT_TYPE_LONG_INT3) == UE_BREAKPOINT_TYPE_LONG_INT3)
            bptype = "LONG INT3";
        const char* symbolicname = SymGetSymbolicName(bp.addr);
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
            BpDelete(bp.addr, BPNORMAL);
        BpToBridge(&bp, &pluginBp);
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
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    uint cip = GetContextDataEx(hActiveThread, UE_CIP);
    BREAKPOINT bp;
    BRIDGEBP pluginBp;
    PLUG_CB_BREAKPOINT bpInfo;
    bpInfo.breakpoint = 0;
    if(!BpGet((uint)ExceptionAddress, BPHARDWARE, 0, &bp))
        dputs("Hardware breakpoint reached not in list!");
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
        const char* symbolicname = SymGetSymbolicName(bp.addr);
        if(symbolicname)
        {
            if(*bp.name)
                dprintf("Hardware breakpoint (%s%s) \"%s\" at %s ("fhex")!\n", bpsize, bptype, bp.name, symbolicname, bp.addr);
            else
                dprintf("Hardware breakpoint (%s%s) at %s ("fhex")!\n", bpsize, bptype, symbolicname, bp.addr);
        }
        else
        {
            if(*bp.name)
                dprintf("Hardware breakpoint (%s%s) \"%s\" at "fhex"!\n", bpsize, bptype, bp.name, bp.addr);
            else
                dprintf("Hardware breakpoint (%s%s) at "fhex"!\n", bpsize, bptype, bp.addr);
        }
        BpToBridge(&bp, &pluginBp);
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
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    uint cip = GetContextDataEx(hActiveThread, UE_CIP);
    uint size;
    uint base = MemFindBaseAddr((uint)ExceptionAddress, &size, true);
    BREAKPOINT bp;
    BRIDGEBP pluginBp;
    PLUG_CB_BREAKPOINT bpInfo;
    bpInfo.breakpoint = 0;
    if(!BpGet(base, BPMEMORY, 0, &bp))
        dputs("Memory breakpoint reached not in list!");
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
        const char* symbolicname = SymGetSymbolicName(bp.addr);
        if(symbolicname)
        {
            if(*bp.name)
                dprintf("Memory breakpoint%s \"%s\" at %s ("fhex", "fhex")!\n", bptype, bp.name, symbolicname, bp.addr, ExceptionAddress);
            else
                dprintf("Memory breakpoint%s at %s ("fhex", "fhex")!\n", bptype, symbolicname, bp.addr, ExceptionAddress);
        }
        else
        {
            if(*bp.name)
                dprintf("Memory breakpoint%s \"%s\" at "fhex" ("fhex")!\n", bptype, bp.name, bp.addr, ExceptionAddress);
            else
                dprintf("Memory breakpoint%s at "fhex" ("fhex")!\n", bptype, bp.addr, ExceptionAddress);
        }
        BpToBridge(&bp, &pluginBp);
        bpInfo.breakpoint = &pluginBp;
    }
    if(bp.singleshoot)
        BpDelete(bp.addr, BPMEMORY); //delete from breakpoint list
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
            strcpy_s(newtext, len + 1, text);
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
            dprintf("Could not set breakpoint "fhex"!\n", bp->addr);
    }
    break;

    case BPMEMORY:
    {
        uint size = 0;
        MemFindBaseAddr(bp->addr, &size);
        if(!SetMemoryBPXEx(bp->addr, size, bp->titantype, !bp->singleshoot, (void*)cbMemoryBreakpoint))
            dprintf("Could not set memory breakpoint "fhex"!\n", bp->addr);
    }
    break;

    case BPHARDWARE:
    {
        DWORD drx = 0;
        if(!GetUnusedHardwareBreakPointRegister(&drx))
        {
            dputs("You can only set 4 hardware breakpoints");
            return false;
        }
        int titantype = bp->titantype;
        TITANSETDRX(titantype, drx);
        BpSetTitanType(bp->addr, BPHARDWARE, titantype);
        if(!SetHardwareBreakPoint(bp->addr, drx, TITANGETTYPE(bp->titantype), TITANGETSIZE(bp->titantype), (void*)cbHardwareBreakpoint))
            dprintf("Could not set hardware breakpoint "fhex"!\n", bp->addr);
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
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
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
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
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
    MemRead((void*)cip, &ch, 1, 0);
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

static void cbCreateProcess(CREATE_PROCESS_DEBUG_INFO* CreateProcessInfo)
{
    void* base = CreateProcessInfo->lpBaseOfImage;
    char DebugFileName[deflen] = "";
    if(!GetFileNameFromHandle(CreateProcessInfo->hFile, DebugFileName))
    {
        wchar_t wszFileName[MAX_PATH] = L"";
        if(!DevicePathFromFileHandleW(CreateProcessInfo->hFile, wszFileName, sizeof(wszFileName)))
            strcpy_s(DebugFileName, "??? (GetFileNameFromHandle failed!)");
        else
            strcpy_s(DebugFileName, MAX_PATH, StringUtils::Utf16ToUtf8(wszFileName).c_str());
    }
    dprintf("Process Started: "fhex" %s\n", base, DebugFileName);

    MemUpdateMap(fdProcessInfo->hProcess);
    GuiDumpAt(MemFindBaseAddr(GetContextData(UE_CIP), 0) + PAGE_SIZE); //dump somewhere

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
    SafeSymSetOptions(SYMOPT_DEBUG | SYMOPT_LOAD_LINES | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | SYMOPT_FAVOR_COMPRESSED | SYMOPT_IGNORE_NT_SYMPATH);
    GuiSymbolLogClear();
    char szServerSearchPath[MAX_PATH * 2] = "";
    sprintf_s(szServerSearchPath, "SRV*%s", szSymbolCachePath);
    SafeSymInitialize(fdProcessInfo->hProcess, szServerSearchPath, false); //initialize symbols
    SafeSymRegisterCallback64(fdProcessInfo->hProcess, SymRegisterCallbackProc64, 0);
    SafeSymLoadModuleEx(fdProcessInfo->hProcess, CreateProcessInfo->hFile, DebugFileName, 0, (DWORD64)base, 0, 0, 0);
    IMAGEHLP_MODULE64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct = sizeof(modInfo);
    if(SafeSymGetModuleInfo64(fdProcessInfo->hProcess, (DWORD64)base, &modInfo))
        ModLoad((uint)base, modInfo.ImageSize, modInfo.ImageName);
    dbggetprivateusage(fdProcessInfo->hProcess, true);
    MemUpdateMap(fdProcessInfo->hProcess); //update memory map
    char modname[256] = "";
    if(ModNameFromAddr((uint)base, modname, true))
        BpEnumAll(cbSetModuleBreakpoints, modname);
    GuiUpdateBreakpointsView();
    if(!bFileIsDll and !bIsAttached) //Set entry breakpoint
    {
        pDebuggedBase = (uint)CreateProcessInfo->lpBaseOfImage; //debugged base = executable
        char command[256] = "";

        if(settingboolget("Events", "TlsCallbacks"))
        {
            DWORD NumberOfCallBacks = 0;
            TLSGrabCallBackDataW(StringUtils::Utf8ToUtf16(DebugFileName).c_str(), 0, &NumberOfCallBacks);
            if(NumberOfCallBacks)
            {
                dprintf("TLS Callbacks: %d\n", NumberOfCallBacks);
                Memory<uint*> TLSCallBacks(NumberOfCallBacks * sizeof(uint), "cbCreateProcess:TLSCallBacks");
                if(!TLSGrabCallBackDataW(StringUtils::Utf8ToUtf16(DebugFileName).c_str(), TLSCallBacks, &NumberOfCallBacks))
                    dputs("Failed to get TLS callback addresses!");
                else
                {
                    uint ImageBase = GetPE32DataW(StringUtils::Utf8ToUtf16(DebugFileName).c_str(), 0, UE_IMAGEBASE);
                    for(unsigned int i = 0; i < NumberOfCallBacks; i++)
                    {
                        sprintf(command, "bp "fhex",\"TLS Callback %d\",ss", TLSCallBacks[i] - ImageBase + pDebuggedBase, i + 1);
                        cmddirectexec(dbggetcommandlist(), command);
                    }
                }
            }
        }

        if(settingboolget("Events", "EntryBreakpoint"))
        {
            sprintf(command, "bp "fhex",\"entry breakpoint\",ss", (uint)CreateProcessInfo->lpStartAddress);
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
    ThreadCreate(&threadInfo);
}

static void cbExitProcess(EXIT_PROCESS_DEBUG_INFO* ExitProcess)
{
    PLUG_CB_EXITPROCESS callbackInfo;
    callbackInfo.ExitProcess = ExitProcess;
    plugincbcall(CB_EXITPROCESS, &callbackInfo);
    //Cleanup
    SafeSymCleanup(fdProcessInfo->hProcess);
}

static void cbCreateThread(CREATE_THREAD_DEBUG_INFO* CreateThread)
{
    ThreadCreate(CreateThread); //update thread list
    DWORD dwThreadId = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    hActiveThread = ThreadGetHandle(dwThreadId);

    if(settingboolget("Events", "ThreadEntry"))
    {
        char command[256] = "";
        sprintf(command, "bp "fhex",\"Thread %X\",ss", (uint)CreateThread->lpStartAddress, dwThreadId);
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
        MemUpdateMap(fdProcessInfo->hProcess); //update memory map
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
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    DWORD dwThreadId = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    PLUG_CB_EXITTHREAD callbackInfo;
    callbackInfo.ExitThread = ExitThread;
    callbackInfo.dwThreadId = dwThreadId;
    plugincbcall(CB_EXITTHREAD, &callbackInfo);
    ThreadExit(dwThreadId);
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
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    //log message
    if(bIsAttached)
        dputs("Attach breakpoint reached!");
    else
        dputs("System breakpoint reached!");
    bSkipExceptions = false; //we are not skipping first-chance exceptions
    uint cip = GetContextDataEx(hActiveThread, UE_CIP);
    GuiDumpAt(MemFindBaseAddr(cip, 0, true)); //dump somewhere

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
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    void* base = LoadDll->lpBaseOfDll;
    char DLLDebugFileName[deflen] = "";
    if(!GetFileNameFromHandle(LoadDll->hFile, DLLDebugFileName))
    {
        wchar_t wszFileName[MAX_PATH] = L"";
        if(!DevicePathFromFileHandleW(LoadDll->hFile, wszFileName, sizeof(wszFileName)))
            strcpy_s(DLLDebugFileName, "??? (GetFileNameFromHandle failed!)");
        else
            strcpy_s(DLLDebugFileName, MAX_PATH, StringUtils::Utf16ToUtf8(wszFileName).c_str());
    }
    SafeSymLoadModuleEx(fdProcessInfo->hProcess, LoadDll->hFile, DLLDebugFileName, 0, (DWORD64)base, 0, 0, 0);
    IMAGEHLP_MODULE64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
    if(SafeSymGetModuleInfo64(fdProcessInfo->hProcess, (DWORD64)base, &modInfo))
        ModLoad((uint)base, modInfo.ImageSize, modInfo.ImageName);
    dbggetprivateusage(fdProcessInfo->hProcess, true);
    MemUpdateMap(fdProcessInfo->hProcess); //update memory map
    char modname[256] = "";
    if(ModNameFromAddr((uint)base, modname, true))
        BpEnumAll(cbSetModuleBreakpoints, modname);
    GuiUpdateBreakpointsView();
    bool bAlreadySetEntry = false;

    char command[256] = "";
    bool bIsDebuggingThis = false;
    if(bFileIsDll and !_stricmp(DLLDebugFileName, szFileName) and !bIsAttached) //Set entry breakpoint
    {
        bIsDebuggingThis = true;
        pDebuggedBase = (uint)base;
        if(settingboolget("Events", "EntryBreakpoint"))
        {
            bAlreadySetEntry = true;
            sprintf(command, "bp "fhex",\"entry breakpoint\",ss", pDebuggedBase + pDebuggedEntry);
            cmddirectexec(dbggetcommandlist(), command);
        }
    }
    GuiUpdateBreakpointsView();

    if(settingboolget("Events", "TlsCallbacks"))
    {
        DWORD NumberOfCallBacks = 0;
        TLSGrabCallBackDataW(StringUtils::Utf8ToUtf16(DLLDebugFileName).c_str(), 0, &NumberOfCallBacks);
        if(NumberOfCallBacks)
        {
            dprintf("TLS Callbacks: %d\n", NumberOfCallBacks);
            Memory<uint*> TLSCallBacks(NumberOfCallBacks * sizeof(uint), "cbLoadDll:TLSCallBacks");
            if(!TLSGrabCallBackDataW(StringUtils::Utf8ToUtf16(DLLDebugFileName).c_str(), TLSCallBacks, &NumberOfCallBacks))
                dputs("Failed to get TLS callback addresses!");
            else
            {
                uint ImageBase = GetPE32DataW(StringUtils::Utf8ToUtf16(DLLDebugFileName).c_str(), 0, UE_IMAGEBASE);
                for(unsigned int i = 0; i < NumberOfCallBacks; i++)
                {
                    if(bIsDebuggingThis)
                        sprintf(command, "bp "fhex",\"TLS Callback %d\",ss", TLSCallBacks[i] - ImageBase + (uint)base, i + 1);
                    else
                        sprintf(command, "bp "fhex",\"TLS Callback %d (%s)\",ss", TLSCallBacks[i] - ImageBase + (uint)base, i + 1, modname);
                    cmddirectexec(dbggetcommandlist(), command);
                }
            }
        }
    }

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
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    PLUG_CB_UNLOADDLL callbackInfo;
    callbackInfo.UnloadDll = UnloadDll;
    plugincbcall(CB_UNLOADDLL, &callbackInfo);

    void* base = UnloadDll->lpBaseOfDll;
    char modname[256] = "???";
    if(ModNameFromAddr((uint)base, modname, true))
        BpEnumAll(cbRemoveModuleBreakpoints, modname);
    GuiUpdateBreakpointsView();
    SafeSymUnloadModule64(fdProcessInfo->hProcess, (DWORD64)base);
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

    ModUnload((uint)base);
}

static void cbOutputDebugString(OUTPUT_DEBUG_STRING_INFO* DebugString)
{

    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    PLUG_CB_OUTPUTDEBUGSTRING callbackInfo;
    callbackInfo.DebugString = DebugString;
    plugincbcall(CB_OUTPUTDEBUGSTRING, &callbackInfo);

    if(!DebugString->fUnicode) //ASCII
    {
        Memory<char*> DebugText(DebugString->nDebugStringLength + 1, "cbOutputDebugString:DebugText");
        if(MemRead(DebugString->lpDebugStringData, DebugText, DebugString->nDebugStringLength, 0))
        {
            String str = String(DebugText);
            if(str != lastDebugText) //fix for every string being printed twice
            {
                if(str != "\n")
                    dprintf("DebugString: \"%s\"\n", StringUtils::Escape(str).c_str());
                lastDebugText = str;
            }
            else
                lastDebugText = "";
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
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
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
                dputs("Detached!");
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
    else if(ExceptionData->ExceptionRecord.ExceptionCode == MS_VC_EXCEPTION) //SetThreadName exception
    {
        THREADNAME_INFO nameInfo;
        memcpy(&nameInfo, ExceptionData->ExceptionRecord.ExceptionInformation, sizeof(THREADNAME_INFO));
        if(nameInfo.dwThreadID == -1) //current thread
            nameInfo.dwThreadID = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
        if(nameInfo.dwType == 0x1000 and nameInfo.dwFlags == 0 and ThreadIsValid(nameInfo.dwThreadID)) //passed basic checks
        {
            Memory<char*> ThreadName(MAX_THREAD_NAME_SIZE, "cbException:ThreadName");
            if(MemRead((void *)nameInfo.szName, ThreadName, MAX_THREAD_NAME_SIZE - 1, 0))
            {
                String ThreadNameEscaped = StringUtils::Escape(ThreadName);
                dprintf("SetThreadName(%X, \"%s\")\n", nameInfo.dwThreadID, ThreadNameEscaped.c_str());
                ThreadSetName(nameInfo.dwThreadID, ThreadNameEscaped.c_str());
            }
        }
    }
    const char* exceptionName = ExceptionCodeToName(ExceptionCode);
    if(ExceptionData->dwFirstChance) //first chance exception
    {
        if(exceptionName)
            dprintf("First chance exception on "fhex" (%.8X, %s)!\n", addr, ExceptionCode, exceptionName);
        else
            dprintf("First chance exception on "fhex" (%.8X)!\n", addr, ExceptionCode);
        SetNextDbgContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
        if(bSkipExceptions || dbgisignoredexception(ExceptionCode))
            return;
    }
    else //lock the exception
    {
        if(exceptionName)
            dprintf("Last chance exception on "fhex" (%.8X, %s)!\n", addr, ExceptionCode, exceptionName);
        else
            dprintf("Last chance exception on "fhex" (%.8X)!\n", addr, ExceptionCode);
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
    lock(WAITID_STOP); //we are running
    //initialize
    bIsAttached = false;
    bSkipExceptions = false;
    bBreakOnNextDll = false;
    INIT_STRUCT* init = (INIT_STRUCT*)lpParameter;
    bFileIsDll = IsFileDLLW(StringUtils::Utf8ToUtf16(init->exe).c_str(), 0);
    pDebuggedEntry = GetPE32DataW(StringUtils::Utf8ToUtf16(init->exe).c_str(), 0, UE_OEP);
    strcpy_s(szFileName, init->exe);
    if(bFileIsDll)
        fdProcessInfo = (PROCESS_INFORMATION*)InitDLLDebugW(StringUtils::Utf8ToUtf16(init->exe).c_str(), false, StringUtils::Utf8ToUtf16(init->commandline).c_str(), StringUtils::Utf8ToUtf16(init->currentfolder).c_str(), 0);
    else
        fdProcessInfo = (PROCESS_INFORMATION*)InitDebugW(StringUtils::Utf8ToUtf16(init->exe).c_str(), StringUtils::Utf8ToUtf16(init->commandline).c_str(), StringUtils::Utf8ToUtf16(init->currentfolder).c_str());
    if(!fdProcessInfo)
    {
        fdProcessInfo = &g_pi;
        dputs("Error starting process (invalid pe?)!");
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
    strcpy_s(szBaseFileName, szFileName);
    int len = (int)strlen(szBaseFileName);
    while(szBaseFileName[len] != '\\' and len)
        len--;
    if(len)
        strcpy_s(szBaseFileName, szBaseFileName + len + 1);
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
    ModClear();
    ThreadClear();
    GuiSetDebugState(stopped);
    dputs("debugging stopped!");
    varset("$hp", (uint)0, true);
    varset("$pid", (uint)0, true);
    unlock(WAITID_STOP); //we are done
    return 0;
}

bool cbDeleteAllBreakpoints(const BREAKPOINT* bp)
{
    if(BpDelete(bp->addr, BPNORMAL) and (!bp->enabled or DeleteBPX(bp->addr)))
        return true;

    dprintf("Delete breakpoint failed: "fhex"\n", bp->addr);
    return false;
}

bool cbEnableAllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPNORMAL or bp->enabled)
        return true;

    if(!BpEnable(bp->addr, BPNORMAL, true) or !SetBPX(bp->addr, bp->titantype, (void*)cbUserBreakpoint))
    {
        dprintf("Could not enable breakpoint "fhex"\n", bp->addr);
        return false;
    }
    return true;
}

bool cbDisableAllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPNORMAL or !bp->enabled)
        return true;

    if(!BpEnable(bp->addr, BPNORMAL, false) or !DeleteBPX(bp->addr))
    {
        dprintf("Could not disable breakpoint "fhex"\n", bp->addr);
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
        dprintf("Did not enable hardware breakpoint "fhex" (all slots full)\n", bp->addr);
        return true;
    }
    int titantype = bp->titantype;
    TITANSETDRX(titantype, drx);
    BpSetTitanType(bp->addr, BPHARDWARE, titantype);
    if(!BpEnable(bp->addr, BPHARDWARE, true) or !SetHardwareBreakPoint(bp->addr, drx, TITANGETTYPE(bp->titantype), TITANGETSIZE(bp->titantype), (void*)cbHardwareBreakpoint))
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
    if(!BpEnable(bp->addr, BPHARDWARE, false) or !DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype)))
    {
        dprintf("Could not disable hardware breakpoint "fhex"\n", bp->addr);
        return false;
    }
    return true;
}

bool cbEnableAllMemoryBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPMEMORY or bp->enabled)
        return true;
    uint size = 0;
    MemFindBaseAddr(bp->addr, &size);
    if(!BpEnable(bp->addr, BPMEMORY, true) or !SetMemoryBPXEx(bp->addr, size, bp->titantype, !bp->singleshoot, (void*)cbMemoryBreakpoint))
    {
        dprintf("Could not enable memory breakpoint "fhex"\n", bp->addr);
        return false;
    }
    return true;
}

bool cbDisableAllMemoryBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPMEMORY or !bp->enabled)
        return true;
    if(!BpEnable(bp->addr, BPMEMORY, false) or !DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype)))
    {
        dprintf("Could not disable memory breakpoint "fhex"\n", bp->addr);
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
    MemFindBaseAddr(bp->addr, &size);
    if(!BpDelete(bp->addr, BPMEMORY) or !RemoveMemoryBPX(bp->addr, size))
    {
        dprintf("Delete memory breakpoint failed: "fhex"\n", bp->addr);
        return STATUS_ERROR;
    }
    return true;
}

bool cbDeleteAllHardwareBreakpoints(const BREAKPOINT* bp)
{
    if(!bp->enabled)
        return true;
    if(!BpDelete(bp->addr, BPHARDWARE) or !DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype)))
    {
        dprintf("Delete hardware breakpoint failed: "fhex"\n", bp->addr);
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
    ModClear();
    ThreadClear();
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
        dputs("Detached!");
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
    return !!IsAdminMember;
}

static bool readwritejitkey(wchar_t* jit_key_value, DWORD* jit_key_vale_size, char* key, arch arch_in, arch* arch_out, readwritejitkey_error_t* error, bool write)
{
    DWORD key_flags;
    DWORD lRv;
    HKEY hKey;
    DWORD dwDisposition;

    if(error != NULL)
        *error = ERROR_RW;

    if(write)
    {
        if(!IsProcessElevated())
        {
            if(error != NULL)
                *error = ERROR_RW_NOTADMIN;
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
#ifdef _WIN64
            *arch_out = x64;
#else //x86
            *arch_out = x32;
#endif //_WIN64
        }
        else
            *arch_out = arch_in;
    }

    if(arch_in == x64)
    {
#ifndef _WIN64
        if(!IsWow64())
        {
            if(error != NULL)
                *error = ERROR_RW_NOTWOW64;
            return false;
        }
        key_flags |= KEY_WOW64_64KEY;
#endif //_WIN64
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

        lRv = RegSetValueExW(hKey, StringUtils::Utf8ToUtf16(key).c_str(), 0, REG_SZ, (BYTE*)jit_key_value, (DWORD)(*jit_key_vale_size) + 1);
    }
    else
    {
        lRv = RegOpenKeyEx(HKEY_LOCAL_MACHINE, JIT_REG_KEY, 0, key_flags, &hKey);
        if(lRv != ERROR_SUCCESS)
        {
            if(error != NULL)
                *error = ERROR_RW_FILE_NOT_FOUND;
            return false;
        }

        lRv = RegQueryValueExW(hKey, StringUtils::Utf8ToUtf16(key).c_str(), 0, NULL, (LPBYTE)jit_key_value, jit_key_vale_size);
        if(lRv != ERROR_SUCCESS)
        {
            if(error != NULL)
                *error = ERROR_RW_FILE_NOT_FOUND;
        }
    }

    RegCloseKey(hKey);
    return (lRv == ERROR_SUCCESS);
}

bool dbgpagerightstostring(DWORD protect, char* rights)
{
    memset(rights, 0, RIGHTS_STRING_SIZE);

    switch(protect & 0xFF)
    {
    case PAGE_EXECUTE:
        strcpy_s(rights, RIGHTS_STRING_SIZE, "E---");
        break;
    case PAGE_EXECUTE_READ:
        strcpy_s(rights, RIGHTS_STRING_SIZE, "ER--");
        break;
    case PAGE_EXECUTE_READWRITE:
        strcpy_s(rights, RIGHTS_STRING_SIZE, "ERW-");
        break;
    case PAGE_EXECUTE_WRITECOPY:
        strcpy_s(rights, RIGHTS_STRING_SIZE, "ERWC");
        break;
    case PAGE_NOACCESS:
        strcpy_s(rights, RIGHTS_STRING_SIZE, "----");
        break;
    case PAGE_READONLY:
        strcpy_s(rights, RIGHTS_STRING_SIZE, "-R--");
        break;
    case PAGE_READWRITE:
        strcpy_s(rights, RIGHTS_STRING_SIZE, "-RW-");
        break;
    case PAGE_WRITECOPY:
        strcpy_s(rights, RIGHTS_STRING_SIZE, "-RWC");
        break;
    }

    if(protect & PAGE_GUARD)
        strcat_s(rights, RIGHTS_STRING_SIZE, "G");
    else
        strcat_s(rights, RIGHTS_STRING_SIZE, "-");

    return true;
}

static uint dbggetpageligned(uint addr)
{
#ifdef _WIN64
    addr &=  0xFFFFFFFFFFFFF000;
#else // _WIN32
    addr &= 0xFFFFF000;
#endif // _WIN64
    return addr;
}

static bool dbgpagerightsfromstring(DWORD* protect, const char* rights_string)
{
    if(strlen(rights_string) < 2)
        return false;

    *protect = 0;
    if(rights_string[0] == 'G' || rights_string[0] == 'g')
    {
        *protect |= PAGE_GUARD;
        rights_string++;
    }

    if(_strcmpi(rights_string, "Execute") == 0)
        *protect |= PAGE_EXECUTE;
    else if(_strcmpi(rights_string, "ExecuteRead") == 0)
        *protect |= PAGE_EXECUTE_READ;
    else if(_strcmpi(rights_string, "ExecuteReadWrite") == 0)
        *protect |= PAGE_EXECUTE_READWRITE;
    else if(_strcmpi(rights_string, "ExecuteWriteCopy") == 0)
        *protect |= PAGE_EXECUTE_WRITECOPY;
    else if(_strcmpi(rights_string, "NoAccess") == 0)
        *protect |= PAGE_NOACCESS;
    else if(_strcmpi(rights_string, "ReadOnly") == 0)
        *protect |= PAGE_READONLY;
    else if(_strcmpi(rights_string, "ReadWrite") == 0)
        *protect |= PAGE_READWRITE;
    else if(_strcmpi(rights_string, "WriteCopy") == 0)
        *protect |= PAGE_WRITECOPY;

    if(*protect == 0)
        return false;

    return true;
}

bool dbgsetpagerights(uint addr, const char* rights_string)
{
    DWORD protect;
    DWORD old_protect;

    addr = dbggetpageligned(addr);

    if(!dbgpagerightsfromstring(& protect, rights_string))
        return false;

    if(VirtualProtectEx(fdProcessInfo->hProcess, (void*)addr, PAGE_SIZE, protect, & old_protect) == 0)
        return false;

    return true;
}

bool dbggetpagerights(uint addr, char* rights)
{
    addr = dbggetpageligned(addr);

    MEMORY_BASIC_INFORMATION mbi;
    if(VirtualQueryEx(fdProcessInfo->hProcess, (const void*)addr, &mbi, sizeof(mbi)) == 0)
        return false;

    return dbgpagerightstostring(mbi.Protect, rights);
}

bool dbggetjitauto(bool* auto_on, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out)
{
    wchar_t jit_entry[4] = L"";
    DWORD jit_entry_size = sizeof(jit_entry) - 1;
    readwritejitkey_error_t rw_error;

    if(!readwritejitkey(jit_entry, &jit_entry_size, "Auto", arch_in, arch_out, &rw_error, false))
    {
        if(rw_error == ERROR_RW_FILE_NOT_FOUND)
        {
            if(rw_error_out != NULL)
                *rw_error_out = rw_error;
            return true;
        }
        return false;
    }
    if(_wcsicmp(jit_entry, L"1") == 0)
        *auto_on = true;
    else if(_wcsicmp(jit_entry, L"0") == 0)
        *auto_on = false;
    else
        return false;
    return true;
}

bool dbgsetjitauto(bool auto_on, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out)
{
    DWORD auto_string_size = sizeof(L"1");
    readwritejitkey_error_t rw_error;
    if(!auto_on)
    {
        wchar_t jit_entry[4] = L"";
        DWORD jit_entry_size = sizeof(jit_entry) - 1;
        if(!readwritejitkey(jit_entry, &jit_entry_size, "Auto", arch_in, arch_out, &rw_error, false))
        {
            if(rw_error == ERROR_RW_FILE_NOT_FOUND)
                return true;
        }
    }
    if(!readwritejitkey(auto_on ? L"1" : L"0", &auto_string_size, "Auto", arch_in, arch_out, &rw_error, true))
    {
        if(rw_error_out != NULL)
            *rw_error_out = rw_error;
        return false;
    }
    return true;
}

bool dbggetjit(char jit_entry[JIT_ENTRY_MAX_SIZE], arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out)
{
    wchar_t wszJitEntry[JIT_ENTRY_MAX_SIZE] = L"";
    DWORD jit_entry_size = JIT_ENTRY_MAX_SIZE * sizeof(wchar_t);
    readwritejitkey_error_t rw_error;
    if(!readwritejitkey(wszJitEntry, &jit_entry_size, "Debugger", arch_in, arch_out, &rw_error, false))
    {
        if(rw_error_out != NULL)
            *rw_error_out = rw_error;
        return false;
    }
    strcpy_s(jit_entry, JIT_ENTRY_MAX_SIZE, StringUtils::Utf16ToUtf8(wszJitEntry).c_str());
    return true;
}

bool dbggetdefjit(char* jit_entry)
{
    char path[JIT_ENTRY_DEF_SIZE];
    path[0] = '"';
    wchar_t wszPath[MAX_PATH] = L"";
    GetModuleFileNameW(GetModuleHandleW(NULL), wszPath, MAX_PATH);
    strcpy_s(&path[1], JIT_ENTRY_DEF_SIZE - 1, StringUtils::Utf16ToUtf8(wszPath).c_str());
    strcat(path, ATTACH_CMD_LINE);
    strcpy_s(jit_entry, JIT_ENTRY_DEF_SIZE, path);
    return true;
}

bool dbgsetjit(char* jit_cmd, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out)
{
    DWORD jit_cmd_size = (DWORD)strlen(jit_cmd) * sizeof(wchar_t);
    readwritejitkey_error_t rw_error;
    if(!readwritejitkey((wchar_t*)StringUtils::Utf8ToUtf16(jit_cmd).c_str(), & jit_cmd_size, "Debugger", arch_in, arch_out, & rw_error, true))
    {
        if(rw_error_out != NULL)
            *rw_error_out = rw_error;
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
        wchar_t szExePath[MAX_PATH] = L"";
        if(GetModuleFileNameExW(hProcess, 0, szExePath, MAX_PATH))
            strcpy_s(pe32.szExeFile, StringUtils::Utf16ToUtf8(szExePath).c_str());
        list->push_back(pe32);
    }
    while(Process32Next(hProcessSnap, &pe32));
    return true;
}

static bool getcommandlineaddr(uint* addr, cmdline_error_t* cmd_line_error)
{
    SIZE_T size;
    uint pprocess_parameters;

    cmd_line_error->addr = (uint)GetPEBLocation(fdProcessInfo->hProcess);

    if(cmd_line_error->addr == 0)
    {
        cmd_line_error->type = CMDL_ERR_GET_PEB;
        return false;
    }

    //cast-trick to calculate the address of the remote peb field ProcessParameters
    cmd_line_error->addr = (uint) & (((PPEB) cmd_line_error->addr)->ProcessParameters);
    if(!MemRead((void*)cmd_line_error->addr, &pprocess_parameters, sizeof(pprocess_parameters), &size))
    {
        cmd_line_error->type = CMDL_ERR_READ_PEBBASE;
        return false;
    }

    *addr = (uint) & (((RTL_USER_PROCESS_PARAMETERS*) pprocess_parameters)->CommandLine);
    return true;
}

static bool patchcmdline(uint getcommandline, uint new_command_line, cmdline_error_t* cmd_line_error)
{
    uint command_line_stored = 0;
    uint aux = 0;
    SIZE_T size;
    unsigned char data[100];

    cmd_line_error->addr = getcommandline;
    if(!MemRead((void*) cmd_line_error->addr, & data, sizeof(data), & size))
    {
        cmd_line_error->type = CMDL_ERR_READ_GETCOMMANDLINEBASE;
        return false;
    }

#ifdef _WIN64
    /*
    00007FFC5B91E3C8 | 48 8B 05 19 1D 0E 00     | mov rax,qword ptr ds:[7FFC5BA000E8]
    00007FFC5B91E3CF | C3                       | ret                                     |
    This is a relative offset then to get the symbol: next instruction of getmodulehandle (+7 bytes) + offset to symbol
    (the last 4 bytes of the instruction)
    */
    if(data[0] != 0x48 ||  data[1] != 0x8B || data[2] != 0x05 || data[7] != 0xC3)
    {
        cmd_line_error->type = CMDL_ERR_CHECK_GETCOMMANDLINESTORED;
        return false;
    }
    DWORD offset = * ((DWORD*) & data[3]);
    command_line_stored = getcommandline + 7 + offset;
#else //x86
    /*
    750FE9CA | A1 CC DB 1A 75           | mov eax,dword ptr ds:[751ADBCC]         |
    750FE9CF | C3                       | ret                                     |
    */
    if(data[0] != 0xA1 ||  data[5] != 0xC3)
    {
        cmd_line_error->type = CMDL_ERR_CHECK_GETCOMMANDLINESTORED;
        return false;
    }
    command_line_stored = * ((uint*) & data[1]);
#endif

    //update the pointer in the debuggee
    if(!MemWrite((void*)command_line_stored, &new_command_line, sizeof(new_command_line), &size))
    {
        cmd_line_error->addr = command_line_stored;
        cmd_line_error->type = CMDL_ERR_WRITE_GETCOMMANDLINESTORED;
        return false;
    }

    return true;
}

static bool fixgetcommandlinesbase(uint new_command_line_unicode, uint new_command_line_ascii, cmdline_error_t* cmd_line_error)
{
    uint getcommandline;

    if(!valfromstring("kernelbase:GetCommandLineA", &getcommandline))
    {
        if(!valfromstring("kernel32:GetCommandLineA", &getcommandline))
        {
            cmd_line_error->type = CMDL_ERR_GET_GETCOMMANDLINE;
            return false;
        }
    }
    if(!patchcmdline(getcommandline, new_command_line_ascii, cmd_line_error))
        return false;

    if(!valfromstring("kernelbase:GetCommandLineW", &getcommandline))
    {
        if(!valfromstring("kernel32:GetCommandLineW", &getcommandline))
        {
            cmd_line_error->type = CMDL_ERR_GET_GETCOMMANDLINE;
            return false;
        }
    }
    if(!patchcmdline(getcommandline, new_command_line_unicode, cmd_line_error))
        return false;

    return true;
}

bool dbgsetcmdline(const char* cmd_line, cmdline_error_t* cmd_line_error)
{
    cmdline_error_t cmd_line_error_aux;
    UNICODE_STRING new_command_line;
    SIZE_T size;
    uint command_line_addr;

    if(cmd_line_error == NULL)
        cmd_line_error = &cmd_line_error_aux;

    if(!getcommandlineaddr(&cmd_line_error->addr, cmd_line_error))
        return false;

    command_line_addr = cmd_line_error->addr;

    SIZE_T cmd_line_size =  strlen(cmd_line);
    new_command_line.Length = (USHORT)(strlen(cmd_line) + 1) * sizeof(WCHAR);
    new_command_line.MaximumLength = new_command_line.Length;

    Memory<wchar_t*> command_linewstr(new_command_line.Length);

    // Covert to Unicode.
    if(!MultiByteToWideChar(CP_UTF8, 0, cmd_line, (int)cmd_line_size + 1, command_linewstr, (int)cmd_line_size + 1))
    {
        cmd_line_error->type = CMDL_ERR_CONVERTUNICODE;
        return false;
    }

    new_command_line.Buffer = command_linewstr;

    uint mem = (uint)MemAllocRemote(0, new_command_line.Length * 2, PAGE_READWRITE);
    if(!mem)
    {
        cmd_line_error->type = CMDL_ERR_ALLOC_UNICODEANSI_COMMANDLINE;
        return false;
    }

    if(!MemWrite((void*)mem, new_command_line.Buffer, new_command_line.Length, &size))
    {
        cmd_line_error->addr = mem;
        cmd_line_error->type = CMDL_ERR_WRITE_UNICODE_COMMANDLINE;
        return false;
    }

    if(!MemWrite((void*)(mem + new_command_line.Length), (void *)cmd_line, strlen(cmd_line) + 1, &size))
    {
        cmd_line_error->addr = mem + new_command_line.Length;
        cmd_line_error->type = CMDL_ERR_WRITE_ANSI_COMMANDLINE;
        return false;
    }

    if(!fixgetcommandlinesbase(mem, mem + new_command_line.Length, cmd_line_error))
        return false;

    new_command_line.Buffer = (PWSTR) mem;
    if(!MemWrite((void*)command_line_addr, &new_command_line, sizeof(new_command_line), &size))
    {
        cmd_line_error->addr = command_line_addr;
        cmd_line_error->type = CMDL_ERR_WRITE_PEBUNICODE_COMMANDLINE;
        return false;
    }

    return true;
}

bool dbggetcmdline(char** cmd_line, cmdline_error_t* cmd_line_error)
{
    SIZE_T size;
    UNICODE_STRING CommandLine;
    cmdline_error_t cmd_line_error_aux;

    if(cmd_line_error == NULL)
        cmd_line_error = &cmd_line_error_aux;

    if(!getcommandlineaddr(&cmd_line_error->addr, cmd_line_error))
        return false;

    if(!MemRead((void*)cmd_line_error->addr, &CommandLine, sizeof(CommandLine), &size))
    {
        cmd_line_error->type = CMDL_ERR_READ_PROCPARM_PTR;
        return false;
    }

    Memory<wchar_t*> wstr_cmd(CommandLine.Length + sizeof(wchar_t));

    cmd_line_error->addr = (uint) CommandLine.Buffer;
    if(!MemRead((void*)cmd_line_error->addr, wstr_cmd, CommandLine.Length, &size))
    {
        cmd_line_error->type = CMDL_ERR_READ_PROCPARM_CMDLINE;
        return false;
    }

    SIZE_T wstr_cmd_size = wcslen(wstr_cmd) + 1;
    SIZE_T cmd_line_size = wstr_cmd_size * 2;

    *cmd_line = (char*)emalloc(cmd_line_size, "dbggetcmdline:cmd_line");

    //Convert TO UTF-8
    if(!WideCharToMultiByte(CP_UTF8, 0, wstr_cmd, (int)wstr_cmd_size, * cmd_line, (int)cmd_line_size, NULL, NULL))
    {
        efree(*cmd_line);
        cmd_line_error->type = CMDL_ERR_CONVERTUNICODE;
        return false;
    }
    return true;
}