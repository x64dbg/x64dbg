#include "debugger.h"
#include "argument.h"
#include "variable.h"
#include "console.h"
#include "threading.h"
#include "value.h"
#include "instruction.h"
#include "memory.h"
#include "_exports.h"
#include "addrinfo.h"

static PROCESS_INFORMATION g_pi= {0,0,0,0};
PROCESS_INFORMATION* fdProcessInfo=&g_pi;
static char szFileName[deflen]="";
bool bFileIsDll=false;
uint pDebuggedDllBase=0;
BREAKPOINT* bplist=0;
static bool isStepping=false;
static bool isPausedByUser=false;
static bool bScyllaLoaded=false;

//static functions
static void cbStep();
static void cbSystemBreakpoint(void* ExceptionData);
static void cbEntryBreakpoint();
static void cbUserBreakpoint();

void dbgdisablebpx()
{
    BREAKPOINT* cur=bplist;
    if(!cur or !cur->addr)
        return;
    bool bNext=true;
    while(bNext)
    {
        if(cur->type==BPNORMAL or cur->type==BPSINGLESHOOT)
            if(IsBPXEnabled(cur->addr) and DeleteBPX(cur->addr))
                cur->enabled=false;
        cur=cur->next;
        if(!cur)
            bNext=false;
    }
}

void dbgenablebpx()
{
    BREAKPOINT* cur=bplist;
    if(!cur or !cur->addr)
        return;
    bool bNext=true;
    while(bNext)
    {
        if(cur->type==BPNORMAL or cur->type==BPSINGLESHOOT)
            if(!IsBPXEnabled(cur->addr) and SetBPX(cur->addr, cur->type, (void*)cbUserBreakpoint))
                cur->enabled=true;
        cur=cur->next;
        if(!cur)
            bNext=false;
    }
}

bool dbgisrunning()
{
    if(!waitislocked(WAITID_RUN))
        return true;
    return false;
}

void DebugUpdateGui(uint disasm_addr)
{
    GuiUpdateRegisterView();
    GuiDisasmAt(disasm_addr, (duint)GetContextData(UE_CIP));
}

static void cbUserBreakpoint()
{
    BREAKPOINT* cur=bpfind(bplist, 0, (uint)GetContextData(UE_CIP), 0, BPNORMAL);
    if(!cur)
        dputs("breakpoint reached not in list!");
    else
    {
        const char* apiname=(const char*)ImporterGetAPINameFromDebugee(fdProcessInfo->hProcess, cur->addr);
        char log[deflen]="";
        if(apiname)
        {
            const char* dllname_=(const char*)ImporterGetDLLNameFromDebugee(fdProcessInfo->hProcess, cur->addr);
            char dllname[256]="";
            strcpy(dllname, dllname_);
            _strlwr(dllname);
            int len=strlen(dllname);
            for(int i=len-1; i!=0; i--)
                if(dllname[i]=='.')
                {
                    dllname[i]=0;
                    break;
                }
            if(cur->name)
                sprintf(log, "breakpoint \"%s\" at %s.%s ("fhex")!", cur->name, dllname, apiname, cur->addr);
            else
                sprintf(log, "breakpoint at %s.%s ("fhex")!", dllname, apiname, cur->addr);
        }
        else
        {
            if(cur->name)
                sprintf(log, "breakpoint \"%s\" at "fhex"!", cur->name, cur->addr);
            else
                sprintf(log, "breakpoint at "fhex"!", cur->addr);
        }
        dputs(log);
        if(cur->type==BPSINGLESHOOT)
            bpdel(bplist, 0, cur->addr, BPNORMAL);
    }
    DebugUpdateGui(GetContextData(UE_CIP));
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    wait(WAITID_RUN);
}

static void cbHardwareBreakpoint(void* ExceptionAddress)
{
    uint cip=GetContextData(UE_CIP);
    BREAKPOINT* cur=bpfind(bplist, 0, (uint)ExceptionAddress, 0, BPHARDWARE);
    if(!cur)
        dputs("hardware breakpoint reached not in list!");
    else
    {
        //TODO: type
        char log[50]="";
        if(cur->name)
            sprintf(log, "hardware breakpoint \"%s\" "fhex"!", cur->name, cur->addr);
        else
            sprintf(log, "hardware breakpoint "fhex"!", cur->addr);
        dputs(log);
    }
    DebugUpdateGui(cip);
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    wait(WAITID_RUN);
}

static void cbMemoryBreakpoint(void* ExceptionAddress)
{
    uint cip=GetContextData(UE_CIP);
    uint size;
    uint base=memfindbaseaddr(fdProcessInfo->hProcess, (uint)ExceptionAddress, &size);
    BREAKPOINT* cur=bpfind(bplist, 0, base, 0, BPMEMORY);
    if(!cur)
        dputs("memory breakpoint reached not in list!");
    else
    {
        //unsigned char type=cur->oldbytes&0xF;
        char log[50]="";
        if(cur->name)
            sprintf(log, "memory breakpoint \"%s\" on "fhex"!", cur->name, cur->addr);
        else
            sprintf(log, "memory breakpoint on "fhex"!", cur->addr);
        dputs(log);
    }
    if(!(cur->oldbytes>>4)) //is auto-restoring?
        bpdel(bplist, 0, base, BPMEMORY); //delete from breakpoint list
    DebugUpdateGui(cip);
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    wait(WAITID_RUN);
}

static void cbEntryBreakpoint()
{
    pDebuggedDllBase=GetDebuggedDLLBaseAddress();
    dputs("entry point reached!");
    DebugUpdateGui(GetContextData(UE_CIP));
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    wait(WAITID_RUN);
}

static int ecount=0;

static void cbException(void* ExceptionData)
{
    EXCEPTION_DEBUG_INFO* edi=(EXCEPTION_DEBUG_INFO*)ExceptionData;
    uint addr=(uint)edi->ExceptionRecord.ExceptionAddress;
    if(edi->ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT)
    {
        if(isPausedByUser)
        {
            dputs("paused!");
            SetNextDbgContinueStatus(DBG_CONTINUE);
            DebugUpdateGui(GetContextData(UE_CIP));
            GuiSetDebugState(paused);
            //lock
            lock(WAITID_RUN);
            wait(WAITID_RUN);
            return;
        }
        SetContextData(UE_CIP, (uint)edi->ExceptionRecord.ExceptionAddress);
    }

    char msg[1024]="";
    if(edi->dwFirstChance) //first chance exception
    {
        sprintf(msg, "first chance exception on "fhex" (%.8X)!", addr, edi->ExceptionRecord.ExceptionCode);
        SetNextDbgContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    }
    else //lock the exception
    {
        sprintf(msg, "last chance exception on "fhex" (%.8X)!", addr, edi->ExceptionRecord.ExceptionCode);
        SetNextDbgContinueStatus(DBG_CONTINUE);
    }

    dputs(msg);
    DebugUpdateGui(GetContextData(UE_CIP));
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    wait(WAITID_RUN);
}

static void cbLoadDll(LOAD_DLL_DEBUG_INFO* LoadDll)
{
    void* base=LoadDll->lpBaseOfDll;
    char DLLDebugFileName[deflen]="";
    if(!GetMappedFileNameA(fdProcessInfo->hProcess, base, DLLDebugFileName, deflen))
        strcpy(DLLDebugFileName, "??? (GetMappedFileName failed)");
    else
        DevicePathToPath(DLLDebugFileName, DLLDebugFileName, deflen);
    dprintf("DLL Loaded: "fhex" %s\n", base, DLLDebugFileName);
    SymLoadModuleEx(fdProcessInfo->hProcess, LoadDll->hFile, DLLDebugFileName, 0, (DWORD64)base, 0, 0, 0);
}

static void cbUnloadDll(UNLOAD_DLL_DEBUG_INFO* UnloadDll)
{
    void* base=UnloadDll->lpBaseOfDll;
    char DLLDebugFileName[deflen]="";
    if(!GetMappedFileNameA(fdProcessInfo->hProcess, base, DLLDebugFileName, deflen))
        strcpy(DLLDebugFileName, "??? (GetMappedFileName failed)");
    else
        DevicePathToPath(DLLDebugFileName, DLLDebugFileName, deflen);
    dprintf("DLL Unloaded: "fhex" %s\n", base, DLLDebugFileName);
    SymUnloadModule64(fdProcessInfo->hProcess, (DWORD64)base);
}

static void cbCreateProcess(CREATE_PROCESS_DEBUG_INFO* CreateProcessInfo)
{
    void* base=CreateProcessInfo->lpBaseOfImage;
    char DebugFileName[deflen]="";
    if(!GetMappedFileNameA(fdProcessInfo->hProcess, base, DebugFileName, deflen))
        strcpy(DebugFileName, "??? (GetMappedFileName failed)");
    else
        DevicePathToPath(DebugFileName, DebugFileName, deflen);
    dprintf("Process Started: "fhex" %s\n", base, DebugFileName);

    SymInitialize(fdProcessInfo->hProcess, 0, false); //initialize symbols
    SymLoadModuleEx(fdProcessInfo->hProcess, CreateProcessInfo->hFile, DebugFileName, 0, (DWORD64)base, 0, 0, 0);
}

static void cbSystemBreakpoint(void* ExceptionData)
{
    //TODO: handle stuff (TLS, main entry, etc)
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, 0);
    dputs("system breakpoint reached!");
    //NOTE: call GUI
    DebugUpdateGui(GetContextData(UE_CIP));
    GuiSetDebugState(paused);
    //unlock
    unlock(WAITID_SYSBREAK);
    //lock
    lock(WAITID_RUN);
    wait(WAITID_RUN);

    /*//my code

     //list memorymap(cbListPage)
     ReadMemory(va)
     setBP(va, type, callback)

     //gui
     GuiChangeCIP(va, base, size)


     //gui
     cbClearMap
     cbAddPage(MEMORY_BASIC_INFO, modulename)
     cbEndMap

     //dbg
     MemoryMap(cbClear, cbAddPage, cbEndMap)
     */
}

static void cbStep()
{
    isStepping=false;
    DebugUpdateGui(GetContextData(UE_CIP));
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    wait(WAITID_RUN);
}

static void cbRtrFinalStep()
{
    DebugUpdateGui(GetContextData(UE_CIP));
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    wait(WAITID_RUN);
}

static unsigned char getCIPch()
{
    char ch;
    dbgdisablebpx();
    memread(fdProcessInfo->hProcess, (void*)GetContextData(UE_CIP), &ch, 1, 0);
    dbgenablebpx();
    return ch;
}

static void cbRtrStep()
{
    unsigned int cipch=getCIPch();
    if(cipch==0xC3 or cipch==0xC2)
        cbRtrFinalStep();
    else
        StepOver((void*)cbRtrStep);
}

static DWORD WINAPI threadDebugLoop(void* lpParameter)
{
    //initialize
    INIT_STRUCT* init=(INIT_STRUCT*)lpParameter;
    bFileIsDll=IsFileDLL(init->exe, 0);
    if(bFileIsDll)
        fdProcessInfo=(PROCESS_INFORMATION*)InitDLLDebug(init->exe, false, init->commandline, init->currentfolder, (void*)cbEntryBreakpoint);
    else
        fdProcessInfo=(PROCESS_INFORMATION*)InitDebugEx(init->exe, init->commandline, init->currentfolder, (void*)cbEntryBreakpoint);
    if(!fdProcessInfo)
    {
        fdProcessInfo=&g_pi;
        dputs("error starting process (invalid pe?)!");
        unlock(WAITID_SYSBREAK);
        return 0;
    }
    strcpy(szFileName, init->exe);
    efree(init); //free init struct
    varset("$hp", (uint)fdProcessInfo->hProcess, true);
    varset("$pid", fdProcessInfo->dwProcessId, true);
    ecount=0;
    bplist=bpinit(bplist);
    //NOTE: set custom handlers
    SetCustomHandler(UE_CH_CREATEPROCESS, (void*)cbCreateProcess);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (void*)cbSystemBreakpoint);
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (void*)cbException);
    SetCustomHandler(UE_CH_LOADDLL, (void*)cbLoadDll);
    SetCustomHandler(UE_CH_UNLOADDLL, (void*)cbUnloadDll);
    //inform GUI start we started without problems
    GuiSetDebugState(initialized);
    //run debug loop (returns when process debugging is stopped)
    DebugLoop();
    DeleteFileA("DLLLoader.exe");
    //message the user/do final stuff
    GuiSetDebugState(stopped);
    dputs("debugging stopped!");
    varset("$hp", 0, true);
    varset("$pid", 0, true);
    waitclear();
    return 0;
}

CMDRESULT cbDebugInit(const char* cmd)
{
    if(IsFileBeingDebugged())
    {
        dputs("already debugging!");
        return STATUS_ERROR;
    }

    char arg1[deflen]="";
    if(!argget(cmd, arg1, 0, false))
        return STATUS_ERROR;
    if(!FileExists(arg1))
    {
        dputs("file does not exsist!");
        return STATUS_ERROR;
    }

    char arg2[deflen]="";
    argget(cmd, arg2, 1, true);
    char* commandline=0;
    if(strlen(arg2))
        commandline=arg2;

    char arg3[deflen]="";
    argget(cmd, arg3, 2, true);

    char currentfolder[deflen]="";
    strcpy(currentfolder, arg1);
    int len=strlen(currentfolder);
    while(currentfolder[len]!='\\' and len!=0)
        len--;
    currentfolder[len]=0;
    if(DirExists(arg3))
        strcpy(currentfolder, arg3);
    INIT_STRUCT* init=(INIT_STRUCT*)emalloc(sizeof(INIT_STRUCT));
    memset(init, 0, sizeof(INIT_STRUCT));
    init->exe=arg1;
    init->commandline=commandline;
    if(*currentfolder)
        init->currentfolder=currentfolder;

    //initialize
    waitclear(); //clear waiting flags
    lock(WAITID_SYSBREAK);
    if(!CreateThread(0, 0, threadDebugLoop, init, 0, 0))
    {
        dputs("failed creating debug thread!");
        return STATUS_ERROR;
    }
    wait(WAITID_SYSBREAK);
    return STATUS_CONTINUE;
}

CMDRESULT cbStopDebug(const char* cmd)
{
    StopDebug();
    unlock(WAITID_RUN);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugRun(const char* cmd)
{
    if(!waitislocked(WAITID_RUN))
    {
        dputs("program is already running");
        return STATUS_ERROR;
    }
    GuiSetDebugState(running);
    unlock(WAITID_RUN);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetBPXOptions(const char* cmd)
{
    char argtype[deflen]="";
    uint type=0;
    if(!argget(cmd, argtype, 0, false))
        return STATUS_ERROR;
    const char* a=0;
    if(strstr(argtype, "long"))
    {
        a="TYPE_LONG_INT3";
        type=UE_BREAKPOINT_TYPE_LONG_INT3;
    }
    else if(strstr(argtype, "ud2"))
    {
        a="TYPE_UD2";
        type=UE_BREAKPOINT_TYPE_UD2;
    }
    else if(strstr(argtype, "short"))
    {
        a="TYPE_INT3";
        type=UE_BREAKPOINT_TYPE_INT3;
    }
    else
    {
        dputs("invalid type specified!");
        return STATUS_ERROR;
    }
    SetBPXOptions(type);
    dprintf("default breakpoint type set to: %s\n", a);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetBPX(const char* cmd) //bp addr [,name [,type]]
{
    char argaddr[deflen]="";
    if(!argget(cmd, argaddr, 0, true))
        if(!_strnicmp(cmd, "bp", 2))
            return cbBadCmd(cmd);
    if(!argget(cmd, argaddr, 0, false))
        return STATUS_ERROR;
    char argname[deflen]="";
    argget(cmd, argname, 1, true);
    char argtype[deflen]="";
    bool has_arg2=argget(cmd, argtype, 2, true);
    if(!has_arg2 and (scmp(argname, "ss") or scmp(argname, "long") or scmp(argname, "ud2")))
    {
        strcpy(argtype, argname);
        *argname=0;
    }
    _strlwr(argtype);
    uint addr=0;
    if(!valfromstring(argaddr, &addr, 0, 0, false, 0))
    {
        dprintf("invalid addr: \"%s\"\n", argaddr);
        return STATUS_ERROR;
    }
    if(addr==(uint)(GetPE32Data(szFileName, 0, UE_OEP)+GetPE32Data(szFileName, 0, UE_IMAGEBASE)))
    {
        dputs("entry breakpoint will be set automatically");
        return STATUS_ERROR;
    }
    uint type=0;
    BP_TYPE list_type;
    if(strstr(argtype, "ss"))
    {
        type|=UE_SINGLESHOOT;
        list_type=BPSINGLESHOOT;
    }
    else
    {
        type|=UE_BREAKPOINT;
        list_type=BPNORMAL;
    }
    if(strstr(argtype, "long"))
        type|=UE_BREAKPOINT_TYPE_LONG_INT3;
    else if(strstr(argtype, "ud2"))
        type|=UE_BREAKPOINT_TYPE_UD2;
    else if(strstr(argtype, "short"))
        type|=UE_BREAKPOINT_TYPE_INT3;
    short oldbytes;
    BREAKPOINT* found=bpfind(bplist, 0, addr, 0, BPNORMAL);
    if(IsBPXEnabled(addr) or !memread(fdProcessInfo->hProcess, (void*)addr, &oldbytes, sizeof(short), 0) or found or !SetBPX(addr, type, (void*)cbUserBreakpoint))
    {
        dprintf("error setting breakpoint at "fhex"!\n", addr);
        return STATUS_ERROR;
    }
    if(bpnew(bplist, argname, addr, oldbytes, list_type))
        dprintf("breakpoint at "fhex" set!\n", addr);
    else
        dputs("problem setting breakpoint!");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugEnableBPX(const char* cmd)
{
    char arg1[deflen]="";
    if(!argget(cmd, arg1, 0, true)) //enable all breakpoints
    {
        BREAKPOINT* cur=bplist;
        if(!cur or !cur->addr)
        {
            dputs("no breakpoints!");
            return STATUS_ERROR;
        }
        bool bNext=true;
        CMDRESULT res=STATUS_CONTINUE;
        while(bNext)
        {
            if(!SetBPX(cur->addr, cur->type, (void*)cbUserBreakpoint))
            {
                dprintf("could not enable %.8X\n", cur->addr);
                res=STATUS_ERROR;
            }
            else
                cur->enabled=true;
            cur=cur->next;
            if(!cur)
                bNext=false;
        }
        dputs("all breakpoints enabled!");
        return res;
    }
    BREAKPOINT* bp=bpfind(bplist, arg1, 0, 0, BPNORMAL);
    if(!bp)
    {
        uint addr=0;
        if(!valfromstring(arg1, &addr, 0, 0, false, 0))
        {
            dprintf("invalid addr: \"%s\"\n", arg1);
            return STATUS_ERROR;
        }
        bp=bpfind(bplist, 0, addr, 0, BPNORMAL);
        if(!bp)
        {
            dprintf("no such breakpoint: \"%s\"\n", arg1);
            return STATUS_ERROR;
        }
    }
    if(bp->type!=BPNORMAL and bp->type!=BPSINGLESHOOT)
    {
        dputs("this breakpoint type cannot be enabled");
        return STATUS_ERROR;
    }
    if(bp->enabled)
    {
        dputs("breakpoint already enabled!");
        return STATUS_ERROR;
    }
    if(!SetBPX(bp->addr, bp->type, (void*)cbUserBreakpoint))
        dputs("could not enable breakpoint");
    else
        bp->enabled=true;
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDisableBPX(const char* cmd)
{
    char arg1[deflen]="";
    if(!argget(cmd, arg1, 0, true)) //disable all breakpoints
    {
        BREAKPOINT* cur=bplist;
        if(!cur or !cur->addr)
        {
            dputs("no breakpoints!");
            return STATUS_ERROR;
        }
        bool bNext=true;
        CMDRESULT res=STATUS_CONTINUE;
        while(bNext)
        {
            if(!DeleteBPX(cur->addr))
            {
                dprintf("could not disable %.8X\n", cur->addr);
                res=STATUS_ERROR;
            }
            else
                cur->enabled=false;
            cur=cur->next;
            if(!cur)
                bNext=false;
        }
        dputs("all breakpoints disabled!");
        return res;
    }
    BREAKPOINT* bp=bpfind(bplist, arg1, 0, 0, BPNORMAL);
    if(!bp)
    {
        uint addr=0;
        if(!valfromstring(arg1, &addr, 0, 0, false, 0))
        {
            dprintf("invalid addr: \"%s\"\n", arg1);
            return STATUS_ERROR;
        }
        bp=bpfind(bplist, 0, addr, 0, BPNORMAL);
        if(!bp)
        {
            dprintf("no such breakpoint: \"%s\"\n", arg1);
            return STATUS_ERROR;
        }
    }
    if(bp->type!=BPNORMAL and bp->type!=BPSINGLESHOOT)
    {
        dputs("this breakpoint type cannot be disabled");
        return STATUS_ERROR;
    }
    if(!bp->enabled)
    {
        dputs("breakpoint already disabled!");
        return STATUS_ERROR;
    }
    if(!DeleteBPX(bp->addr))
        dputs("could not disable breakpoint");
    else
        bp->enabled=false;
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugToggleBPX(const char* cmd)
{
    char arg1[deflen]="";
    if(!argget(cmd, arg1, 0, false))
        return STATUS_ERROR;
    BREAKPOINT* bp=bpfind(bplist, arg1, 0, 0, BPNORMAL);
    if(!bp)
    {
        uint addr=0;
        if(!valfromstring(arg1, &addr, 0, 0, false, 0))
        {
            dprintf("invalid addr: \"%s\"\n", arg1);
            return STATUS_ERROR;
        }
        bp=bpfind(bplist, 0, addr, 0, BPNORMAL);
        if(!bp)
        {
            dprintf("no such breakpoint: \"%s\"\n", arg1);
            return STATUS_ERROR;
        }
    }
    if(bp->type!=BPNORMAL and bp->type!=BPSINGLESHOOT)
    {
        dputs("this breakpoint type cannot be toggled");
        return STATUS_ERROR;
    }
    bool disable=bp->enabled;
    if(disable)
    {
        if(!DeleteBPX(bp->addr))
        {
            dputs("could not disable breakpoint");
            return STATUS_ERROR;
        }
        else
        {
            bp->enabled=false;
            dputs("breakpoint disabled!");
        }
    }
    else
    {
        if(!SetBPX(bp->addr, bp->type, (void*)cbUserBreakpoint))
        {
            dputs("could not disable breakpoint");
            return STATUS_ERROR;
        }
        else
        {
            bp->enabled=true;
            dputs("breakpoint enabled!");
        }
    }
    varset("$res", (uint)disable, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDeleteBPX(const char* cmd)
{
    char arg1[deflen]="";
    if(!argget(cmd, arg1, 0, true)) //delete all breakpoints
    {
        BREAKPOINT* cur=bplist;
        if(!cur or !cur->addr)
        {
            dputs("no breakpoints!");
            return STATUS_ERROR;
        }
        bool bNext=true;
        while(bNext)
        {
            BREAKPOINT* next=cur->next;
            if(cur->type==BPNORMAL or cur->type==BPSINGLESHOOT)
            {
                DeleteBPX(cur->addr);
                bpdel(bplist, 0, cur->addr, BPNORMAL);
            }
            cur=next;
            if(!cur)
                bNext=false;
        }
        //memset(bplist, 0, sizeof(BREAKPOINT));
        dputs("all breakpoints deleted!");
        return STATUS_CONTINUE;
    }
    BREAKPOINT* bp=bpfind(bplist, arg1, 0, 0, BPNORMAL);
    if(!bp)
    {
        uint addr=0;
        if(!valfromstring(arg1, &addr, 0, 0, false, 0))
        {
            dprintf("invalid addr: \"%s\"\n", arg1);
            return STATUS_ERROR;
        }
        bp=bpfind(bplist, 0, addr, 0, BPNORMAL);
        if(!bp)
        {
            dprintf("no such breakpoint: \"%s\"\n", arg1);
            return STATUS_ERROR;
        }
    }
    if(!DeleteBPX(bp->addr))
    {
        dprintf("delete breakpoint failed: "fhex"\n", bp->addr);
        return STATUS_ERROR;
    }
    bpdel(bplist, 0, bp->addr, BPNORMAL);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugBplist(const char* cmd)
{
    BREAKPOINT* cur=bplist;
    if(!cur or !cur->addr)
    {
        dputs("no breakpoints!");
        return STATUS_CONTINUE;
    }
    bool bNext=true;
    while(bNext)
    {
        const char* type=0;
        if(cur->type==BPNORMAL)
            type="BP";
        if(cur->type==BPSINGLESHOOT)
            type="SS";
        if(cur->type==BPHARDWARE)
            type="HW";
        if(cur->type==BPMEMORY)
            type="GP";
        bool enabled=cur->enabled;
        if(cur->name)
            dprintf("%d:%s:"fhex":\"%s\"\n", enabled, type, cur->addr, cur->name);
        else
            dprintf("%d:%s:"fhex"\n", enabled, type, cur->addr);
        cur=cur->next;
        if(!cur)
            bNext=false;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugStepInto(const char* cmd)
{
    StepInto((void*)cbStep);
    isStepping=true;
    return cbDebugRun(cmd);
}

CMDRESULT cbDebugStepOver(const char* cmd)
{
    StepOver((void*)cbStep);
    isStepping=true;
    return cbDebugRun(cmd);
}

CMDRESULT cbDebugSingleStep(const char* cmd)
{
    char arg1[deflen]="";
    uint stepcount=1;
    if(argget(cmd, arg1, 0, true))
    {
        if(!valfromstring(arg1, &stepcount, 0, 0, true, 0))
            stepcount=1;
    }

    SingleStep((DWORD)stepcount, (void*)cbStep);
    isStepping=true;
    return cbDebugRun(cmd);
}

CMDRESULT cbDebugHide(const char* cmd)
{
    if(HideDebugger(fdProcessInfo->hProcess, UE_HIDE_BASIC))
        dputs("debugger hidden");
    else
        dputs("something went wrong");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDisasm(const char* cmd)
{
    char arg1[deflen]="";
    uint addr=GetContextData(UE_CIP);
    if(argget(cmd, arg1, 0, true))
        if(!valfromstring(arg1, &addr, 0, 0, true, 0))
            addr=GetContextData(UE_CIP);
    DebugUpdateGui(addr);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugMemoryBpx(const char* cmd)
{
    char arg1[deflen]=""; //addr
    if(!argget(cmd, arg1, 0, false))
        return STATUS_ERROR;
    uint addr;
    if(!valfromstring(arg1, &addr, 0, 0, false, 0))
        return STATUS_ERROR;
    bool restore=false;
    char arg2[deflen]=""; //restore
    char arg3[deflen]=""; //type
    argget(cmd, arg3, 2, true);
    if(argget(cmd, arg2, 1, true))
    {
        if(*arg2=='1')
            restore=true;
        else if(*arg2=='0')
            restore=false;
        else
            strcpy(arg3, arg2);
    }
    uint type=UE_MEMORY;
    if(*arg3)
    {
        switch(*arg3)
        {
        case 'r':
            type=UE_MEMORY_READ;
            break;
        case 'w':
            type=UE_MEMORY_WRITE;
            break;
        case 'x':
            type=UE_MEMORY_EXECUTE; //EXECUTE
            break;
        default:
            dputs("invalid type (argument ignored)");
            break;
        }
    }
    uint size=0;
    uint base=memfindbaseaddr(fdProcessInfo->hProcess, addr, &size);
    BREAKPOINT* found=bpfind(bplist, 0, base, 0, BPMEMORY);
    if(found or !SetMemoryBPXEx(base, size, type, restore, (void*)cbMemoryBreakpoint))
    {
        dputs("error setting memory breakpoint!");
        return STATUS_ERROR;
    }
    if(bpnew(bplist, 0, addr, (restore<<4)|type, BPMEMORY))
        dprintf("memory breakpoint at "fhex" set!\n", addr);
    else
        dputs("problem setting breakpoint (report please)!");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugRtr(const char* cmd)
{
    StepOver((void*)cbRtrStep);
    cbDebugRun(cmd);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetHardwareBreakpoint(const char* cmd)
{
    char arg1[deflen]=""; //addr
    if(!argget(cmd, arg1, 0, false))
        return STATUS_ERROR;
    uint addr;
    if(!valfromstring(arg1, &addr, 0, 0, false, 0))
        return STATUS_ERROR;
    uint type=UE_HARDWARE_EXECUTE;
    char arg2[deflen]=""; //type
    if(argget(cmd, arg2, 1, true))
    {
        switch(*arg2)
        {
        case 'r':
            type=UE_HARDWARE_READWRITE;
            break;
        case 'w':
            type=UE_HARDWARE_WRITE;
            break;
        case 'x':
            break;
        default:
            dputs("invlalid type, assuming 'x'");
            break;
        }
    }
    char arg3[deflen]=""; //size
    uint size=UE_HARDWARE_SIZE_1;
    if(argget(cmd, arg3, 2, true))
    {
        if(!valfromstring(arg3, &size, 0, 0, false, 0))
            return STATUS_ERROR;
        switch(size)
        {
        case 2:
            size=UE_HARDWARE_SIZE_2;
            break;
        case 4:
            size=UE_HARDWARE_SIZE_4;
            break;
#ifdef _WIN64
        case 8:
            size=UE_HARDWARE_SIZE_8;
            break;
#endif // _WIN64
        default:
            dputs("invalid size, using 1");
            break;
        }
        if(addr%size)
        {
            dprintf("address not aligned to %d\n", size);
            return STATUS_ERROR;
        }
    }
    DWORD drx=0;
    if(!GetUnusedHardwareBreakPointRegister(&drx))
    {
        dputs("no free debug register");
        return STATUS_ERROR;
    }
    BREAKPOINT* found=bpfind(bplist, 0, addr, 0, BPHARDWARE);
    if(found or !SetHardwareBreakPoint(addr, drx, type, size, (void*)cbHardwareBreakpoint))
    {
        dputs("error setting hardware breakpoint!");
        return STATUS_ERROR;
    }
    if(bpnew(bplist, 0, addr, (drx<<8)|(type<<4)|size, BPHARDWARE))
        dprintf("hardware breakpoint at "fhex" set!\n", addr);
    else
        dputs("problem setting breakpoint (report please)!");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugAlloc(const char* cmd)
{
    char arg1[deflen]=""; //size
    uint size=0x1000;
    if(argget(cmd, arg1, 0, true))
        if(!valfromstring(arg1, &size, 0, 0, false, 0))
            return STATUS_ERROR;
    uint mem=(uint)memalloc(fdProcessInfo->hProcess, 0, size, PAGE_EXECUTE_READWRITE);
    if(!mem)
        dputs("VirtualAllocEx failed");
    else
        dprintf(fhex"\n", mem);
    if(mem)
        varset("$lastalloc", mem, true);
    varset("$res", mem, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugFree(const char* cmd)
{
    uint lastalloc;
    varget("$lastalloc", &lastalloc, 0, 0);
    char arg1[deflen]=""; //addr
    uint addr=lastalloc;
    if(argget(cmd, arg1, 0, true))
    {
        if(!valfromstring(arg1, &addr, 0, 0, false, 0))
            return STATUS_ERROR;
    }
    else if(!lastalloc)
        dputs("lastalloc is zero, provide a page address");
    if(addr==lastalloc)
        varset("$lastalloc", 0, true);
    bool ok=VirtualFreeEx(fdProcessInfo->hProcess, (void*)addr, 0, MEM_RELEASE);
    if(!ok)
        dputs("VirtualFreeEx failed");
    varset("$res", ok, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugMemset(const char* cmd)
{
    char arg1[deflen]=""; //addr
    char arg2[deflen]=""; //value
    char arg3[deflen]=""; //size
    uint addr;
    uint value;
    uint size;
    if(!argget(cmd, arg1, 0, false) or !argget(cmd, arg2, 1, false))
        return STATUS_ERROR;
    if(!valfromstring(arg1, &addr, 0, 0, false, 0) or !valfromstring(arg2, &value, 0, 0, false, 0))
        return STATUS_ERROR;
    if(argget(cmd, arg3, 2, true))
    {
        if(!valfromstring(arg3, &size, 0, 0, false, 0))
            return STATUS_ERROR;
    }
    else
    {
        uint base=memfindbaseaddr(fdProcessInfo->hProcess, addr, &size);
        if(!base)
        {
            dputs("invalid address specified");
            return STATUS_ERROR;
        }
        uint diff=addr-base;
        addr=base+diff;
        size-=diff;
    }
    BYTE fi=value&0xFF;
    if(!Fill((void*)addr, size&0xFFFFFFFF, &fi))
        dputs("memset failed");
    else
        dprintf("memory "fhex" (size: %.8X) set to %.2X\n", addr, size&0xFFFFFFFF, value&0xFF);
    return STATUS_CONTINUE;
}

CMDRESULT cbBenchmark(const char* cmd)
{
    char arg1[deflen]="";
    if(!argget(cmd, arg1, 0, false))
        return STATUS_ERROR;
    uint addr=0;
    if(!valfromstring(arg1, &addr, 0, 0, false, 0))
        return STATUS_ERROR;
    uint ticks=GetTickCount();
    for(int i=0; i<1000; i++)
    {
        unsigned char dest[0x1000];
        _dbg_memread(addr, dest, 0x1000, 0);
    }
    dprintf("%ums\n", GetTickCount()-ticks);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugPause(const char* cmd)
{
    if(waitislocked(WAITID_RUN))
    {
        dputs("program is not running");
        return STATUS_ERROR;
    }
    isPausedByUser=true;
    DebugBreakProcess(fdProcessInfo->hProcess);
    return STATUS_CONTINUE;
}

CMDRESULT cbMemWrite(const char* cmd)
{
    char arg1[deflen]="";
    if(!argget(cmd, arg1, 0, false))
        return STATUS_ERROR;
    uint addr=0;
    if(!valfromstring(arg1, &addr, 0, 0, false, 0))
        return STATUS_ERROR;
    unsigned char* blub=(unsigned char*)emalloc(0x2123);
    memread(fdProcessInfo->hProcess, (const void*)addr, blub, 0x2123, 0);
    //memwrite(fdProcessInfo->hProcess, (void*)addr, blub, 0x2123, 0);
    return STATUS_CONTINUE;
}

DWORD WINAPI scyllaThread(void* lpParam)
{
    typedef INT (WINAPI * SCYLLASTARTGUI)(DWORD pid, HINSTANCE mod);
    SCYLLASTARTGUI ScyllaStartGui=0;
    HINSTANCE hScylla=LoadLibraryA("Scylla.dll");
    if(!hScylla)
    {
        dputs("error loading Scylla.dll!");
        bScyllaLoaded=false;
        return 0;
    }
    ScyllaStartGui=(SCYLLASTARTGUI)GetProcAddress(hScylla, "ScyllaStartGui");
    if(!ScyllaStartGui)
    {
        dputs("could not find export 'ScyllaStartGui' inside Scylla.dll");
        bScyllaLoaded=false;
        return 0;
    }
    if(bFileIsDll)
        ScyllaStartGui(fdProcessInfo->dwProcessId, (HINSTANCE)pDebuggedDllBase);
    else
        ScyllaStartGui(fdProcessInfo->dwProcessId, 0);
    FreeLibrary(hScylla);
    bScyllaLoaded=false;
    return 0;
}

CMDRESULT cbStartScylla(const char* cmd)
{
    if(bScyllaLoaded)
    {
        dputs("Scylla is already loaded");
        return STATUS_ERROR;
    }
    bScyllaLoaded=true;
    CreateThread(0, 0, scyllaThread, 0, 0, 0);
    return STATUS_CONTINUE;
}
