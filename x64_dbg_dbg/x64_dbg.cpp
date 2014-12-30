#include "_global.h"
#include "argument.h"
#include "command.h"
#include "variable.h"
#include "instruction.h"
#include "debugger.h"
#include "simplescript.h"
#include "console.h"
#include "math.h"
#include "x64_dbg.h"
#include "msgqueue.h"
#include "addrinfo.h"
#include "threading.h"
#include "plugin_loader.h"
#include "assemble.h"
#include "_dbgfunctions.h"
#include "debugger_commands.h"

static MESSAGE_STACK* gMsgStack = 0;
static COMMAND* command_list = 0;
static HANDLE hCommandLoopThread = 0;
static char alloctrace[MAX_PATH] = "";

static CMDRESULT cbStrLen(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    dprintf("\"%s\"[%d]\n", argv[1], strlen(argv[1]));
    return STATUS_CONTINUE;
}

static CMDRESULT cbCls(int argc, char* argv[])
{
    GuiLogClear();
    return STATUS_CONTINUE;
}

static CMDRESULT cbPrintf(int argc, char* argv[])
{
    if(argc < 2)
        dprintf("\n");
    else
        dprintf("%s", argv[1]);
    return STATUS_CONTINUE;
}

static void registercommands()
{
    COMMAND* cmd = command_list = cmdinit();

    //debug control
    dbgcmdnew("InitDebug\1init\1initdbg", cbDebugInit, false); //init debugger arg1:exefile,[arg2:commandline]
    dbgcmdnew("StopDebug\1stop\1dbgstop", cbStopDebug, true); //stop debugger
    dbgcmdnew("AttachDebugger\1attach", cbDebugAttach, false); //attach
    dbgcmdnew("DetachDebugger\1detach", cbDebugDetach, true); //detach
    dbgcmdnew("run\1go\1r\1g", cbDebugRun, true); //unlock WAITID_RUN
    dbgcmdnew("erun\1egun\1er\1eg", cbDebugErun, true); //run + skip first chance exceptions
    dbgcmdnew("pause", cbDebugPause, true); //pause debugger
    dbgcmdnew("StepInto\1sti", cbDebugStepInto, true); //StepInto
    dbgcmdnew("eStepInto\1esti", cbDebugeStepInto, true); //StepInto + skip first chance exceptions
    dbgcmdnew("StepOver\1step\1sto\1st", cbDebugStepOver, true); //StepOver
    dbgcmdnew("eStepOver\1estep\1esto\1est", cbDebugeStepOver, true); //StepOver + skip first chance exceptions
    dbgcmdnew("SingleStep\1sstep\1sst", cbDebugSingleStep, true); //SingleStep arg1:count
    dbgcmdnew("eSingleStep\1esstep\1esst", cbDebugeSingleStep, true); //SingleStep arg1:count + skip first chance exceptions
    dbgcmdnew("StepOut\1rtr", cbDebugRtr, true); //rtr
    dbgcmdnew("eStepOut\1ertr", cbDebugeRtr, true); //rtr + skip first chance exceptions
    dbgcmdnew("DebugContinue\1con", cbDebugContinue, true); //set continue status
    dbgcmdnew("LibrarianSetBreakPoint\1bpdll", cbDebugBpDll, true); //set dll breakpoint
    dbgcmdnew("LibrarianRemoveBreakPoint\1bcdll", cbDebugBcDll, true); //remove dll breakpoint
    dbgcmdnew("switchthread\1threadswitch", cbDebugSwitchthread, true); //switch thread
    dbgcmdnew("suspendthread\1threadsuspend", cbDebugSuspendthread, true); //suspend thread
    dbgcmdnew("resumethread\1threadresume", cbDebugResumethread, true); //resume thread
    dbgcmdnew("killthread\1threadkill", cbDebugKillthread, true); //kill thread
    dbgcmdnew("suspendallthreads\1threadsuspendall", cbDebugSuspendAllThreads, true); //suspend all threads
    dbgcmdnew("resumeallthreads\1threadresumeall", cbDebugResumeAllThreads, true); //resume all threads
    dbgcmdnew("setthreadpriority\1setprioritythread\1threadsetpriority", cbDebugSetPriority, true); //set thread priority
    dbgcmdnew("symdownload\1downloadsym", cbDebugDownloadSymbol, true); //download symbols
    dbgcmdnew("setjit\1jitset", cbDebugSetJIT, false); //set JIT
    dbgcmdnew("getjit\1jitget", cbDebugGetJIT, false); //get JIT
    dbgcmdnew("getjitauto\1jitgetauto", cbDebugGetJITAuto, false); //get JIT Auto
    dbgcmdnew("setjitauto\1jitsetauto", cbDebugSetJITAuto, false); //set JIT Auto
    dbgcmdnew("getcmdline\1getcommandline", cbDebugGetCmdline, true); //Get CmdLine
    dbgcmdnew("setcmdline\1setcommandline", cbDebugSetCmdline, true); //Set CmdLine
    dbgcmdnew("loadlib", cbDebugLoadLib, true); //Load DLL
    dbgcmdnew("skip", cbDebugSkip, true); //skip one instruction

    //breakpoints
    dbgcmdnew("bplist", cbDebugBplist, true); //breakpoint list
    dbgcmdnew("SetBPXOptions\1bptype", cbDebugSetBPXOptions, false); //breakpoint type
    dbgcmdnew("SetBPX\1bp\1bpx", cbDebugSetBPX, true); //breakpoint
    dbgcmdnew("DeleteBPX\1bpc\1bc", cbDebugDeleteBPX, true); //breakpoint delete
    dbgcmdnew("EnableBPX\1bpe\1be", cbDebugEnableBPX, true); //breakpoint enable
    dbgcmdnew("DisableBPX\1bpd\1bd", cbDebugDisableBPX, true); //breakpoint disable
    dbgcmdnew("SetHardwareBreakpoint\1bph\1bphws", cbDebugSetHardwareBreakpoint, true); //hardware breakpoint
    dbgcmdnew("DeleteHardwareBreakpoint\1bphc\1bphwc", cbDebugDeleteHardwareBreakpoint, true); //delete hardware breakpoint
    dbgcmdnew("EnableHardwareBreakpoint\1bphe\1bphwe", cbDebugEnableHardwareBreakpoint, true); //enable hardware breakpoint
    dbgcmdnew("DisableHardwareBreakpoint\1bphd\1bphwd", cbDebugDisableHardwareBreakpoint, true); //disable hardware breakpoint
    dbgcmdnew("SetMemoryBPX\1membp\1bpm", cbDebugSetMemoryBpx, true); //SetMemoryBPX
    dbgcmdnew("DeleteMemoryBPX\1membpc\1bpmc", cbDebugDeleteMemoryBreakpoint, true); //delete memory breakpoint
    dbgcmdnew("EnableMemoryBreakpoint\1membpe\1bpme", cbDebugEnableMemoryBreakpoint, true); //enable memory breakpoint
    dbgcmdnew("DisableMemoryBreakpoint\1membpd\1bpmd", cbDebugDisableMemoryBreakpoint, true); //enable memory breakpoint

    //variables
    dbgcmdnew("varnew\1var", cbInstrVar, false); //make a variable arg1:name,[arg2:value]
    dbgcmdnew("vardel", cbInstrVarDel, false); //delete a variable, arg1:variable name
    dbgcmdnew("varlist", cbInstrVarList, false); //list variables[arg1:type filter]
    dbgcmdnew("mov\1set", cbInstrMov, false); //mov a variable, arg1:dest,arg2:src

    //misc
    dbgcmdnew("strlen\1charcount\1ccount", cbStrLen, false); //get strlen, arg1:string
    dbgcmdnew("cls\1lc\1lclr", cbCls, false); //clear the log
    dbgcmdnew("chd", cbInstrChd, false); //Change directory
    dbgcmdnew("disasm\1dis\1d", cbDebugDisasm, true); //doDisasm
    dbgcmdnew("HideDebugger\1dbh\1hide", cbDebugHide, true); //HideDebugger
    dbgcmdnew("dump", cbDebugDump, true); //dump at address
    dbgcmdnew("sdump", cbDebugStackDump, true); //dump at stack address
    dbgcmdnew("refinit", cbInstrRefinit, false);
    dbgcmdnew("refadd", cbInstrRefadd, false);
    dbgcmdnew("asm", cbAssemble, true); //assemble instruction
    dbgcmdnew("sleep", cbInstrSleep, false); //Sleep

    //user database
    dbgcmdnew("cmt\1cmtset\1commentset", cbInstrCmt, true); //set/edit comment
    dbgcmdnew("cmtc\1cmtdel\1commentdel", cbInstrCmtdel, true); //delete comment
    dbgcmdnew("lbl\1lblset\1labelset", cbInstrLbl, true); //set/edit label
    dbgcmdnew("lblc\1lbldel\1labeldel", cbInstrLbldel, true); //delete label
    dbgcmdnew("bookmark\1bookmarkset", cbInstrBookmarkSet, true); //set bookmark
    dbgcmdnew("bookmarkc\1bookmarkdel", cbInstrBookmarkDel, true); //delete bookmark
    dbgcmdnew("savedb\1dbsave", cbSavedb, true); //save program database
    dbgcmdnew("loaddb\1dbload", cbLoaddb, true); //load program database
    dbgcmdnew("functionadd\1func", cbFunctionAdd, true); //function
    dbgcmdnew("functiondel\1funcc", cbFunctionDel, true); //function
    dbgcmdnew("commentlist", cbInstrCommentList, true); //list comments
    dbgcmdnew("labellist", cbInstrLabelList, true); //list labels
    dbgcmdnew("bookmarklist", cbInstrBookmarkList, true); //list bookmarks
    dbgcmdnew("functionlist", cbInstrFunctionList, true); //list functions

    //memory operations
    dbgcmdnew("alloc", cbDebugAlloc, true); //allocate memory
    dbgcmdnew("free", cbDebugFree, true); //free memory
    dbgcmdnew("Fill\1memset", cbDebugMemset, true); //memset
    dbgcmdnew("getpagerights\1getrightspage", cbDebugGetPageRights, true);
    dbgcmdnew("setpagerights\1setrightspage", cbDebugSetPageRights, true);

    //plugins
    dbgcmdnew("StartScylla\1scylla\1imprec", cbDebugStartScylla, false); //start scylla

    //general purpose
    dbgcmdnew("cmp", cbInstrCmp, false); //compare
    dbgcmdnew("gpa", cbInstrGpa, true);
    dbgcmdnew("add", cbInstrAdd, false);
    dbgcmdnew("and", cbInstrAnd, false);
    dbgcmdnew("dec", cbInstrDec, false);
    dbgcmdnew("div", cbInstrDiv, false);
    dbgcmdnew("inc", cbInstrInc, false);
    dbgcmdnew("mul", cbInstrMul, false);
    dbgcmdnew("neg", cbInstrNeg, false);
    dbgcmdnew("not", cbInstrNot, false);
    dbgcmdnew("or", cbInstrOr, false);
    dbgcmdnew("rol", cbInstrRol, false);
    dbgcmdnew("ror", cbInstrRor, false);
    dbgcmdnew("shl", cbInstrShl, false);
    dbgcmdnew("shr", cbInstrShr, false);
    dbgcmdnew("sub", cbInstrSub, false);
    dbgcmdnew("test", cbInstrTest, false);
    dbgcmdnew("xor", cbInstrXor, false);

    //script
    dbgcmdnew("scriptload", cbScriptLoad, false);
    dbgcmdnew("msg", cbScriptMsg, false);
    dbgcmdnew("msgyn", cbScriptMsgyn, false);

    //data
    dbgcmdnew("reffind\1findref\1ref", cbInstrRefFind, true); //find references to a value
    dbgcmdnew("refstr\1strref", cbInstrRefStr, true); //find string references
    dbgcmdnew("find", cbInstrFind, true); //find a pattern
    dbgcmdnew("findall", cbInstrFindAll, true); //find all patterns
    dbgcmdnew("modcallfind", cbInstrModCallFind, true); //find intermodular calls
    dbgcmdnew("findasm\1asmfind", cbInstrFindAsm, true); //find instruction
    dbgcmdnew("reffindrange\1findrefrange\1refrange", cbInstrRefFindRange, true);

    //undocumented
    dbgcmdnew("bench", cbDebugBenchmark, true); //benchmark test (readmem etc)
    dbgcmdnew("dprintf", cbPrintf, false); //printf
    dbgcmdnew("setstr\1strset", cbInstrSetstr, false); //set a string variable
    dbgcmdnew("getstr\1strget", cbInstrGetstr, false); //get a string variable
    dbgcmdnew("copystr\1strcpy", cbInstrCopystr, true); //write a string variable to memory
    dbgcmdnew("looplist", cbInstrLoopList, true); //list loops
}

static bool cbCommandProvider(char* cmd, int maxlen)
{
    MESSAGE msg;
    msgwait(gMsgStack, &msg);
    char* newcmd = (char*)msg.param1;
    if(strlen(newcmd) >= deflen)
    {
        dprintf("command cut at ~%d characters\n", deflen);
        newcmd[deflen - 2] = 0;
    }
    strcpy(cmd, newcmd);
    efree(newcmd, "cbCommandProvider:newcmd"); //free allocated command
    return true;
}

extern "C" DLL_EXPORT bool _dbg_dbgcmdexec(const char* cmd)
{
    int len = (int)strlen(cmd);
    char* newcmd = (char*)emalloc((len + 1) * sizeof(char), "_dbg_dbgcmdexec:newcmd");
    strcpy(newcmd, cmd);
    return msgsend(gMsgStack, 0, (uint)newcmd, 0);
}

static DWORD WINAPI DbgCommandLoopThread(void* a)
{
    cmdloop(command_list, cbBadCmd, cbCommandProvider, cmdfindmain, false);
    return 0;
}

static void* emalloc_json(size_t size)
{
    return emalloc(size, "json:ptr");
}

static void efree_json(void* ptr)
{
    efree(ptr, "json:ptr");
}

extern "C" DLL_EXPORT const char* _dbg_dbginit()
{
    if(!EngineCheckStructAlignment(UE_STRUCT_TITAN_ENGINE_CONTEXT, sizeof(TITAN_ENGINE_CONTEXT_t)))
        return "Invalid TITAN_ENGINE_CONTEXT_t alignment!";
    if(sizeof(TITAN_ENGINE_CONTEXT_t) != sizeof(REGISTERCONTEXT))
        return "Invalid REGISTERCONTEXT alignment!";
    dbginit();
    dbgfunctionsinit();
    json_set_alloc_funcs(emalloc_json, efree_json);
    wchar_t wszDir[deflen] = L"";
    if(!GetModuleFileNameW(hInst, wszDir, deflen))
        return "GetModuleFileNameW failed!";
    char dir[deflen] = "";
    strcpy_s(dir, StringUtils::Utf16ToUtf8(wszDir).c_str());
    int len = (int)strlen(dir);
    while(dir[len] != '\\')
        len--;
    dir[len] = 0;
    strcpy(alloctrace, dir);
    PathAppendA(alloctrace, "\\alloctrace.txt");
    DeleteFileW(StringUtils::Utf8ToUtf16(alloctrace).c_str());
    setalloctrace(alloctrace);
    strcpy(dbbasepath, dir); //debug directory
    PathAppendA(dbbasepath, "db");
    CreateDirectoryW(StringUtils::Utf8ToUtf16(dbbasepath).c_str(), 0); //create database directory
    strcpy(szSymbolCachePath, dir);
    PathAppendA(szSymbolCachePath, "symbols");
    SetCurrentDirectoryW(StringUtils::Utf8ToUtf16(dir).c_str());;
    gMsgStack = msgallocstack();
    if(!gMsgStack)
        return "Could not allocate message stack!";
    varinit();
    registercommands();
    hCommandLoopThread = CreateThread(0, 0, DbgCommandLoopThread, 0, 0, 0);
    char plugindir[deflen] = "";
    strcpy(plugindir, dir);
    PathAppendA(plugindir, "plugins");
    CreateDirectoryW(StringUtils::Utf8ToUtf16(plugindir).c_str(), 0);
    pluginload(plugindir);
    //handle command line
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if(argc == 2) //we have an argument
    {
        String str = "init \"";
        str += StringUtils::Utf16ToUtf8(argv[1]);
        str += "\"";
        DbgCmdExec(str.c_str());
    }
    else if(argc == 5) //4 arguments (JIT)
    {
        if(_wcsicmp(argv[1], L"-a") == 0 && !_wcsicmp(argv[3], L"-e"))
        {
            String str = "attach .";
            str += StringUtils::Utf16ToUtf8(argv[2]);
            str += ", .";
            str += StringUtils::Utf16ToUtf8(argv[4]);
            DbgCmdExec(str.c_str());
        }
    }
    LocalFree(argv);

    return 0;
}

extern "C" DLL_EXPORT void _dbg_dbgexitsignal()
{
    cbStopDebug(0, 0);
    scriptabort();
    wait(WAITID_STOP); //after this, debugging stopped
    pluginunload();
    TerminateThread(hCommandLoopThread, 0);
    CloseHandle(hCommandLoopThread);
    cmdfree(command_list);
    varfree();
    msgfreestack(gMsgStack);
    if(memleaks())
    {
        char msg[256] = "";
        sprintf(msg, "%d memory leak(s) found!\n\nPlease send contact the authors of x64_dbg.", memleaks());
        MessageBoxA(0, msg, "error", MB_ICONERROR | MB_SYSTEMMODAL);
    }
    else
        DeleteFileA(alloctrace);
    CriticalSectionLocker::Deinitialize();
}

extern "C" DLL_EXPORT bool _dbg_dbgcmddirectexec(const char* cmd)
{
    if(cmddirectexec(command_list, cmd) == STATUS_ERROR)
        return false;
    return true;
}

COMMAND* dbggetcommandlist()
{
    return command_list;
}
