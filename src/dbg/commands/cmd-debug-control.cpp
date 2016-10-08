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

CMDRESULT cbDebugRunInternal(int argc, char* argv[])
{
    // Don't "run" twice if the program is already running
    if(dbgisrunning())
        return STATUS_ERROR;
    dbgsetispausedbyuser(false);
    GuiSetDebugStateAsync(running);
    unlock(WAITID_RUN);
    PLUG_CB_RESUMEDEBUG callbackInfo;
    callbackInfo.reserved = 0;
    plugincbcall(CB_RESUMEDEBUG, &callbackInfo);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugInit(int argc, char* argv[])
{
    cbDebugStop(argc, argv);

    static char arg1[deflen] = "";
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "not enough arguments!"));
        return STATUS_ERROR;
    }
    strcpy_s(arg1, argv[1]);
    char szResolvedPath[MAX_PATH] = "";
    if(ResolveShortcut(GuiGetWindowHandle(), StringUtils::Utf8ToUtf16(arg1).c_str(), szResolvedPath, _countof(szResolvedPath)))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Resolved shortcut \"%s\"->\"%s\"\n"), arg1, szResolvedPath);
        strcpy_s(arg1, szResolvedPath);
    }
    if(!FileExists(arg1))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "File does not exist!"));
        return STATUS_ERROR;
    }
    Handle hFile = CreateFileW(StringUtils::Utf8ToUtf16(arg1).c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Could not open file!"));
        return STATUS_ERROR;
    }
    GetFileNameFromHandle(hFile, arg1); //get full path of the file
    hFile.Close();

    //do some basic checks
    switch(GetFileArchitecture(arg1))
    {
    case invalid:
        dputs(QT_TRANSLATE_NOOP("DBG", "Invalid PE file!"));
        return STATUS_ERROR;
#ifdef _WIN64
    case x32:
        dputs(QT_TRANSLATE_NOOP("DBG", "Use x32dbg to debug this file!"));
#else //x86
    case x64:
        dputs(QT_TRANSLATE_NOOP("DBG", "Use x64dbg to debug this file!"));
#endif //_WIN64
        return STATUS_ERROR;
    case dotnet:
        dputs(QT_TRANSLATE_NOOP("DBG", "This file is a dotNET application."));
        break;
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
    _dbg_animatestop();
    StopDebug();
    //history
    HistoryClear();
    DWORD BeginTick = GetTickCount();
    while(waitislocked(WAITID_STOP))   //custom waiting
    {
        unlock(WAITID_RUN);
        Sleep(100);
        DWORD CurrentTick = GetTickCount();
        if(CurrentTick - BeginTick > 10000)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "The debuggee does not stop after 10 seconds. The debugger state may be corrupted."));
            return STATUS_ERROR;
        }
        if(CurrentTick - BeginTick >= 300)
            TerminateProcess(fdProcessInfo->hProcess, -1);
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugAttach(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Not enough arguments!"));
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
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not open process %X!\n"), DWORD(pid));
        return STATUS_ERROR;
    }
    BOOL wow64 = false, mewow64 = false;
    if(!IsWow64Process(hProcess, &wow64) || !IsWow64Process(GetCurrentProcess(), &mewow64))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "IsWow64Process failed!"));
        return STATUS_ERROR;
    }
    if((mewow64 && !wow64) || (!mewow64 && wow64))
    {
#ifdef _WIN64
        dputs(QT_TRANSLATE_NOOP("DBG", "Use x32dbg to debug this process!"));
#else
        dputs(QT_TRANSLATE_NOOP("DBG", "Use x64dbg to debug this process!"));
#endif // _WIN64
        return STATUS_ERROR;
    }
    wchar_t wszFileName[MAX_PATH] = L"";
    if(!GetModuleFileNameExW(hProcess, 0, wszFileName, MAX_PATH))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Could not get module filename %X!\n"), DWORD(pid));
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

CMDRESULT cbDebugRun(int argc, char* argv[])
{
    HistoryClear();
    return cbDebugRunInternal(argc, argv);
}

CMDRESULT cbDebugErun(int argc, char* argv[])
{
    HistoryClear();
    if(!dbgisrunning())
        dbgsetskipexceptions(true);
    else
    {
        dbgsetskipexceptions(false);
        return STATUS_CONTINUE;
    }
    return cbDebugRunInternal(argc, argv);
}

CMDRESULT cbDebugSerun(int argc, char* argv[])
{
    cbDebugContinue(argc, argv);
    return cbDebugRunInternal(argc, argv);
}

CMDRESULT cbDebugPause(int argc, char* argv[])
{
    if(_dbg_isanimating())
    {
        _dbg_animatestop(); // pause when animating
        return STATUS_CONTINUE;
    }
    if(!DbgIsDebugging())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Not debugging!"));
        return STATUS_ERROR;
    }
    if(!dbgisrunning())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Program is not running"));
        return STATUS_ERROR;
    }
    if(SuspendThread(hActiveThread) == -1)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error suspending thread"));
        return STATUS_ERROR;
    }
    duint CIP = GetContextDataEx(hActiveThread, UE_CIP);
    if(!SetBPX(CIP, UE_BREAKPOINT, (void*)cbPauseBreakpoint))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error setting breakpoint at %p! (SetBPX)\n"), CIP);
        if(ResumeThread(hActiveThread) == -1)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Error resuming thread"));
            return STATUS_ERROR;
        }
        return STATUS_ERROR;
    }
    dbgsetispausedbyuser(true);
    if(ResumeThread(hActiveThread) == -1)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error resuming thread"));
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugContinue(int argc, char* argv[])
{
    if(argc < 2)
    {
        SetNextDbgContinueStatus(DBG_CONTINUE);
        dputs(QT_TRANSLATE_NOOP("DBG", "Exception will be swallowed"));
    }
    else
    {
        SetNextDbgContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
        dputs(QT_TRANSLATE_NOOP("DBG", "Exception will be thrown in the program"));
    }
    return STATUS_CONTINUE;
}

static bool skipInt3Stepping(int argc, char* argv[])
{
    if(!bSkipInt3Stepping || dbgisrunning())
        return false;
    duint cip = GetContextDataEx(hActiveThread, UE_CIP);
    unsigned char ch;
    MemRead(cip, &ch, sizeof(ch));
    if(ch == 0xCC && getLastExceptionInfo().ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Skipped INT3!"));
        cbDebugSkip(argc, argv);
        return true;
    }
    return false;
}

CMDRESULT cbDebugStepInto(int argc, char* argv[])
{
    if(skipInt3Stepping(argc, argv))
        return STATUS_CONTINUE;
    StepInto((void*)cbStep);
    // History
    HistoryAdd();
    dbgsetstepping(true);
    return cbDebugRunInternal(argc, argv);
}

CMDRESULT cbDebugeStepInto(int argc, char* argv[])
{
    dbgsetskipexceptions(true);
    return cbDebugStepInto(argc, argv);
}

CMDRESULT cbDebugseStepInto(int argc, char* argv[])
{
    cbDebugContinue(argc, argv);
    return cbDebugStepInto(argc, argv);
}

CMDRESULT cbDebugStepOver(int argc, char* argv[])
{
    if(skipInt3Stepping(argc, argv))
        return STATUS_CONTINUE;
    StepOver((void*)cbStep);
    // History
    HistoryClear();
    dbgsetstepping(true);
    return cbDebugRunInternal(argc, argv);
}

CMDRESULT cbDebugeStepOver(int argc, char* argv[])
{
    dbgsetskipexceptions(true);
    return cbDebugStepOver(argc, argv);
}

CMDRESULT cbDebugseStepOver(int argc, char* argv[])
{
    cbDebugContinue(argc, argv);
    return cbDebugStepOver(argc, argv);
}

CMDRESULT cbDebugSingleStep(int argc, char* argv[])
{
    duint stepcount = 1;
    if(argc > 1)
        if(!valfromstring(argv[1], &stepcount))
            stepcount = 1;
    SingleStep((DWORD)stepcount, (void*)cbStep);
    HistoryClear();
    dbgsetstepping(true);
    return cbDebugRunInternal(argc, argv);
}

CMDRESULT cbDebugeSingleStep(int argc, char* argv[])
{
    dbgsetskipexceptions(true);
    return cbDebugSingleStep(argc, argv);
}

CMDRESULT cbDebugStepOut(int argc, char* argv[])
{
    HistoryClear();
    StepOver((void*)cbRtrStep);
    return cbDebugRunInternal(argc, argv);
}

CMDRESULT cbDebugeStepOut(int argc, char* argv[])
{
    dbgsetskipexceptions(true);
    return cbDebugStepOut(argc, argv);
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
    DebugUpdateGuiAsync(cip, false); //update GUI
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrInstrUndo(int argc, char* argv[])
{
    HistoryRestore();
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}