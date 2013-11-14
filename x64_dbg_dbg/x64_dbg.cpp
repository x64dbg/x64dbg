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

static MESSAGE_STACK* gMsgStack;

static CMDRESULT cbStrLen(const char* cmd)
{
    char arg1[deflen]="";
    if(argget(cmd, arg1, 0, false))
        dprintf("\"%s\"[%d]\n", arg1, strlen(arg1));
    return STATUS_CONTINUE;
}

static CMDRESULT cbExit(const char* cmd)
{
    return STATUS_EXIT;
}

static CMDRESULT cbCls(const char* cmd)
{
    GuiLogClear();
    return STATUS_CONTINUE;
}

static COMMAND* command_list=0;

static void registercommands()
{
    COMMAND* cmd=command_list=cmdinit();
    //cmdnew(cmd, "exit\1x", cbExit, false); //quit debugger
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
    cmdnew(cmd, "EnableBPX\1bpe\1be", cbDebugEnableBPX, true); //breakpoint enable
    cmdnew(cmd, "DisableBPX\1bpd\1bd", cbDebugDisableBPX, true); //breakpoint disable
    cmdnew(cmd, "ToggleBPX\1bpt\1bt", cbDebugToggleBPX, true); //breakpoint toggle
    cmdnew(cmd, "DeleteBPX\1bpc\1bc", cbDebugDeleteBPX, true); //breakpoint delete
    cmdnew(cmd, "bplist", cbDebugBplist, true); //breakpoint list
    cmdnew(cmd, "StepInto\1sti", cbDebugStepInto, true); //StepInto
    cmdnew(cmd, "StepOver\1step\1sto\1st", cbDebugStepOver, true); //StepOver
    cmdnew(cmd, "SingleStep\1sstep\1sst", cbDebugSingleStep, true); //SingleStep arg1:count
    cmdnew(cmd, "HideDebugger\1dbh\1hide", cbDebugHide, true); //HideDebugger
    cmdnew(cmd, "disasm\1dis\1d", cbDebugDisasm, true); //doDisasm
    cmdnew(cmd, "SetMemoryBPX\1membp\1bpm", cbDebugMemoryBpx, true); //SetMemoryBPX
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
}

static bool cbCommandProvider(char* cmd, int maxlen)
{
    MESSAGE msg;
    msgwait(gMsgStack, &msg);
    char* newcmd=(char*)msg.param1;
    if(strlen(newcmd)>=deflen)
        newcmd[deflen-1]=0;
    strcpy(cmd, newcmd);
    efree(newcmd); //free allocated command
    return true;
}

extern "C" DLL_EXPORT bool _dbg_dbgcmdexec(const char* cmd)
{
    int len=strlen(cmd);
    char* newcmd=(char*)emalloc((len+1)*sizeof(char));
    strcpy(newcmd, cmd);
    return msgsend(gMsgStack, 0, (uint)newcmd, 0);
}

static DWORD WINAPI ConsoleReadLoopThread(void* a)
{
    char cmd[deflen];
    while(1)
    {
        fgets(cmd, deflen, stdin);
        cmd[strlen(cmd)-1]=0;
        while(!_dbg_dbgcmdexec(cmd)) //retry until the command came through
            Sleep(100);
    }
    return 0;
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
    GetModuleFileNameA(hInst, dir, deflen);
    int len=strlen(dir);
    while(dir[len]!='\\')
        len--;
    dir[len]=0;
    SetCurrentDirectoryA(dir);
    gMsgStack=msgallocstack();
    if(!gMsgStack)
        return "Could not allocate message stack!";
    varinit();
    registercommands();
    scriptSetList(command_list);
    CreateThread(0, 0, DbgCommandLoopThread, 0, 0, 0);
    //CreateThread(0, 0, ConsoleReadLoopThread, 0, 0, 0);
    return 0;
}
