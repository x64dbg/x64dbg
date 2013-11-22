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

static MESSAGE_STACK* gMsgStack=0;
static COMMAND* command_list=0;

static CMDRESULT cbStrLen(const char* cmd)
{
    char arg1[deflen]="";
    if(argget(cmd, arg1, 0, false))
        dprintf("\"%s\"[%d]\n", arg1, strlen(arg1));
    return STATUS_CONTINUE;
}

static CMDRESULT cbCls(const char* cmd)
{
    GuiLogClear();
    return STATUS_CONTINUE;
}

static void registercommands()
{
    COMMAND* cmd=command_list=cmdinit();
    cmdnew(cmd, "strlen\1charcount\1ccount", cbStrLen, false); //get strlen, arg1:string
    cmdnew(cmd, "varnew\1var", cbInstrVar, false); //make a variable arg1:name,[arg2:value]
    cmdnew(cmd, "vardel", cbInstrVarDel, false); //delete a variable, arg1:variable name
    cmdnew(cmd, "mov\1set", cbInstrMov, false); //mov a variable, arg1:dest,arg2:src
    cmdnew(cmd, "cls", cbCls, false); //clear the screen
    cmdnew(cmd, "varlist", cbInstrVarList, false); //list variables[arg1:type filter]
    cmdnew(cmd, "InitDebug\1init\1initdbg", cbDebugInit, false); //init debugger arg1:exefile,[arg2:commandline]
    cmdnew(cmd, "StopDebug\1stop\1dbgstop", cbStopDebug, true); //stop debugger
    cmdnew(cmd, "run\1go\1r\1g", cbDebugRun, true); //unlock WAITID_RUN
    cmdnew(cmd, "SetBPXOptions\1bptype", cbDebugSetBPXOptions, false); //breakpoint type
    cmdnew(cmd, "SetBPX\1bp\1bpx", cbDebugSetBPX, true); //breakpoint
    cmdnew(cmd, "DeleteBPX\1bpc\1bc", cbDebugDeleteBPX, true); //breakpoint delete
    cmdnew(cmd, "EnableBPX\1bpe\1be", cbDebugEnableBPX, true); //breakpoint enable
    cmdnew(cmd, "DisableBPX\1bpd\1bd", cbDebugDisableBPX, true); //breakpoint disable
    cmdnew(cmd, "bplist", cbDebugBplist, true); //breakpoint list
    cmdnew(cmd, "StepInto\1sti", cbDebugStepInto, true); //StepInto
    cmdnew(cmd, "StepOver\1step\1sto\1st", cbDebugStepOver, true); //StepOver
    cmdnew(cmd, "SingleStep\1sstep\1sst", cbDebugSingleStep, true); //SingleStep arg1:count
    cmdnew(cmd, "HideDebugger\1dbh\1hide", cbDebugHide, true); //HideDebugger
    cmdnew(cmd, "disasm\1dis\1d", cbDebugDisasm, true); //doDisasm
    cmdnew(cmd, "SetMemoryBPX\1membp\1bpm", cbDebugSetMemoryBpx, true); //SetMemoryBPX
    cmdnew(cmd, "chd", cbInstrChd, false); //Change directory
    cmdnew(cmd, "rtr", cbDebugRtr, true); //rtr
    cmdnew(cmd, "SetHardwareBreakpoint\1bph\1bphws", cbDebugSetHardwareBreakpoint, true); //hardware breakpoint
    cmdnew(cmd, "alloc", cbDebugAlloc, true); //allocate memory
    cmdnew(cmd, "free", cbDebugFree, true); //free memory
    cmdnew(cmd, "Fill\1memset", cbDebugMemset, true); //memset
    cmdnew(cmd, "scr\1script", cbScript, false); //script testing
    cmdnew(cmd, "bench", cbBenchmark, true); //benchmark test (readmem etc)
    cmdnew(cmd, "pause", cbDebugPause, true); //pause debugger
    cmdnew(cmd, "memwrite", cbMemWrite, true); //memwrite test
    cmdnew(cmd, "StartScylla\1scylla\1imprec", cbStartScylla, false); //start scylla
    cmdnew(cmd, "cmt\1cmtset\1commentset", cbInstrCmt, true); //set/edit comment
    cmdnew(cmd, "cmtc\1cmtdel\1commentdel", cbInstrCmtdel, true); //delete comment
    cmdnew(cmd, "lbl\1lblset\1labelset", cbInstrLbl, true); //set/edit label
    cmdnew(cmd, "lblc\1lbldel\1labeldel", cbInstrLbldel, true); //delete label
    cmdnew(cmd, "bookmark\1bookmarkset", cbInstrBookmarkSet, true); //set bookmark
    cmdnew(cmd, "bookmarkc\1bookmarkdel", cbInstrBookmarkDel, true); //delete bookmark
    cmdnew(cmd, "savedb\1dbsave", cbSavedb, true); //save program database
    cmdnew(cmd, "loaddb\1dbload", cbLoaddb, true); //load program database
    cmdnew(cmd, "DeleteHardwareBreakpoint\1bphc\1bphwc", cbDebugDeleteHardwareBreakpoint, true); //delete hardware breakpoint
    cmdnew(cmd, "DeleteMemoryBPX\1membpc\1bpmc", cbDebugDeleteMemoryBreakpoint, true); //delete memory breakpoint
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
    scriptSetList(command_list);
    CreateThread(0, 0, DbgCommandLoopThread, 0, 0, 0);
    char plugindir[deflen]="";
    strcpy(plugindir, dir);
    PathAppendA(plugindir, "plugins");
    pluginload(plugindir);
    return 0;
}

extern "C" DLL_EXPORT void _dbg_dbgexitsignal()
{
    //TODO: handle exit signal
    cbStopDebug("");
    wait(WAITID_STOP); //after this, debugging stopped
    pluginunload();
    DeleteFileA("DLLLoader.exe");
    cmdfree(command_list);
    varfree();
    msgfreestack(gMsgStack);
}
