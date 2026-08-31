#include "ntdll/ntdll.h"
#include "cmd-debug-control.h"
#include "console.h"
#include "debugger.h"
#include "animate.h"
#include "historycontext.h"
#include "threading.h"
#include "memory.h"
#include "disasm_fast.h"
#include "plugin_loader.h"
#include "value.h"
#include "variable.h"
#include "TraceRecord.h"
#include "handle.h"
#include "thread.h"
#include "GetPeArch.h"
#include "database.h"
#include "exception.h"
#include "stringformat.h"
#include "simplescript.h"

static bool isInt3Exception()
{
    if(getLastExceptionInfo().ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT)
        return false;

    auto exceptionAddress = (duint)getLastExceptionInfo().ExceptionRecord.ExceptionAddress;
    unsigned char data[MAX_DISASM_BUFFER];
    MemRead(exceptionAddress, data, sizeof(data));
    Zydis zydis;
    return zydis.Disassemble(exceptionAddress, data) && zydis.IsInt3();
}

static bool skipInt3Stepping(int argc, char* argv[])
{
    if(bSkipInt3Stepping && !dbgisrunning() && isInt3Exception())
    {
        //Don't allow skipping of multiple consecutive INT3 instructions
        getLastExceptionInfo().ExceptionRecord.ExceptionCode = 0;
        dbgsetcontinuestatus(DBG_CONTINUE); // swallow the exception
        dputs(QT_TRANSLATE_NOOP("DBG", "Skipped INT3!"));
        return true;
    }
    return false;
}

bool cbDebugRunInternal(int argc, char* argv[], HistoryAction history)
{
    if(dbggetsessionkind() != UE_SESSION_NONE && !dbghassessioncapability(UE_SESSION_CAP_FORWARD_EXECUTION))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "This replay artifact does not support execution."));
        return false;
    }
    // History handling
    if(history == history_record)
        HistoryRecord();
    else
        HistoryClear();
    // Set a singleshot breakpoint at the first parameter
    if(argc >= 2 && !DbgCmdExecDirect(StringUtils::sprintf("bp \"%s\", ss", argv[1]).c_str()))
        return false;
    // Don't "run" twice if the program is already running
    if(dbgisrunning())
        return false;
    GuiSetDebugStateAsync(running);
    unlock(WAITID_RUN);
    PLUG_CB_RESUMEDEBUG callbackInfo;
    callbackInfo.reserved = 0;
    plugincbcall(CB_RESUMEDEBUG, &callbackInfo);
    return true;
}

bool cbDebugInit(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;

    EXCLUSIVE_ACQUIRE(LockDebugStartStop);
    cbDebugStop(argc, argv);
    ASSERT_TRUE(hDebugLoopThread == nullptr);

    char arg1[deflen] = "";
    strcpy_s(arg1, argv[1]);

    char arg2[deflen] = "";
    if(argc > 2)
        strcpy_s(arg2, argv[2]);

    char arg3[deflen] = "";
    if(argc > 3)
        strcpy_s(arg3, argv[3]);

    // Process .lnk files
    std::wstring executable, arguments, workingDir;
    if(ResolveShortcut(GuiGetWindowHandle(), StringUtils::Utf8ToUtf16(arg1).c_str(), executable, arguments, workingDir))
    {
        auto resolvedPathUtf8 = StringUtils::Utf16ToUtf8(executable);
        dprintf(QT_TRANSLATE_NOOP("DBG", "Resolved shortcut \"%s\" -> \"%s\"\n"), arg1, resolvedPathUtf8.c_str());
        strcpy_s(arg1, resolvedPathUtf8.c_str());

        // Assign a command line from the shortcut if it wasn't overwritten
        if(argc <= 2)
        {
            auto resolvedArgsUtf8 = StringUtils::Utf16ToUtf8(arguments);
            dprintf(QT_TRANSLATE_NOOP("DBG", "Resolved arguments from shortcut \"%s\"\n"), resolvedArgsUtf8.c_str());
            strcpy_s(arg2, resolvedArgsUtf8.c_str());
        }

        // Assign a working directory from the shortcut if it wasn't overwritten
        if(argc <= 3)
        {
            auto resolvedWorkingDirUtf8 = StringUtils::Utf16ToUtf8(workingDir);
            dprintf(QT_TRANSLATE_NOOP("DBG", "Resolved working directory from shortcut \"%s\"\n"), resolvedWorkingDirUtf8.c_str());
            strcpy_s(arg3, resolvedWorkingDirUtf8.c_str());
        }
    }

    if(!FileExists(arg1))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "File does not exist!"));
        return false;
    }

    auto arg1w = StringUtils::Utf8ToUtf16(arg1);
    Handle hFile = CreateFileW(arg1w.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Could not open file!"));
        return false;
    }
    GetFileNameFromHandle(hFile, arg1, _countof(arg1)); //get full path of the file
    dprintf(QT_TRANSLATE_NOOP("DBG", "Debugging: %s\n"), arg1);
    hFile.Close();

    uint32_t entryPointRva = 0;
    bool isDll = false;
    auto arch = GetPeArch(arg1w.c_str(), &entryPointRva, &isDll);

    // Translate Any CPU to the actual architecture
    if(arch == PeArch::DotnetAnyCpu)
        arch = ArchValue(IsWow64() ? PeArch::Dotnet64 : PeArch::Dotnet86, PeArch::Dotnet64);

    // Make sure the architecture is right
    switch(arch)
    {
    case PeArch::Invalid:
        dputs(QT_TRANSLATE_NOOP("DBG", "Invalid PE file!"));
        return false;
#ifdef _WIN64
    case PeArch::Native86:
    case PeArch::Dotnet86:
    case PeArch::DotnetAnyCpuPrefer32:
        dputs(QT_TRANSLATE_NOOP("DBG", "Use x32dbg to debug this file!"));
#else // x86
    case PeArch::Native64:
    case PeArch::Dotnet64:
        dputs(QT_TRANSLATE_NOOP("DBG", "Use x64dbg to debug this file!"));
#endif // _WIN64
        return false;
    default:
        break;
    }

    // Default to the application folder
    char currentfolder[deflen] = "";
    strcpy_s(currentfolder, arg1);
    int len = (int)strlen(currentfolder);
    while(currentfolder[len] != '\\' && len != 0)
        len--;
    currentfolder[len] = 0;

    // Allow the user to overwrite the working directory
    if(DirExists(arg3))
        strcpy_s(currentfolder, arg3);

    static INIT_STRUCT init;
    init.exe = arg1;
    init.commandline = arg2;
    init.currentfolder = currentfolder;
    init.entryPointRva = entryPointRva;
    init.isDll = isDll;

    dbgcreatedebugthread(&init);
    return true;
}

bool cbDebugInitReplay(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;

    EXCLUSIVE_ACQUIRE(LockDebugStartStop);
    cbDebugStop(argc, argv);
    ASSERT_TRUE(hDebugLoopThread == nullptr);

    char artifact[deflen] = "";
    strcpy_s(artifact, argv[1]);
    if(!FileExists(artifact))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Replay artifact does not exist!"));
        return false;
    }

    const auto artifactW = StringUtils::Utf8ToUtf16(artifact);
    Handle file = CreateFileW(artifactW.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if(file == INVALID_HANDLE_VALUE)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Could not open replay artifact!"));
        return false;
    }
    GetFileNameFromHandle(file, artifact, _countof(artifact));
    file.Close();

    const auto lower = StringUtils::ToLower(String(artifact));
    const auto isTtd = lower.size() >= 4 && lower.compare(lower.size() - 4, 4, ".run") == 0;
    dprintf(QT_TRANSLATE_NOOP("DBG", "Opening replay artifact: %s\n"), artifact);

    static INIT_STRUCT init;
    init = {};
    init.exe = artifact;
    init.replayKind = isTtd ? UE_SESSION_TTD : UE_SESSION_MINIDUMP;
    dbgcreatedebugthread(&init);
    return true;
}

bool cbReplayGetPosition(int argc, char* argv[])
{
    TITAN_REPLAY_POSITION position = {};
    if(!ReplayGetPosition(&position))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Unable to query the replay position (error %lu).\n"), GetLastError());
        return false;
    }
    dprintf("Replay position: %llX:%llX\n", position.sequence, position.steps);
    varset("$result", (duint)position.sequence, false);
    varset("$result1", (duint)position.steps, false);
    return true;
}

bool cbReplayGetExtent(int argc, char* argv[])
{
    TITAN_REPLAY_POSITION first = {}, last = {};
    if(!ReplayGetExtent(&first, &last))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Unable to query the replay extent (error %lu).\n"), GetLastError());
        return false;
    }
    dprintf("Replay extent: %llX:%llX-%llX:%llX\n", first.sequence, first.steps, last.sequence, last.steps);
    varset("$result", (duint)first.sequence, false);
    varset("$result1", (duint)first.steps, false);
    varset("$result2", (duint)last.sequence, false);
    varset("$result3", (duint)last.steps, false);
    return true;
}

bool cbReplaySetPosition(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    unsigned long long sequence = 0, steps = 0;
    char trailing = 0;
    if(sscanf_s(argv[1], "%llx:%llx%c", &sequence, &steps, &trailing, 1) != 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Expected a replay position in sequence:steps hexadecimal form."));
        return false;
    }
    TITAN_REPLAY_POSITION position { sequence, steps };
    if(!ReplaySetPosition(&position))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Unable to seek to the replay position (error %lu).\n"), GetLastError());
        return false;
    }
    hActiveThread = ThreadGetHandle(GetDebugData()->dwThreadId);
    DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), paused);
    dprintf("Replay position set: %llX:%llX\n", sequence, steps);
    return true;
}

bool cbReplayStepBack(int argc, char* argv[])
{
    if(!dbghassessioncapability(UE_SESSION_CAP_REVERSE_EXECUTION))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "This session does not support reverse execution."));
        return false;
    }
    if(!ReplayStep(true, false, cbStep))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Unable to reverse step (error %lu).\n"), GetLastError());
        return false;
    }
    dbgsetsteprepeat(true, 1);
    return cbDebugRunInternal(1, argv, history_record);
}

bool cbReplayRunBack(int argc, char* argv[])
{
    if(!dbghassessioncapability(UE_SESSION_CAP_REVERSE_EXECUTION))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "This session does not support reverse execution."));
        return false;
    }
    if(!ReplayRun(true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Unable to start reverse execution (error %lu).\n"), GetLastError());
        return false;
    }
    return cbDebugRunInternal(1, argv, history_record);
}

bool cbDebugStop(int argc, char* argv[])
{
    EXCLUSIVE_ACQUIRE(LockDebugStartStop);
    if(!hDebugLoopThread)
        return false;

    // Give the plugins a chance to perform clean-up
    PLUG_CB_STOPPINGDEBUG stoppingInfo;
    stoppingInfo.reserved = 0;
    plugincbcall(CB_STOPPINGDEBUG, &stoppingInfo);

    auto hDebugLoopThreadCopy = hDebugLoopThread;
    hDebugLoopThread = nullptr;

    // HACK: TODO: Don't kill script on debugger ending a process
    //scriptreset(); //reset the currently-loaded script
    _dbg_animatestop();
    StopDebug();
    //history
    HistoryClear();
    DWORD BeginTick = GetTickCount();
    bool shownWarning = false;

    while(true)
    {
        switch(WaitForSingleObject(hDebugLoopThreadCopy, 100))
        {
        case WAIT_OBJECT_0:
            CloseHandle(hDebugLoopThreadCopy);
            return true;

        case WAIT_TIMEOUT:
        {
            unlock(WAITID_RUN);
            DWORD CurrentTick = GetTickCount();
            DWORD TimeElapsed = CurrentTick - BeginTick;
            if(TimeElapsed >= 10000)
            {
                if(!shownWarning)
                {
                    shownWarning = true;
                    dputs(QT_TRANSLATE_NOOP("DBG", "Finalizing the debugger thread took more than 10 seconds. This can happen if you are loading large symbol files or saving a large database."));
                }
                if(IsFileBeingDebugged() || TimeElapsed >= 100000)
                {
                    dputs(QT_TRANSLATE_NOOP("DBG", "The debuggee did not stop after 10 seconds of requesting termination. The debugger state may be corrupted. It is recommended to restart x64dbg."));
                    DbSave(DbLoadSaveType::All);
                    TerminateThread(hDebugLoopThreadCopy, 1); // TODO: this will lose state and cause possible corruption if a critical section is still owned
                    CloseHandle(hDebugLoopThreadCopy);
                    bIsDebugging = false;
                    return false;
                }
            }
            if(TimeElapsed >= 300 && dbghassessioncapability(UE_SESSION_CAP_PROCESS_CONTROL))
                TitanTerminateProcess(fdProcessInfo->hProcess, -1);
        }
        break;

        case WAIT_FAILED:
            String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", GetLastError()));
            dprintf_untranslated("WAIT_FAILED, GetLastError() = %s\n", error.c_str());
            return false;
        }
    }
}

bool cbDebugAttach(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint pid = 0;
    if(!valfromstring(argv[1], &pid, false))
        return false;

    EXCLUSIVE_ACQUIRE(LockDebugStartStop);

    // Detach instead of terminate when attaching to a new process (if setting enabled)
    if(bIsDebugging && settingboolget("Engine", "DetachOnAttach", false))
    {
        cbDebugDetach(argc, argv);

        if(hDebugLoopThread)
        {
            WaitForSingleObject(hDebugLoopThread, INFINITE);
            CloseHandle(hDebugLoopThread);
            hDebugLoopThread = nullptr;
        }
    }
    else
    {
        cbDebugStop(argc, argv);
    }

    ASSERT_TRUE(hDebugLoopThread == nullptr);

    TitanHandle hProcess = TitanOpenProcess(PROCESS_ALL_ACCESS, false, (DWORD)pid);
    if(!hProcess)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not open process %X!\n"), DWORD(pid));
        return false;
    }

    BOOL debuggerPresent = FALSE;
    if(CheckRemoteDebuggerPresent(hProcess, &debuggerPresent) && debuggerPresent)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Process is already being debugged!"));
        return false;
    }

    BOOL wow64 = false, meow64 = false;
    if(!IsWow64Process(hProcess, &wow64) || !IsWow64Process(GetCurrentProcess(), &meow64))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "IsWow64Process failed!"));
        return false;
    }
    if(meow64 != wow64)
    {
#ifdef _WIN64
        dputs(QT_TRANSLATE_NOOP("DBG", "Use x32dbg to debug this process!"));
#else
        dputs(QT_TRANSLATE_NOOP("DBG", "Use x64dbg to debug this process!"));
#endif // _WIN64
        return false;
    }

    if(!GetFileNameFromProcessHandle(hProcess, szDebuggeePath, _countof(szDebuggeePath)))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not get module filename %X!\n"), DWORD(pid));
        return false;
    }

    if(argc > 2) //event handle (JIT)
    {
        duint eventHandle = 0;
        if(!valfromstring(argv[2], &eventHandle, false))
            return false;
        if(eventHandle)
            dbgsetattachevent((HANDLE)eventHandle);
    }

    if(argc > 3) //thread id to resume (PLMDebug)
    {
        duint tid = 0;
        if(!valfromstring(argv[3], &tid, false))
            return false;
        if(tid)
            dbgsetresumetid(tid);
    }

    static INIT_STRUCT init;
    init.attach = true;
    init.pid = (DWORD)pid;
    init.pauseAtAttach = ScriptIsExecutingCommand();
    dbgcreatedebugthread(&init);
    return true;
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

bool cbDebugDetach(int argc, char* argv[])
{
    PLUG_CB_DETACH detachInfo;
    detachInfo.fdProcessInfo = fdProcessInfo;
    plugincbcall(CB_DETACH, &detachInfo);
    BpEnumAll(dbgdetachDisableAllBreakpoints); // Disable all software breakpoints before detaching.
    if(!DetachDebuggerEx(fdProcessInfo->dwProcessId))
        dputs(QT_TRANSLATE_NOOP("DBG", "DetachDebuggerEx failed..."));
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Detached!"));
    _dbg_animatestop(); // Stop animating
    unlock(WAITID_RUN); // run to resume the debug loop if necessary
    HistoryClear();
    return true;
}

bool cbDebugRun(int argc, char* argv[])
{
    skipInt3Stepping(1, argv);
    return cbDebugRunInternal(argc, argv, history_clear);
}

bool cbDebugErun(int argc, char* argv[])
{
    if(!dbgisrunning())
        dbgsetskipexceptions(true);
    else
    {
        dbgsetskipexceptions(false);
        return true;
    }
    return cbDebugRunInternal(argc, argv, history_clear);
}

bool cbDebugSerun(int argc, char* argv[])
{
    cbDebugContinue(argc, argv);
    return cbDebugRunInternal(argc, argv, history_clear);
}

bool cbDebugPause(int argc, char* argv[])
{
    if(dbggetsessionkind() != UE_SESSION_NONE && !dbghassessioncapability(UE_SESSION_CAP_FORWARD_EXECUTION))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "This replay artifact is already paused."));
        return false;
    }
    if(_dbg_isanimating())
    {
        _dbg_animatestop(); // pause when animating
        return true;
    }
    if(dbgtraceactive())
    {
        dbgforcebreaktrace(); // pause when tracing
        return true;
    }
    if(dbgstepactive())
    {
        dbgforcebreakstep(); // pause when stepping (out/user/system)
        return true;
    }
    if(!DbgIsDebugging())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Not debugging!"));
        return false;
    }
    if(!dbgisrunning())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Program is not running"));
        return false;
    }
    // If the previous pause request could not break the debuggee and no debug
    // events happened since, the debuggee is stuck in a wait that the code
    // below cannot interrupt. Requesting a pause again after a few seconds
    // falls back to a break-in thread. This is not done right away because the
    // extra thread can be used by the debuggee to detect the debugger.
    static ULONGLONG lastPauseRequestTime = 0;
    static duint lastPauseRequestEventCount = 0;
    auto now = GetTickCount64();
    auto eventCount = dbggetdbgeventcount();
    auto stuck = lastPauseRequestTime != 0
                 && now - lastPauseRequestTime >= 2000
                 && eventCount == lastPauseRequestEventCount;
    lastPauseRequestTime = now;
    lastPauseRequestEventCount = eventCount;
    if(stuck && dbgspawnbreakinthread())
        return true;
    // After attaching, the active thread is whatever thread reported the last
    // attach event (usually an idle worker that never wakes up). Target the
    // main thread instead until a real debug event selects an active thread.
    HANDLE hPauseThread = hActiveThread;
    if(auto mainThreadId = dbggetattachmainthread())
    {
        auto hMainThread = ThreadGetHandle(mainThreadId);
        if(hMainThread)
            hPauseThread = hMainThread;
    }
    // As soon as SetBPX plants the INT3, another thread can hit it and the
    // breakpoint callback can reassign hActiveThread. Keep using this local
    // handle so SuspendThread and ResumeThread target the same thread.
    DWORD dwPauseThreadId = TitanGetThreadId(hPauseThread);
    // TODO: get suspend count instead, this can be detected
    // Interesting behavior found by JustMagic, if the active thread is suspended pause would fail
    auto previousSuspendCount = TitanSuspendThread(hPauseThread);
    if(previousSuspendCount != 0)
    {
        if(previousSuspendCount != -1)
            TitanResumeThread(hPauseThread);
        dputs(QT_TRANSLATE_NOOP("DBG", "The active thread is suspended, switch to a running thread to pause the process"));
        // TODO: perhaps inject an INT3 in the process as an alternative to failing?
        return false;
    }
    duint CIP = GetContextDataEx(hPauseThread, UE_CIP);
    if(!SetBPX(CIP, UE_BREAKPOINT, cbPauseBreakpoint))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting breakpoint at %p! (SetBPX)\n"), CIP);
        if(TitanResumeThread(hPauseThread) == -1)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Error resuming thread"));
            return false;
        }
        // Some engine backends cannot modify their breakpoint table while the
        // target is running. Fall back to the explicit break-in operation.
        return dbgspawnbreakinthread();
    }
    //WORKAROUND: If a program is stuck in NtUserGetMessage (GetMessage was called), this
    //will send a WM_NULL to stop the waiting. This only works if the message is not filtered.
    //OllyDbg also does this in a similar way.
    PostThreadMessageA(dwPauseThreadId, WM_NULL, 0, 0);
    if(TitanResumeThread(hPauseThread) == -1)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error resuming thread"));
        return false;
    }
    return true;
}

bool cbDebugContinue(int argc, char* argv[])
{
    if(dbggetsessionkind() != UE_SESSION_NONE && !dbghassessioncapability(UE_SESSION_CAP_EXCEPTION_CONTINUE))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Exception continuation is unavailable for this replay artifact."));
        return false;
    }
    if(argc < 2)
    {
        dbgsetcontinuestatus(DBG_CONTINUE);
        dputs(QT_TRANSLATE_NOOP("DBG", "Exception will be swallowed"));
    }
    else
    {
        dbgsetcontinuestatus(DBG_EXCEPTION_NOT_HANDLED);
        dputs(QT_TRANSLATE_NOOP("DBG", "Exception will be thrown in the program"));
    }
    return true;
}

bool cbDebugStepInto(int argc, char* argv[])
{
    if(dbggetsessionkind() != UE_SESSION_NONE && !dbghassessioncapability(UE_SESSION_CAP_FORWARD_EXECUTION))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "This replay artifact does not support stepping."));
        return false;
    }
    duint steprepeat = 1;
    if(argc > 1 && !valfromstring(argv[1], &steprepeat, false))
        return false;
    if(!steprepeat) //nothing to be done
        return true;
    if(skipInt3Stepping(1, argv) && !--steprepeat)
        return true;
    StepIntoWow64(cbStep);
    dbgsetsteprepeat(true, steprepeat);
    return cbDebugRunInternal(1, argv, steprepeat == 1 ? history_record : history_clear);
}

bool cbDebugeStepInto(int argc, char* argv[])
{
    dbgsetskipexceptions(true);
    return cbDebugStepInto(argc, argv);
}

bool cbDebugseStepInto(int argc, char* argv[])
{
    cbDebugContinue(argc, argv);
    return cbDebugStepInto(argc, argv);
}

static bool IsRepeated(const Zydis & zydis)
{
    // https://www.felixcloutier.com/x86/rep:repe:repz:repne:repnz
    // TODO: allow extracting the affected range
    switch(zydis.GetId())
    {
    // INS
    case ZYDIS_MNEMONIC_INSB:
    case ZYDIS_MNEMONIC_INSW:
    case ZYDIS_MNEMONIC_INSD:
    // OUTS
    case ZYDIS_MNEMONIC_OUTSB:
    case ZYDIS_MNEMONIC_OUTSW:
    case ZYDIS_MNEMONIC_OUTSD:
    // MOVS
    case ZYDIS_MNEMONIC_MOVSB:
    case ZYDIS_MNEMONIC_MOVSW:
    case ZYDIS_MNEMONIC_MOVSD:
    case ZYDIS_MNEMONIC_MOVSQ:
    // LODS
    case ZYDIS_MNEMONIC_LODSB:
    case ZYDIS_MNEMONIC_LODSW:
    case ZYDIS_MNEMONIC_LODSD:
    case ZYDIS_MNEMONIC_LODSQ:
    // STOS
    case ZYDIS_MNEMONIC_STOSB:
    case ZYDIS_MNEMONIC_STOSW:
    case ZYDIS_MNEMONIC_STOSD:
    case ZYDIS_MNEMONIC_STOSQ:
    // CMPS
    case ZYDIS_MNEMONIC_CMPSB:
    case ZYDIS_MNEMONIC_CMPSW:
    case ZYDIS_MNEMONIC_CMPSD:
    case ZYDIS_MNEMONIC_CMPSQ:
    // SCAS
    case ZYDIS_MNEMONIC_SCASB:
    case ZYDIS_MNEMONIC_SCASW:
    case ZYDIS_MNEMONIC_SCASD:
    case ZYDIS_MNEMONIC_SCASQ:
        return (zydis.GetInstr()->info.attributes & (ZYDIS_ATTRIB_HAS_REP | ZYDIS_ATTRIB_HAS_REPZ | ZYDIS_ATTRIB_HAS_REPNZ)) != 0;
    default:
        break;
    }
    return false;
}

bool cbDebugStepOver(int argc, char* argv[])
{
    if(dbggetsessionkind() != UE_SESSION_NONE && !dbghassessioncapability(UE_SESSION_CAP_FORWARD_EXECUTION))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "This replay artifact does not support stepping."));
        return false;
    }
    duint steprepeat = 1;
    if(argc > 1 && !valfromstring(argv[1], &steprepeat, false))
        return false;
    if(!steprepeat) //nothing to be done
        return true;
    if(dbggetsessionkind() == UE_SESSION_TTD)
    {
        if(steprepeat != 1)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Repeated TTD step-over is not supported."));
            return false;
        }
        Zydis replayInstruction;
        const auto cip = GetContextDataEx(hActiveThread, UE_CIP);
        disasm(replayInstruction, cip);
        if(replayInstruction.IsBranchType(Zydis::BTCallSem) || IsRepeated(replayInstruction))
        {
            const auto next = cip + replayInstruction.Size();
            if(!SetBPX(next, UE_BREAKPOINT | UE_SINGLESHOOT, cbStep) || !ReplayRun(false))
            {
                DeleteBPX(next);
                dputs(QT_TRANSLATE_NOOP("DBG", "Unable to schedule the TTD step-over breakpoint."));
                return false;
            }
        }
        else if(!ReplayStep(false, false, cbStep))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Unable to step over in this TTD position."));
            return false;
        }
        dbgsetsteprepeat(false, 1);
        return cbDebugRunInternal(1, argv, history_record);
    }
    if(skipInt3Stepping(1, argv) && !--steprepeat)
        return true;
    auto history = history_clear;
    if(steprepeat == 1)
    {
        Zydis zydis;
        disasm(zydis, GetContextDataEx(hActiveThread, UE_CIP));
        if(!zydis.IsBranchType(Zydis::BTCallSem) && !IsRepeated(zydis))
            history = history_record;
    }
    StepOverWrapper(cbStep);
    dbgsetsteprepeat(false, steprepeat);
    return cbDebugRunInternal(1, argv, history);
}

bool cbDebugeStepOver(int argc, char* argv[])
{
    dbgsetskipexceptions(true);
    return cbDebugStepOver(1, argv);
}

bool cbDebugseStepOver(int argc, char* argv[])
{
    cbDebugContinue(argc, argv);
    return cbDebugStepOver(argc, argv);
}

bool cbDebugStepOut(int argc, char* argv[])
{
    if(dbggetsessionkind() != UE_SESSION_NONE && !dbghassessioncapability(UE_SESSION_CAP_FORWARD_EXECUTION))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "This replay artifact does not support stepping."));
        return false;
    }
    duint steprepeat = 1;
    if(argc > 1 && !valfromstring(argv[1], &steprepeat, false))
        return false;
    if(!steprepeat) //nothing to be done
        return true;
    gRtrPreviousCSP = GetContextDataEx(hActiveThread, UE_CSP);
    StepOverWrapper(cbRtrStep);
    dbgsetsteprepeat(false, steprepeat);
    return cbDebugRunInternal(1, argv, history_clear);
}

bool cbDebugeStepOut(int argc, char* argv[])
{
    dbgsetskipexceptions(true);
    return cbDebugStepOut(argc, argv);
}

bool cbDebugSkip(int argc, char* argv[])
{
    if(dbggetsessionkind() != UE_SESSION_NONE && !dbghassessioncapability(UE_SESSION_CAP_CONTEXT_WRITE))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Register mutation is unavailable for this replay artifact."));
        return false;
    }
    duint skiprepeat = 1;
    if(argc > 1 && !valfromstring(argv[1], &skiprepeat, false))
        return false;

    // Because an int3 causes CIP to be advanced we start disassembling from there
    duint cip;
    if(isInt3Exception())
        cip = (duint)getLastExceptionInfo().ExceptionRecord.ExceptionAddress;
    else
        cip = GetContextDataEx(hActiveThread, UE_CIP);

    dbgsetcontinuestatus(DBG_CONTINUE); //swallow the exception
    BASIC_INSTRUCTION_INFO basicinfo;
    while(skiprepeat--)
    {
        disasmfast(cip, &basicinfo);
        cip += basicinfo.size;
        dbgtraceexecute(cip);
    }
    SetContextDataEx(hActiveThread, UE_CIP, cip);
    DebugUpdateGuiAsync(cip, false); //update GUI
    return true;
}

bool cbInstrInstrUndo(int argc, char* argv[])
{
    HistoryRestore();
    GuiUpdateAllViews();
    return true;
}

bool cbDebugStepUserInto(int argc, char* argv[])
{
    StepIntoUser(cbStep);
    return cbDebugRunInternal(1, argv, history_clear);
}

bool cbDebugStepSystemInto(int argc, char* argv[])
{
    StepIntoSystem(cbStep);
    return cbDebugRunInternal(1, argv, history_clear);
}
