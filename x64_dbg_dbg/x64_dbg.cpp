#include "_global.h"
#include "argument.h"
#include "command.h"
#include "variable.h"
#include "instruction.h"
#include "debugger.h"
#include "data.h"
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

static MESSAGE_STACK* gMsgStack=0;
static COMMAND* command_list=0;
static HANDLE hCommandLoopThread=0;
static char alloctrace[MAX_PATH]="";

//Original code by Aurel from http://www.codeguru.com/cpp/w-p/win32/article.php/c1427/A-Simple-Win32-CommandLine-Parser.htm
static void commandlinefree(int argc, char** argv)
{
    for(int i=0; i<argc; i++)
        efree(argv[i]);
    efree(argv);
}

static char** commandlineparse(int* argc)
{
    if(!argc)
        return NULL;
    LPWSTR wcCommandLine=GetCommandLineW();
    LPWSTR* argw=CommandLineToArgvW(wcCommandLine, argc);
    char** argv=(char**)emalloc(sizeof(void*)*(*argc+1));
    for(int i=0; i<*argc; i++)
    {
        int bufSize=WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, argw[i], -1, NULL, 0, NULL, NULL);
        argv[i]=(char*)emalloc(bufSize+1);
        WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, argw[i], bufSize*sizeof(WCHAR), argv[i], bufSize, NULL, NULL);
    }
    LocalFree(argw);
    return argv;
}

static CMDRESULT cbStrLen(int argc, char* argv[])
{
    if(argc<2)
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
    if(argc<2)
        dprintf("\n");
    else
        dprintf("%s", argv[1]);
    return STATUS_CONTINUE;
}

static void registercommands()
{
    COMMAND* cmd=command_list=cmdinit();

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

    //breakpoints
    dbgcmdnew("bplist", cbDebugBplist, true); //breakpoint list
    dbgcmdnew("SetBPXOptions\1bptype", cbDebugSetBPXOptions, false); //breakpoint type
    dbgcmdnew("SetBPX\1bp\1bpx", cbDebugSetBPX, true); //breakpoint
    dbgcmdnew("DeleteBPX\1bpc\1bc", cbDebugDeleteBPX, true); //breakpoint delete
    dbgcmdnew("EnableBPX\1bpe\1be", cbDebugEnableBPX, true); //breakpoint enable
    dbgcmdnew("DisableBPX\1bpd\1bd", cbDebugDisableBPX, true); //breakpoint disable
    dbgcmdnew("SetHardwareBreakpoint\1bph\1bphws", cbDebugSetHardwareBreakpoint, true); //hardware breakpoint
    dbgcmdnew("DeleteHardwareBreakpoint\1bphc\1bphwc", cbDebugDeleteHardwareBreakpoint, true); //delete hardware breakpoint
    dbgcmdnew("SetMemoryBPX\1membp\1bpm", cbDebugSetMemoryBpx, true); //SetMemoryBPX
    dbgcmdnew("DeleteMemoryBPX\1membpc\1bpmc", cbDebugDeleteMemoryBreakpoint, true); //delete memory breakpoint

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

    //memory operations
    dbgcmdnew("alloc", cbDebugAlloc, true); //allocate memory
    dbgcmdnew("free", cbDebugFree, true); //free memory
    dbgcmdnew("Fill\1memset", cbDebugMemset, true); //memset

    //plugins
    dbgcmdnew("StartScylla\1scylla\1imprec", cbStartScylla, false); //start scylla

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
    dbgcmdnew("reffind\1findref\1ref", cbInstrRefFind, true);
    dbgcmdnew("refstr\1strref", cbInstrRefStr, true);
    dbgcmdnew("find", cbInstrFind, true); //find a pattern
    dbgcmdnew("findall", cbInstrFindAll, true); //find all patterns

    //undocumented
    dbgcmdnew("bench", cbBenchmark, true); //benchmark test (readmem etc)
    dbgcmdnew("memwrite", cbMemWrite, true); //memwrite test
    dbgcmdnew("asm", cbAssemble, true); //assemble instruction
    dbgcmdnew("dprintf", cbPrintf, false); //printf
    dbgcmdnew("refinit", cbInstrRefinit, false);
    dbgcmdnew("refadd", cbInstrRefadd, false);
    dbgcmdnew("setstr\1strset", cbInstrSetstr, false); //set a string variable
    dbgcmdnew("getstr\1strget", cbInstrGetstr, false); //get a string variable
    dbgcmdnew("copystr\1strcpy", cbInstrCopystr, true); //write a string variable to memory
    dbgcmdnew("DebugContinue\1con", cbDebugContinue, true); //set continue status
    dbgcmdnew("bpdll", cbBpDll, true); //set dll breakpoint
    dbgcmdnew("bcdll", cbBcDll, true); //remove dll breakpoint
    dbgcmdnew("modcallfind", cbInstrModCallFind, true); //find intermodular calls
    dbgcmdnew("commentlist", cbInstrCommentList, true); //list comments
    dbgcmdnew("labellist", cbInstrLabelList, true); //list labels
    dbgcmdnew("bookmarklist", cbInstrBookmarkList, true); //list bookmarks
    dbgcmdnew("functionlist", cbInstrFunctionList, true); //list functions
    dbgcmdnew("looplist", cbInstrLoopList, true); //list loops
}

static bool cbCommandProvider(char* cmd, int maxlen)
{
    MESSAGE msg;
    msgwait(gMsgStack, &msg);
    char* newcmd=(char*)msg.param1;
    if(strlen(newcmd)>=deflen)
        newcmd[deflen-1]=0;
    strcpy(cmd, newcmd);
    efree(newcmd, "cbCommandProvider:newcmd"); //free allocated command
    return true;
}

extern "C" DLL_EXPORT bool _dbg_dbgcmdexec(const char* cmd)
{
    int len=strlen(cmd);
    char* newcmd=(char*)emalloc((len+1)*sizeof(char), "_dbg_dbgcmdexec:newcmd");
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
    dbginit();
    dbgfunctionsinit();
    json_set_alloc_funcs(emalloc_json, efree_json);
    char dir[deflen]="";
    if(!GetModuleFileNameA(hInst, dir, deflen))
        return "GetModuleFileNameA failed!";
    int len=strlen(dir);
    while(dir[len]!='\\')
        len--;
    dir[len]=0;
    strcpy(alloctrace, dir);
    PathAppendA(alloctrace, "\\alloctrace.txt");
    DeleteFileA(alloctrace);
    setalloctrace(alloctrace);
    strcpy(dbbasepath, dir); //debug directory
    PathAppendA(dbbasepath, "db");
    CreateDirectoryA(dbbasepath, 0); //create database directory
    SetCurrentDirectoryA(dir);
    gMsgStack=msgallocstack();
    if(!gMsgStack)
        return "Could not allocate message stack!";
    varinit();
    registercommands();
    hCommandLoopThread=CreateThread(0, 0, DbgCommandLoopThread, 0, 0, 0);
    char plugindir[deflen]="";
    strcpy(plugindir, dir);
    PathAppendA(plugindir, "plugins");
    pluginload(plugindir);
    //handle command line
    int argc=0;
    char** argv=commandlineparse(&argc);
    if(argc>1) //we have an argument
    {
        std::string str="init \"";
        str+=argv[1];
        str+="\"";
        DbgCmdExec(str.c_str());
    }
    commandlinefree(argc, argv);
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
        char msg[256]="";
        sprintf(msg, "%d memory leak(s) found!\n\nPlease send 'alloctrace.txt' to the authors of x64_dbg.", memleaks());
        MessageBoxA(0, msg, "error", MB_ICONERROR|MB_SYSTEMMODAL);
    }
    else
        DeleteFileA(alloctrace);
}

extern "C" DLL_EXPORT bool _dbg_dbgcmddirectexec(const char* cmd)
{
    if(cmddirectexec(command_list, cmd)==STATUS_ERROR)
        return false;
    return true;
}

COMMAND* dbggetcommandlist()
{
    return command_list;
}
