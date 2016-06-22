/**
 @file x64_dbg.cpp

 @brief Implements the 64 debug class.
 */

#include "_global.h"
#include "command.h"
#include "variable.h"
#include "instruction.h"
#include "debugger.h"
#include "simplescript.h"
#include "console.h"
#include "x64_dbg.h"
#include "msgqueue.h"
#include "threading.h"
#include "plugin_loader.h"
#include "_dbgfunctions.h"
#include "debugger_commands.h"
#include <capstone_wrapper.h>
#include "_scriptapi_gui.h"
#include "filehelper.h"
#include "database.h"
#include "mnemonichelp.h"
#include "datainst_helper.h"
#include "error.h"
#include "exception.h"

static MESSAGE_STACK* gMsgStack = 0;
static HANDLE hCommandLoopThread = 0;
static bool bStopCommandLoopThread = false;
static char alloctrace[MAX_PATH] = "";
static bool bIsStopped = true;
static char scriptDllDir[MAX_PATH] = "";
static String notesFile;

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

static bool DbgScriptDllExec(const char* dll);

static CMDRESULT cbScriptDll(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    return DbgScriptDllExec(argv[1]) ? STATUS_CONTINUE : STATUS_ERROR;
}

static void registercommands()
{
    cmdinit();

    //debug control
    dbgcmdnew("InitDebug\1init\1initdbg", cbDebugInit, false); //init debugger arg1:exefile,[arg2:commandline]
    dbgcmdnew("StopDebug\1stop\1dbgstop", cbDebugStop, true); //stop debugger
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
    dbgcmdnew("StepOut\1rtr", cbDebugRtr, true); //StepOut
    dbgcmdnew("eStepOut\1ertr", cbDebugeRtr, true); //rtr + skip first chance exceptions
    dbgcmdnew("TraceOverConditional\1tocnd", cbDebugTocnd, true); //Trace over conditional
    dbgcmdnew("TraceIntoConditional\1ticnd", cbDebugTicnd, true); //Trace into conditional
    dbgcmdnew("TraceIntoBeyondTraceRecord\1tibt", cbDebugTibt, true); //Trace into beyond trace record
    dbgcmdnew("TraceOverBeyondTraceRecord\1tobt", cbDebugTobt, true); //Trace over beyond trace record
    dbgcmdnew("TraceIntoIntoTraceRecord\1tiit", cbDebugTiit, true); //Trace into into trace record
    dbgcmdnew("TraceOverIntoTraceRecord\1toit", cbDebugToit, true); //Trace over into trace record
    dbgcmdnew("DebugContinue\1con", cbDebugContinue, true); //set continue status
    dbgcmdnew("switchthread\1threadswitch", cbDebugSwitchthread, true); //switch thread
    dbgcmdnew("suspendthread\1threadsuspend", cbDebugSuspendthread, true); //suspend thread
    dbgcmdnew("resumethread\1threadresume", cbDebugResumethread, true); //resume thread
    dbgcmdnew("killthread\1threadkill", cbDebugKillthread, true); //kill thread
    dbgcmdnew("suspendallthreads\1threadsuspendall", cbDebugSuspendAllThreads, true); //suspend all threads
    dbgcmdnew("resumeallthreads\1threadresumeall", cbDebugResumeAllThreads, true); //resume all threads
    dbgcmdnew("setthreadpriority\1setprioritythread\1threadsetpriority", cbDebugSetPriority, true); //set thread priority
    dbgcmdnew("symdownload\1downloadsym", cbDebugDownloadSymbol, true); //download symbols
    dbgcmdnew("getcmdline\1getcommandline", cbDebugGetCmdline, true); //Get CmdLine
    dbgcmdnew("setcmdline\1setcommandline", cbDebugSetCmdline, true); //Set CmdLine
    dbgcmdnew("skip", cbDebugSkip, true); //skip one instruction
    dbgcmdnew("RunToParty", cbDebugRunToParty, true); //Run to code in a party
    dbgcmdnew("RunToUserCode\1rtu", cbDebugRtu, true); //Run to user code

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
    dbgcmdnew("LibrarianSetBreakPoint\1bpdll", cbDebugBpDll, true); //set dll breakpoint
    dbgcmdnew("LibrarianRemoveBreakPoint\1bcdll", cbDebugBcDll, true); //remove dll breakpoint

    //breakpoints (conditional)
    dbgcmdnew("SetBreakpointName\1bpname", cbDebugSetBPXName, true); //set breakpoint name
    dbgcmdnew("SetBreakpointCondition\1bpcond\1bpcnd", cbDebugSetBPXCondition, true); //set breakpoint breakCondition
    dbgcmdnew("SetBreakpointLog\1bplog\1bpl", cbDebugSetBPXLog, true); //set breakpoint logText
    dbgcmdnew("SetBreakpointLogCondition\1bplogcondition", cbDebugSetBPXLogCondition, true); //set breakpoint logCondition
    dbgcmdnew("SetBreakpointCommand", cbDebugSetBPXCommand, true); //set breakpoint command on hit
    dbgcmdnew("SetBreakpointCommandCondition", cbDebugSetBPXCommandCondition, true); //set breakpoint commandCondition
    dbgcmdnew("SetBreakpointFastResume", cbDebugSetBPXFastResume, true); //set breakpoint fast resume
    dbgcmdnew("GetBreakpointHitCount", cbDebugGetBPXHitCount, true); //get breakpoint hit count
    dbgcmdnew("ResetBreakpointHitCount", cbDebugResetBPXHitCount, true); //reset breakpoint hit count
    dbgcmdnew("SetHardwareBreakpointName\1bphwname", cbDebugSetBPXHardwareName, true); //set breakpoint name
    dbgcmdnew("SetHardwareBreakpointCondition\1bphwcond", cbDebugSetBPXHardwareCondition, true); //set breakpoint breakCondition
    dbgcmdnew("SetHardwareBreakpointLog\1bphwlog", cbDebugSetBPXHardwareLog, true); //set breakpoint logText
    dbgcmdnew("SetHardwareBreakpointLogCondition\1bphwlogcondition", cbDebugSetBPXHardwareLogCondition, true); //set breakpoint logText
    dbgcmdnew("SetHardwareBreakpointCommand", cbDebugSetBPXHardwareCommand, true); //set breakpoint command on hit
    dbgcmdnew("SetHardwareBreakpointCommandCondition", cbDebugSetBPXHardwareCommandCondition, true); //set breakpoint commandCondition
    dbgcmdnew("SetHardwareBreakpointFastResume", cbDebugSetBPXHardwareFastResume, true); //set breakpoint fast resume
    dbgcmdnew("GetHardwareBreakpointHitCount", cbDebugGetBPXHardwareHitCount, true); //get breakpoint hit count
    dbgcmdnew("ResetHardwareBreakpointHitCount", cbDebugResetBPXHardwareHitCount, true); //reset breakpoint hit count
    dbgcmdnew("SetMemoryBreakpointName\1bpmname", cbDebugSetBPXMemoryName, true); //set breakpoint name
    dbgcmdnew("SetMemoryBreakpointCondition\1bpmcond", cbDebugSetBPXMemoryCondition, true); //set breakpoint breakCondition
    dbgcmdnew("SetMemoryBreakpointLog\1bpmlog", cbDebugSetBPXMemoryLog, true); //set breakpoint log
    dbgcmdnew("SetMemoryBreakpointLogCondition\1bpmlogcondition", cbDebugSetBPXMemoryLogCondition, true); //set breakpoint logCondition
    dbgcmdnew("SetMemoryBreakpointCommand", cbDebugSetBPXMemoryCommand, true); //set breakpoint command on hit
    dbgcmdnew("SetMemoryBreakpointCommandCondition", cbDebugSetBPXMemoryCommandCondition, true); //set breakpoint commandCondition
    dbgcmdnew("SetMemoryBreakpointFastResume", cbDebugSetBPXMemoryFastResume, true); //set breakpoint fast resume
    dbgcmdnew("SetMemoryGetBreakpointHitCount", cbDebugGetBPXMemoryHitCount, true); //get breakpoint hit count
    dbgcmdnew("ResetMemoryBreakpointHitCount", cbDebugResetBPXMemoryHitCount, true); //reset breakpoint hit count

    dbgcmdnew("bpgoto", cbDebugSetBPGoto, true);

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
    dbgcmdnew("asm", cbInstrAssemble, true); //assemble instruction
    dbgcmdnew("sleep", cbInstrSleep, false); //Sleep
    dbgcmdnew("setfreezestack", cbDebugSetfreezestack, false); //freeze the stack from auto updates
    dbgcmdnew("setjit\1jitset", cbDebugSetJIT, false); //set JIT
    dbgcmdnew("getjit\1jitget", cbDebugGetJIT, false); //get JIT
    dbgcmdnew("getjitauto\1jitgetauto", cbDebugGetJITAuto, false); //get JIT Auto
    dbgcmdnew("setjitauto\1jitsetauto", cbDebugSetJITAuto, false); //set JIT Auto
    dbgcmdnew("loadlib", cbDebugLoadLib, true); //Load DLL

    //user database
    dbgcmdnew("cmt\1cmtset\1commentset", cbInstrCmt, true); //set/edit comment
    dbgcmdnew("cmtc\1cmtdel\1commentdel", cbInstrCmtdel, true); //delete comment
    dbgcmdnew("lbl\1lblset\1labelset", cbInstrLbl, true); //set/edit label
    dbgcmdnew("lblc\1lbldel\1labeldel", cbInstrLbldel, true); //delete label
    dbgcmdnew("bookmark\1bookmarkset", cbInstrBookmarkSet, true); //set bookmark
    dbgcmdnew("bookmarkc\1bookmarkdel", cbInstrBookmarkDel, true); //delete bookmark
    dbgcmdnew("savedb\1dbsave", cbInstrSavedb, true); //save program database
    dbgcmdnew("loaddb\1dbload", cbInstrLoaddb, true); //load program database
    dbgcmdnew("functionadd\1func", cbInstrFunctionAdd, true); //function
    dbgcmdnew("functiondel\1funcc", cbInstrFunctionDel, true); //function
    dbgcmdnew("commentlist", cbInstrCommentList, true); //list comments
    dbgcmdnew("labellist", cbInstrLabelList, true); //list labels
    dbgcmdnew("bookmarklist", cbInstrBookmarkList, true); //list bookmarks
    dbgcmdnew("functionlist", cbInstrFunctionList, true); //list functions
    dbgcmdnew("functionclear", cbInstrFunctionClear, false); //delete all functions

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
    dbgcmdnew("push", cbInstrPush, true);
    dbgcmdnew("pop", cbInstrPop, true);

    //script
    dbgcmdnew("scriptload", cbScriptLoad, false);
    dbgcmdnew("msg", cbScriptMsg, false);
    dbgcmdnew("msgyn", cbScriptMsgyn, false);
    dbgcmdnew("log", cbInstrLog, false); //log command with superawesome hax

    //data
    dbgcmdnew("reffind\1findref\1ref", cbInstrRefFind, true); //find references to a value
    dbgcmdnew("refstr\1strref", cbInstrRefStr, true); //find string references
    dbgcmdnew("find", cbInstrFind, true); //find a pattern
    dbgcmdnew("findall", cbInstrFindAll, true); //find all patterns
    dbgcmdnew("modcallfind", cbInstrModCallFind, true); //find intermodular calls
    dbgcmdnew("findasm\1asmfind", cbInstrFindAsm, true); //find instruction
    dbgcmdnew("reffindrange\1findrefrange\1refrange", cbInstrRefFindRange, true);
    dbgcmdnew("yara", cbInstrYara, true); //yara test command
    dbgcmdnew("yaramod", cbInstrYaramod, true); //yara rule on module
    dbgcmdnew("analyse\1analyze\1anal", cbInstrAnalyse, true); //secret analysis command

    //undocumented
    dbgcmdnew("bench", cbDebugBenchmark, true); //benchmark test (readmem etc)
    dbgcmdnew("dprintf", cbPrintf, false); //printf
    dbgcmdnew("setstr\1strset", cbInstrSetstr, false); //set a string variable
    dbgcmdnew("getstr\1strget", cbInstrGetstr, false); //get a string variable
    dbgcmdnew("copystr\1strcpy", cbInstrCopystr, true); //write a string variable to memory
    dbgcmdnew("looplist", cbInstrLoopList, true); //list loops
    dbgcmdnew("capstone", cbInstrCapstone, true); //disassemble using capstone
    dbgcmdnew("visualize", cbInstrVisualize, true); //visualize analysis
    dbgcmdnew("meminfo", cbInstrMeminfo, true); //command to debug memory map bugs
    dbgcmdnew("cfanal\1cfanalyse\1cfanalyze", cbInstrCfanalyse, true); //control flow analysis
    dbgcmdnew("analyse_nukem\1analyze_nukem\1anal_nukem", cbInstrAnalyseNukem, true); //secret analysis command #2
    dbgcmdnew("exanal\1exanalyse\1exanalyze", cbInstrExanalyse, true); //exception directory analysis
    dbgcmdnew("virtualmod", cbInstrVirtualmod, true); //virtual module
    dbgcmdnew("findallmem\1findmemall", cbInstrFindMemAll, true); //memory map pattern find
    dbgcmdnew("setmaxfindresult\1findsetmaxresult", cbInstrSetMaxFindResult, false); //set the maximum number of occurences found
    dbgcmdnew("savedata", cbInstrSavedata, true); //save data to disk
    dbgcmdnew("scriptdll\1dllscript", cbScriptDll, false); //execute a script DLL
    dbgcmdnew("mnemonichelp", cbInstrMnemonichelp, false); //mnemonic help
    dbgcmdnew("mnemonicbrief", cbInstrMnemonicbrief, false); //mnemonic brief
    dbgcmdnew("GetPrivilegeState", cbGetPrivilegeState, true); //get priv state
    dbgcmdnew("EnablePrivilege", cbEnablePrivilege, true); //enable priv
    dbgcmdnew("DisablePrivilege", cbDisablePrivilege, true); //disable priv
    dbgcmdnew("handleclose", cbHandleClose, true); //close remote handle
    dbgcmdnew("briefcheck", cbInstrBriefcheck, true); //check if mnemonic briefs are missing
    dbgcmdnew("analrecur\1analr", cbInstrAnalrecur, true); //analyze a single function
    dbgcmdnew("analxrefs\1analx", cbInstrAnalxrefs, true); //analyze xrefs
    dbgcmdnew("analadv", cbInstrAnalyseadv, true); //analyze xref,function and data
    dbgcmdnew("guiupdatedisable", cbInstrDisableGuiUpdate, true); //disable gui message
    dbgcmdnew("guiupdateenable", cbInstrEnableGuiUpdate, true); //enable gui message
    dbgcmdnew("exhandlers", cbInstrExhandlers, true); //enumerate exception handlers
}

static bool cbCommandProvider(char* cmd, int maxlen)
{
    MESSAGE msg;
    MsgWait(gMsgStack, &msg);
    if(bStopCommandLoopThread)
        return false;
    char* newcmd = (char*)msg.param1;
    if(strlen(newcmd) >= deflen)
    {
        dprintf("command cut at ~%d characters\n", deflen);
        newcmd[deflen - 2] = 0;
    }
    strcpy_s(cmd, deflen, newcmd);
    efree(newcmd, "cbCommandProvider:newcmd"); //free allocated command
    return true;
}

extern "C" DLL_EXPORT bool _dbg_dbgcmdexec(const char* cmd)
{
    int len = (int)strlen(cmd);
    char* newcmd = (char*)emalloc((len + 1) * sizeof(char), "_dbg_dbgcmdexec:newcmd");
    strcpy_s(newcmd, len + 1, cmd);
    return MsgSend(gMsgStack, 0, (duint)newcmd, 0);
}

static DWORD WINAPI DbgCommandLoopThread(void* a)
{
    cmdloop(cbBadCmd, cbCommandProvider, cmdfindmain, false);
    return 0;
}

typedef void(*SCRIPTDLLSTART)();

struct DLLSCRIPTEXECTHREADINFO
{
    DLLSCRIPTEXECTHREADINFO(HINSTANCE hScriptDll, SCRIPTDLLSTART AsyncStart)
        : hScriptDll(hScriptDll),
          AsyncStart(AsyncStart)
    {
    }

    HINSTANCE hScriptDll;
    SCRIPTDLLSTART AsyncStart;
};

static DWORD WINAPI DbgScriptDllExecThread(void* a)
{
    auto info = (DLLSCRIPTEXECTHREADINFO*)a;
    auto AsyncStart = info->AsyncStart;
    auto hScriptDll = info->hScriptDll;
    delete info;

    dprintf("[Script DLL] Calling export \"AsyncStart\"...\n");
    AsyncStart();
    dprintf("[Script DLL] \"AsyncStart\" returned!\n");

    dprintf("[Script DLL] Calling FreeLibrary...");
    if(FreeLibrary(hScriptDll))
        dprintf("success!\n");
    else
        dprintf("failure (%08X)...\n", GetLastError());

    return 0;
}

static bool DbgScriptDllExec(const char* dll)
{
    String dllPath = dll;
    if(dllPath.find('\\') == String::npos)
        dllPath = String(scriptDllDir) + String(dll);

    dprintf("[Script DLL] Loading Script DLL \"%s\"...\n", dllPath.c_str());

    auto hScriptDll = LoadLibraryW(StringUtils::Utf8ToUtf16(dllPath).c_str());
    if(hScriptDll)
    {
        dprintf("[Script DLL] DLL loaded on 0x%p!\n", hScriptDll);

        auto AsyncStart = SCRIPTDLLSTART(GetProcAddress(hScriptDll, "AsyncStart"));
        if(AsyncStart)
        {
            dprintf("[Script DLL] Creating thread to call the export \"AsyncStart\"...\n");
            CloseHandle(CreateThread(nullptr, 0, DbgScriptDllExecThread, new DLLSCRIPTEXECTHREADINFO(hScriptDll, AsyncStart), 0, nullptr)); //on-purpose memory leak here
        }
        else
        {
            auto Start = SCRIPTDLLSTART(GetProcAddress(hScriptDll, "Start"));
            if(Start)
            {
                dprintf("[Script DLL] Calling export \"Start\"...\n");
                Start();
                dprintf("[Script DLL] \"Start\" returned!\n");
            }
            else
                dprintf("[Script DLL] Failed to find the exports \"AsyncStart\" or \"Start\" (%08X)!\n", GetLastError());

            dprintf("[Script DLL] Calling FreeLibrary...");
            if(FreeLibrary(hScriptDll))
                dprintf("success!\n");
            else
                dprintf("failure (%08X)...\n", GetLastError());
        }
    }
    else
        dprintf("[Script DLL] LoadLibary failed (%08X)!\n", GetLastError());

    return true;
}

extern "C" DLL_EXPORT const char* _dbg_dbginit()
{
    if(!EngineCheckStructAlignment(UE_STRUCT_TITAN_ENGINE_CONTEXT, sizeof(TITAN_ENGINE_CONTEXT_t)))
        return "Invalid TITAN_ENGINE_CONTEXT_t alignment!";

    if(sizeof(TITAN_ENGINE_CONTEXT_t) != sizeof(REGISTERCONTEXT))
        return "Invalid REGISTERCONTEXT alignment!";

    dputs("Initializing wait objects...");
    waitinitialize();
    dputs("Initializing debugger...");
    dbginit();
    dputs("Initializing debugger functions...");
    dbgfunctionsinit();
    dputs("Setting JSON memory management functions...");
    json_set_alloc_funcs(json_malloc, json_free);
    dputs("Initializing capstone...");
    Capstone::GlobalInitialize();
    dputs("Initializing Yara...");
    if(yr_initialize() != ERROR_SUCCESS)
        return "Failed to initialize Yara!";
    dputs("Getting directory information...");
    wchar_t wszDir[deflen] = L"";
    if(!GetModuleFileNameW(hInst, wszDir, deflen))
        return "GetModuleFileNameW failed!";
    char dir[deflen] = "";
    strcpy_s(dir, StringUtils::Utf16ToUtf8(wszDir).c_str());
    int len = (int)strlen(dir);
    while(dir[len] != '\\')
        len--;
    dir[len] = 0;
    strcpy_s(alloctrace, dir);
    strcat_s(alloctrace, "\\alloctrace.txt");
    strcpy_s(scriptDllDir, dir);
    strcat_s(scriptDllDir, "\\scripts\\");
    DeleteFileW(StringUtils::Utf8ToUtf16(alloctrace).c_str());
    setalloctrace(alloctrace);
    initDataInstMap();

    // Load mnemonic help database
    String mnemonicHelpData;
    if(FileHelper::ReadAllText(StringUtils::sprintf("%s\\..\\mnemdb.json", dir), mnemonicHelpData))
    {
        if(MnemonicHelp::loadFromText(mnemonicHelpData.c_str()))
            dputs("Mnemonic help database loaded!");
        else
            dputs("Failed to load mnemonic help database...");
    }
    else
        dputs("Failed to read mnemonic help database...");

    // Load error codes
    if(ErrorCodeInit(StringUtils::sprintf("%s\\..\\errordb.txt", dir)))
        dputs("Error codes database loaded!");
    else
        dputs("Failed to load error codes...");

    // Load exception codes
    if(ExceptionCodeInit(StringUtils::sprintf("%s\\..\\exceptiondb.txt", dir)))
        dputs("Exception codes database loaded!");
    else
        dputs("Failed to load exception codes...");

    // Create database directory in the local debugger folder
    DbSetPath(StringUtils::sprintf("%s\\db", dir).c_str(), nullptr);

    char szLocalSymbolPath[MAX_PATH] = "";
    strcpy_s(szLocalSymbolPath, dir);
    strcat_s(szLocalSymbolPath, "\\symbols");

    char cachePath[MAX_SETTING_SIZE];
    if(!BridgeSettingGet("Symbols", "CachePath", cachePath) || !*cachePath)
    {
        strcpy_s(szSymbolCachePath, szLocalSymbolPath);
        BridgeSettingSet("Symbols", "CachePath", ".\\symbols");
    }
    else
    {
        if(_strnicmp(cachePath, ".\\", 2) == 0)
        {
            strncpy_s(szSymbolCachePath, dir, _TRUNCATE);
            strncat_s(szSymbolCachePath, cachePath + 1, _TRUNCATE);
        }
        else
        {
            // Trim the buffer to fit inside MAX_PATH
            strncpy_s(szSymbolCachePath, cachePath, _TRUNCATE);
        }

        if(strstr(szSymbolCachePath, "http://") || strstr(szSymbolCachePath, "https://"))
        {
            if(Script::Gui::MessageYesNo("It is strongly discouraged to use symbol servers in your path directly (use the store option instead).\n\nDo you want me to fix this?"))
            {
                strcpy_s(szSymbolCachePath, szLocalSymbolPath);
                BridgeSettingSet("Symbols", "CachePath", ".\\symbols");
            }
        }
    }
    dprintf("Symbol Path: %s\n", szSymbolCachePath);
    SetCurrentDirectoryW(StringUtils::Utf8ToUtf16(dir).c_str());
    dputs("Allocating message stack...");
    gMsgStack = MsgAllocStack();
    if(!gMsgStack)
        return "Could not allocate message stack!";
    dputs("Initializing global script variables...");
    varinit();
    dputs("Registering debugger commands...");
    registercommands();
    dputs("Registering GUI command handler...");
    SCRIPTTYPEINFO info;
    strcpy_s(info.name, "Default");
    info.id = 0;
    info.execute = DbgCmdExec;
    info.completeCommand = nullptr;
    GuiRegisterScriptLanguage(&info);
    SCRIPTTYPEINFO infoDll;
    strcpy_s(info.name, "Script DLL");
    infoDll.id = 0;
    info.execute = DbgScriptDllExec;
    info.completeCommand = nullptr;
    GuiRegisterScriptLanguage(&info);
    dputs("Starting command loop...");
    hCommandLoopThread = CreateThread(0, 0, DbgCommandLoopThread, 0, 0, 0);
    char plugindir[deflen] = "";
    strcpy_s(plugindir, dir);
    strcat_s(plugindir, "\\plugins");
    CreateDirectoryW(StringUtils::Utf8ToUtf16(plugindir).c_str(), 0);
    dputs("Loading plugins...");
    pluginload(plugindir);
    dputs("Handling command line...");
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
    dputs("Reading notes file...");
    notesFile = String(dir) + "\\notes.txt";
    String text;
    FileHelper::ReadAllText(notesFile, text);
    GuiSetGlobalNotes(text.c_str());
    dputs("Initialization successful!");
    bIsStopped = false;
    return nullptr;
}

extern "C" DLL_EXPORT void _dbg_dbgexitsignal()
{
    dputs("Stopping running debuggee...");
    cbDebugStop(0, 0);
    dputs("Aborting scripts...");
    scriptabort();
    dputs("Waiting for the debuggee to be stopped...");
    wait(WAITID_STOP); //after this, debugging stopped
    dputs("Unloading plugins...");
    pluginunload();
    dputs("Stopping command thread...");
    bStopCommandLoopThread = true;
    MsgFreeStack(gMsgStack);
    WaitForThreadTermination(hCommandLoopThread);
    dputs("Cleaning up allocated data...");
    cmdfree();
    varfree();
    yr_finalize();
    Capstone::GlobalFinalize();
    dputs("Checking for mem leaks...");
    if(memleaks())
        dprintf("%d memory leak(s) found!\n", memleaks());
    else
        DeleteFileW(StringUtils::Utf8ToUtf16(alloctrace).c_str());
    dputs("Cleaning up wait objects...");
    waitdeinitialize();
    dputs("Cleaning up debugger threads...");
    dbgstop();
    dputs("Saving notes...");
    char* text = nullptr;
    GuiGetGlobalNotes(&text);
    if(text)
    {
        FileHelper::WriteAllText(notesFile, String(text));
        BridgeFree(text);
    }
    else
        DeleteFileW(StringUtils::Utf8ToUtf16(notesFile).c_str());
    dputs("Exit signal processed successfully!");
    bIsStopped = true;
}

extern "C" DLL_EXPORT bool _dbg_dbgcmddirectexec(const char* cmd)
{
    if(cmddirectexec(cmd) == STATUS_ERROR)
        return false;
    return true;
}

bool dbgisstopped()
{
    return bIsStopped;
}
