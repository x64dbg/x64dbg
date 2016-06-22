/**
 @file debugger.cpp

 @brief Implements the debugger class.
 */

#include "debugger.h"
#include "console.h"
#include "memory.h"
#include "threading.h"
#include "command.h"
#include "database.h"
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
#include "commandline.h"
#include "stackinfo.h"
#include "stringformat.h"
#include "TraceRecord.h"

struct TraceCondition
{
    ExpressionParser condition;
    duint steps;
    duint maxSteps;

    explicit TraceCondition(String expression, duint maxCount)
        : condition(expression), steps(0), maxSteps(maxCount) {}

    bool ContinueTrace()
    {
        steps++;
        if(steps >= maxSteps)
            return false;
        duint value = 1;
        return condition.Calculate(value, valuesignedcalc()) && !value;
    }
};

static PROCESS_INFORMATION g_pi = {0, 0, 0, 0};
static char szBaseFileName[MAX_PATH] = "";
static TraceCondition* traceCondition = nullptr;
static bool bFileIsDll = false;
static duint pDebuggedBase = 0;
static duint pCreateProcessBase = 0;
static duint pDebuggedEntry = 0;
static bool isStepping = false;
static bool isPausedByUser = false;
static bool isDetachedByUser = false;
static bool bIsAttached = false;
static bool bSkipExceptions = false;
static bool bBreakOnNextDll = false;
static bool bFreezeStack = false;
static int ecount = 0;
static std::vector<ExceptionRange> ignoredExceptionRange;
static HANDLE hEvent = 0;
static HANDLE hProcess = 0;
static HANDLE hMemMapThread = 0;
static bool bStopMemMapThread = false;
static HANDLE hTimeWastedCounterThread = 0;
static bool bStopTimeWastedCounterThread = false;
static String lastDebugText;
static duint timeWastedDebugging = 0;
char szFileName[MAX_PATH] = "";
char szSymbolCachePath[MAX_PATH] = "";
char sqlitedb[deflen] = "";
std::vector<std::pair<duint, duint>> RunToUserCodeBreakpoints;
PROCESS_INFORMATION* fdProcessInfo = &g_pi;
HANDLE hActiveThread;
HANDLE hProcessToken;
bool bUndecorateSymbolNames = true;
bool bEnableSourceDebugging = true;
duint DbgEvents = 0;

static duint dbgcleartracecondition()
{
    duint steps = 0;
    if(traceCondition)
    {
        steps = traceCondition->steps;
        delete traceCondition;
    }
    traceCondition = nullptr;
    return steps;
}

bool dbgsettracecondition(String expression, duint maxSteps)
{
    if(dbgtraceactive())
        return false;
    traceCondition = new TraceCondition(expression, maxSteps);
    if(traceCondition->condition.IsValidExpression())
        return true;
    dbgcleartracecondition();
    return false;
}

bool dbgtraceactive()
{
    return traceCondition != nullptr;
}

static DWORD WINAPI memMapThread(void* ptr)
{
    while(!bStopMemMapThread)
    {
        while(!DbgIsDebugging())
        {
            if(bStopMemMapThread)
                break;
            Sleep(1);
        }

        // Execute the update only if the delta if >= 1 second
        if((GetTickCount() - memMapThreadCounter) >= 1000)
        {
            MemUpdateMap();
            GuiUpdateMemoryView();

            memMapThreadCounter = GetTickCount();
        }

        Sleep(50);
    }

    return 0;
}

static DWORD WINAPI timeWastedCounterThread(void* ptr)
{
    if(!BridgeSettingGetUint("Engine", "TimeWastedDebugging", &timeWastedDebugging))
        timeWastedDebugging = 0;
    GuiUpdateTimeWastedCounter();
    while(!bStopTimeWastedCounterThread)
    {
        while(!DbgIsDebugging())
        {
            if(bStopTimeWastedCounterThread)
                break;
            Sleep(1);
        }
        if(bStopTimeWastedCounterThread)
            break;
        timeWastedDebugging++;
        GuiUpdateTimeWastedCounter();
        Sleep(1000);
    }
    BridgeSettingSetUint("Engine", "TimeWastedDebugging", timeWastedDebugging);
    return 0;
}

void dbginit()
{
    hMemMapThread = CreateThread(nullptr, 0, memMapThread, nullptr, 0, nullptr);
    hTimeWastedCounterThread = CreateThread(nullptr, 0, timeWastedCounterThread, nullptr, 0, nullptr);
}

void dbgstop()
{
    bStopMemMapThread = true;
    memMapThreadCounter = 0;
    bStopTimeWastedCounterThread = true;
    WaitForThreadTermination(hMemMapThread);
    WaitForThreadTermination(hTimeWastedCounterThread);
}

duint dbgdebuggedbase()
{
    return pDebuggedBase;
}

duint dbggettimewastedcounter()
{
    return timeWastedDebugging;
}

bool dbgisrunning()
{
    return !waitislocked(WAITID_RUN);
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

void dbgsetfreezestack(bool freeze)
{
    bFreezeStack = freeze;
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
    if(!cmdnew(name, cbCommand, debugonly))
        return false;
    GuiAutoCompleteAddCmd(name);
    return true;
}

bool dbgcmddel(const char* name)
{
    if(!cmddel(name))
        return false;
    GuiAutoCompleteDelCmd(name);
    return true;
}

duint dbggetdbgevents()
{
    return InterlockedExchange(&DbgEvents, 0);
}

DWORD WINAPI updateCallStackThread(void* ptr)
{
    stackupdatecallstack(duint(ptr));
    GuiUpdateCallStack();
    return 0;
}

DWORD WINAPI updateSEHChainThread(void* ptr)
{
    GuiUpdateSEHChain();
    stackupdateseh();
    GuiUpdateDumpView();
    return 0;
}

void DebugUpdateGui(duint disasm_addr, bool stack)
{
    if(GuiIsUpdateDisabled())
        return;
    duint cip = GetContextDataEx(hActiveThread, UE_CIP);
    if(MemIsValidReadPtr(disasm_addr))
    {
        if(bEnableSourceDebugging)
        {
            char szSourceFile[MAX_STRING_SIZE] = "";
            int line = 0;
            if(SymGetSourceLine(cip, szSourceFile, &line))
                GuiLoadSourceFile(szSourceFile, line);
        }
        GuiDisasmAt(disasm_addr, cip);
    }
    duint csp = GetContextDataEx(hActiveThread, UE_CSP);
    if(stack)
        DebugUpdateStack(csp, csp);
    static duint cacheCsp = 0;
    if(csp != cacheCsp)
    {
        InterlockedExchange(&cacheCsp, csp);
        CloseHandle(CreateThread(nullptr, 0, updateCallStackThread, LPVOID(csp), 0, nullptr));
        CloseHandle(CreateThread(nullptr, 0, updateSEHChainThread, nullptr, 0, nullptr));
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
    GuiFocusView(GUI_DISASSEMBLY);
}

void DebugUpdateStack(duint dumpAddr, duint csp, bool forceDump)
{
    if(GuiIsUpdateDisabled())
        return;
    if(!forceDump && bFreezeStack)
    {
        SELECTIONDATA selection;
        if(GuiSelectionGet(GUI_STACK, &selection))
            dumpAddr = selection.start;
    }
    GuiStackDumpAt(dumpAddr, csp);
}

static void printSoftBpInfo(const BREAKPOINT & bp)
{
    auto bptype = "INT3";
    int titantype = bp.titantype;
    if((titantype & UE_BREAKPOINT_TYPE_UD2) == UE_BREAKPOINT_TYPE_UD2)
        bptype = "UD2";
    else if((titantype & UE_BREAKPOINT_TYPE_LONG_INT3) == UE_BREAKPOINT_TYPE_LONG_INT3)
        bptype = "LONG INT3";
    auto symbolicname = SymGetSymbolicName(bp.addr);
    if(symbolicname.length())
    {
        if(*bp.name)
            dprintf("%s breakpoint \"%s\" at %s (" fhex ")!\n", bptype, bp.name, symbolicname.c_str(), bp.addr);
        else
            dprintf("%s breakpoint at %s (" fhex ")!\n", bptype, symbolicname.c_str(), bp.addr);
    }
    else
    {
        if(*bp.name)
            dprintf("%s breakpoint \"%s\" at " fhex "!\n", bptype, bp.name, bp.addr);
        else
            dprintf("%s breakpoint at " fhex "!\n", bptype, bp.addr);
    }
}

static void printHwBpInfo(const BREAKPOINT & bp)
{
    auto bpsize = "";
    switch(TITANGETSIZE(bp.titantype))   //size
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
    auto bptype = "";
    switch(TITANGETTYPE(bp.titantype))   //type
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
    auto symbolicname = SymGetSymbolicName(bp.addr);
    if(symbolicname.length())
    {
        if(*bp.name)
            dprintf("Hardware breakpoint (%s%s) \"%s\" at %s (" fhex ")!\n", bpsize, bptype, bp.name, symbolicname.c_str(), bp.addr);
        else
            dprintf("Hardware breakpoint (%s%s) at %s (" fhex ")!\n", bpsize, bptype, symbolicname.c_str(), bp.addr);
    }
    else
    {
        if(*bp.name)
            dprintf("Hardware breakpoint (%s%s) \"%s\" at " fhex "!\n", bpsize, bptype, bp.name, bp.addr);
        else
            dprintf("Hardware breakpoint (%s%s) at " fhex "!\n", bpsize, bptype, bp.addr);
    }
}

static void printMemBpInfo(const BREAKPOINT & bp, const void* ExceptionAddress)
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
    auto symbolicname = SymGetSymbolicName(bp.addr);
    if(symbolicname.length())
    {
        if(*bp.name)
            dprintf("Memory breakpoint%s \"%s\" at %s (" fhex ", " fhex ")!\n", bptype, bp.name, symbolicname.c_str(), bp.addr, ExceptionAddress);
        else
            dprintf("Memory breakpoint%s at %s (" fhex ", " fhex ")!\n", bptype, symbolicname.c_str(), bp.addr, ExceptionAddress);
    }
    else
    {
        if(*bp.name)
            dprintf("Memory breakpoint%s \"%s\" at " fhex " (" fhex ")!\n", bptype, bp.name, bp.addr, ExceptionAddress);
        else
            dprintf("Memory breakpoint%s at " fhex " (" fhex ")!\n", bptype, bp.addr, ExceptionAddress);
    }
}

static bool getConditionValue(const char* expression)
{
    auto word = *(uint16*)expression;
    if(word == '0')  // short circuit for condition "0\0"
        return false;
    if(word == '1')  //short circuit for condition "1\0"
        return true;
    duint value;
    if(valfromstring(expression, &value))
        return value != 0;
    return true;
}

void cbPauseBreakpoint()
{
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    auto CIP = GetContextDataEx(hActiveThread, UE_CIP);
    DeleteBPX(CIP);
    GuiSetDebugState(paused);
    DebugUpdateGui(CIP, true);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved = nullptr;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    wait(WAITID_RUN);
}

static void cbGenericBreakpoint(BP_TYPE bptype, void* ExceptionAddress = nullptr)
{
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    auto CIP = GetContextDataEx(hActiveThread, UE_CIP);
    BREAKPOINT* bpPtr = nullptr;
    SHARED_ACQUIRE(LockBreakpoints);
    switch(bptype)
    {
    case BPNORMAL:
        bpPtr = BpInfoFromAddr(bptype, CIP);
        break;
    case BPHARDWARE:
        bpPtr = BpInfoFromAddr(bptype, duint(ExceptionAddress));
        break;
    case BPMEMORY:
        bpPtr = BpInfoFromAddr(bptype, MemFindBaseAddr(duint(ExceptionAddress), nullptr, true));
    default:
        break;
    }
    if(!(bpPtr && bpPtr->enabled))  //invalid / disabled breakpoint hit (most likely a bug)
    {
        SHARED_RELEASE();
        dputs("Breakpoint reached not in list!");
        GuiSetDebugState(paused);
        DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), true);
        //lock
        lock(WAITID_RUN);
        SetForegroundWindow(GuiGetWindowHandle());
        bSkipExceptions = false;
        PLUG_CB_PAUSEDEBUG pauseInfo;
        pauseInfo.reserved = nullptr;
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        wait(WAITID_RUN);
        return;
    }

    // increment hit count
    InterlockedIncrement(&bpPtr->hitcount);

    auto bp = *bpPtr;
    SHARED_RELEASE();
    bp.addr += ModBaseFromAddr(CIP);
    bp.active = true; //a breakpoint that has been hit is active

    //get condition values
    bool breakCondition;
    bool logCondition;
    bool commandCondition;
    if(*bp.breakCondition)
        breakCondition = getConditionValue(bp.breakCondition);
    else
        breakCondition = true; //break if no condition is set
    if(bp.fastResume && !breakCondition)  // fast resume: ignore GUI/Script/Plugin/Other if the debugger would not break
        return;
    if(*bp.logCondition)
        logCondition = getConditionValue(bp.logCondition);
    else
        logCondition = true; //log if no condition is set
    if(*bp.commandCondition)
        commandCondition = getConditionValue(bp.commandCondition);
    else
        commandCondition = breakCondition; //if no condition is set, execute the command when the debugger would break

    lock(WAITID_RUN);
    if(breakCondition)
    {
        if(bp.singleshoot)
            BpDelete(bp.addr, bptype);
        switch(bptype)
        {
        case BPNORMAL:
            printSoftBpInfo(bp);
            break;
        case BPHARDWARE:
            printHwBpInfo(bp);
            break;
        case BPMEMORY:
            printMemBpInfo(bp, ExceptionAddress);
            break;
        default:
            break;
        }
        GuiSetDebugState(paused);
        DebugUpdateGui(CIP, true);
    }

    // plugin interaction
    if(breakCondition)
    {
        PLUG_CB_PAUSEDEBUG pauseInfo;
        pauseInfo.reserved = nullptr;
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    }
    PLUG_CB_BREAKPOINT bpInfo;
    BRIDGEBP bridgebp;
    memset(&bridgebp, 0, sizeof(bridgebp));
    bpInfo.breakpoint = &bridgebp;
    BpToBridge(&bp, &bridgebp);
    plugincbcall(CB_BREAKPOINT, &bpInfo);

    // Trace record
    _dbg_dbgtraceexecute(CIP);

    if(*bp.logText && logCondition)  //log
    {
        dprintf("%s\n", stringformatinline(bp.logText).c_str());
    }
    if(*bp.commandText && commandCondition)  //command
    {
        //TODO: commands like run/step etc will fuck up your shit
        DbgCmdExec(bp.commandText);
    }
    if(breakCondition)  //break the debugger
    {
        dbgcleartracecondition();
        SetForegroundWindow(GuiGetWindowHandle());
        bSkipExceptions = false;
    }
    else //resume immediately
        unlock(WAITID_RUN);

    //wait until the user resumes
    wait(WAITID_RUN);
}

void cbUserBreakpoint()
{
    cbGenericBreakpoint(BPNORMAL);
}

void cbHardwareBreakpoint(void* ExceptionAddress)
{
    cbGenericBreakpoint(BPHARDWARE, ExceptionAddress);
}

void cbMemoryBreakpoint(void* ExceptionAddress)
{
    cbGenericBreakpoint(BPMEMORY, ExceptionAddress);
}

void cbRunToUserCodeBreakpoint(void* ExceptionAddress)
{
    EXCLUSIVE_ACQUIRE(LockRunToUserCode);
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    auto CIP = GetContextDataEx(hActiveThread, UE_CIP);
    auto symbolicname = SymGetSymbolicName(CIP);
    dprintf("User code reached at %s (" fhex ")!", symbolicname.c_str(), CIP);
    for(auto i : RunToUserCodeBreakpoints)
    {
        BREAKPOINT bp;
        if(!BpGet(i.first, BPMEMORY, nullptr, &bp))
            RemoveMemoryBPX(i.first, i.second);
    }
    RunToUserCodeBreakpoints.clear();
    lock(WAITID_RUN);
    EXCLUSIVE_RELEASE();
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved = nullptr;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    _dbg_dbgtraceexecute(CIP);
    GuiSetDebugState(paused);
    DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), true);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions = false;
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
        auto strText = StringUtils::Utf16ToUtf8((const wchar_t*)evt->desc);
        const char* text = strText.c_str();
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
            strcpy_s(newtext(), len + 1, text);
            strstr(newtext(), " bytes -  ")[8] = 0;
            GuiSymbolLogAdd(newtext());
            suspress = true;
        }
        else if(strstr(text, " copied         "))
        {
            GuiSymbolSetProgress(100);
            GuiSymbolLogAdd(" downloaded!\n");
            suspress = true;
            zerobar = true;
        }
        else if(sscanf(text, "%*s %d percent", &percent) == 1 || sscanf(text, "%d percent", &percent) == 1)
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

bool cbSetModuleBreakpoints(const BREAKPOINT* bp)
{
    if(!bp->enabled)
        return true;
    switch(bp->type)
    {
    case BPNORMAL:
    {
        unsigned short oldbytes;
        if(MemRead(bp->addr, &oldbytes, sizeof(oldbytes)))
        {
            if(oldbytes != bp->oldbytes)
            {
                dprintf("Breakpoint " fhex " has been disabled because the bytes don't match! Expected: %02X %02X, Found: %02X %02X\n",
                        bp->addr,
                        ((unsigned char*)&bp->oldbytes)[0], ((unsigned char*)&bp->oldbytes)[1],
                        ((unsigned char*)&oldbytes)[0], ((unsigned char*)&oldbytes)[1]);
                BpEnable(bp->addr, BPNORMAL, false);
            }
            else if(!SetBPX(bp->addr, bp->titantype, (void*)cbUserBreakpoint))
                dprintf("Could not set breakpoint " fhex "! (SetBPX)\n", bp->addr);
        }
        else
            dprintf("MemRead failed on breakpoint address" fhex "!\n", bp->addr);
    }
    break;

    case BPMEMORY:
    {
        duint size = 0;
        MemFindBaseAddr(bp->addr, &size);
        if(!SetMemoryBPXEx(bp->addr, size, bp->titantype, !bp->singleshoot, (void*)cbMemoryBreakpoint))
            dprintf("Could not set memory breakpoint " fhex "! (SetMemoryBPXEx)\n", bp->addr);
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
            dprintf("Could not set hardware breakpoint " fhex "! (SetHardwareBreakPoint)\n", bp->addr);
        else
            dprintf("Set hardware breakpoint on " fhex "!\n", bp->addr);
    }
    break;

    default:
        break;
    }
    return true;
}

static bool cbRemoveModuleBreakpoints(const BREAKPOINT* bp)
{
    if(!bp->enabled)
        return true;
    switch(bp->type)
    {
    case BPNORMAL:
        if(!DeleteBPX(bp->addr))
            dprintf("Could not delete breakpoint " fhex "! (DeleteBPX)\n", bp->addr);
        break;
    case BPMEMORY:
        if(!RemoveMemoryBPX(bp->addr, 0))
            dprintf("Could not delete memory breakpoint " fhex "! (RemoveMemoryBPX)\n", bp->addr);
        break;
    case BPHARDWARE:
        if(!DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype)))
            dprintf("Could not delete hardware breakpoint " fhex "! (DeleteHardwareBreakPoint)\n", bp->addr);
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
    duint CIP = GetContextDataEx(hActiveThread, UE_CIP);
    DebugUpdateGui(CIP, true);
    // Trace record
    _dbg_dbgtraceexecute(CIP);
    // Plugin interaction
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
    dbgcleartracecondition();
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    GuiSetDebugState(paused);
    duint CIP = GetContextDataEx(hActiveThread, UE_CIP);
    // Trace record
    _dbg_dbgtraceexecute(CIP);
    DebugUpdateGui(CIP, true);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions = false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved = 0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    wait(WAITID_RUN);
}

void cbRtrStep()
{
    unsigned char ch = 0x90;
    duint cip = GetContextDataEx(hActiveThread, UE_CIP);
    MemRead(cip, &ch, 1);
    if(ch == 0xC3 || ch == 0xC2)
        cbRtrFinalStep();
    else
        StepOver((void*)cbRtrStep);
}

void cbTOCNDStep()
{
    if(traceCondition && traceCondition->ContinueTrace())
        StepOver((void*)cbTOCNDStep);
    else
    {
        auto steps = dbgcleartracecondition();
        dprintf("Trace finished after %" fext "u steps!\n", steps);
        cbRtrFinalStep();
    }
}

void cbTICNDStep()
{
    if(traceCondition && traceCondition->ContinueTrace())
        StepInto((void*)cbTICNDStep);
    else
    {
        auto steps = dbgcleartracecondition();
        dprintf("Trace finished after %" fext "u steps!\n", steps);
        cbRtrFinalStep();
    }
}

void cbTIBTStep()
{
    // Trace record
    duint CIP = GetContextDataEx(hActiveThread, UE_CIP);
    if(!traceCondition)
    {
        _dbg_dbgtraceexecute(CIP);
        dprintf("Bad tracing state.\n");
        cbRtrFinalStep();
        return;
    }
    if((TraceRecord.getTraceRecordType(CIP) != TraceRecordManager::TraceRecordNone && TraceRecord.getHitCount(CIP) == 0) || !traceCondition->ContinueTrace())
    {
        _dbg_dbgtraceexecute(CIP);
        auto steps = dbgcleartracecondition();
        dprintf("Trace finished after %" fext "u steps!\n", steps);
        cbRtrFinalStep();
        return;
    }
    StepInto((void*)cbTIBTStep);
}

void cbTOBTStep()
{
    // Trace record
    duint CIP = GetContextDataEx(hActiveThread, UE_CIP);
    if(!traceCondition)
    {
        _dbg_dbgtraceexecute(CIP);
        dprintf("Bad tracing state.\n");
        cbRtrFinalStep();
        return;
    }
    if((TraceRecord.getTraceRecordType(CIP) != TraceRecordManager::TraceRecordNone && TraceRecord.getHitCount(CIP) == 0) || !traceCondition->ContinueTrace())
    {
        _dbg_dbgtraceexecute(CIP);
        auto steps = dbgcleartracecondition();
        dprintf("Trace finished after %" fext "u steps!\n", steps);
        cbRtrFinalStep();
        return;
    }
    StepOver((void*)cbTOBTStep);
}

void cbTIITStep()
{
    // Trace record
    duint CIP = GetContextDataEx(hActiveThread, UE_CIP);
    if(!traceCondition)
    {
        _dbg_dbgtraceexecute(CIP);
        dprintf("Bad tracing state.\n");
        cbRtrFinalStep();
        return;
    }
    if((TraceRecord.getTraceRecordType(CIP) != TraceRecordManager::TraceRecordNone && TraceRecord.getHitCount(CIP) != 0) || !traceCondition->ContinueTrace())
    {
        _dbg_dbgtraceexecute(CIP);
        auto steps = dbgcleartracecondition();
        dprintf("Trace finished after %" fext "u steps!\n", steps);
        cbRtrFinalStep();
        return;
    }
    StepInto((void*)cbTIITStep);
}

void cbTOITStep()
{
    // Trace record
    duint CIP = GetContextDataEx(hActiveThread, UE_CIP);
    if(!traceCondition)
    {
        _dbg_dbgtraceexecute(CIP);
        dprintf("Bad tracing state.\n");
        cbRtrFinalStep();
        return;
    }
    if((TraceRecord.getTraceRecordType(CIP) != TraceRecordManager::TraceRecordNone && TraceRecord.getHitCount(CIP) != 0) || !traceCondition->ContinueTrace())
    {
        _dbg_dbgtraceexecute(CIP);
        auto steps = dbgcleartracecondition();
        dprintf("Trace finished after %" fext "u steps!\n", steps);
        cbRtrFinalStep();
        return;
    }
    StepOver((void*)cbTOITStep);
}

static void cbCreateProcess(CREATE_PROCESS_DEBUG_INFO* CreateProcessInfo)
{
    void* base = CreateProcessInfo->lpBaseOfImage;

    char DebugFileName[deflen] = "";
    if(!GetFileNameFromHandle(CreateProcessInfo->hFile, DebugFileName) && !GetFileNameFromProcessHandle(CreateProcessInfo->hProcess, DebugFileName))
        strcpy_s(DebugFileName, "??? (GetFileNameFromHandle failed)");
    dprintf("Process Started: " fhex " %s\n", base, DebugFileName);

    //update memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    GuiDumpAt(MemFindBaseAddr(GetContextDataEx(CreateProcessInfo->hThread, UE_CIP), 0) + PAGE_SIZE); //dump somewhere

    // Init program database
    DbLoad(DbLoadSaveType::DebugData);

    SafeSymSetOptions(SYMOPT_IGNORE_CVREC | SYMOPT_DEBUG | SYMOPT_LOAD_LINES | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | SYMOPT_FAVOR_COMPRESSED | SYMOPT_IGNORE_NT_SYMPATH);
    GuiSymbolLogClear();
    char szServerSearchPath[MAX_PATH * 2] = "";
    sprintf_s(szServerSearchPath, "SRV*%s", szSymbolCachePath);
    SafeSymInitializeW(fdProcessInfo->hProcess, StringUtils::Utf8ToUtf16(szServerSearchPath).c_str(), false); //initialize symbols
    SafeSymRegisterCallbackW64(fdProcessInfo->hProcess, SymRegisterCallbackProc64, 0);
    SafeSymLoadModuleExW(fdProcessInfo->hProcess, CreateProcessInfo->hFile, StringUtils::Utf8ToUtf16(DebugFileName).c_str(), 0, (DWORD64)base, 0, 0, 0);

    IMAGEHLP_MODULEW64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct = sizeof(modInfo);
    if(SafeSymGetModuleInfoW64(fdProcessInfo->hProcess, (DWORD64)base, &modInfo))
        ModLoad((duint)base, modInfo.ImageSize, StringUtils::Utf16ToUtf8(modInfo.ImageName).c_str());

    char modname[256] = "";
    if(ModNameFromAddr((duint)base, modname, true))
        BpEnumAll(cbSetModuleBreakpoints, modname);
    BpEnumAll(cbSetModuleBreakpoints, "");
    GuiUpdateBreakpointsView();
    pCreateProcessBase = (duint)CreateProcessInfo->lpBaseOfImage;
    if(!bFileIsDll && !bIsAttached) //Set entry breakpoint
    {
        pDebuggedBase = pCreateProcessBase; //debugged base = executable
        char command[256] = "";

        if(settingboolget("Events", "TlsCallbacks"))
        {
            DWORD NumberOfCallBacks = 0;
            TLSGrabCallBackDataW(StringUtils::Utf8ToUtf16(DebugFileName).c_str(), 0, &NumberOfCallBacks);
            if(NumberOfCallBacks)
            {
                dprintf("TLS Callbacks: %d\n", NumberOfCallBacks);
                Memory<duint*> TLSCallBacks(NumberOfCallBacks * sizeof(duint), "cbCreateProcess:TLSCallBacks");
                if(!TLSGrabCallBackDataW(StringUtils::Utf8ToUtf16(DebugFileName).c_str(), TLSCallBacks(), &NumberOfCallBacks))
                    dputs("Failed to get TLS callback addresses!");
                else
                {
                    duint ImageBase = GetPE32DataW(StringUtils::Utf8ToUtf16(DebugFileName).c_str(), 0, UE_IMAGEBASE);
                    int invalidCount = 0;
                    for(unsigned int i = 0; i < NumberOfCallBacks; i++)
                    {
                        duint callbackVA = TLSCallBacks()[i] - ImageBase + pDebuggedBase;
                        if(MemIsValidReadPtr(callbackVA))
                        {
                            sprintf(command, "bp " fhex ",\"TLS Callback %d\",ss", callbackVA, i + 1);
                            cmddirectexec(command);
                        }
                        else
                            invalidCount++;
                    }
                    if(invalidCount)
                        dprintf("%d invalid TLS callback addresses...\n", invalidCount);
                }
            }
        }

        if(settingboolget("Events", "EntryBreakpoint"))
        {
            sprintf(command, "bp " fhex ",\"entry breakpoint\",ss", (duint)CreateProcessInfo->lpStartAddress);
            cmddirectexec(command);
        }
    }
    GuiUpdateBreakpointsView();

    //call plugin callback
    PLUG_CB_CREATEPROCESS callbackInfo;
    callbackInfo.CreateProcessInfo = CreateProcessInfo;
    IMAGEHLP_MODULE64 modInfoUtf8;
    memset(&modInfoUtf8, 0, sizeof(modInfoUtf8));
    modInfoUtf8.SizeOfStruct = sizeof(modInfoUtf8);
    modInfoUtf8.BaseOfImage = modInfo.BaseOfImage;
    modInfoUtf8.ImageSize = modInfo.ImageSize;
    modInfoUtf8.TimeDateStamp = modInfo.TimeDateStamp;
    modInfoUtf8.CheckSum = modInfo.CheckSum;
    modInfoUtf8.NumSyms = modInfo.NumSyms;
    modInfoUtf8.SymType = modInfo.SymType;
    strcpy_s(modInfoUtf8.ModuleName, StringUtils::Utf16ToUtf8(modInfo.ModuleName).c_str());
    strcpy_s(modInfoUtf8.ImageName, StringUtils::Utf16ToUtf8(modInfo.ImageName).c_str());
    strcpy_s(modInfoUtf8.LoadedImageName, StringUtils::Utf16ToUtf8(modInfo.LoadedImageName).c_str());
    strcpy_s(modInfoUtf8.LoadedPdbName, StringUtils::Utf16ToUtf8(modInfo.LoadedPdbName).c_str());
    modInfoUtf8.CVSig = modInfo.CVSig;
    strcpy_s(modInfoUtf8.CVData, StringUtils::Utf16ToUtf8(modInfo.CVData).c_str());
    modInfoUtf8.PdbSig = modInfo.PdbSig;
    modInfoUtf8.PdbSig70 = modInfo.PdbSig70;
    modInfoUtf8.PdbAge = modInfo.PdbAge;
    modInfoUtf8.PdbUnmatched = modInfo.PdbUnmatched;
    modInfoUtf8.DbgUnmatched = modInfo.DbgUnmatched;
    modInfoUtf8.LineNumbers = modInfo.LineNumbers;
    modInfoUtf8.GlobalSymbols = modInfo.GlobalSymbols;
    modInfoUtf8.TypeInfo = modInfo.TypeInfo;
    modInfoUtf8.SourceIndexed = modInfo.SourceIndexed;
    modInfoUtf8.Publics = modInfo.Publics;
    callbackInfo.modInfo = &modInfoUtf8;
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
    dprintf("Process stopped with exit code 0x%X\n", ExitProcess->dwExitCode);
    PLUG_CB_EXITPROCESS callbackInfo;
    callbackInfo.ExitProcess = ExitProcess;
    plugincbcall(CB_EXITPROCESS, &callbackInfo);
    //unload main module
    SafeSymUnloadModule64(fdProcessInfo->hProcess, pCreateProcessBase);
    ModClear(); //clear all modules
}

static void cbCreateThread(CREATE_THREAD_DEBUG_INFO* CreateThread)
{
    ThreadCreate(CreateThread); //update thread list
    DWORD dwThreadId = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    hActiveThread = ThreadGetHandle(dwThreadId);

    if(settingboolget("Events", "ThreadEntry"))
    {
        char command[256] = "";
        sprintf(command, "bp " fhex ",\"Thread %X\",ss", (duint)CreateThread->lpStartAddress, dwThreadId);
        cmddirectexec(command);
    }

    PLUG_CB_CREATETHREAD callbackInfo;
    callbackInfo.CreateThread = CreateThread;
    callbackInfo.dwThreadId = dwThreadId;
    plugincbcall(CB_CREATETHREAD, &callbackInfo);

    dprintf("Thread %X created, Entry: " fhex "\n", dwThreadId, CreateThread->lpStartAddress);

    if(settingboolget("Events", "ThreadStart"))
    {
        //update memory map
        MemUpdateMap();
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
    // Not called when the main (last) thread exits. Instead
    // EXIT_PROCESS_DEBUG_EVENT is signalled.
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

    // Update GUI (this should be the first triggered event)
    duint cip = GetContextDataEx(hActiveThread, UE_CIP);
    GuiSetDebugState(running);
    GuiDumpAt(MemFindBaseAddr(cip, 0, true)); //dump somewhere
    DebugUpdateGui(cip, true);

    //log message
    if(bIsAttached)
        dputs("Attach breakpoint reached!");
    else
        dputs("System breakpoint reached!");
    bSkipExceptions = false; //we are not skipping first-chance exceptions

    //plugin callbacks
    PLUG_CB_SYSTEMBREAKPOINT callbackInfo;
    callbackInfo.reserved = 0;
    plugincbcall(CB_SYSTEMBREAKPOINT, &callbackInfo);

    if(bIsAttached ? settingboolget("Events", "AttachBreakpoint") : settingboolget("Events", "SystemBreakpoint"))
    {
        //lock
        GuiSetDebugState(paused);
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
        strcpy_s(DLLDebugFileName, "??? (GetFileNameFromHandle failed)");

    SafeSymLoadModuleExW(fdProcessInfo->hProcess, LoadDll->hFile, StringUtils::Utf8ToUtf16(DLLDebugFileName).c_str(), 0, (DWORD64)base, 0, 0, 0);
    IMAGEHLP_MODULEW64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct = sizeof(modInfo);
    if(SafeSymGetModuleInfoW64(fdProcessInfo->hProcess, (DWORD64)base, &modInfo))
        ModLoad((duint)base, modInfo.ImageSize, StringUtils::Utf16ToUtf8(modInfo.ImageName).c_str());

    // Update memory map
    MemUpdateMapAsync();

    char modname[256] = "";
    if(ModNameFromAddr((duint)base, modname, true))
        BpEnumAll(cbSetModuleBreakpoints, modname);
    GuiUpdateBreakpointsView();
    bool bAlreadySetEntry = false;

    char command[256] = "";
    bool bIsDebuggingThis = false;
    if(bFileIsDll && !_stricmp(DLLDebugFileName, szFileName) && !bIsAttached) //Set entry breakpoint
    {
        bIsDebuggingThis = true;
        pDebuggedBase = (duint)base;
        if(settingboolget("Events", "EntryBreakpoint"))
        {
            bAlreadySetEntry = true;
            sprintf(command, "bp " fhex ",\"entry breakpoint\",ss", pDebuggedBase + pDebuggedEntry);
            cmddirectexec(command);
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
            Memory<duint*> TLSCallBacks(NumberOfCallBacks * sizeof(duint), "cbLoadDll:TLSCallBacks");
            if(!TLSGrabCallBackDataW(StringUtils::Utf8ToUtf16(DLLDebugFileName).c_str(), TLSCallBacks(), &NumberOfCallBacks))
                dputs("Failed to get TLS callback addresses!");
            else
            {
                duint ImageBase = GetPE32DataW(StringUtils::Utf8ToUtf16(DLLDebugFileName).c_str(), 0, UE_IMAGEBASE);
                int invalidCount = 0;
                for(unsigned int i = 0; i < NumberOfCallBacks; i++)
                {
                    duint callbackVA = TLSCallBacks()[i] - ImageBase + (duint)base;
                    if(MemIsValidReadPtr(callbackVA))
                    {
                        if(bIsDebuggingThis)
                            sprintf(command, "bp " fhex ",\"TLS Callback %d\",ss", callbackVA, i + 1);
                        else
                            sprintf(command, "bp " fhex ",\"TLS Callback %d (%s)\",ss", callbackVA, i + 1, modname);
                        cmddirectexec(command);
                    }
                    else
                        invalidCount++;
                }
                if(invalidCount)
                    dprintf("%d invalid TLS callback addresses...\n", invalidCount);
            }
        }
    }

    if((bBreakOnNextDll || settingboolget("Events", "DllEntry")) && !bAlreadySetEntry)
    {
        duint oep = GetPE32DataW(StringUtils::Utf8ToUtf16(DLLDebugFileName).c_str(), 0, UE_OEP);
        if(oep)
        {
            char command[256] = "";
            sprintf(command, "bp " fhex ",\"DllMain (%s)\",ss", oep + (duint)base, modname);
            cmddirectexec(command);
        }
    }

    dprintf("DLL Loaded: " fhex " %s\n", base, DLLDebugFileName);

    //plugin callback
    PLUG_CB_LOADDLL callbackInfo;
    callbackInfo.LoadDll = LoadDll;
    IMAGEHLP_MODULE64 modInfoUtf8;
    memset(&modInfoUtf8, 0, sizeof(modInfoUtf8));
    modInfoUtf8.SizeOfStruct = sizeof(modInfoUtf8);
    modInfoUtf8.BaseOfImage = modInfo.BaseOfImage;
    modInfoUtf8.ImageSize = modInfo.ImageSize;
    modInfoUtf8.TimeDateStamp = modInfo.TimeDateStamp;
    modInfoUtf8.CheckSum = modInfo.CheckSum;
    modInfoUtf8.NumSyms = modInfo.NumSyms;
    modInfoUtf8.SymType = modInfo.SymType;
    strcpy_s(modInfoUtf8.ModuleName, StringUtils::Utf16ToUtf8(modInfo.ModuleName).c_str());
    strcpy_s(modInfoUtf8.ImageName, StringUtils::Utf16ToUtf8(modInfo.ImageName).c_str());
    strcpy_s(modInfoUtf8.LoadedImageName, StringUtils::Utf16ToUtf8(modInfo.LoadedImageName).c_str());
    strcpy_s(modInfoUtf8.LoadedPdbName, StringUtils::Utf16ToUtf8(modInfo.LoadedPdbName).c_str());
    modInfoUtf8.CVSig = modInfo.CVSig;
    strcpy_s(modInfoUtf8.CVData, StringUtils::Utf16ToUtf8(modInfo.CVData).c_str());
    modInfoUtf8.PdbSig = modInfo.PdbSig;
    modInfoUtf8.PdbSig70 = modInfo.PdbSig70;
    modInfoUtf8.PdbAge = modInfo.PdbAge;
    modInfoUtf8.PdbUnmatched = modInfo.PdbUnmatched;
    modInfoUtf8.DbgUnmatched = modInfo.DbgUnmatched;
    modInfoUtf8.LineNumbers = modInfo.LineNumbers;
    modInfoUtf8.GlobalSymbols = modInfo.GlobalSymbols;
    modInfoUtf8.TypeInfo = modInfo.TypeInfo;
    modInfoUtf8.SourceIndexed = modInfo.SourceIndexed;
    modInfoUtf8.Publics = modInfo.Publics;
    callbackInfo.modInfo = &modInfoUtf8;
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
    if(ModNameFromAddr((duint)base, modname, true))
        BpEnumAll(cbRemoveModuleBreakpoints, modname);
    GuiUpdateBreakpointsView();
    SafeSymUnloadModule64(fdProcessInfo->hProcess, (DWORD64)base);
    dprintf("DLL Unloaded: " fhex " %s\n", base, modname);

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

    ModUnload((duint)base);

    //update memory map
    MemUpdateMapAsync();
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
        if(MemRead((duint)DebugString->lpDebugStringData, DebugText(), DebugString->nDebugStringLength))
        {
            String str = String(DebugText());
            if(str != lastDebugText)  //fix for every string being printed twice
            {
                if(str != "\n")
                    dprintf("DebugString: \"%s\"\n", StringUtils::Escape(str).c_str());
                lastDebugText = str;
            }
            else
                lastDebugText.clear();
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

    duint addr = (duint)ExceptionData->ExceptionRecord.ExceptionAddress;
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
            //update memory map
            MemUpdateMap();
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
        SetContextDataEx(hActiveThread, UE_CIP, (duint)ExceptionData->ExceptionRecord.ExceptionAddress);
    }
    else if(ExceptionData->ExceptionRecord.ExceptionCode == MS_VC_EXCEPTION) //SetThreadName exception
    {
        THREADNAME_INFO nameInfo; //has no valid local pointers
        memcpy(&nameInfo, ExceptionData->ExceptionRecord.ExceptionInformation, sizeof(THREADNAME_INFO));
        if(nameInfo.dwThreadID == -1) //current thread
            nameInfo.dwThreadID = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
        if(nameInfo.dwType == 0x1000 && nameInfo.dwFlags == 0 && ThreadIsValid(nameInfo.dwThreadID)) //passed basic checks
        {
            Memory<char*> ThreadName(MAX_THREAD_NAME_SIZE, "cbException:ThreadName");
            if(MemRead((duint)nameInfo.szName, ThreadName(), MAX_THREAD_NAME_SIZE - 1))
            {
                String ThreadNameEscaped = StringUtils::Escape(ThreadName());
                dprintf("SetThreadName(%X, \"%s\")\n", nameInfo.dwThreadID, ThreadNameEscaped.c_str());
                ThreadSetName(nameInfo.dwThreadID, ThreadNameEscaped.c_str());
            }
        }
    }
    auto exceptionName = ExceptionCodeToName(ExceptionCode);
    if(ExceptionData->dwFirstChance) //first chance exception
    {
        if(exceptionName.size())
            dprintf("First chance exception on " fhex " (%.8X, %s)!\n", addr, ExceptionCode, exceptionName.c_str());
        else
            dprintf("First chance exception on " fhex " (%.8X)!\n", addr, ExceptionCode);
        SetNextDbgContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
        if(bSkipExceptions || dbgisignoredexception(ExceptionCode))
            return;
    }
    else //lock the exception
    {
        if(exceptionName.size())
            dprintf("Last chance exception on " fhex " (%.8X, %s)!\n", addr, ExceptionCode, exceptionName.c_str());
        else
            dprintf("Last chance exception on " fhex " (%.8X)!\n", addr, ExceptionCode);
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
    InterlockedIncrement(&DbgEvents);
    PLUG_CB_DEBUGEVENT debugEventInfo;
    debugEventInfo.DebugEvent = DebugEvent;
    plugincbcall(CB_DEBUGEVENT, &debugEventInfo);
}

bool cbDeleteAllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPNORMAL)
        return true;
    if(!BpDelete(bp->addr, BPNORMAL))
    {
        dprintf("Delete breakpoint failed (BpDelete): " fhex "\n", bp->addr);
        return false;
    }
    if(bp->enabled && !DeleteBPX(bp->addr))
    {
        dprintf("Delete breakpoint failed (DeleteBPX): " fhex "\n", bp->addr);
        return false;
    }
    return true;
}

bool cbEnableAllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPNORMAL || bp->enabled)
        return true;

    if(!SetBPX(bp->addr, bp->titantype, (void*)cbUserBreakpoint))
    {
        if(!MemIsValidReadPtr(bp->addr))
            return true;
        dprintf("Could not enable breakpoint " fhex " (SetBPX)\n", bp->addr);
        return false;
    }
    if(!BpEnable(bp->addr, BPNORMAL, true))
    {
        dprintf("Could not enable breakpoint " fhex " (BpEnable)\n", bp->addr);
        return false;
    }
    return true;
}

bool cbDisableAllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPNORMAL || !bp->enabled)
        return true;

    if(!BpEnable(bp->addr, BPNORMAL, false))
    {
        dprintf("Could not disable breakpoint " fhex " (BpEnable)\n", bp->addr);
        return false;
    }
    if(!DeleteBPX(bp->addr))
    {
        dprintf("Could not disable breakpoint " fhex " (DeleteBPX)\n", bp->addr);
        return false;
    }
    return true;
}

bool cbEnableAllHardwareBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPHARDWARE || bp->enabled)
        return true;
    DWORD drx = 0;
    if(!GetUnusedHardwareBreakPointRegister(&drx))
    {
        dprintf("Did not enable hardware breakpoint " fhex " (all slots full)\n", bp->addr);
        return true;
    }
    int titantype = bp->titantype;
    TITANSETDRX(titantype, drx);
    BpSetTitanType(bp->addr, BPHARDWARE, titantype);
    if(!BpEnable(bp->addr, BPHARDWARE, true))
    {
        dprintf("Could not enable hardware breakpoint " fhex " (BpEnable)\n", bp->addr);
        return false;
    }
    if(!SetHardwareBreakPoint(bp->addr, drx, TITANGETTYPE(bp->titantype), TITANGETSIZE(bp->titantype), (void*)cbHardwareBreakpoint))
    {
        dprintf("Could not enable hardware breakpoint " fhex " (SetHardwareBreakPoint)\n", bp->addr);
        return false;
    }
    return true;
}

bool cbDisableAllHardwareBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPHARDWARE)
        return true;
    if(!BpEnable(bp->addr, BPHARDWARE, false))
    {
        dprintf("Could not disable hardware breakpoint " fhex " (BpEnable)\n", bp->addr);
        return false;
    }
    if(bp->enabled && !DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype)))
    {
        dprintf("Could not disable hardware breakpoint " fhex " (DeleteHardwareBreakPoint)\n", bp->addr);
        return false;
    }
    return true;
}

bool cbEnableAllMemoryBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPMEMORY || bp->enabled)
        return true;
    duint size = 0;
    MemFindBaseAddr(bp->addr, &size);
    if(!BpEnable(bp->addr, BPMEMORY, true))
    {
        dprintf("Could not enable memory breakpoint " fhex " (BpEnable)\n", bp->addr);
        return false;
    }
    if(!SetMemoryBPXEx(bp->addr, size, bp->titantype, !bp->singleshoot, (void*)cbMemoryBreakpoint))
    {
        dprintf("Could not enable memory breakpoint " fhex " (SetMemoryBPXEx)\n", bp->addr);
        return false;
    }
    return true;
}

bool cbDisableAllMemoryBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPMEMORY || !bp->enabled)
        return true;
    if(!BpEnable(bp->addr, BPMEMORY, false))
    {
        dprintf("Could not disable memory breakpoint " fhex " (BpEnable)\n", bp->addr);
        return false;
    }
    if(!RemoveMemoryBPX(bp->addr, 0))
    {
        dprintf("Could not disable memory breakpoint " fhex " (RemoveMemoryBPX)\n", bp->addr);
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
        dprintf("%d:%s:" fhex ":\"%s\"\n", enabled, type, bp->addr, bp->name);
    else
        dprintf("%d:%s:" fhex "\n", enabled, type, bp->addr);
    return true;
}

bool cbDeleteAllMemoryBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPMEMORY)
        return true;
    duint size;
    MemFindBaseAddr(bp->addr, &size);
    if(!BpDelete(bp->addr, BPMEMORY))
    {
        dprintf("Delete memory breakpoint failed (BpDelete): " fhex "\n", bp->addr);
        return false;
    }
    if(bp->enabled && !RemoveMemoryBPX(bp->addr, size))
    {
        dprintf("Delete memory breakpoint failed (RemoveMemoryBPX): " fhex "\n", bp->addr);
        return false;
    }
    return true;
}

bool cbDeleteAllHardwareBreakpoints(const BREAKPOINT* bp)
{
    if(bp->type != BPHARDWARE)
        return true;
    if(!BpDelete(bp->addr, BPHARDWARE))
    {
        dprintf("Delete hardware breakpoint failed (BpDelete): " fhex "\n", bp->addr);
        return false;
    }
    if(bp->enabled && !DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype)))
    {
        dprintf("Delete hardware breakpoint failed (DeleteHardwareBreakPoint): " fhex "\n", bp->addr);
        return false;
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
    hProcess = fdProcessInfo->hProcess;
    varset("$hp", (duint)fdProcessInfo->hProcess, true);
    varset("$pid", fdProcessInfo->dwProcessId, true);
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
        if(pe32.th32ProcessID == 0 || pe32.th32ProcessID == 4) // System process and Idle process have special PID.
            continue;
        Handle hProcess = TitanOpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pe32.th32ProcessID);
        if(!hProcess)
            continue;
        BOOL wow64 = false, mewow64 = false;
        if(!IsWow64Process(hProcess, &wow64) || !IsWow64Process(GetCurrentProcess(), &mewow64))
            continue;
        if((mewow64 && !wow64) || (!mewow64 && wow64))
            continue;
        wchar_t szExePath[MAX_PATH] = L"";
        if(GetModuleFileNameExW(hProcess, 0, szExePath, MAX_PATH))
            strcpy_s(pe32.szExeFile, StringUtils::Utf16ToUtf8(szExePath).c_str());
        list->push_back(pe32);
    }
    while(Process32Next(hProcessSnap, &pe32));
    return true;
}

static bool getcommandlineaddr(duint* addr, cmdline_error_t* cmd_line_error)
{
    duint pprocess_parameters;

    cmd_line_error->addr = (duint)GetPEBLocation(fdProcessInfo->hProcess);

    if(cmd_line_error->addr == 0)
    {
        cmd_line_error->type = CMDL_ERR_GET_PEB;
        return false;
    }

    //cast-trick to calculate the address of the remote peb field ProcessParameters
    cmd_line_error->addr = (duint) & (((PPEB) cmd_line_error->addr)->ProcessParameters);
    if(!MemRead(cmd_line_error->addr, &pprocess_parameters, sizeof(pprocess_parameters)))
    {
        cmd_line_error->type = CMDL_ERR_READ_PEBBASE;
        return false;
    }

    *addr = (duint) & (((RTL_USER_PROCESS_PARAMETERS*) pprocess_parameters)->CommandLine);
    return true;
}

static bool patchcmdline(duint getcommandline, duint new_command_line, cmdline_error_t* cmd_line_error)
{
    duint command_line_stored = 0;
    unsigned char data[100];

    cmd_line_error->addr = getcommandline;
    if(!MemRead(cmd_line_error->addr, & data, sizeof(data)))
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
    command_line_stored = * ((duint*) & data[1]);
#endif

    //update the pointer in the debuggee
    if(!MemWrite(command_line_stored, &new_command_line, sizeof(new_command_line)))
    {
        cmd_line_error->addr = command_line_stored;
        cmd_line_error->type = CMDL_ERR_WRITE_GETCOMMANDLINESTORED;
        return false;
    }

    return true;
}

static bool fixgetcommandlinesbase(duint new_command_line_unicode, duint new_command_line_ascii, cmdline_error_t* cmd_line_error)
{
    duint getcommandline;

    if(!valfromstring("kernelBase:GetCommandLineA", &getcommandline))
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
    duint command_line_addr;

    if(cmd_line_error == NULL)
        cmd_line_error = &cmd_line_error_aux;

    if(!getcommandlineaddr(&cmd_line_error->addr, cmd_line_error))
        return false;

    command_line_addr = cmd_line_error->addr;

    SIZE_T cmd_line_size = strlen(cmd_line);
    new_command_line.Length = (USHORT)(strlen(cmd_line) + 1) * sizeof(WCHAR);
    new_command_line.MaximumLength = new_command_line.Length;

    Memory<wchar_t*> command_linewstr(new_command_line.Length);

    // Covert to Unicode.
    if(!MultiByteToWideChar(CP_UTF8, 0, cmd_line, (int)cmd_line_size + 1, command_linewstr(), (int)cmd_line_size + 1))
    {
        cmd_line_error->type = CMDL_ERR_CONVERTUNICODE;
        return false;
    }

    new_command_line.Buffer = command_linewstr();

    duint mem = (duint)MemAllocRemote(0, new_command_line.Length * 2);
    if(!mem)
    {
        cmd_line_error->type = CMDL_ERR_ALLOC_UNICODEANSI_COMMANDLINE;
        return false;
    }

    if(!MemWrite(mem, new_command_line.Buffer, new_command_line.Length))
    {
        cmd_line_error->addr = mem;
        cmd_line_error->type = CMDL_ERR_WRITE_UNICODE_COMMANDLINE;
        return false;
    }

    if(!MemWrite((mem + new_command_line.Length), (void*)cmd_line, strlen(cmd_line) + 1))
    {
        cmd_line_error->addr = mem + new_command_line.Length;
        cmd_line_error->type = CMDL_ERR_WRITE_ANSI_COMMANDLINE;
        return false;
    }

    if(!fixgetcommandlinesbase(mem, mem + new_command_line.Length, cmd_line_error))
        return false;

    new_command_line.Buffer = (PWSTR) mem;
    if(!MemWrite(command_line_addr, &new_command_line, sizeof(new_command_line)))
    {
        cmd_line_error->addr = command_line_addr;
        cmd_line_error->type = CMDL_ERR_WRITE_PEBUNICODE_COMMANDLINE;
        return false;
    }

    // Copy command line
    copyCommandLine(cmd_line);

    return true;
}

bool dbggetcmdline(char** cmd_line, cmdline_error_t* cmd_line_error)
{
    UNICODE_STRING CommandLine;
    cmdline_error_t cmd_line_error_aux;

    if(!cmd_line_error)
        cmd_line_error = &cmd_line_error_aux;

    if(!getcommandlineaddr(&cmd_line_error->addr, cmd_line_error))
        return false;

    if(!MemRead(cmd_line_error->addr, &CommandLine, sizeof(CommandLine)))
    {
        cmd_line_error->type = CMDL_ERR_READ_PROCPARM_PTR;
        return false;
    }

    Memory<wchar_t*> wstr_cmd(CommandLine.Length + sizeof(wchar_t));

    cmd_line_error->addr = (duint) CommandLine.Buffer;
    if(!MemRead(cmd_line_error->addr, wstr_cmd(), CommandLine.Length))
    {
        cmd_line_error->type = CMDL_ERR_READ_PROCPARM_CMDLINE;
        return false;
    }

    SIZE_T wstr_cmd_size = wcslen(wstr_cmd()) + 1;
    SIZE_T cmd_line_size = wstr_cmd_size * 2;

    *cmd_line = (char*)emalloc(cmd_line_size, "dbggetcmdline:cmd_line");

    //Convert TO UTF-8
    if(!WideCharToMultiByte(CP_UTF8, 0, wstr_cmd(), (int)wstr_cmd_size, * cmd_line, (int)cmd_line_size, NULL, NULL))
    {
        efree(*cmd_line);
        cmd_line_error->type = CMDL_ERR_CONVERTUNICODE;
        return false;
    }

    return true;
}

static DWORD WINAPI scriptThread(void* data)
{
    CBPLUGINSCRIPT cbScript = (CBPLUGINSCRIPT)data;
    cbScript();
    return 0;
}

void dbgstartscriptthread(CBPLUGINSCRIPT cbScript)
{
    CloseHandle(CreateThread(0, 0, scriptThread, cbScript, 0, 0));
}

duint dbggetdebuggedbase()
{
    return pDebuggedBase;
}

static void debugLoopFunction(void* lpParameter, bool attach)
{
    //we are running
    EXCLUSIVE_ACQUIRE(LockDebugStartStop);
    lock(WAITID_STOP);

    //initialize variables
    bIsAttached = attach;
    bSkipExceptions = false;
    bBreakOnNextDll = false;
    bFreezeStack = false;
    ecount = 0;

    //prepare attach/createprocess
    DWORD pid;
    INIT_STRUCT* init;
    if(attach)
    {
        pid = DWORD(lpParameter);
        static PROCESS_INFORMATION pi_attached;
        memset(&pi_attached, 0, sizeof(pi_attached));
        fdProcessInfo = &pi_attached;
    }
    else
    {
        init = (INIT_STRUCT*)lpParameter;
        pDebuggedEntry = GetPE32DataW(StringUtils::Utf8ToUtf16(init->exe).c_str(), 0, UE_OEP);
        strcpy_s(szFileName, init->exe);
    }

    bFileIsDll = IsFileDLLW(StringUtils::Utf8ToUtf16(szFileName).c_str(), 0);
    DbSetPath(nullptr, szFileName);

    if(!attach)
    {
        // Load command line if it exists in DB
        DbLoad(DbLoadSaveType::CommandLine);
        if(!isCmdLineEmpty())
        {
            char* commandLineArguments = NULL;
            commandLineArguments = getCommandLineArgs();

            if(commandLineArguments)
                init->commandline = commandLineArguments;
        }

        //start the process
        if(bFileIsDll)
            fdProcessInfo = (PROCESS_INFORMATION*)InitDLLDebugW(StringUtils::Utf8ToUtf16(init->exe).c_str(), false, StringUtils::Utf8ToUtf16(init->commandline).c_str(), StringUtils::Utf8ToUtf16(init->currentfolder).c_str(), 0);
        else
            fdProcessInfo = (PROCESS_INFORMATION*)InitDebugW(StringUtils::Utf8ToUtf16(init->exe).c_str(), StringUtils::Utf8ToUtf16(init->commandline).c_str(), StringUtils::Utf8ToUtf16(init->currentfolder).c_str());
        if(!fdProcessInfo)
        {
            fdProcessInfo = &g_pi;
            dprintf("Error starting process (CreateProcess, %s)!\n", ErrorCodeToName(GetLastError()).c_str());
            unlock(WAITID_STOP);
            return;
        }

        //check for WOW64
        BOOL wow64 = false, mewow64 = false;
        if(!IsWow64Process(fdProcessInfo->hProcess, &wow64) || !IsWow64Process(GetCurrentProcess(), &mewow64))
        {
            dputs("IsWow64Process failed!");
            StopDebug();
            unlock(WAITID_STOP);
            return;
        }
        if((mewow64 && !wow64) || (!mewow64 && wow64))
        {
#ifdef _WIN64
            dputs("Use x32dbg to debug this process!");
#else
            dputs("Use x64dbg to debug this process!");
#endif // _WIN64
            unlock(WAITID_STOP);
            return;
        }

        //set script variables
        varset("$hp", (duint)fdProcessInfo->hProcess, true);
        varset("$pid", fdProcessInfo->dwProcessId, true);

        if(!OpenProcessToken(fdProcessInfo->hProcess, TOKEN_ALL_ACCESS, &hProcessToken))
            hProcessToken = 0;
    }

    //set custom handlers
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
    GuiAddRecentFile(szFileName);

    //set GUI title
    strcpy_s(szBaseFileName, szFileName);
    int len = (int)strlen(szBaseFileName);
    while(szBaseFileName[len] != '\\' && len)
        len--;
    if(len)
        strcpy_s(szBaseFileName, szBaseFileName + len + 1);
    GuiUpdateWindowTitle(szBaseFileName);

    //call plugin callback
    PLUG_CB_INITDEBUG initInfo;
    initInfo.szFileName = szFileName;
    plugincbcall(CB_INITDEBUG, &initInfo);

    //call plugin callback (attach)
    if(attach)
    {
        PLUG_CB_ATTACH attachInfo;
        attachInfo.dwProcessId = (DWORD)pid;
        plugincbcall(CB_ATTACH, &attachInfo);
    }

    //run debug loop (returns when process debugging is stopped)
    if(attach)
    {
        AttachDebugger(pid, true, fdProcessInfo, (void*)cbAttachDebugger);
    }
    else
    {
        hProcess = fdProcessInfo->hProcess;
        DebugLoop();
    }

    //call plugin callback
    PLUG_CB_STOPDEBUG stopInfo;
    stopInfo.reserved = 0;
    plugincbcall(CB_STOPDEBUG, &stopInfo);

    //cleanup dbghelp
    SafeSymRegisterCallbackW64(hProcess, nullptr, 0);
    SafeSymCleanup(hProcess);

    //message the user/do final stuff
    RemoveAllBreakPoints(UE_OPTION_REMOVEALL); //remove all breakpoints

    //cleanup
    DbClose();
    ModClear();
    ThreadClear();
    TraceRecord.clear();
    GuiSetDebugState(stopped);
    GuiUpdateAllViews();
    dputs("Debugging stopped!");
    varset("$hp", (duint)0, true);
    varset("$pid", (duint)0, true);
    if(hProcessToken)
        CloseHandle(hProcessToken);
    unlock(WAITID_STOP); //we are done
    pDebuggedEntry = 0;
    pDebuggedBase = 0;
    pCreateProcessBase = 0;
    isDetachedByUser = false;
}

DWORD WINAPI threadDebugLoop(void* lpParameter)
{
    debugLoopFunction(lpParameter, false);
    return 0;
}

DWORD WINAPI threadAttachLoop(void* lpParameter)
{
    debugLoopFunction(lpParameter, true);
    return 0;
}
