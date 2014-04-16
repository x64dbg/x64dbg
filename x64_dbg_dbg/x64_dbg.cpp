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

static MESSAGE_STACK* gMsgStack=0;
static COMMAND* command_list=0;

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
    cmdnew(cmd, "InitDebug\1init\1initdbg", cbDebugInit, false); //init debugger arg1:exefile,[arg2:commandline]
    cmdnew(cmd, "StopDebug\1stop\1dbgstop", cbStopDebug, true); //stop debugger
    cmdnew(cmd, "AttachDebugger\1attach", cbDebugAttach, false); //attach
    cmdnew(cmd, "DetachDebugger\1detach", cbDebugDetach, true); //detach
    cmdnew(cmd, "run\1go\1r\1g", cbDebugRun, true); //unlock WAITID_RUN
    cmdnew(cmd, "erun\1egun\1er\1eg", cbDebugErun, true); //run + skip first chance exceptions
    cmdnew(cmd, "pause", cbDebugPause, true); //pause debugger
    cmdnew(cmd, "StepInto\1sti", cbDebugStepInto, true); //StepInto
    cmdnew(cmd, "eStepInto\1esti", cbDebugeStepInto, true); //StepInto + skip first chance exceptions
    cmdnew(cmd, "StepOver\1step\1sto\1st", cbDebugStepOver, true); //StepOver
    cmdnew(cmd, "eStepOver\1estep\1esto\1est", cbDebugeStepOver, true); //StepOver + skip first chance exceptions
    cmdnew(cmd, "SingleStep\1sstep\1sst", cbDebugSingleStep, true); //SingleStep arg1:count
    cmdnew(cmd, "eSingleStep\1esstep\1esst", cbDebugeSingleStep, true); //SingleStep arg1:count + skip first chance exceptions
    cmdnew(cmd, "StepOut\1rtr", cbDebugRtr, true); //rtr
    cmdnew(cmd, "eStepOut\1ertr", cbDebugeRtr, true); //rtr + skip first chance exceptions

    //breakpoints
    cmdnew(cmd, "bplist", cbDebugBplist, true); //breakpoint list
    cmdnew(cmd, "SetBPXOptions\1bptype", cbDebugSetBPXOptions, false); //breakpoint type
    cmdnew(cmd, "SetBPX\1bp\1bpx", cbDebugSetBPX, true); //breakpoint
    cmdnew(cmd, "DeleteBPX\1bpc\1bc", cbDebugDeleteBPX, true); //breakpoint delete
    cmdnew(cmd, "EnableBPX\1bpe\1be", cbDebugEnableBPX, true); //breakpoint enable
    cmdnew(cmd, "DisableBPX\1bpd\1bd", cbDebugDisableBPX, true); //breakpoint disable
    cmdnew(cmd, "SetHardwareBreakpoint\1bph\1bphws", cbDebugSetHardwareBreakpoint, true); //hardware breakpoint
    cmdnew(cmd, "DeleteHardwareBreakpoint\1bphc\1bphwc", cbDebugDeleteHardwareBreakpoint, true); //delete hardware breakpoint
    cmdnew(cmd, "SetMemoryBPX\1membp\1bpm", cbDebugSetMemoryBpx, true); //SetMemoryBPX
    cmdnew(cmd, "DeleteMemoryBPX\1membpc\1bpmc", cbDebugDeleteMemoryBreakpoint, true); //delete memory breakpoint

    //variables
    cmdnew(cmd, "varnew\1var", cbInstrVar, false); //make a variable arg1:name,[arg2:value]
    cmdnew(cmd, "vardel", cbInstrVarDel, false); //delete a variable, arg1:variable name
    cmdnew(cmd, "varlist", cbInstrVarList, false); //list variables[arg1:type filter]
    cmdnew(cmd, "mov\1set", cbInstrMov, false); //mov a variable, arg1:dest,arg2:src

    //misc
    cmdnew(cmd, "strlen\1charcount\1ccount", cbStrLen, false); //get strlen, arg1:string
    cmdnew(cmd, "cls\1lc\1lclr", cbCls, false); //clear the log
    cmdnew(cmd, "chd", cbInstrChd, false); //Change directory
    cmdnew(cmd, "disasm\1dis\1d", cbDebugDisasm, true); //doDisasm
    cmdnew(cmd, "HideDebugger\1dbh\1hide", cbDebugHide, true); //HideDebugger

    //user database
    cmdnew(cmd, "cmt\1cmtset\1commentset", cbInstrCmt, true); //set/edit comment
    cmdnew(cmd, "cmtc\1cmtdel\1commentdel", cbInstrCmtdel, true); //delete comment
    cmdnew(cmd, "lbl\1lblset\1labelset", cbInstrLbl, true); //set/edit label
    cmdnew(cmd, "lblc\1lbldel\1labeldel", cbInstrLbldel, true); //delete label
    cmdnew(cmd, "bookmark\1bookmarkset", cbInstrBookmarkSet, true); //set bookmark
    cmdnew(cmd, "bookmarkc\1bookmarkdel", cbInstrBookmarkDel, true); //delete bookmark
    cmdnew(cmd, "savedb\1dbsave", cbSavedb, true); //save program database
    cmdnew(cmd, "loaddb\1dbload", cbLoaddb, true); //load program database
    cmdnew(cmd, "functionadd\1func", cbFunctionAdd, true); //function
    cmdnew(cmd, "functiondel\1funcc", cbFunctionDel, true); //function

    //memory operations
    cmdnew(cmd, "alloc", cbDebugAlloc, true); //allocate memory
    cmdnew(cmd, "free", cbDebugFree, true); //free memory
    cmdnew(cmd, "Fill\1memset", cbDebugMemset, true); //memset

    //plugins
    cmdnew(cmd, "StartScylla\1scylla\1imprec", cbStartScylla, false); //start scylla

    //general purpose
    cmdnew(cmd, "cmp", cbInstrCmp, false); //compare
    cmdnew(cmd, "gpa", cbInstrGpa, true);
    cmdnew(cmd, "add", cbInstrAdd, false);
    cmdnew(cmd, "and", cbInstrAnd, false);
    cmdnew(cmd, "dec", cbInstrDec, false);
    cmdnew(cmd, "div", cbInstrDiv, false);
    cmdnew(cmd, "inc", cbInstrInc, false);
    cmdnew(cmd, "mul", cbInstrMul, false);
    cmdnew(cmd, "neg", cbInstrNeg, false);
    cmdnew(cmd, "not", cbInstrNot, false);
    cmdnew(cmd, "or", cbInstrOr, false);
    cmdnew(cmd, "rol", cbInstrRol, false);
    cmdnew(cmd, "ror", cbInstrRor, false);
    cmdnew(cmd, "shl", cbInstrShl, false);
    cmdnew(cmd, "shr", cbInstrShr, false);
    cmdnew(cmd, "sub", cbInstrSub, false);
    cmdnew(cmd, "test", cbInstrTest, false);
    cmdnew(cmd, "xor", cbInstrXor, false);

    //script
    cmdnew(cmd, "scriptload", cbScriptLoad, false);
    cmdnew(cmd, "msg", cbScriptMsg, false);
    cmdnew(cmd, "msgyn", cbScriptMsgyn, false);

    //undocumented
    cmdnew(cmd, "bench", cbBenchmark, true); //benchmark test (readmem etc)

    cmdnew(cmd, "memwrite", cbMemWrite, true); //memwrite test
    cmdnew(cmd, "asm", cbAssemble, true); //assemble instruction

    cmdnew(cmd, "dump", cbDebugDump, true); //dump at address
    cmdnew(cmd, "sdump", cbDebugStackDump, true); //dump at stack address
    cmdnew(cmd, "dprintf", cbPrintf, false); //printf

    cmdnew(cmd, "refinit", cbInstrRefinit, false);
    cmdnew(cmd, "refadd", cbInstrRefadd, false);
    cmdnew(cmd, "reffind\1findref\1ref", cbInstrRefFind, true);
    cmdnew(cmd, "refstr\1strref", cbInstrRefStr, true);

    cmdnew(cmd, "setstr\1strset", cbInstrSetstr, false); //set a string variable
    cmdnew(cmd, "getstr\1strget", cbInstrGetstr, false); //get a string variable
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

extern "C" DLL_EXPORT const char* _dbg_dbginit()
{
    DeleteFileA("DLLLoader.exe");
    DeleteFileA("alloctrace.txt");
    char dir[deflen]="";
    if(!GetModuleFileNameA(hInst, dir, deflen))
        return "GetModuleFileNameA failed!";
    int len=strlen(dir);
    while(dir[len]!='\\')
        len--;
    dir[len]=0;
    strcpy(sqlitedb_basedir, dir); //debug directory
    PathAppendA(sqlitedb_basedir, "db");
    SetCurrentDirectoryA(dir);
    gMsgStack=msgallocstack();
    if(!gMsgStack)
        return "Could not allocate message stack!";
    varinit();
    registercommands();
    CreateThread(0, 0, DbgCommandLoopThread, 0, 0, 0);
    char plugindir[deflen]="";
    strcpy(plugindir, dir);
    PathAppendA(plugindir, "plugins");
    pluginload(plugindir);
    return 0;
}

extern "C" DLL_EXPORT void _dbg_dbgexitsignal()
{
    cbStopDebug(0, 0);
    wait(WAITID_STOP); //after this, debugging stopped
    pluginunload();
    DeleteFileA("DLLLoader.exe");
    cmdfree(command_list);
    varfree();
    msgfreestack(gMsgStack);
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
