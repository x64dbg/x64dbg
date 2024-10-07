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
#include "watch.h"
#include "thread.h"
#include "plugin_loader.h"
#include "breakpoint.h"
#include "symbolinfo.h"
#include "variable.h"
#include "x64dbg.h"
#include "exception.h"
#include "module.h"
#include "commandline.h"
#include "stackinfo.h"
#include "stringformat.h"
#include "TraceRecord.h"
#include "historycontext.h"
#include "taskthread.h"
#include "animate.h"
#include "simplescript.h"
#include "zydis_wrapper.h"
#include "cmd-watch-control.h"
#include "filemap.h"
#include "jit.h"
#include "handle.h"
#include "dbghelp_safe.h"
#include "exprfunc.h"
#include "debugger_cookie.h"
#include "debugger_tracing.h"
#include "handles.h"

// Debugging variables
static PROCESS_INFORMATION g_pi = {0, 0, 0, 0};
static char szBaseFileName[MAX_PATH] = "";
static TraceState traceState;
static bool bFileIsDll = false;
static bool bEntryIsInMzHeader = false;
static duint pDebuggedBase = 0;
static duint pDebuggedEntry = 0;
static bool bRepeatIn = false;
static duint stepRepeat = 0;
static bool bIsAttached = false;
static bool bSkipExceptions = false;
static duint skipExceptionCount = 0;
static bool bFreezeStack = false;
static std::vector<ExceptionFilter> exceptionFilters;
static HANDLE hEvent = 0;
static duint tidToResume = 0;
static HANDLE hMemMapThread = 0;
static bool bStopMemMapThread = false;
static HANDLE hTimeWastedCounterThread = 0;
static bool bStopTimeWastedCounterThread = false;
static HANDLE hDumpRefreshThread = 0;
static bool bStopDumpRefreshThread = false;
static String lastDebugText;
static duint timeWastedDebugging = 0;
static EXCEPTION_DEBUG_INFO lastExceptionInfo = { 0 };
static char szDebuggeeInitializationScript[MAX_PATH] = "";
static WString gInitExe, gInitCmd, gInitDir, gDllLoader;
static CookieQuery cookie;
static duint exceptionDispatchAddr = 0;
static bool bPausedOnException = false;
static HANDLE DebugDLLFileMapping = 0;
static DWORD nextContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
char szProgramDir[MAX_PATH] = "";
char szUserDir[MAX_PATH] = "";
char szDebuggeePath[MAX_PATH] = "";
char szDllLoaderPath[MAX_PATH] = "";
char szSymbolCachePath[MAX_PATH] = "";
std::vector<std::pair<duint, duint>> RunToUserCodeBreakpoints;
PROCESS_INFORMATION* fdProcessInfo = &g_pi;
HANDLE hActiveThread;
HANDLE hProcessToken;
bool bUndecorateSymbolNames = true;
bool bEnableSourceDebugging = false;
bool bSkipInt3Stepping = false;
bool bIgnoreInconsistentBreakpoints = false;
bool bNoForegroundWindow = true;
bool bVerboseExceptionLogging = true;
bool bNoWow64SingleStepWorkaround = false;
bool bTraceBrowserNeedsUpdate = false;
bool bForceLoadSymbols = false;
bool bNewStringAlgorithm = false;
bool bPidTidInHex = false;
bool bWindowLongPath = false;
duint DbgEvents = 0;
duint maxSkipExceptionCount = 0;
HANDLE mProcHandle;
HANDLE mForegroundHandle;
duint gRtrPreviousCSP = 0;
static bool bAbortStepping = false;
static TITANCBSTEP gStepIntoPartyCallback;
HANDLE hDebugLoopThread = nullptr;
DWORD dwDebugFlags = 0;

static duint dbgcleartracestate()
{
    auto steps = traceState.StepCount();
    traceState.Clear();
    return steps;
}

static void dbgClearRtuBreakpoints()
{
    EXCLUSIVE_ACQUIRE(LockRunToUserCode);
    for(auto & i : RunToUserCodeBreakpoints)
    {
        BREAKPOINT bp;
        if(!BpGet(i.first, BPMEMORY, nullptr, &bp))
            RemoveMemoryBPX(i.first, i.second);
    }
    RunToUserCodeBreakpoints.clear();
}

bool dbgsettracecondition(const String & expression, duint maxSteps)
{
    if(dbgtraceactive())
        return false;
    if(!traceState.InitTraceCondition(expression, maxSteps))
        return false;
    if(traceState.InitLogFile())
        return true;
    dbgcleartracestate();
    return false;
}

bool dbgsettracelog(const String & expression, const String & text)
{
    if(dbgtraceactive())
        return false;
    return traceState.InitLogCondition(expression, text);
}

bool dbgsettracecmd(const String & expression, const String & text)
{
    if(dbgtraceactive())
        return false;
    return traceState.InitCmdCondition(expression, text);
}

bool dbgtraceactive()
{
    return traceState.IsActive();
}

void dbgforcebreaktrace()
{
    if(traceState.IsActive())
        traceState.SetForceBreakTrace();
}

bool dbgstepactive()
{
    return stepRepeat > 1 || gRtrPreviousCSP != 0 || gStepIntoPartyCallback != nullptr;
}

void dbgforcebreakstep()
{
    if(dbgstepactive())
    {
        bAbortStepping = true;
    }
}

bool dbgsettracelogfile(const char* fileName)
{
    traceState.SetLogFile(fileName);
    return true;
}

static DWORD WINAPI memMapThread(void* ptr)
{
    while(!bStopMemMapThread)
    {
        while(!DbgIsDebugging())
        {
            if(bStopMemMapThread)
                break;
            Sleep(10);
        }
        if(bStopMemMapThread)
            break;
        MemUpdateMapAsync();
        ThreadUpdateWaitReasons();
        GuiUpdateThreadView();
        Sleep(2000);
    }

    return 0;
}

static bool isUserIdle()
{
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    GetLastInputInfo(&lii);
    return GetTickCount() - lii.dwTime > 1000 * 60; //60 seconds without input is considered idle
}

static DWORD WINAPI timeWastedCounterThread(void* ptr)
{
    if(!BridgeSettingGetUint("Engine", "TimeWastedDebugging", &timeWastedDebugging))
        timeWastedDebugging = 0;
    GuiUpdateTimeWastedCounter();
    while(!bStopTimeWastedCounterThread)
    {
        while(!DbgIsDebugging() || isUserIdle())
        {
            if(bStopTimeWastedCounterThread)
                break;
            Sleep(10);
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

static DWORD WINAPI dumpRefreshThread(void* ptr)
{
    while(!bStopDumpRefreshThread)
    {
        while(!DbgIsDebugging())
        {
            if(bStopDumpRefreshThread)
                break;
            Sleep(100);
        }
        if(bStopDumpRefreshThread)
            break;
        GuiUpdateDumpView();
        GuiUpdateWatchView();
        if(bTraceBrowserNeedsUpdate)
        {
            bTraceBrowserNeedsUpdate = false;
            GuiUpdateTraceBrowser();
        }
        Sleep(400);
    }
    return 0;
}

/**
\brief Called when the debugger pauses.
*/
void cbDebuggerPaused()
{
    // Clear tracing conditions
    dbgcleartracestate();
    dbgClearRtuBreakpoints();
    bAbortStepping = false;
    stepRepeat = 0;
    // Trace record is not handled by this function currently.
    // Signal thread switch warning
    if(settingboolget("Engine", "HardcoreThreadSwitchWarning"))
    {
        static DWORD PrevThreadId = 0;
        if(PrevThreadId == 0)
            PrevThreadId = fdProcessInfo->dwThreadId; // Initialize to Main Thread
        DWORD currentThreadId = ThreadGetId(hActiveThread);
        if(currentThreadId != PrevThreadId && PrevThreadId != 0)
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Thread switched from %X to %X !\n"), PrevThreadId, currentThreadId);
            PrevThreadId = currentThreadId;
        }
    }
    // Watchdog
    cbCheckWatchdog(0, nullptr);
    // Flush breakpoint logs
    BpLogFileFlush();
}

void dbginit()
{
    hTimeWastedCounterThread = CreateThread(nullptr, 0, timeWastedCounterThread, nullptr, 0, nullptr);
    hMemMapThread = CreateThread(nullptr, 0, memMapThread, nullptr, 0, nullptr);
    hDumpRefreshThread = CreateThread(nullptr, 0, dumpRefreshThread, nullptr, 0, nullptr);
}

void dbgstop()
{
    bStopTimeWastedCounterThread = true;
    bStopMemMapThread = true;
    bStopDumpRefreshThread = true;
    HANDLE hThreads[] = { hTimeWastedCounterThread, hMemMapThread, hDumpRefreshThread };
    WaitForMultipleThreadsTermination(hThreads, _countof(hThreads), 10000); // Total time out is 10 seconds.
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

void dbgsetresumetid(duint tid)
{
    tidToResume = tid;
}

void dbgsetskipexceptions(bool skip)
{
    bSkipExceptions = skip;
    skipExceptionCount = 0;
}

void dbgsetsteprepeat(bool steppingIn, duint repeat)
{
    bRepeatIn = steppingIn;
    stepRepeat = repeat;
}

void dbgsetfreezestack(bool freeze)
{
    bFreezeStack = freeze;
}

void dbgclearexceptionfilters()
{
    exceptionFilters.clear();
}

void dbgaddexceptionfilter(ExceptionFilter filter)
{
    exceptionFilters.push_back(filter);
}

const ExceptionFilter & dbggetexceptionfilter(unsigned int exception)
{
    for(unsigned int i = 0; i < exceptionFilters.size(); i++)
    {
        const ExceptionFilter & filter = exceptionFilters.at(i);
        unsigned int curStart = filter.range.start;
        unsigned int curEnd = filter.range.end;
        if(exception >= curStart && exception <= curEnd)
            return filter;
    }

    // no filter found, return the catch-all filter for unknown exceptions
    for(unsigned int i = 0; i < exceptionFilters.size(); i++)
    {
        const ExceptionFilter & filter = exceptionFilters.at(i);
        if(filter.range.start == 0 && filter.range.start == filter.range.end)
            return filter;
    }

    // the unknown exceptions filter is not yet present in settings, add it now
    ExceptionFilter unknownExceptionsFilter;
    unknownExceptionsFilter.range.start = unknownExceptionsFilter.range.end = 0;
    unknownExceptionsFilter.breakOn = ExceptionBreakOn::FirstChance;
    unknownExceptionsFilter.logException = true;
    unknownExceptionsFilter.handledBy = ExceptionHandledBy::Debuggee;
    exceptionFilters.push_back(unknownExceptionsFilter);
    return exceptionFilters.back();
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
    return InterlockedExchange((volatile long*)&DbgEvents, 0);
}

void dbgtracebrowserneedsupdate()
{
    bTraceBrowserNeedsUpdate = true;
}

static std::unordered_map<std::string, std::pair<DWORD, bool>> dllBreakpoints;

bool dbgsetdllbreakpoint(const char* mod, DWORD type, bool singleshoot)
{
    EXCLUSIVE_ACQUIRE(LockDllBreakpoints);
    return dllBreakpoints.insert({ mod, { type, singleshoot } }).second;
}

bool dbgdeletedllbreakpoint(const char* mod, DWORD type)
{
    EXCLUSIVE_ACQUIRE(LockDllBreakpoints);
    auto found = dllBreakpoints.find(mod);
    if(found == dllBreakpoints.end())
        return false;
    dllBreakpoints.erase(found);
    return true;
}

void dbgsetdebugflags(DWORD flags)
{
    dwDebugFlags = flags;
}

bool dbghandledllbreakpoint(const char* mod, bool loadDll)
{
    EXCLUSIVE_ACQUIRE(LockDllBreakpoints);
    auto shouldBreak = false;
    auto found = dllBreakpoints.find(mod);
    if(found != dllBreakpoints.end())
    {
        if(found->second.first == UE_ON_LIB_ALL || found->second.first == (loadDll ? UE_ON_LIB_LOAD : UE_ON_LIB_UNLOAD))
            shouldBreak = true;
        if(found->second.second)
            dllBreakpoints.erase(found);
    }
    return shouldBreak;
}

static DWORD WINAPI updateCallStackThread(duint ptr)
{
    stackupdatecallstack(ptr);
    GuiUpdateCallStack();
    return 0;
}

void updateCallStackAsync(duint ptr)
{
    static TaskThread_<decltype(&updateCallStackThread), duint> updateCallStackTask(&updateCallStackThread);
    updateCallStackTask.WakeUp(ptr);
}

DWORD WINAPI updateSEHChainThread()
{
    GuiUpdateSEHChain();
    stackupdateseh();
    GuiUpdateDumpView();
    return 0;
}

void updateSEHChainAsync()
{
    static TaskThread_<decltype(&updateSEHChainThread)> updateSEHChainTask(&updateSEHChainThread);
    updateSEHChainTask.WakeUp();
}

static void DebugUpdateTitle(duint disasm_addr, bool analyzeThreadSwitch)
{
    if(GuiIsUpdateDisabled() || !DbgIsDebugging())
        return;

    char modname[MAX_MODULE_SIZE] = "";
    char modtext[MAX_MODULE_SIZE * 2] = "";
    if(!ModNameFromAddr(disasm_addr, modname, true))
        *modname = 0;
    else
        _snprintf_s(modtext, _TRUNCATE, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Module: %s - ")), modname);
    char threadswitch[256] = "";
    DWORD currentThreadId = ThreadGetId(hActiveThread);
    if(analyzeThreadSwitch)
    {
        static DWORD PrevThreadId = 0;
        if(PrevThreadId == 0)
            PrevThreadId = fdProcessInfo->dwThreadId; // Initialize to Main Thread
        if(currentThreadId != PrevThreadId && PrevThreadId != 0)
        {
            char threadName2[MAX_THREAD_NAME_SIZE] = "";
            if(!ThreadGetName(PrevThreadId, threadName2) || threadName2[0] == 0)
                strcpy_s(threadName2, formatpidtid(PrevThreadId).c_str());
            _snprintf_s(threadswitch, _TRUNCATE, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", " (switched from %s)")), threadName2);
            PrevThreadId = currentThreadId;
        }
    }
    char title[deflen] = "";
    char threadName[MAX_THREAD_NAME_SIZE + 1] = "";
    if(ThreadGetName(currentThreadId, threadName) && *threadName)
        strcat_s(threadName, " ");
    // choose between title here
    auto debugeeName = bWindowLongPath ? szDebuggeePath : szBaseFileName;
    _snprintf_s(title, _TRUNCATE, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "%s - PID: %s - %sThread: %s%s%s")), debugeeName, formatpidtid(fdProcessInfo->dwProcessId).c_str(), modtext, threadName, formatpidtid(currentThreadId).c_str(), threadswitch);

    GuiUpdateWindowTitle(title);
}

void DebugUpdateGui(duint disasm_addr, bool stack)
{
    if(GuiIsUpdateDisabled())
        return;
    duint cip = GetContextDataEx(hActiveThread, UE_CIP);
    //Check if the addresses are in the memory map and force update if they are not
    if(!MemIsValidReadPtr(disasm_addr, true) || !MemIsValidReadPtr(cip, true))
        MemUpdateMap();
    else
        MemUpdateMapAsync();
    if(MemIsValidReadPtr(disasm_addr))
    {
        if(bEnableSourceDebugging)
        {
            char szSourceFile[MAX_STRING_SIZE] = "";
            int line = 0;
            if(SymGetSourceLine(cip, szSourceFile, &line))
                GuiLoadSourceFileEx(szSourceFile, cip);
        }
        GuiDisasmAt(disasm_addr, cip);
    }
    duint csp = GetContextDataEx(hActiveThread, UE_CSP);
    if(stack)
        DebugUpdateStack(csp, csp);
    static volatile duint cacheCsp = 0;
    if(csp != cacheCsp)
    {
#ifdef _WIN64
        InterlockedExchange((volatile unsigned long long*)&cacheCsp, csp);
#else
        InterlockedExchange((volatile unsigned long*)&cacheCsp, csp);
#endif //_WIN64
        updateCallStackAsync(csp);
        updateSEHChainAsync();
    }
    DebugUpdateTitle(disasm_addr, true);
    GuiUpdateRegisterView();
    GuiUpdateDisassemblyView();
    GuiUpdateThreadView();
    GuiUpdateSideBar();
}

void GuiSetDebugStateAsync(DBGSTATE state)
{
    GuiSetDebugStateFast(state);
    static TaskThread_<decltype(&GuiSetDebugState), DBGSTATE> GuiSetDebugStateTask(&GuiSetDebugState, 300);
    GuiSetDebugStateTask.WakeUp(state);
}

void DebugUpdateGuiAsync(duint disasm_addr, bool stack)
{
    static TaskThread_<decltype(&DebugUpdateGui), duint, bool> DebugUpdateGuiTask(&DebugUpdateGui);
    DebugUpdateGuiTask.WakeUp(disasm_addr, stack);
}

void DebugUpdateTitleAsync(duint disasm_addr, bool analyzeThreadSwitch)
{
    static TaskThread_<decltype(&DebugUpdateTitle), duint, bool> DebugUpdateTitleTask(&DebugUpdateTitle);
    DebugUpdateTitleTask.WakeUp(disasm_addr, analyzeThreadSwitch);
}

void DebugUpdateGuiSetStateAsync(duint disasm_addr, DBGSTATE state)
{
    // call paused routine to clean up various tracing states.
    if(state == paused)
        cbDebuggerPaused();
    GuiSetDebugStateAsync(state);
    DebugUpdateGuiAsync(disasm_addr, true);
}

void DebugUpdateBreakpointsViewAsync()
{
    static TaskThread_<decltype(&GuiUpdateBreakpointsView)> BreakpointsUpdateGuiTask(&GuiUpdateBreakpointsView);
    BreakpointsUpdateGuiTask.WakeUp();
}

void DebugUpdateStack(duint dumpAddr, duint csp, bool forceDump)
{
    if(GuiIsUpdateDisabled())
        return;
    if(!forceDump && bFreezeStack)
        dumpAddr = 0;
    GuiStackDumpAt(dumpAddr, csp);
    GuiUpdateArgumentWidget();
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
    if(!bp.name.empty())
        dprintf(QT_TRANSLATE_NOOP("DBG", "%s breakpoint \"%s\" at %s!\n"), bptype, bp.name.c_str(), symbolicname.c_str());
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "%s breakpoint at %s!\n"), bptype, symbolicname.c_str());
}

static void printHwBpInfo(const BREAKPOINT & bp)
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
    char* bptype;
    switch(TITANGETTYPE(bp.titantype)) //type
    {
    case UE_HARDWARE_EXECUTE:
        bptype = _strdup(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "execute")));
        bpsize = "";
        break;
    case UE_HARDWARE_READWRITE:
        bptype = _strdup(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "read/write")));
        break;
    case UE_HARDWARE_WRITE:
        bptype = _strdup(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "write")));
        break;
    default:
        bptype = _strdup(" ");
    }
    auto symbolicname = SymGetSymbolicName(bp.addr);
    if(!bp.name.empty())
        dprintf(QT_TRANSLATE_NOOP("DBG", "Hardware breakpoint (%s%s) \"%s\" at %s!\n"), bpsize, bptype, bp.name.c_str(), symbolicname.c_str());
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "Hardware breakpoint (%s%s) at %s!\n"), bpsize, bptype, symbolicname.c_str());
    free(bptype);
}

static void printMemBpInfo(const BREAKPOINT & bp, const void* ExceptionAddress)
{
    char* bptype;
    switch(bp.titantype)
    {
    case UE_MEMORY_READ:
        bptype = _strdup(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", " (read)")));
        break;
    case UE_MEMORY_WRITE:
        bptype = _strdup(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", " (write)")));
        break;
    case UE_MEMORY_EXECUTE:
        bptype = _strdup(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", " (execute)")));
        break;
    case UE_MEMORY:
        bptype = _strdup(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", " (read/write/execute)")));
        break;
    default:
        bptype = _strdup("");
    }
    auto symbolicname = SymGetSymbolicName(bp.addr);
    if(!bp.name.empty())
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Memory breakpoint%s \"%s\" at %s, exception address: %s!\n"),
                bptype,
                bp.name.c_str(),
                symbolicname.c_str(),
                SymGetSymbolicName(duint(ExceptionAddress)).c_str()
               );
    }
    else
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Memory breakpoint%s at %s, exception address: %s!\n"),
                bptype,
                symbolicname.c_str(),
                SymGetSymbolicName(duint(ExceptionAddress)).c_str()
               );
    }
    free(bptype);
}

static void printDllBpInfo(const BREAKPOINT & bp)
{
    char* bptype;
    switch(bp.titantype)
    {
    case UE_ON_LIB_LOAD:
        bptype = _strdup(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "DLL Load")));
        break;
    case UE_ON_LIB_UNLOAD:
        bptype = _strdup(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "DLL Unload")));
        break;
    case UE_ON_LIB_ALL:
        bptype = _strdup(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "DLL Load and unload")));
        break;
    default:
        bptype = _strdup("");
    }
    if(!bp.name.empty())
        dprintf(QT_TRANSLATE_NOOP("DBG", "DLL Breakpoint %s (%s): Module %s\n"), bp.name.c_str(), bptype, bp.module.c_str());
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "DLL Breakpoint (%s): Module %s\n"), bptype, bp.module.c_str());
    free(bptype);
}

static void printExceptionBpInfo(const BREAKPOINT & bp, duint CIP)
{
    if(!bp.name.empty())
        dprintf(QT_TRANSLATE_NOOP("DBG", "Exception Breakpoint %s (%p) at %p!\n"), bp.name.c_str(), bp.addr, CIP);
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "Exception Breakpoint %s (%p) at %p!\n"), ExceptionCodeToName((unsigned int)bp.addr).c_str(), bp.addr, CIP);
}

// Return value: 0:False, 1:True, -1:Error
static char getConditionValue(const char* expression)
{
    auto word = *(uint16*)expression;
    if(word == '0') // short circuit for condition "0\0"
        return 0; //False
    if(word == '1') //short circuit for condition "1\0"
        return 1; //True
    duint value;
    if(valfromstring(expression, &value))
        return value != 0 ? 1 : 0;
    else
        return -1; // Error
}

static char getConditionValue(const std::string & expression)
{
    if(expression.empty())
        return -1; // Error
    return getConditionValue(expression.c_str());
}

void cbPauseBreakpoint()
{
    dputs(QT_TRANSLATE_NOOP("DBG", "paused!"));
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    auto CIP = GetContextDataEx(hActiveThread, UE_CIP);
    DeleteBPX(CIP);
    DebugUpdateGuiSetStateAsync(CIP, paused);
    _dbg_animatestop(); // Stop animating when paused
    // Trace record
    dbgtraceexecute(CIP);
    //lock
    lock(WAITID_RUN);
    // Plugin callback
    PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    dbgsetforeground();
    dbgsetskipexceptions(false);
    wait(WAITID_RUN);
}

static void handleBreakCondition(const BREAKPOINT & bp, const void* ExceptionAddress, duint CIP)
{
    if(bp.singleshoot)
    {
        BpDelete(bp.addr, bp.type);
        if(bp.type == BPHARDWARE)  // Remove this singleshoot hardware breakpoint
        {
            if(TITANDRXVALID(bp.titantype) && !DeleteHardwareBreakPoint(TITANGETDRX(bp.titantype)))
                dprintf(QT_TRANSLATE_NOOP("DBG", "Delete hardware breakpoint failed: %p (DeleteHardwareBreakPoint)\n"), bp.addr);
        }
    }
    if(!bp.silent)
    {
        switch(bp.type)
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
        case BPDLL:
            printDllBpInfo(bp);
            break;
        case BPEXCEPTION:
            printExceptionBpInfo(bp, CIP);
            break;
        default:
            break;
        }
    }
    DebugUpdateGuiSetStateAsync(CIP, paused);
    // Plugin callback
    PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    _dbg_animatestop(); // Stop animating when a breakpoint is hit
}

static void cbGenericBreakpoint(BP_TYPE bptype, const void* ExceptionAddress = nullptr)
{
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    auto CIP = GetContextDataEx(hActiveThread, UE_CIP);

    //handle process cookie retrieval
    if(bptype == BPNORMAL && cookie.HandleBreakpoint(CIP))
        return;

    BREAKPOINT* bpPtr = nullptr;
    //NOTE: this locking is very tricky, make sure you understand it before modifying anything
    EXCLUSIVE_ACQUIRE(LockBreakpoints);
    duint breakpointExceptionAddress = 0;
    switch(bptype)
    {
    case BPNORMAL:
        bpPtr = BpInfoFromAddr(BPNORMAL, CIP);
        breakpointExceptionAddress = CIP;
        break;
    case BPHARDWARE:
        bpPtr = BpInfoFromAddr(BPHARDWARE, duint(ExceptionAddress));
        breakpointExceptionAddress = duint(ExceptionAddress);
        break;
    case BPMEMORY:
        bpPtr = BpInfoFromAddr(BPMEMORY, MemFindBaseAddr(duint(ExceptionAddress), nullptr, true));
        breakpointExceptionAddress = duint(ExceptionAddress);
        break;
    case BPDLL:
        bpPtr = BpInfoFromAddr(BPDLL, BpGetDLLBpAddr(reinterpret_cast<const char*>(ExceptionAddress)));
        breakpointExceptionAddress = 0; //makes no sense
        break;
    case BPEXCEPTION:
        bpPtr = BpInfoFromAddr(BPEXCEPTION, ((EXCEPTION_DEBUG_INFO*)ExceptionAddress)->ExceptionRecord.ExceptionCode);
        breakpointExceptionAddress = (duint)((EXCEPTION_DEBUG_INFO*)ExceptionAddress)->ExceptionRecord.ExceptionAddress;
        break;
    default:
        break;
    }
    varset("$breakpointexceptionaddress", breakpointExceptionAddress, true);
    if(!(bpPtr && bpPtr->enabled)) //invalid / disabled breakpoint hit (most likely a bug)
    {
        if(bptype != BPDLL || !BpUpdateDllPath(reinterpret_cast<const char*>(ExceptionAddress), &bpPtr))
        {
            // release the breakpoint lock to prevent deadlocks during the wait
            EXCLUSIVE_RELEASE();
            dputs(QT_TRANSLATE_NOOP("DBG", "Breakpoint reached not in list!"));
            DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), paused);
            //lock
            lock(WAITID_RUN);
            // Plugin callback
            PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
            plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
            dbgsetforeground();
            dbgsetskipexceptions(false);
            wait(WAITID_RUN);
            return;
        }
    }

    // increment hit count
    InterlockedIncrement((volatile long*)&bpPtr->hitcount);

    // copy the breakpoint structure and release the breakpoint lock to prevent deadlocks during the wait
    auto bp = *bpPtr;
    EXCLUSIVE_RELEASE();

    if(bptype != BPDLL && bptype != BPEXCEPTION)
        bp.addr += ModBaseFromName(bp.module.c_str());
    bp.active = true; //a breakpoint that has been hit is active

    varset("$breakpointcounter", bp.hitcount, true); //save the breakpoint counter as a variable

    //get condition values
    char breakCondition;
    char logCondition;
    char commandCondition;
    if(!bp.breakCondition.empty())
    {
        breakCondition = getConditionValue(bp.breakCondition);
        if(breakCondition == -1)
            dputs(QT_TRANSLATE_NOOP("DBG", "Error when evaluating break condition."));
    }
    else
        breakCondition = 1; //break if no condition is set
    if(bp.fastResume && breakCondition == 0) // fast resume: ignore GUI/Script/Plugin/Other if the debugger would not break
        return;
    if(!bp.logCondition.empty())
    {
        logCondition = getConditionValue(bp.logCondition);
        if(logCondition == -1)
        {
            dputs_untranslated(stringformatinline(bp.logText).c_str()); // Always log when an error occurs, and log before the error message
            dputs(QT_TRANSLATE_NOOP("DBG", "Error when evaluating log condition.")); // Then an error message is printed
            breakCondition = -1; // Force breaking when an error occurs
            logCondition = 0; // No need to log twice
        }
    }
    else
        logCondition = 1; //log if no condition is set
    if(!bp.commandCondition.empty())
    {
        commandCondition = getConditionValue(bp.commandCondition);
        if(commandCondition == -1)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Error when evaluating command condition."));
            breakCondition = -1; // Force breaking when an error occurs
            commandCondition = 0; // Don't execute any command if an error occurs
        }
    }
    else
    {
        // NOTE: This behavior was changed in a breaking way, but too many people were confused
        if(breakCondition != -1)
            commandCondition = 1; // If no condition is set, always execute the command
        else
            commandCondition = 0; // Don't execute any command if an error occurs
    }

    lock(WAITID_RUN);

    PLUG_CB_BREAKPOINT bpInfo;
    BRIDGEBP bridgebp;
    memset(&bridgebp, 0, sizeof(bridgebp));
    bpInfo.breakpoint = &bridgebp;
    BpToBridge(&bp, &bridgebp);
    plugincbcall(CB_BREAKPOINT, &bpInfo);

    // Trace record
    dbgtraceexecute(CIP);

    // Watchdog
    cbCheckWatchdog(0, nullptr);

    // Update breakpoint view
    DebugUpdateBreakpointsViewAsync();

    DWORD logFileError = ERROR_SUCCESS;
    if(!bp.logText.empty() && logCondition == 1) //log
    {
        auto formattedText = stringformatinline(bp.logText);
        if(!bp.logFile.empty())
        {
            auto logFile = BpLogFileOpen(bp.logFile);
            if(logFile == INVALID_HANDLE_VALUE)
            {
                // Pause and display the error
                breakCondition = 1;
                logFileError = GetLastError();

                // Show the log in the regular log tab
                dputs_untranslated(formattedText.c_str());
            }
            else
            {
                formattedText += "\n";
                DWORD written = 0;
                if(WriteFile(logFile, formattedText.c_str(), (DWORD)formattedText.length(), &written, nullptr) == FALSE)
                {
                    // WriteFile failed
                    logFileError = GetLastError();
                };
            }
        }
        else
        {
            dputs_untranslated(formattedText.c_str());
        }
    }
    if(!bp.commandText.empty() && commandCondition) //command
    {
        //TODO: commands like run/step etc will fuck up your shit
        varset("$breakpointcondition", (breakCondition != 0) ? 1 : 0, false);
        varset("$breakpointlogcondition", (logCondition != 0) ? 1 : 0, true);
        duint script_breakcondition;
        if(!cmddirectexec(bp.commandText.c_str()))
        {
            breakCondition = -1; // Error executing the command (The error message is probably already printed)
        }
        else if(varget("$breakpointcondition", &script_breakcondition, nullptr, nullptr))
        {
            if(script_breakcondition != 0)
                breakCondition = 1;
            else
                breakCondition = 0; // It is safe to clear break condition here because when an error occurs, no command can be executed
        }
    }
    if(breakCondition != 0) //break the debugger
    {
        handleBreakCondition(bp, ExceptionAddress, CIP);
        dbgsetforeground();
        dbgsetskipexceptions(false);
    }
    else //resume immediately
        unlock(WAITID_RUN);

    // Make sure the log file error is displayed last
    if(logFileError != ERROR_SUCCESS)
    {
        String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", GetLastError()));
        dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to open breakpoint log: %s (%s)\n"), bp.logFile.c_str(), error.c_str());
    }

    //wait until the user resumes
    wait(WAITID_RUN);
}

void cbUserBreakpoint()
{
    lastExceptionInfo = ((DEBUG_EVENT*)GetDebugData())->u.Exception;
    cbGenericBreakpoint(BPNORMAL);
}

void cbHardwareBreakpoint(const void* ExceptionAddress)
{
    lastExceptionInfo = ((DEBUG_EVENT*)GetDebugData())->u.Exception;
    cbGenericBreakpoint(BPHARDWARE, ExceptionAddress);
}

void cbMemoryBreakpoint(const void* ExceptionAddress)
{
    lastExceptionInfo = ((DEBUG_EVENT*)GetDebugData())->u.Exception;
    cbGenericBreakpoint(BPMEMORY, ExceptionAddress);
}

void cbRunToUserCodeBreakpoint(const void* ExceptionAddress)
{
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    auto CIP = GetContextDataEx(hActiveThread, UE_CIP);
    dprintf(QT_TRANSLATE_NOOP("DBG", "User code reached at %s"), SymGetSymbolicName(CIP).c_str());
    // lock
    lock(WAITID_RUN);
    // Trace record
    dbgtraceexecute(CIP);
    // Update GUI
    DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), paused);
    // Plugin callback
    PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    dbgsetforeground();
    dbgsetskipexceptions(false);
    wait(WAITID_RUN);
}

static BOOL CALLBACK SymRegisterCallbackProc64(HANDLE, ULONG ActionCode, ULONG64 CallbackData, ULONG64)
{
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
        else if(sscanf_s(text, "%*s %d percent", &percent) == 1 || sscanf_s(text, "%d percent", &percent) == 1)
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
            if(oldbytes != bp->oldbytes && !bIgnoreInconsistentBreakpoints)
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Breakpoint %p has been disabled because the bytes don't match! Expected: %02X %02X, Found: %02X %02X\n"),
                        bp->addr,
                        ((unsigned char*)&bp->oldbytes)[0], ((unsigned char*)&bp->oldbytes)[1],
                        ((unsigned char*)&oldbytes)[0], ((unsigned char*)&oldbytes)[1]);
                BpEnable(bp->addr, BPNORMAL, false);
            }
            else if(!SetBPX(bp->addr, bp->titantype, cbUserBreakpoint))
                dprintf(QT_TRANSLATE_NOOP("DBG", "Could not set breakpoint %p! (SetBPX)\n"), bp->addr);
        }
        else
            dprintf(QT_TRANSLATE_NOOP("DBG", "MemRead failed on breakpoint address %p!\n"), bp->addr);
    }
    break;

    case BPMEMORY:
    {
        duint size = 0;
        MemFindBaseAddr(bp->addr, &size);
        if(!SetMemoryBPXEx(bp->addr, size, bp->titantype, !bp->singleshoot, cbMemoryBreakpoint))
            dprintf(QT_TRANSLATE_NOOP("DBG", "Could not set memory breakpoint %p! (SetMemoryBPXEx)\n"), bp->addr);
    }
    break;

    case BPHARDWARE:
    {
        DWORD drx = 0;
        if(!GetUnusedHardwareBreakPointRegister(&drx))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "You can only set 4 hardware breakpoints"));
            return false;
        }
        int titantype = bp->titantype;
        TITANSETDRX(titantype, drx);
        BpSetTitanType(bp->addr, BPHARDWARE, titantype);
        if(!SetHardwareBreakPoint(bp->addr, drx, TITANGETTYPE(bp->titantype), TITANGETSIZE(bp->titantype), cbHardwareBreakpoint))
            dprintf(QT_TRANSLATE_NOOP("DBG", "Could not set hardware breakpoint %p! (SetHardwareBreakPoint)\n"), bp->addr);
        else
            dprintf(QT_TRANSLATE_NOOP("DBG", "Set hardware breakpoint on %p!\n"), bp->addr);
    }
    break;

    default:
        break;
    }
    return true;
}

bool cbSetDLLBreakpoints(const BREAKPOINT* bp)
{
    if(!bp->enabled)
        return true;
    if(bp->type != BPDLL)
        return true;
    dbgsetdllbreakpoint(bp->module.c_str(), bp->titantype, bp->singleshoot);
    return true;
}

EXCEPTION_DEBUG_INFO & getLastExceptionInfo()
{
    return lastExceptionInfo;
}

static bool cbRemoveModuleBreakpoints(const BREAKPOINT* bp)
{
    if(!bp->enabled)
        return true;
    switch(bp->type)
    {
    case BPNORMAL:
        if(!DeleteBPX(bp->addr))
            dprintf(QT_TRANSLATE_NOOP("DBG", "Could not delete breakpoint %p! (DeleteBPX)\n"), bp->addr);
        break;
    case BPMEMORY:
        if(!RemoveMemoryBPX(bp->addr, 0))
            dprintf(QT_TRANSLATE_NOOP("DBG", "Could not delete memory breakpoint %p! (RemoveMemoryBPX)\n"), bp->addr);
        break;
    case BPHARDWARE:
        if(TITANDRXVALID(bp->titantype) && !DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype)))
            dprintf(QT_TRANSLATE_NOOP("DBG", "Could not delete hardware breakpoint %p! (DeleteHardwareBreakPoint)\n"), bp->addr);
        break;
    default:
        break;
    }
    return true;
}

void DebugRemoveBreakpoints()
{
    BpEnumAll(cbRemoveModuleBreakpoints);
}

void DebugSetBreakpoints()
{
    BpEnumAll(cbSetModuleBreakpoints);
}

void cbStep()
{
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    duint CIP = GetContextDataEx(hActiveThread, UE_CIP);
    if(bAbortStepping || !stepRepeat || !--stepRepeat)
    {
        DebugUpdateGuiSetStateAsync(CIP, paused);
        // Trace record
        dbgtraceexecute(CIP);
        // Plugin interaction
        PLUG_CB_STEPPED stepInfo;
        stepInfo.reserved = 0;
        //lock
        lock(WAITID_RUN);
        // Plugin callback
        PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        dbgsetforeground();
        dbgsetskipexceptions(false);
        plugincbcall(CB_STEPPED, &stepInfo);
        wait(WAITID_RUN);
    }
    else
    {
        dbgtraceexecute(CIP);
        (bRepeatIn ? StepIntoWow64 : StepOverWrapper)(cbStep);
    }
}

static void cbRtrFinalStep(bool checkRepeat)
{
    if(bAbortStepping || !checkRepeat || !stepRepeat || !--stepRepeat)
    {
        hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
        duint CIP = GetContextDataEx(hActiveThread, UE_CIP);
        // Trace record
        dbgtraceexecute(CIP);
        DebugUpdateGuiSetStateAsync(CIP, paused);
        //lock
        lock(WAITID_RUN);
        // Plugin callback
        PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        dbgsetforeground();
        dbgsetskipexceptions(false);
        wait(WAITID_RUN);
    }
    else
        StepOverWrapper(cbRtrStep);
}

void cbRtrStep()
{
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    unsigned char data[MAX_DISASM_BUFFER];
    memset(data, 0x90, sizeof(data));
    duint cip = GetContextDataEx(hActiveThread, UE_CIP);
    duint csp = GetContextDataEx(hActiveThread, UE_CSP);
    MemRead(cip, data, sizeof(data));
    dbgtraceexecute(cip);
    bool reachedReturn = false;
    if(gRtrPreviousCSP <= csp) //"Run until return" should break only if RSP is bigger than or equal to current value
    {
        if(data[0] == 0xC3 || data[0] == 0xC2)  //retn instruction
            reachedReturn = true;
        else if(data[0] == 0x26 || data[0] == 0x36 || data[0] == 0x2e || data[0] == 0x3e || (data[0] >= 0x64 && data[0] <= 0x67) || data[0] == 0xf2 || data[0] == 0xf3 //instruction prefixes
#ifdef _WIN64
                || (data[0] >= 0x40 && data[0] <= 0x4f)
#endif //_WIN64
               )
        {
            Zydis zydis;
            if(zydis.Disassemble(cip, data) && zydis.IsRet())
                reachedReturn = true;
        }
    }

    if(reachedReturn || bAbortStepping)
    {
        // Clean up internal state
        gRtrPreviousCSP = 0;
        cbRtrFinalStep(true);
    }
    else
    {
        StepOverWrapper(cbRtrStep);
    }
}

static void __forceinline cbTraceUniversalConditionalStep(duint cip, STEPFUNCTION stepFunction, TITANCBSTEP callback, bool forceBreakTrace)
{
    PLUG_CB_TRACEEXECUTE info;
    info.cip = cip;
    auto breakCondition = traceState.BreakTrace();
    if(forceBreakTrace && breakCondition == 0)
        breakCondition = 1;
    if(breakCondition == -1)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error when evaluating break condition."));
    }
    info.stop = (breakCondition != 0);
    if(traceState.IsExtended()) //only set when needed
        varset("$tracecounter", traceState.StepCount(), true);
    plugincbcall(CB_TRACEEXECUTE, &info);
    breakCondition = info.stop;
    auto logCondition = traceState.EvaluateLog(); //true
    auto cmdCondition = traceState.EvaluateCmd(breakCondition == 1 ? 1 : 0); //breakCondition
    if(logCondition != 0) //log
    {
        traceState.LogWrite(stringformatinline(traceState.LogText()));
    }
    if(logCondition == -1)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error when evaluating log condition."));
        logCondition = 1;
        breakCondition = -1;
    }
    if(cmdCondition == -1)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error when evaluating command condition."));
        cmdCondition = 0;
        breakCondition = -1;
    }
    else if(cmdCondition  == 1 && breakCondition != -1) //command
    {
        //TODO: commands like run/step etc will fuck up your shit
        varset("$tracecondition", breakCondition ? 1 : 0, false);
        varset("$tracelogcondition", logCondition ? 1 : 0, true);
        duint script_breakcondition;
        if(cmddirectexec(traceState.CmdText().c_str()))
        {
            if(varget("$tracecondition", &script_breakcondition, nullptr, nullptr))
                breakCondition = script_breakcondition != 0;
        }
        else
        {
            breakCondition = -1; //Error when executing the command
        }
    }
    if(breakCondition != 0 || traceState.ForceBreakTrace()) //break the debugger
    {
        auto steps = dbgcleartracestate();
        varset("$tracecounter", steps, true);
#ifdef _WIN64
        dprintf(QT_TRANSLATE_NOOP("DBG", "Trace finished after %llu steps!\n"), steps);
#else //x86
        dprintf(QT_TRANSLATE_NOOP("DBG", "Trace finished after %u steps!\n"), steps);
#endif //_WIN64
        cbRtrFinalStep(false); // TODO: replace with generic pause function
    }
    else //continue tracing
    {
        dbgtraceexecute(cip);
        stepFunction(callback);
    }
}

void cbTraceXConditionalStep(STEPFUNCTION stepFunction, TITANCBSTEP callback)
{
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    cbTraceUniversalConditionalStep(GetContextDataEx(hActiveThread, UE_CIP), stepFunction, callback, false);
}

static void cbTraceXXTraceRecordStep(STEPFUNCTION stepFunction, bool bInto, TITANCBSTEP callback)
{
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    auto cip = GetContextDataEx(hActiveThread, UE_CIP);
    auto forceBreakTrace = TraceRecord.getTraceRecordType(cip) != TraceRecordManager::TraceRecordNone && (TraceRecord.getHitCount(cip) == 0) ^ bInto;
    cbTraceUniversalConditionalStep(cip, stepFunction, callback, forceBreakTrace);
}

#define STEP_FUNCTION(into) (into ? StepIntoWow64 : StepOverWrapper)

void cbTraceOverConditionalStep()
{
    cbTraceXConditionalStep(STEP_FUNCTION(false), cbTraceOverConditionalStep);
}

void cbTraceIntoConditionalStep()
{
    cbTraceXConditionalStep(STEP_FUNCTION(true), cbTraceIntoConditionalStep);
}

void cbTraceIntoBeyondTraceRecordStep()
{
    cbTraceXXTraceRecordStep(STEP_FUNCTION(true), false, cbTraceIntoBeyondTraceRecordStep);
}

void cbTraceOverBeyondTraceRecordStep()
{
    cbTraceXXTraceRecordStep(STEP_FUNCTION(false), false, cbTraceOverBeyondTraceRecordStep);
}

void cbTraceIntoIntoTraceRecordStep()
{
    cbTraceXXTraceRecordStep(STEP_FUNCTION(true), true, cbTraceIntoIntoTraceRecordStep);
}

void cbTraceOverIntoTraceRecordStep()
{
    cbTraceXXTraceRecordStep(STEP_FUNCTION(false), true, cbTraceOverIntoTraceRecordStep);
}

static void cbCreateProcess(CREATE_PROCESS_DEBUG_INFO* CreateProcessInfo)
{
    hActiveThread = CreateProcessInfo->hThread;
    fdProcessInfo->hProcess = CreateProcessInfo->hProcess;
    fdProcessInfo->hThread = CreateProcessInfo->hThread;
    varset("$hp", (duint)fdProcessInfo->hProcess, true);

    auto base = (duint)CreateProcessInfo->lpBaseOfImage;
    pDebuggedBase = base; //debugged base = executable

    char DebugFileName[MAX_PATH] = "";
    if(!GetFileNameFromHandle(CreateProcessInfo->hFile, DebugFileName, _countof(DebugFileName)))
    {
        if(!GetFileNameFromProcessHandle(CreateProcessInfo->hProcess, DebugFileName, _countof(DebugFileName)))
            strcpy_s(DebugFileName, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "??? (GetFileNameFromHandle failed)")));
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "Process Started: %p %s\n"), base, DebugFileName);

    char* cmdline = nullptr;
    if(dbggetcmdline(&cmdline, nullptr, fdProcessInfo->hProcess))
    {
        // Parse the command line from the debuggee
        int argc = 0;
        wchar_t** argv = CommandLineToArgvW(StringUtils::Utf8ToUtf16(cmdline).c_str(), &argc);

        // Print the command line to the log
        dprintf_untranslated("  %s\n", cmdline);
        for(int i = 0; i < argc; i++)
            dprintf_untranslated("  argv[%i]: %s\n", i, StringUtils::Utf16ToUtf8(argv[i]).c_str());

        LocalFree(argv);
        efree(cmdline);
    }

    //update memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    //dump/disassemble somewhere
    DebugUpdateGui(pDebuggedBase + pDebuggedEntry, true);
    GuiDumpAt(pDebuggedBase);

    ModLoad(base, 1, DebugFileName);

    char modname[MAX_MODULE_SIZE] = "";
    if(ModNameFromAddr(base, modname, true))
        BpEnumAll(cbSetModuleBreakpoints, modname, base);
    BpEnumAll(cbSetDLLBreakpoints);
    BpEnumAll(cbSetModuleBreakpoints, "");

    DbCheckHash(ModContentHashFromAddr(pDebuggedBase)); //Check hash mismatch
    if(!bFileIsDll && !bIsAttached) //Set entry breakpoint
    {
        char command[deflen] = "";

        if(settingboolget("Events", "TlsCallbacks"))
        {
            SHARED_ACQUIRE(LockModules);
            auto modInfo = ModInfoFromAddr(base);
            int invalidCount = 0;
            for(size_t i = 0; i < modInfo->tlsCallbacks.size(); i++)
            {
                auto callbackVA = modInfo->tlsCallbacks.at(i);
                if(MemIsValidReadPtr(callbackVA))
                {
                    String breakpointname = StringUtils::sprintf(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "TLS Callback %d")), i + 1);
                    sprintf_s(command, "bp %p,\"%s\",ss", (void*)callbackVA, breakpointname.c_str());
                    cmddirectexec(command);
                }
                else
                    invalidCount++;
            }
            if(invalidCount)
                dprintf(QT_TRANSLATE_NOOP("DBG", "%d invalid TLS callback addresses...\n"), invalidCount);
        }

        if(settingboolget("Events", "EntryBreakpoint") && !bEntryIsInMzHeader)
        {
            sprintf_s(command, "bp %p,\"%s\",ss", (void*)(pDebuggedBase + pDebuggedEntry), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "entry breakpoint")));
            cmddirectexec(command);
        }
    }
    else if(bFileIsDll && strstr(DebugFileName, "DLLLoader" ArchValue("32", "64"))) //DLL Loader
        gDllLoader = StringUtils::Utf8ToUtf16(DebugFileName);

    DebugUpdateBreakpointsViewAsync();

    //update thread list
    CREATE_THREAD_DEBUG_INFO threadInfo;
    threadInfo.hThread = CreateProcessInfo->hThread;
    threadInfo.lpStartAddress = CreateProcessInfo->lpStartAddress;
    threadInfo.lpThreadLocalBase = CreateProcessInfo->lpThreadLocalBase;
    ThreadCreate(&threadInfo);

    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);

    //call plugin callback
    PLUG_CB_CREATEPROCESS callbackInfo;
    callbackInfo.CreateProcessInfo = CreateProcessInfo;
    IMAGEHLP_MODULE64 modInfoUtf8;
    memset(&modInfoUtf8, 0, sizeof(modInfoUtf8));
    modInfoUtf8.SizeOfStruct = sizeof(modInfoUtf8);
    modInfoUtf8.BaseOfImage = base;
    modInfoUtf8.ImageSize = 0;
    modInfoUtf8.TimeDateStamp = 0;
    modInfoUtf8.CheckSum = 0;
    modInfoUtf8.NumSyms = 1;
    modInfoUtf8.SymType = SymDia;
    strncpy_s(modInfoUtf8.ModuleName, DebugFileName, _TRUNCATE);
    strncpy_s(modInfoUtf8.ImageName, DebugFileName, _TRUNCATE);
    strncpy_s(modInfoUtf8.LoadedImageName, "", _TRUNCATE);
    strncpy_s(modInfoUtf8.LoadedPdbName, "", _TRUNCATE);

    modInfoUtf8.CVSig = 0;
    strncpy_s(modInfoUtf8.CVData, "", _TRUNCATE);
    modInfoUtf8.PdbSig = 0;
    modInfoUtf8.PdbAge = 0;
    modInfoUtf8.PdbUnmatched = FALSE;
    modInfoUtf8.DbgUnmatched = FALSE;
    modInfoUtf8.LineNumbers = TRUE;
    modInfoUtf8.GlobalSymbols = 0;
    modInfoUtf8.TypeInfo = TRUE;
    modInfoUtf8.SourceIndexed = TRUE;
    modInfoUtf8.Publics = TRUE;

    callbackInfo.modInfo = &modInfoUtf8;
    callbackInfo.DebugFileName = DebugFileName;
    callbackInfo.fdProcessInfo = fdProcessInfo;
    plugincbcall(CB_CREATEPROCESS, &callbackInfo);
}

static void cbExitProcess(EXIT_PROCESS_DEBUG_INFO* ExitProcess)
{
    {
        auto exitCode = ExitProcess->dwExitCode;
        auto exitDescription = StringUtils::sprintf("0x%X (%d)", exitCode, exitCode);
        if((exitCode & 0x80000000) != 0)
        {
            auto statusName = NtStatusCodeToName(exitCode);
            if(!statusName.empty())
                exitDescription = StringUtils::sprintf("0x%X (%s)", exitCode, statusName.c_str());
        }
        dprintf(QT_TRANSLATE_NOOP("DBG", "Process stopped with exit code %s\n"), exitDescription.c_str());
    }

    const bool breakHere = settingboolget("Events", "NtTerminateProcess");
    if(breakHere)
    {
        // lock
        DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), paused);
        lock(WAITID_RUN);
    }
    // plugin callback
    PLUG_CB_EXITPROCESS callbackInfo;
    callbackInfo.ExitProcess = ExitProcess;
    plugincbcall(CB_EXITPROCESS, &callbackInfo);
    _dbg_animatestop(); // Stop animating
    //history
    dbgcleartracestate();
    dbgClearRtuBreakpoints();
    HistoryClear();
    if(breakHere)
    {
        dbgsetforeground();
        dbgsetskipexceptions(false);
        wait(WAITID_RUN);
    }
}

static void cbCreateThread(CREATE_THREAD_DEBUG_INFO* CreateThread)
{
    ThreadCreate(CreateThread); //update thread list
    DWORD dwThreadId = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    hActiveThread = ThreadGetHandle(dwThreadId);

    PLUG_CB_CREATETHREAD callbackInfo;
    callbackInfo.CreateThread = CreateThread;
    callbackInfo.dwThreadId = dwThreadId;
    plugincbcall(CB_CREATETHREAD, &callbackInfo);

    auto entry = duint(CreateThread->lpStartAddress);
    auto parameter = GetContextDataEx(hActiveThread, ArchValue(UE_EBX, UE_RDX));
    dprintf(QT_TRANSLATE_NOOP("DBG", "Thread %s created, Entry: %s, Parameter: %s\n"),
            formatpidtid(dwThreadId).c_str(),
            SymGetSymbolicName(entry).c_str(),
            SymGetSymbolicName(parameter).c_str()
           );

    if(settingboolget("Events", "ThreadEntry"))
    {
        String command;
        command = StringUtils::sprintf("bp %p,\"%s %X\",ss", entry, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Thread Entry")), dwThreadId);
        cmddirectexec(command.c_str());
    }

    if(settingboolget("Events", "ThreadStart"))
    {
        HistoryClear();
        //update memory map
        MemUpdateMap();
        //update GUI
        DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), paused);
        //lock
        lock(WAITID_RUN);
        // Plugin callback
        PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        dbgsetforeground();
        dbgsetskipexceptions(false);
        wait(WAITID_RUN);
    }
    else
    {
        //insert the thread stack as a dummy page to prevent cache misses (issue #1475)
        NT_TIB tib;
        if(ThreadGetTib(ThreadGetLocalBase(dwThreadId), &tib))
        {
            MEMPAGE page;
            auto limit = duint(tib.StackLimit);
            auto base = duint(tib.StackBase);
            sprintf_s(page.info, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Thread %s Stack")), formatpidtid(dwThreadId).c_str());
            page.mbi.BaseAddress = page.mbi.AllocationBase = tib.StackLimit;
            page.mbi.Protect = page.mbi.AllocationProtect = PAGE_READWRITE;
            page.mbi.RegionSize = base - limit;
            page.mbi.State = MEM_COMMIT;
            page.mbi.Type = MEM_PRIVATE;

            EXCLUSIVE_ACQUIRE(LockMemoryPages);
            memoryPages.insert({ Range(limit, base - 1), page });
        }
    }
}

static void cbExitThread(EXIT_THREAD_DEBUG_INFO* ExitThread)
{
    // Not called when the main (last) thread exits. Instead
    // EXIT_PROCESS_DEBUG_EVENT is signalled.
    // Switch to the main thread (because the thread is terminated).
    hActiveThread = ThreadGetHandle(fdProcessInfo->dwThreadId);
    if(!hActiveThread)
    {
        std::vector<THREADINFO> threads;
        ThreadGetList(threads);
        if(threads.size())
            hActiveThread = threads[0].Handle;
        else
            dputs(QT_TRANSLATE_NOOP("DBG", "No threads left to switch to (bug?)"));
    }
    DWORD dwThreadId = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    PLUG_CB_EXITTHREAD callbackInfo;
    callbackInfo.ExitThread = ExitThread;
    callbackInfo.dwThreadId = dwThreadId;
    plugincbcall(CB_EXITTHREAD, &callbackInfo);
    HistoryClear();
    ThreadExit(dwThreadId);
    dprintf(QT_TRANSLATE_NOOP("DBG", "Thread %s exit\n"), formatpidtid(dwThreadId).c_str());

    if(settingboolget("Events", "ThreadEnd"))
    {
        //update GUI
        DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), paused);
        //lock
        lock(WAITID_RUN);
        // Plugin callback
        PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        dbgsetforeground();
        dbgsetskipexceptions(false);
        wait(WAITID_RUN);
    }
}

static DWORD WINAPI cbInitializationScriptThread(void*)
{
    Memory<char*> script(MAX_SETTING_SIZE + 1);
    if(BridgeSettingGet("Engine", "InitializeScript", script())) // Global script file
    {
        if(scriptLoadSync(script()))
        {
            if(scriptRunSync(0, true))
                scriptunload();
        }
        else
            dputs(QT_TRANSLATE_NOOP("DBG", "Error: Cannot load global initialization script."));
    }
    if(szDebuggeeInitializationScript[0] != 0)
    {
        if(scriptLoadSync(szDebuggeeInitializationScript))
        {
            if(scriptRunSync(0, true))
                scriptunload();
        }
        else
            dputs(QT_TRANSLATE_NOOP("DBG", "Error: Cannot load debuggee initialization script."));
    }
    return 0;
}

static void cbSystemBreakpoint(const void* ExceptionData) // TODO: System breakpoint event shouldn't be dropped
{
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);

    //Get on top of things
    SetForegroundWindow(GuiGetWindowHandle());

    // Update GUI (this should be the first triggered event)
    duint cip = GetContextDataEx(hActiveThread, UE_CIP);
    GuiDumpAt(MemFindBaseAddr(cip, 0, true)); //dump somewhere
    DebugUpdateGuiSetStateAsync(cip, running);

    MemInitRemoteProcessCookie(cookie.cookie);
    GuiUpdateAllViews();

    //log message
    dputs(QT_TRANSLATE_NOOP("DBG", "System breakpoint reached!"));
    dbgsetskipexceptions(false); //we are not skipping first-chance exceptions

    //plugin callbacks
    PLUG_CB_SYSTEMBREAKPOINT callbackInfo;
    callbackInfo.reserved = 0;
    plugincbcall(CB_SYSTEMBREAKPOINT, &callbackInfo);

    lock(WAITID_RUN); // Allow the user to run a script file now
    bool systemBreakpoint = settingboolget("Events", "SystemBreakpoint");
    if(!systemBreakpoint && bEntryIsInMzHeader)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "It has been detected that the debuggee entry point is in the MZ header of the executable. This will cause strange behavior, so the system breakpoint has been enabled regardless of your setting. Be careful!"));
        systemBreakpoint = true;
    }
    if(systemBreakpoint)
    {
        //lock
        GuiSetDebugStateAsync(paused);
        // Plugin callback
        PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        dbgsetforeground();
        CloseHandle(CreateThread(NULL, 0, cbInitializationScriptThread, NULL, 0, NULL));
    }
    else
    {
        CloseHandle(CreateThread(NULL, 0, cbInitializationScriptThread, NULL, 0, NULL));
        unlock(WAITID_RUN);
    }
    wait(WAITID_RUN);
}

static void cbLoadDll(LOAD_DLL_DEBUG_INFO* LoadDll)
{
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    void* base = LoadDll->lpBaseOfDll;

    char DLLDebugFileName[MAX_PATH] = "";
    if(!GetFileNameFromHandle(LoadDll->hFile, DLLDebugFileName, _countof(DLLDebugFileName)))
    {
        if(!GetFileNameFromModuleHandle(fdProcessInfo->hProcess, HMODULE(base), DLLDebugFileName, _countof(DLLDebugFileName)))
            strcpy_s(DLLDebugFileName, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "??? (GetFileNameFromHandle failed)")));
    }

    ModLoad((duint)base, 1, DLLDebugFileName);

    // Update memory map
    MemUpdateMapAsync();

    char modname[MAX_MODULE_SIZE] = "";
    if(ModNameFromAddr(duint(base), modname, true))
        BpEnumAll(cbSetModuleBreakpoints, modname, duint(base));
    DebugUpdateBreakpointsViewAsync();
    bool bAlreadySetEntry = false;

    char command[MAX_PATH * 2] = "";
    bool bIsDebuggingThis = false;
    if(bFileIsDll && !_stricmp(DLLDebugFileName, szDebuggeePath) && !bIsAttached) //Set entry breakpoint
    {
        CloseHandle(DebugDLLFileMapping);
        DebugDLLFileMapping = 0;
        bIsDebuggingThis = true;
        pDebuggedBase = (duint)base;
        DbCheckHash(ModContentHashFromAddr(pDebuggedBase)); //Check hash mismatch
        if(settingboolget("Events", "EntryBreakpoint"))
        {
            bAlreadySetEntry = true;
            sprintf_s(command, "bp %p,\"%s\",ss", (void*)(pDebuggedBase + pDebuggedEntry), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "entry breakpoint")));
            cmddirectexec(command);
        }
    }
    DebugUpdateBreakpointsViewAsync();

    int party = ModGetParty(duint(base));

    if(settingboolget("Events", "TlsCallbacks") && party != mod_system || settingboolget("Events", "TlsCallbacksSystem") && party == mod_system)
    {
        SHARED_ACQUIRE(LockModules);
        auto modInfo = ModInfoFromAddr(duint(base));
        int invalidCount = 0;
        for(size_t i = 0; i < modInfo->tlsCallbacks.size(); i++)
        {
            auto callbackVA = modInfo->tlsCallbacks.at(i);
            if(MemIsValidReadPtr(callbackVA))
            {
                if(bIsDebuggingThis)
                    sprintf_s(command, "bp %p,\"%s %u\",ss", (void*)callbackVA, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "TLS Callback")), (uint32_t)i + 1);
                else
                    sprintf_s(command, "bp %p,\"%s %u (%s)\",ss", (void*)callbackVA, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "TLS Callback")), (uint32_t)i + 1, modname);
                cmddirectexec(command);
            }
            else
                invalidCount++;
        }
        if(invalidCount)
            dprintf(QT_TRANSLATE_NOOP("DBG", "%d invalid TLS callback addresses...\n"), invalidCount);
    }

    auto shouldBreakOnDll = dbghandledllbreakpoint(modname, true);
    auto dllEntrySetting = party == mod_system ? "DllEntrySystem" : "DllEntry";
    if(!bAlreadySetEntry && (shouldBreakOnDll || settingboolget("Events", dllEntrySetting)))
    {
        auto entry = ModEntryFromAddr(duint(base));
        if(entry)
        {
            sprintf_s(command, "bp %p,\"DllMain (%s)\",ss", (void*)entry, modname);
            cmddirectexec(command);
        }
    }

    auto isNtdll = ModNameFromAddr(duint(base), modname, true) && scmp(modname, "ntdll.dll");
    if(isNtdll)
    {
        if(settingboolget("Misc", "QueryProcessCookie"))
            cookie.HandleNtdllLoad(bIsAttached);
        if(settingboolget("Misc", "TransparentExceptionStepping"))
            exceptionDispatchAddr = DbgValFromString("ntdll:KiUserExceptionDispatcher");
        //set debug flags
        if(dwDebugFlags != 0)
        {
            SHARED_ACQUIRE(LockModules);
            auto info = ModInfoFromAddr(duint(base));
            if(info->symbols->isOpen())
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Waiting until ntdll.dll symbols are loaded...\n"));
                info->symbols->waitUntilLoaded();
                SymbolInfo LdrpDebugFlags;
                if(info->symbols->findSymbolByName("LdrpDebugFlags", LdrpDebugFlags, true))
                {
                    if(MemWrite(info->base + LdrpDebugFlags.rva, &dwDebugFlags, sizeof(dwDebugFlags)))
                        dprintf(QT_TRANSLATE_NOOP("DBG", "Set LdrpDebugFlags to 0x%08X successfully!\n"), dwDebugFlags);
                    else
                        dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to write to LdrpDebugFlags\n"));
                }
                else
                {
                    dprintf(QT_TRANSLATE_NOOP("DBG", "Symbol 'LdrpDebugFlags' not found!\n"));
                }
            }
            else
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to find LdrpDebugFlags (you need to load symbols for ntdll.dll)\n"));
            }
        }
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "DLL Loaded: %p %s\n"), base, DLLDebugFileName);


    //plugin callback
    PLUG_CB_LOADDLL callbackInfo;
    callbackInfo.LoadDll = LoadDll;
    IMAGEHLP_MODULE64 modInfoUtf8;
    memset(&modInfoUtf8, 0, sizeof(modInfoUtf8));
    modInfoUtf8.SizeOfStruct = sizeof(modInfoUtf8);
    modInfoUtf8.BaseOfImage = (DWORD64)base;
    modInfoUtf8.ImageSize = 0;
    modInfoUtf8.TimeDateStamp = 0;
    modInfoUtf8.CheckSum = 0;
    modInfoUtf8.NumSyms = 0;
    modInfoUtf8.SymType = SymDia;
    strncpy_s(modInfoUtf8.ModuleName, DLLDebugFileName, _TRUNCATE);
    strncpy_s(modInfoUtf8.ImageName, DLLDebugFileName, _TRUNCATE);
    strncpy_s(modInfoUtf8.LoadedImageName, "", _TRUNCATE);
    strncpy_s(modInfoUtf8.LoadedPdbName, "", _TRUNCATE);
    modInfoUtf8.CVSig = 0;
    strncpy_s(modInfoUtf8.CVData, "", _TRUNCATE);
    modInfoUtf8.PdbSig = 0;
    modInfoUtf8.PdbAge = 0;
    modInfoUtf8.PdbUnmatched = FALSE;
    modInfoUtf8.DbgUnmatched = FALSE;
    modInfoUtf8.LineNumbers = 0;
    modInfoUtf8.GlobalSymbols = TRUE;
    modInfoUtf8.TypeInfo = TRUE;
    modInfoUtf8.SourceIndexed = TRUE;
    modInfoUtf8.Publics = TRUE;
    callbackInfo.modInfo = &modInfoUtf8;
    callbackInfo.modname = modname;
    plugincbcall(CB_LOADDLL, &callbackInfo);

    auto dllLoadSetting = party == mod_system ? "DllLoadSystem" : "DllLoad";
    if(shouldBreakOnDll)
    {
        cbGenericBreakpoint(BPDLL, DLLDebugFileName);
    }
    else if(!isNtdll && settingboolget("Events", dllLoadSetting))
    {
        //update GUI
        DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), paused);
        //lock
        lock(WAITID_RUN);
        // Plugin callback
        PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        dbgsetforeground();
        dbgsetskipexceptions(false);
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
    char modname[MAX_MODULE_SIZE] = "???";
    if(ModNameFromAddr((duint)base, modname, true))
        BpEnumAll(cbRemoveModuleBreakpoints, modname, duint(base));
    int party = ModGetParty(duint(base));
    DebugUpdateBreakpointsViewAsync();
    dprintf(QT_TRANSLATE_NOOP("DBG", "DLL Unloaded: %p %s\n"), base, modname);

    auto dllUnloadSetting = party == mod_system ? "DllUnloadSystem" : "DllUnload";
    if(dbghandledllbreakpoint(modname, false))
    {
        cbGenericBreakpoint(BPDLL, modname);
    }
    else if(settingboolget("Events", dllUnloadSetting))
    {
        //update GUI
        DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), paused);
        //lock
        lock(WAITID_RUN);
        // Plugin callback
        PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        dbgsetforeground();
        dbgsetskipexceptions(false);
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
            if(str != lastDebugText) //fix for every string being printed twice
            {
                if(str != "\n")
                    dprintf(QT_TRANSLATE_NOOP("DBG", "DebugString: \"%s\"\n"), StringUtils::Trim(StringUtils::Escape(str, false)).c_str());
                lastDebugText = str;
            }
            else
                lastDebugText.clear();
        }
    }
    else
    {
        //TODO: implement Windows 10 unicode debug string
    }

    if(settingboolget("Events", "DebugStrings"))
    {
        //update GUI
        DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), paused);
        //lock
        lock(WAITID_RUN);
        // Plugin callback
        PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        dbgsetforeground();
        dbgsetskipexceptions(false);
        wait(WAITID_RUN);
    }
}

static bool dbgdetachDisableAllBreakpoints(const BREAKPOINT* bp)
{
    if(bp->enabled)
    {
        if(bp->type == BPNORMAL)
            DeleteBPX(bp->addr);
        else if(bp->type == BPMEMORY)
            RemoveMemoryBPX(bp->addr, 0);
        else if(bp->type == BPHARDWARE && TITANDRXVALID(bp->titantype))
            DeleteHardwareBreakPoint(TITANGETDRX(bp->titantype));
    }
    return true;
}

static void cbException(EXCEPTION_DEBUG_INFO* ExceptionData)
{
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    PLUG_CB_EXCEPTION callbackInfo;
    callbackInfo.Exception = ExceptionData;
    unsigned int ExceptionCode = ExceptionData->ExceptionRecord.ExceptionCode;
    GuiSetLastException(ExceptionCode);
    lastExceptionInfo = *ExceptionData;

    duint addr = (duint)ExceptionData->ExceptionRecord.ExceptionAddress;
    {
        BREAKPOINT bp;
        if(BpGet(ExceptionCode, BPEXCEPTION, nullptr, &bp) && bp.enabled && ((bp.titantype == 1 && ExceptionData->dwFirstChance) || (bp.titantype == 2 && !ExceptionData->dwFirstChance) || bp.titantype == 3))
        {
            bPausedOnException = true;
            cbGenericBreakpoint(BPEXCEPTION, ExceptionData);
            bPausedOnException = false;
            return;
        }
    }
    if(ExceptionData->ExceptionRecord.ExceptionCode == MS_VC_EXCEPTION) //SetThreadName exception
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
                dprintf(QT_TRANSLATE_NOOP("DBG", "SetThreadName exception on %p (%X, \"%s\")\n"), addr, nameInfo.dwThreadID, ThreadNameEscaped.c_str());
                ThreadSetName(nameInfo.dwThreadID, ThreadNameEscaped.c_str());
                if(!settingboolget("Events", "ThreadNameSet"))
                    return;
            }
        }
    }
    const ExceptionFilter & filter = dbggetexceptionfilter(ExceptionCode);
    if(bVerboseExceptionLogging && filter.logException)
        DbgCmdExecDirect("exinfo"); //show extended exception information
    auto exceptionName = ExceptionCodeToName(ExceptionCode);
    if(!exceptionName.size()) //if no exception was found, try the error codes (RPC_S_*)
        exceptionName = ErrorCodeToName(ExceptionCode);
    if(ExceptionData->dwFirstChance) //first chance exception
    {
        if(filter.logException)
        {
            if(exceptionName.size())
                dprintf(QT_TRANSLATE_NOOP("DBG", "First chance exception on %p (%.8X, %s)!\n"), addr, ExceptionCode, exceptionName.c_str());
            else
                dprintf(QT_TRANSLATE_NOOP("DBG", "First chance exception on %p (%.8X)!\n"), addr, ExceptionCode);
        }
        dbgsetcontinuestatus(filter.handledBy == ExceptionHandledBy::Debuggee ? DBG_EXCEPTION_NOT_HANDLED : DBG_CONTINUE);
        if((bSkipExceptions || filter.breakOn != ExceptionBreakOn::FirstChance) && (!maxSkipExceptionCount || ++skipExceptionCount < maxSkipExceptionCount))
            return;
    }
    else //lock the exception
    {
        if(filter.logException)
        {
            if(exceptionName.size())
                dprintf(QT_TRANSLATE_NOOP("DBG", "Last chance exception on %p (%.8X, %s)!\n"), addr, ExceptionCode, exceptionName.c_str());
            else
                dprintf(QT_TRANSLATE_NOOP("DBG", "Last chance exception on %p (%.8X)!\n"), addr, ExceptionCode);
        }
        // DBG_EXCEPTION_NOT_HANDLED kills the process on a last chance exception, so only pass this if the user really asked for it
        dbgsetcontinuestatus(filter.breakOn == ExceptionBreakOn::DoNotBreak && filter.handledBy == ExceptionHandledBy::Debuggee ? DBG_EXCEPTION_NOT_HANDLED : DBG_CONTINUE);
    }
    if(filter.breakOn == ExceptionBreakOn::DoNotBreak)
        return;

    DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), paused);
    //lock
    lock(WAITID_RUN);
    bPausedOnException = true;
    // Plugin callback
    PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    dbgsetforeground();
    dbgsetskipexceptions(false);
    plugincbcall(CB_EXCEPTION, &callbackInfo);
    wait(WAITID_RUN);
    bPausedOnException = false;
}

static void cbDebugEvent(DEBUG_EVENT* DebugEvent)
{
    nextContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
    hActiveThread = ThreadGetHandle(((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    InterlockedIncrement((volatile long*)&DbgEvents);
    PLUG_CB_DEBUGEVENT debugEventInfo;
    debugEventInfo.DebugEvent = DebugEvent;
    plugincbcall(CB_DEBUGEVENT, &debugEventInfo);
}

static void cbAttachDebugger()
{
    if(hEvent) //Signal the AeDebug event
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
        hEvent = 0;
    }
    if(tidToResume) //Resume a thread
    {
        cmddirectexec(StringUtils::sprintf("resumethread %p", tidToResume).c_str());
        tidToResume = 0;
    }
    varset("$pid", fdProcessInfo->dwProcessId, true);

    //Get on top of things
    SetForegroundWindow(GuiGetWindowHandle());

    // Update GUI (this should be the first triggered event)
    duint cip = GetContextDataEx(hActiveThread, UE_CIP);
    GuiDumpAt(MemFindBaseAddr(cip, 0, true)); //dump somewhere
    DebugUpdateGuiSetStateAsync(cip, running);

    MemInitRemoteProcessCookie(cookie.cookie);
    GuiUpdateAllViews();

    dputs(QT_TRANSLATE_NOOP("DBG", "Attached to process!"));
    dbgsetskipexceptions(false); //we are not skipping first-chance exceptions
}

cmdline_qoutes_placement_t getqoutesplacement(const char* cmdline)
{
    cmdline_qoutes_placement_t quotesPos;
    quotesPos.firstPos = quotesPos.secondPos = 0;

    auto len = strlen(cmdline);

    char quoteSymb = cmdline[0];
    if(quoteSymb == '"' || quoteSymb == '\'')
    {
        for(size_t i = 1; i < len; i++)
        {
            if(cmdline[i] == quoteSymb)
            {
                quotesPos.posEnum = i == len - 1 ? QOUTES_AT_BEGIN_AND_END : QOUTES_AROUND_EXE;
                quotesPos.secondPos = i;
                break;
            }
        }
        if(!quotesPos.secondPos)
            quotesPos.posEnum = NO_CLOSE_QUOTE_FOUND;
    }
    else
    {
        quotesPos.posEnum = NO_QOUTES;
        //try to locate first quote
        for(size_t i = 1; i < len; i++)
            if(cmdline[i] == '"' || cmdline[i] == '\'')
                quotesPos.secondPos = i;
    }

    return quotesPos;
}

BOOL ismainwindow(HWND handle)
{
    // using only OWNER condition allows getting titles of hidden "main windows"
    return !GetWindow(handle, GW_OWNER) && IsWindowVisible(handle);
}

BOOL CALLBACK chkWindowPidCallback(HWND hWnd, LPARAM lParam)
{
    DWORD procId = (DWORD)lParam;
    DWORD hwndPid = 0;
    GetWindowThreadProcessId(hWnd, &hwndPid);
    if(hwndPid == procId)
    {
        if(!mForegroundHandle)  // get the foreground if no owner visible
            mForegroundHandle = hWnd;

        if(ismainwindow(hWnd))
        {
            mProcHandle = hWnd;
            return FALSE;
        }
    }

    return TRUE;
}

bool dbggetwintext(std::vector<std::string>* winTextList, const DWORD dwProcessId)
{
    mProcHandle = NULL;
    mForegroundHandle = NULL;

    EnumWindows(chkWindowPidCallback, dwProcessId);
    if(!mProcHandle && !mForegroundHandle)
        return false;

    wchar_t limitedbuffer[256];
    limitedbuffer[255] = 0;

    if(mProcHandle)  // get info from the "main window" (GW_OWNER + visible)
    {
        if(!GetWindowTextW((HWND)mProcHandle, limitedbuffer, 256))
            GetClassNameW((HWND)mProcHandle, limitedbuffer, 256); // go for the class name if none of the above
    }
    else if(mForegroundHandle)  // get info from the foreground window
    {
        if(!GetWindowTextW((HWND)mForegroundHandle, limitedbuffer, 256))
            GetClassNameW((HWND)mForegroundHandle, limitedbuffer, 256); // go for the class name if none of the above
    }


    if(limitedbuffer[255] != 0)  //Window title too long. Add "..." to the end of buffer.
    {
        if(limitedbuffer[252] < 0xDC00 || limitedbuffer[252] > 0xDFFF)  //protect the last surrogate of UTF-16 surrogate pair
            limitedbuffer[252] = L'.';
        limitedbuffer[253] = L'.';
        limitedbuffer[254] = L'.';
        limitedbuffer[255] = 0;
    }
    auto UTF8WindowTitle = StringUtils::Utf16ToUtf8(limitedbuffer);
    winTextList->push_back(UTF8WindowTitle);
    return true;
}

bool dbglistprocesses(std::vector<PROCESSENTRY32>* infoList, std::vector<std::string>* commandList, std::vector<std::string>* winTextList)
{
    infoList->clear();
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
        char szExePath[MAX_PATH] = "";
        if(GetFileNameFromProcessHandle(hProcess, szExePath, _countof(szExePath)))
            strcpy_s(pe32.szExeFile, szExePath);
        infoList->push_back(pe32);

        if(!dbggetwintext(winTextList, pe32.th32ProcessID))
            winTextList->push_back("");
        cmdline_error_t err = {};
        char* cmdline = nullptr;
        if(!dbggetcmdline(&cmdline, &err, hProcess))
            commandList->push_back(StringUtils::sprintf("ARG_GET_ERROR:%d:%p", err.type, err.addr));
        else
        {
            cmdline_qoutes_placement_t posEnum = getqoutesplacement(cmdline);
            char* cmdLineExe = strstr(cmdline, pe32.szExeFile);
            size_t cmdLineExeSize = cmdLineExe ? strlen(pe32.szExeFile) : 0;

            if(!cmdLineExe)
            {
                char* exeName = strrchr(pe32.szExeFile, '\\') ? strrchr(pe32.szExeFile, '\\') + 1 :
                                strrchr(pe32.szExeFile, '/') ? strrchr(pe32.szExeFile, '/') + 1 : pe32.szExeFile;
                size_t exeNameLen = strlen(exeName);

                char* peNameInCmd = strstr(cmdline, exeName);
                //check for exe name is used in path to exe
                for(char* exeNameInCmdTmp = peNameInCmd; exeNameInCmdTmp;)
                {
                    exeNameInCmdTmp = strstr(exeNameInCmdTmp + exeNameLen, exeName);
                    if(!exeNameInCmdTmp)
                        break;

                    char* nextSlash = strchr(exeNameInCmdTmp, '\\') ? strchr(exeNameInCmdTmp, '\\') :
                                      strchr(exeNameInCmdTmp, '/') ? strchr(exeNameInCmdTmp, '/') : NULL;
                    if(nextSlash && posEnum.posEnum == NO_QOUTES) //if there NO_QOUTES, then the path to PE in cmdline can't contain spaces
                    {
                        if(strchr(exeNameInCmdTmp, ' ') < nextSlash) //slash is in arguments
                        {
                            peNameInCmd = exeNameInCmdTmp;
                            break;
                        }
                        else
                            continue;
                    }
                    else if(nextSlash && posEnum.posEnum == QOUTES_AROUND_EXE)
                    {
                        if((cmdline + posEnum.secondPos) < nextSlash) //slash is in arguments
                        {
                            peNameInCmd = exeNameInCmdTmp;
                            break;
                        }
                        else
                            continue;
                    }
                    else
                    {
                        peNameInCmd = exeNameInCmdTmp;
                        break;
                    }
                }

                if(peNameInCmd)
                    cmdLineExeSize = (size_t)(((LPBYTE)peNameInCmd - (LPBYTE)cmdline) + exeNameLen);
                else
                {
                    //try to locate basic name, without extension
                    Memory<char*> basicName(strlen(exeName) + 1, "dbglistprocesses:basicName");
                    strncpy_s(basicName(), sizeof(char) * strlen(exeName) + 1, exeName, _TRUNCATE);
                    char* dotInName = strrchr(basicName(), '.');
                    if(dotInName != nullptr)
                        dotInName[0] = '\0';
                    size_t basicNameLen = strlen(basicName());
                    peNameInCmd = strstr(cmdline, basicName());
                    //check for basic name is used in path to exe
                    for(char* basicNameInCmdTmp = peNameInCmd; basicNameInCmdTmp;)
                    {
                        basicNameInCmdTmp = strstr(basicNameInCmdTmp + basicNameLen, basicName());
                        if(!basicNameInCmdTmp)
                            break;

                        char* nextSlash = strchr(basicNameInCmdTmp, '\\') ? strchr(basicNameInCmdTmp, '\\') :
                                          strchr(basicNameInCmdTmp, '/') ? strchr(basicNameInCmdTmp, '/') : NULL;
                        if(nextSlash && posEnum.posEnum == NO_QOUTES) //if there NO_QOUTES, then the path to PE in cmdline can't contain spaces
                        {
                            if(strchr(basicNameInCmdTmp, ' ') < nextSlash) //slash is in arguments
                            {
                                peNameInCmd = basicNameInCmdTmp;
                                break;
                            }
                            else
                                continue;
                        }
                        else if(nextSlash && posEnum.posEnum == QOUTES_AROUND_EXE)
                        {
                            if((cmdline + posEnum.secondPos) < nextSlash) //slash is in arguments
                            {
                                peNameInCmd = basicNameInCmdTmp;
                                break;
                            }
                            else
                                continue;
                        }
                        else
                        {
                            peNameInCmd = basicNameInCmdTmp;
                            break;
                        }
                    }

                    if(peNameInCmd)
                        cmdLineExeSize = (size_t)(((LPBYTE)peNameInCmd - (LPBYTE)cmdline) + basicNameLen);
                }
            }

            switch(posEnum.posEnum)
            {
            case NO_CLOSE_QUOTE_FOUND:
                commandList->push_back(cmdline + cmdLineExeSize + 1);
                break;
            case NO_QOUTES:
                if(!posEnum.secondPos)
                    commandList->push_back(cmdline + cmdLineExeSize);
                else
                    commandList->push_back(cmdline + (cmdLineExeSize > posEnum.secondPos + 1 ? cmdLineExeSize : posEnum.secondPos + 1));
                break;
            case QOUTES_AROUND_EXE:
                commandList->push_back(cmdline + cmdLineExeSize + 2);
                break;
            case QOUTES_AT_BEGIN_AND_END:
                cmdline[strlen(cmdline) - 1] = '\0';
                commandList->push_back(cmdline + cmdLineExeSize + 1);
                break;
            }

            if(!commandList->empty())
                commandList->back() = StringUtils::Trim(commandList->back());

            efree(cmdline);
        }
    }
    while(Process32Next(hProcessSnap, &pe32));
    return true;
}

static bool getcommandlineaddr(duint* addr, cmdline_error_t* cmd_line_error, HANDLE hProcess)
{
    duint pprocess_parameters;

    cmd_line_error->addr = (duint)GetPEBLocation(hProcess ? hProcess : fdProcessInfo->hProcess);

    if(cmd_line_error->addr == 0)
    {
        cmd_line_error->type = CMDL_ERR_GET_PEB;
        return false;
    }

    if(hProcess)
    {
        duint NumberOfBytesRead;
        if(!MemoryReadSafe(hProcess, (LPVOID)((cmd_line_error->addr) + offsetof(PEB, ProcessParameters)),
                           &pprocess_parameters, sizeof(duint), &NumberOfBytesRead))
        {
            cmd_line_error->type = CMDL_ERR_READ_PROCPARM_PTR;
            return false;
        }

        *addr = (pprocess_parameters) + offsetof(RTL_USER_PROCESS_PARAMETERS, CommandLine);
    }
    else
    {
        //cast-trick to calculate the address of the remote peb field ProcessParameters
        cmd_line_error->addr = (duint) & (((PPEB)cmd_line_error->addr)->ProcessParameters);
        if(!MemRead(cmd_line_error->addr, &pprocess_parameters, sizeof(pprocess_parameters)))
        {
            cmd_line_error->type = CMDL_ERR_READ_PEBBASE;
            return false;
        }

        *addr = (duint) & (((RTL_USER_PROCESS_PARAMETERS*)pprocess_parameters)->CommandLine);
    }
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

static std::vector<char> Utf16ToAnsi(const wchar_t* wstr)
{
    std::vector<char> buffer;
    auto requiredSize = WideCharToMultiByte(CP_ACP, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if(requiredSize > 0)
    {
        buffer.resize(requiredSize);
        WideCharToMultiByte(CP_ACP, 0, wstr, -1, &buffer[0], requiredSize, nullptr, nullptr);
    }
    return buffer;
}

bool dbgsetcmdline(const char* cmd_line, cmdline_error_t* cmd_line_error)
{
    // Make sure cmd_line_error is a valid pointer
    cmdline_error_t cmd_line_error_aux;
    if(cmd_line_error == NULL)
        cmd_line_error = &cmd_line_error_aux;

    // Get the command line address
    if(!getcommandlineaddr(&cmd_line_error->addr, cmd_line_error, fdProcessInfo->hProcess))
        return false;
    auto command_line_addr = cmd_line_error->addr;

    // Convert the string to UTF-16
    auto command_linewstr = StringUtils::Utf8ToUtf16(cmd_line);
    if(command_linewstr.length() >= 32766) //32766 is maximum character count for a null-terminated UNICODE_STRING
        command_linewstr.resize(32766);
    // Convert the UTF-16 string to ANSI
    auto command_linestr = Utf16ToAnsi(command_linewstr.c_str());

    // Fill the UNICODE_STRING to be set in the debuggee
    UNICODE_STRING new_command_line;
    new_command_line.Length = USHORT(command_linewstr.length() * sizeof(WCHAR)); //max value: 32766 * 2 = 65532
    new_command_line.MaximumLength = new_command_line.Length + sizeof(WCHAR); //max value: 65532 + 2 = 65534
    new_command_line.Buffer = PWSTR(command_linewstr.c_str()); //allow cast from const because the UNICODE_STRING will not be used locally

    // Allocate remote memory for both the UNICODE_STRING.Buffer and the (null terminated) ANSI buffer
    duint mem = MemAllocRemote(0, new_command_line.MaximumLength + command_linestr.size());
    if(!mem)
    {
        cmd_line_error->type = CMDL_ERR_ALLOC_UNICODEANSI_COMMANDLINE;
        return false;
    }

    // Write the UNICODE_STRING.Buffer to the debuggee (UNICODE_STRING.Length is used because the remote memory is zeroed)
    if(!MemWrite(mem, new_command_line.Buffer, new_command_line.Length))
    {
        cmd_line_error->addr = mem;
        cmd_line_error->type = CMDL_ERR_WRITE_UNICODE_COMMANDLINE;
        return false;
    }

    // Write the (null-terminated) ANSI buffer to the debuggee
    if(!MemWrite(mem + new_command_line.MaximumLength, command_linestr.data(), command_linestr.size()))
    {
        cmd_line_error->addr = mem + new_command_line.MaximumLength;
        cmd_line_error->type = CMDL_ERR_WRITE_ANSI_COMMANDLINE;
        return false;
    }

    // Change the pointers to the command line
    if(!fixgetcommandlinesbase(mem, mem + new_command_line.MaximumLength, cmd_line_error))
        return false;

    // Put the remote buffer address in the UNICODE_STRING and write it to the PEB
    new_command_line.Buffer = PWSTR(mem);
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

bool dbggetcmdline(char** cmd_line, cmdline_error_t* cmd_line_error, HANDLE hProcess)
{
    UNICODE_STRING CommandLine;
    Memory<wchar_t*> wstr_cmd;
    cmdline_error_t cmd_line_error_aux;

    if(!cmd_line_error)
        cmd_line_error = &cmd_line_error_aux;

    if(!getcommandlineaddr(&cmd_line_error->addr, cmd_line_error, hProcess))
        return false;

    if(hProcess)
    {
        duint NumberOfBytesRead;
        if(!MemoryReadSafe(hProcess, (LPVOID)cmd_line_error->addr, &CommandLine, sizeof(UNICODE_STRING), &NumberOfBytesRead))
        {
            cmd_line_error->type = CMDL_ERR_READ_GETCOMMANDLINEBASE;
            return false;
        }

        wstr_cmd.realloc(CommandLine.Length + sizeof(wchar_t));

        cmd_line_error->addr = (duint)CommandLine.Buffer;
        if(!MemoryReadSafe(hProcess, (LPVOID)cmd_line_error->addr, wstr_cmd(), CommandLine.Length, &NumberOfBytesRead))
        {
            cmd_line_error->type = CMDL_ERR_GET_GETCOMMANDLINE;
            return false;
        }
    }
    else
    {
        if(!MemRead(cmd_line_error->addr, &CommandLine, sizeof(CommandLine)))
        {
            cmd_line_error->type = CMDL_ERR_READ_PROCPARM_PTR;
            return false;
        }

        wstr_cmd.realloc(CommandLine.Length + sizeof(wchar_t));

        cmd_line_error->addr = (duint)CommandLine.Buffer;
        if(!MemRead(cmd_line_error->addr, wstr_cmd(), CommandLine.Length))
        {
            cmd_line_error->type = CMDL_ERR_READ_PROCPARM_CMDLINE;
            return false;
        }
    }
    SIZE_T wstr_cmd_size = wcslen(wstr_cmd()) + 1;
    SIZE_T cmd_line_size = wstr_cmd_size * 2;

    *cmd_line = (char*)emalloc(cmd_line_size, "dbggetcmdline:cmd_line");

    if(cmd_line_size <= 2)
    {
        *cmd_line[0] = '\0';
        return true;
    }

    //Convert TO UTF-8
    if(!WideCharToMultiByte(CP_UTF8, 0, wstr_cmd(), (int)wstr_cmd_size, *cmd_line, (int)cmd_line_size, NULL, NULL))
    {
        efree(*cmd_line);
        *cmd_line = nullptr;
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
    CloseHandle(CreateThread(0, 0, scriptThread, (LPVOID)cbScript, 0, 0));
}

static void* InitDLLDebugW(const wchar_t* szFileName, const wchar_t* szCommandLine, const wchar_t* szCurrentFolder)
{
    WString loaderFilename = StringUtils::sprintf(L"\\DLLLoader" ArchValue(L"32", L"64") L"_%04X.exe", GetTickCount() & 0xFFFF);
    WString debuggeeLoaderPath = szFileName;
    {
        auto backslashIdx = debuggeeLoaderPath.rfind('\\');
        if(backslashIdx != WString::npos)
            debuggeeLoaderPath.resize(backslashIdx);
    }
    debuggeeLoaderPath += loaderFilename;
    WString loaderPath = StringUtils::Utf8ToUtf16(szDllLoaderPath);
    if(!CopyFileW(loaderPath.c_str(), debuggeeLoaderPath.c_str(), FALSE))
    {
        debuggeeLoaderPath = StringUtils::Utf8ToUtf16(szUserDir);
        debuggeeLoaderPath += loaderFilename;
        if(!CopyFileW(loaderPath.c_str(), debuggeeLoaderPath.c_str(), FALSE))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Error debugging DLL (failed to copy loader)\n"));
            return nullptr;
        }
    }

    PPROCESS_INFORMATION ReturnValue = (PPROCESS_INFORMATION)InitDebugW(debuggeeLoaderPath.c_str(), szCommandLine, szCurrentFolder);
    WString mappingName = StringUtils::sprintf(L"Local\\szLibraryName%X", ReturnValue->dwProcessId);
    const auto mappingSize = 512;
    DebugDLLFileMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, mappingSize * sizeof(wchar_t), mappingName.c_str());
    if(DebugDLLFileMapping)
    {
        wchar_t* szLibraryPathMapping = (wchar_t*)MapViewOfFile(DebugDLLFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, mappingSize * sizeof(wchar_t));
        if(szLibraryPathMapping)
        {
            wcscpy_s(szLibraryPathMapping, mappingSize, szFileName);
            UnmapViewOfFile(szLibraryPathMapping);
        }
    }

    return ReturnValue;
}

static void debugLoopFunction(INIT_STRUCT* init)
{
    //initialize variables
    bIsAttached = init->attach;
    dbgsetskipexceptions(false);
    bFreezeStack = false;

    //prepare attach/createprocess
    if(init->attach)
    {
        gInitExe = StringUtils::Utf8ToUtf16(szDebuggeePath);
        static PROCESS_INFORMATION pi_attached;
        memset(&pi_attached, 0, sizeof(pi_attached));
        fdProcessInfo = &pi_attached;
    }
    else
    {
        gInitExe = StringUtils::Utf8ToUtf16(init->exe);
        strncpy_s(szDebuggeePath, init->exe.c_str(), _TRUNCATE);
    }

    pDebuggedEntry = GetPE32DataW(gInitExe.c_str(), 0, UE_OEP);
    bEntryIsInMzHeader = pDebuggedEntry == 0 || pDebuggedEntry == 1;

    bFileIsDll = IsFileDLLW(StringUtils::Utf8ToUtf16(szDebuggeePath).c_str(), 0);
    if(bFileIsDll && !FileExists(szDllLoaderPath))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error debugging DLL (loaddll.exe not found)\n"));
        return;
    }
    DbSetPath(nullptr, szDebuggeePath);

    if(!init->attach)
    {
        // Load command line if it exists in DB
        DbLoad(DbLoadSaveType::CommandLine);
        if(!isCmdLineEmpty())
        {
            char* commandLineArguments = NULL;
            commandLineArguments = getCommandLineArgs();

            if(commandLineArguments && *commandLineArguments)
                init->commandline = commandLineArguments;
        }

        gInitCmd = StringUtils::Utf8ToUtf16(init->commandline);
        gInitDir = StringUtils::Utf8ToUtf16(init->currentfolder);

        //start the process
        if(bFileIsDll)
            fdProcessInfo = (PROCESS_INFORMATION*)InitDLLDebugW(gInitExe.c_str(), gInitCmd.c_str(), gInitDir.c_str());
        else
            fdProcessInfo = (PROCESS_INFORMATION*)InitDebugW(gInitExe.c_str(), gInitCmd.c_str(), gInitDir.c_str());
        if(!fdProcessInfo)
        {
            auto lastError = GetLastError();
            auto isElevated = BridgeIsProcessElevated();
            String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", lastError));
            if(lastError == ERROR_ELEVATION_REQUIRED && !isElevated)
            {
                auto msg = StringUtils::Utf8ToUtf16(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "The executable you are trying to debug requires elevation. Restart as admin?")));
                auto title = StringUtils::Utf8ToUtf16(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Elevation")));
                auto answer = MessageBoxW(GuiGetWindowHandle(), msg.c_str(), title.c_str(), MB_ICONQUESTION | MB_YESNO);
                wchar_t wszProgramPath[MAX_PATH] = L"";
                if(answer == IDYES && dbgrestartadmin())
                {
                    fdProcessInfo = &g_pi;
                    GuiCloseApplication();
                    return;
                }
            }
            else if(isElevated)
            {
                //This is most likely an application with uiAccess="true"
                //https://github.com/x64dbg/x64dbg/issues/1501
                //https://blogs.techsmith.com/inside-techsmith/devcorner-debug-uiaccess
                error += ", uiAccess=\"true\"";
            }
            fdProcessInfo = &g_pi;
            dprintf(QT_TRANSLATE_NOOP("DBG", "Error starting process (CreateProcess, %s)!\n"), error.c_str());
            return;
        }

        //check for WOW64
        BOOL wow64 = false, mewow64 = false;
        if(!IsWow64Process(fdProcessInfo->hProcess, &wow64) || !IsWow64Process(GetCurrentProcess(), &mewow64))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "IsWow64Process failed!"));
            StopDebug();
            return;
        }
        if((mewow64 && !wow64) || (!mewow64 && wow64))
        {
#ifdef _WIN64
            dputs(QT_TRANSLATE_NOOP("DBG", "Use x32dbg to debug this process!"));
#else
            dputs(QT_TRANSLATE_NOOP("DBG", "Use x64dbg to debug this process!"));
#endif // _WIN64
            return;
        }

        //set script variables
        varset("$pid", fdProcessInfo->dwProcessId, true);

        if(!OpenProcessToken(fdProcessInfo->hProcess, TOKEN_ALL_ACCESS, &hProcessToken))
            hProcessToken = 0;
    }
    else //attach
    {
        gInitCmd.clear();
        gInitDir.clear();
    }

    // signal that fdProcessInfo has been set
    SetEvent(init->event);
    init->event = nullptr;

    //set custom handlers
    SetCustomHandler(UE_CH_CREATEPROCESS, (TITANCBCH)cbCreateProcess);
    SetCustomHandler(UE_CH_EXITPROCESS, (TITANCBCH)cbExitProcess);
    SetCustomHandler(UE_CH_CREATETHREAD, (TITANCBCH)cbCreateThread);
    SetCustomHandler(UE_CH_EXITTHREAD, (TITANCBCH)cbExitThread);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCBCH)cbSystemBreakpoint);
    SetCustomHandler(UE_CH_LOADDLL, (TITANCBCH)cbLoadDll);
    SetCustomHandler(UE_CH_UNLOADDLL, (TITANCBCH)cbUnloadDll);
    SetCustomHandler(UE_CH_OUTPUTDEBUGSTRING, (TITANCBCH)cbOutputDebugString);
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCBCH)cbException);
    SetCustomHandler(UE_CH_DEBUGEVENT, (TITANCBCH)cbDebugEvent);

    //inform GUI we started without problems
    GuiSetDebugState(initialized);
    GuiFocusView(GUI_DISASSEMBLY);
    GuiAddRecentFile(szDebuggeePath);

    //set GUI title
    strcpy_s(szBaseFileName, szDebuggeePath);
    int len = (int)strlen(szBaseFileName);
    while(szBaseFileName[len] != '\\' && len)
        len--;
    if(len)
        strcpy_s(szBaseFileName, szBaseFileName + len + 1);
    GuiUpdateWindowTitle(szBaseFileName);

    //call plugin callback
    PLUG_CB_INITDEBUG initInfo;
    initInfo.szFileName = szDebuggeePath;
    plugincbcall(CB_INITDEBUG, &initInfo);

    //call plugin callback (attach)
    if(init->attach)
    {
        PLUG_CB_ATTACH attachInfo;
        attachInfo.dwProcessId = init->pid;
        plugincbcall(CB_ATTACH, &attachInfo);
    }

    // Init program database
    DbLoad(DbLoadSaveType::DebugData);

    //run debug loop (returns when process debugging is stopped)
    if(init->attach)
    {
        if(!AttachDebugger(init->pid, true, fdProcessInfo, cbAttachDebugger))
        {
            auto status = NtCurrentTeb()->LastStatusValue;
            auto error = stringformatinline(StringUtils::sprintf("{ntstatus@%X}", status));
            if(status == STATUS_PORT_ALREADY_SET)
            {
                error = GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Process is already being debugged!"));
            }
            dprintf(QT_TRANSLATE_NOOP("DBG", "Attach to process failed: %s\n"), error.c_str());
        }
    }
    else
    {
        //close the process and thread handles we got back from CreateProcess, to prevent duplicating the ones we will receive in cbCreateProcess
        CloseHandle(fdProcessInfo->hProcess);
        CloseHandle(fdProcessInfo->hThread);
        fdProcessInfo->hProcess = fdProcessInfo->hThread = nullptr;
        DebugLoop();
    }

    //fixes data loss when attach failed (https://github.com/x64dbg/x64dbg/issues/1899)
    DbClose();

    //call plugin callback
    PLUG_CB_STOPDEBUG stopInfo;
    stopInfo.reserved = 0;
    plugincbcall(CB_STOPDEBUG, &stopInfo);

    //message the user/do final stuff
    RemoveAllBreakPoints(UE_OPTION_REMOVEALL); //remove all breakpoints
    {
        EXCLUSIVE_ACQUIRE(LockDllBreakpoints);
        dllBreakpoints.clear(); //RemoveAllBreakPoints doesn't remove librarian breakpoints
    }

    //cleanup
    dbgcleartracestate();
    dbgClearRtuBreakpoints();
    ModClear();
    ThreadClear();
    WatchClear();
    TraceRecord.clear();
    TraceRecord.enableTraceRecording(false, nullptr); // Stop trace recording
    GuiSetDebugState(stopped);
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "Debugging stopped!"));
    fdProcessInfo->hProcess = fdProcessInfo->hThread = nullptr;
    varset("$hp", (duint)0, true);
    varset("$pid", (duint)0, true);
    if(hProcessToken)
    {
        CloseHandle(hProcessToken);
        hProcessToken = 0;
    }

    if(DebugDLLFileMapping)
    {
        CloseHandle(DebugDLLFileMapping);
        DebugDLLFileMapping = 0;
    }

    pDebuggedEntry = 0;
    pDebuggedBase = 0;
    hActiveThread = nullptr;
    if(!gDllLoader.empty()) //Delete the DLL loader (#1496)
    {
        DeleteFileW(gDllLoader.c_str());
        gDllLoader.clear();
    }
}

void dbgsetdebuggeeinitscript(const char* fileName)
{
    if(fileName)
        strcpy_s(szDebuggeeInitializationScript, fileName);
    else
        szDebuggeeInitializationScript[0] = 0;
}

const char* dbggetdebuggeeinitscript()
{
    return szDebuggeeInitializationScript;
}

void dbgsetforeground()
{
    if(!bNoForegroundWindow)
        SetForegroundWindow(GuiGetWindowHandle());
}

void dbgcreatedebugthread(INIT_STRUCT* init)
{
    if(settingboolget("Misc", "CheckForAntiCheatDrivers"))
    {
        auto loadedDrivers = LoadedAntiCheatDrivers();
        if(!loadedDrivers.empty())
        {
            auto translatedFormat = GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Drivers known to interfere with x64dbg's operation have been detected.\n\nList of drivers:\n%s\n\nDo you want to continue debugging?"));
            auto message = StringUtils::sprintf(translatedFormat, loadedDrivers.c_str());
            auto continueDebugging = GuiScriptMsgyn(message.c_str());
            if(!continueDebugging)
                return;
        }
    }

    auto event = init->event = CreateEventW(nullptr, false, false, nullptr);
    hDebugLoopThread = CreateThread(nullptr, 0, [](LPVOID lpParameter) -> DWORD
    {
        auto init = (INIT_STRUCT*)lpParameter;
        debugLoopFunction(init);

        // Set the event in case debugLoopFunction returned early to prevent a deadlock
        if(init->event)
        {
            SetEvent(init->event);
            init->event = nullptr;
        }
        return 0;
    }, init, 0, nullptr);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
}

String formatpidtid(DWORD pidtid)
{
    return StringUtils::sprintf("%u", pidtid);
}

void dbgsetcontinuestatus(DWORD status)
{
    nextContinueStatus = status;
    SetNextDbgContinueStatus(status);
}

DWORD dbggetcontinuestatus()
{
    return nextContinueStatus;
}

bool dbgrestartadmin()
{
    wchar_t wszProgramPath[MAX_PATH] = L"";
    if(GetModuleFileNameW(GetModuleHandleW(nullptr), wszProgramPath, _countof(wszProgramPath)))
    {
        std::wstring file = wszProgramPath;
        auto last = wcsrchr(wszProgramPath, L'\\');
        if(last)
            *last = L'\0';
        //TODO: possibly escape characters in gInitCmd
        std::wstring params = L"\"" + gInitExe + L"\" \"" + gInitCmd + L"\" \"" + gInitDir + L"\"";
        auto result = ShellExecuteW(NULL, L"runas", file.c_str(), params.c_str(), wszProgramPath, SW_SHOWDEFAULT);
        return INT_PTR(result) > 32 && GetLastError() == ERROR_SUCCESS;
    }
    return false;
}

void StepIntoWow64(TITANCBSTEP callback)
{
#ifndef _WIN64
    //NOTE: this workaround has the potential of detecting x64dbg while tracing, disable it if that happens
    if(!bNoWow64SingleStepWorkaround)
    {
        unsigned char data[7];
        auto cip = GetContextDataEx(hActiveThread, UE_CIP);
        if(MemRead(cip, data, sizeof(data)) && data[0] == 0xEA && data[5] == 0x33 && data[6] == 0x00) //ljmp 33,XXXXXXXX
        {
            auto csp = GetContextDataEx(hActiveThread, UE_CSP);
            duint ret;
            if(MemRead(csp, &ret, sizeof(ret)))
            {
                SetBPX(ret, UE_SINGLESHOOT, callback);
                return;
            }
        }
    }
#endif //_WIN64
    if(bPausedOnException && dbggetcontinuestatus() == DBG_EXCEPTION_NOT_HANDLED && exceptionDispatchAddr && !IsBPXEnabled(exceptionDispatchAddr))
    {
        SetBPX(exceptionDispatchAddr, UE_SINGLESHOOT, callback);
    }
    else
    {
        StepInto(callback);
    }
}

void StepOverWrapper(TITANCBSTEP callback)
{
    if(bPausedOnException && dbggetcontinuestatus() == DBG_EXCEPTION_NOT_HANDLED && exceptionDispatchAddr && !IsBPXEnabled(exceptionDispatchAddr))
    {
        SetBPX(exceptionDispatchAddr, UE_SINGLESHOOT, callback);
    }
    else
    {
        StepOver(callback);
    }
}

template<MODULEPARTY StopParty>
static void cbStepIntoParty()
{
    if(bAbortStepping || ModGetParty(GetContextDataEx(hActiveThread, UE_CIP)) == StopParty)
    {
        bAbortStepping = false;
        gStepIntoPartyCallback();
    }
    else
    {
        StepIntoWow64(cbStepIntoParty<StopParty>);
    }
}

void StepIntoUser(TITANCBSTEP callback)
{
    gStepIntoPartyCallback = callback;
    StepIntoWow64(cbStepIntoParty<mod_user>);
}

void StepIntoSystem(TITANCBSTEP callback)
{
    gStepIntoPartyCallback = callback;
    StepIntoWow64(cbStepIntoParty<mod_system>);
}

bool dbgisdepenabled()
{
    auto depEnabled = false;
#ifndef _WIN64
    typedef BOOL(WINAPI * GETPROCESSDEPPOLICY)(
        _In_  HANDLE  /*hProcess*/,
        _Out_ LPDWORD /*lpFlags*/,
        _Out_ PBOOL   /*lpPermanent*/
    );
    static auto GPDP = GETPROCESSDEPPOLICY(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetProcessDEPPolicy"));
    if(GPDP)
    {
        //If you use fdProcessInfo->hProcess GetProcessDEPPolicy will put garbage in bPermanent.
        auto hProcess = TitanOpenProcess(PROCESS_QUERY_INFORMATION, false, fdProcessInfo->dwProcessId);
        DWORD lpFlags;
        BOOL bPermanent;
        if(GPDP(hProcess, &lpFlags, &bPermanent))
            depEnabled = lpFlags != 0;
        CloseHandle(hProcess);
    }
#else
    depEnabled = true;
#endif //_WIN64
    return depEnabled;
}