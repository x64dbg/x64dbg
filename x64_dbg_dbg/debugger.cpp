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
#include "plugin_loader.h"

static PROCESS_INFORMATION g_pi= {0,0,0,0};
PROCESS_INFORMATION* fdProcessInfo=&g_pi;
static char szFileName[deflen]="";
bool bFileIsDll=false;
uint pDebuggedDllBase=0;
static bool isStepping=false;
static bool isPausedByUser=false;
static bool bScyllaLoaded=false;
static int ecount=0;

//Superglobal variables
char sqlitedb[deflen]="";

//static functions
static void cbStep();
static void cbSystemBreakpoint(void* ExceptionData);
static void cbEntryBreakpoint();
static void cbUserBreakpoint();

void dbgdisablebpx()
{
    BREAKPOINT* list;
    int bpcount=bpgetlist(&list);
    for(int i=0; i<bpcount; i++)
    {
        if(list[i].type==BPNORMAL and IsBPXEnabled(list[i].addr))
            DeleteBPX(list[i].addr);
    }
}

void dbgenablebpx()
{
    BREAKPOINT* list;
    int bpcount=bpgetlist(&list);
    for(int i=0; i<bpcount; i++)
    {
        if(list[i].type==BPNORMAL and !IsBPXEnabled(list[i].addr) and list[i].enabled)
            SetBPX(list[i].addr, list[i].titantype, (void*)cbUserBreakpoint);
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
    GuiUpdateAllViews();
    GuiDisasmAt(disasm_addr, (duint)GetContextData(UE_CIP));
}

static void cbUserBreakpoint()
{
    BREAKPOINT bp;
    if(!bpget(GetContextData(UE_CIP), BPNORMAL, 0, &bp) and bp.enabled)
        dputs("breakpoint reached not in list!");
    else
    {
        const char* bptype="INT3";
        int titantype=bp.titantype;
        if((titantype&UE_BREAKPOINT_TYPE_UD2)==UE_BREAKPOINT_TYPE_UD2)
            bptype="UD2";
        else if((titantype&UE_BREAKPOINT_TYPE_LONG_INT3)==UE_BREAKPOINT_TYPE_LONG_INT3)
            bptype="LONG INT3";
        const char* apiname=(const char*)ImporterGetAPINameFromDebugee(fdProcessInfo->hProcess, bp.addr);
        char log[deflen]="";
        if(apiname)
        {
            const char* dllname_=(const char*)ImporterGetDLLNameFromDebugee(fdProcessInfo->hProcess, bp.addr);
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
            if(*bp.name)
                sprintf(log, "%s breakpoint \"%s\" at %s.%s ("fhex")!", bptype, bp.name, dllname, apiname, bp.addr);
            else
                sprintf(log, "%s breakpoint at %s.%s ("fhex")!", bptype, dllname, apiname, bp.addr);
        }
        else
        {
            if(*bp.name)
                sprintf(log, "%s breakpoint \"%s\" at "fhex"!", bptype, bp.name, bp.addr);
            else
                sprintf(log, "%s breakpoint at "fhex"!", bptype, bp.addr);
        }
        dputs(log);
        if(bp.singleshoot)
            bpdel(bp.addr, BPNORMAL);
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
    BREAKPOINT found;
    if(!bpget((uint)ExceptionAddress, BPHARDWARE, 0, &found))
        dputs("hardware breakpoint reached not in list!");
    else
    {
        //TODO: type
        char log[deflen]="";
        if(*found.name)
            sprintf(log, "hardware breakpoint \"%s\" "fhex"!", found.name, found.addr);
        else
            sprintf(log, "hardware breakpoint "fhex"!", found.addr);
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
    BREAKPOINT found;
    if(!bpget(base, BPMEMORY, 0, &found))
        dputs("memory breakpoint reached not in list!");
    else
    {
        //unsigned char type=cur->oldbytes&0xF;
        char log[50]="";
        if(*found.name)
            sprintf(log, "memory breakpoint \"%s\" on "fhex"!", found.name, found.addr);
        else
            sprintf(log, "memory breakpoint on "fhex"!", found.addr);
        dputs(log);
    }
    if(found.singleshoot)
        bpdel(found.addr, BPMEMORY); //delete from breakpoint list
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

BOOL
CALLBACK
SymRegisterCallbackProc64(
    __in HANDLE hProcess,
    __in ULONG ActionCode,
    __in_opt ULONG64 CallbackData,
    __in_opt ULONG64 UserContext
)
{
    UNREFERENCED_PARAMETER(hProcess);
    UNREFERENCED_PARAMETER(UserContext);

    PIMAGEHLP_CBA_EVENT evt;

    // If SYMOPT_DEBUG is set, then the symbol handler will pass
    // verbose information on its attempt to load symbols.
    // This information be delivered as text strings.

    switch (ActionCode)
    {
    case CBA_EVENT:
        evt = (PIMAGEHLP_CBA_EVENT)CallbackData;
        printf("%s", (PTSTR)evt->desc);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

static bool cbSetModuleBreakpoints(const BREAKPOINT* bp)
{
    //TODO: more breakpoint types
    switch(bp->type)
    {
    case BPNORMAL:
        if(bp->enabled)
        {
            if(!SetBPX(bp->addr, bp->titantype, (void*)cbUserBreakpoint))
                dprintf("could not set breakpoint "fhex"!\n", bp->addr);
        }
        break;
    case BPMEMORY:
        if(bp->enabled)
        {
            uint size=0;
            memfindbaseaddr(fdProcessInfo->hProcess, bp->addr, &size);
            bool restore=false;
            if(!bp->singleshoot)
                restore=true;
            if(!SetMemoryBPXEx(bp->addr, size, bp->titantype, restore, (void*)cbMemoryBreakpoint))
                dprintf("could not set memory breakpoint "fhex"!\n", bp->addr);
        }
        break;
    case BPHARDWARE:
        if(bp->enabled)
        {
            if(!SetHardwareBreakPoint(bp->addr, (bp->titantype>>8)&0xF, (bp->titantype>>4)&0xF, bp->titantype&0xF, (void*)cbHardwareBreakpoint))
                dprintf("could not set hardware breakpoint "fhex"!\n", bp->addr);
        }
        break;
    default:
        break;
    }
    return true;
}

static bool cbRemoveModuleBreakpoints(const BREAKPOINT* bp)
{
    //TODO: more breakpoint types
    switch(bp->type)
    {
    case BPNORMAL:
        if(IsBPXEnabled(bp->addr))
            DeleteBPX(bp->addr);
        break;
    case BPMEMORY:
        break;
    case BPHARDWARE:
        if(bp->enabled)
            DeleteHardwareBreakPoint((bp->titantype>>8)&0xF);
        break;
    default:
        break;
    }
    return true;
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
    unsigned char ch=0x90;
    uint cip=GetContextData(UE_CIP);
    memread(fdProcessInfo->hProcess, (void*)cip, &ch, 1, 0);
    bpfixmemory(cip, &ch, 1);
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

///custom handlers
static void cbCreateProcess(CREATE_PROCESS_DEBUG_INFO* CreateProcessInfo)
{
    void* base=CreateProcessInfo->lpBaseOfImage;
    char DebugFileName[deflen]="";
    if(!GetMappedFileNameA(fdProcessInfo->hProcess, base, DebugFileName, deflen))
        strcpy(DebugFileName, "??? (GetMappedFileName failed)");
    else
        DevicePathToPath(DebugFileName, DebugFileName, deflen);
    dprintf("Process Started: "fhex" %s\n", base, DebugFileName);

    //init program database
    int len=strlen(szFileName);
    while(szFileName[len]!='\\' && len!=0)
        len--;
    if(len)
        len++;
    strcpy(sqlitedb, szFileName+len);
#ifdef _WIN64
    strcat(sqlitedb, ".dd64");
#else
    strcat(sqlitedb, ".dd32");
#endif // _WIN64
    sprintf(dbpath, "%s\\%s", sqlitedb_basedir, sqlitedb);
    dprintf("Database file: %s\n", dbpath);
    dbinit();

    //SymSetOptions(SYMOPT_DEBUG|SYMOPT_LOAD_LINES);
    SymInitialize(fdProcessInfo->hProcess, 0, false); //initialize symbols
    //SymRegisterCallback64(fdProcessInfo->hProcess, SymRegisterCallbackProc64, 0);
    SymLoadModuleEx(fdProcessInfo->hProcess, CreateProcessInfo->hFile, DebugFileName, 0, (DWORD64)base, 0, 0, 0);
    IMAGEHLP_MODULE64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct=sizeof(IMAGEHLP_MODULE64);
    if(SymGetModuleInfo64(fdProcessInfo->hProcess, (DWORD64)base, &modInfo))
        modload((uint)base, modInfo.ImageSize, modInfo.ImageName);
    bpenumall(0);
    char modname[256]="";
    if(modnamefromaddr((uint)base, modname, true))
        bpenumall(cbSetModuleBreakpoints, modname);

    //call plugin callback
    PLUG_CB_CREATEPROCESS callbackInfo;
    callbackInfo.CreateProcessInfo=CreateProcessInfo;
    callbackInfo.modInfo=&modInfo;
    callbackInfo.DebugFileName=DebugFileName;
    plugincbcall(CB_CREATEPROCESS, &callbackInfo);
}

static void cbExitProcess(EXIT_PROCESS_DEBUG_INFO* ExitProcess)
{
    PLUG_CB_EXITPROCESS callbackInfo;
    callbackInfo.ExitProcess=ExitProcess;
    plugincbcall(CB_EXITPROCESS, &callbackInfo);
}

static void cbCreateThread(CREATE_THREAD_DEBUG_INFO* CreateThread)
{
    PLUG_CB_CREATETHREAD callbackInfo;
    callbackInfo.CreateThread=CreateThread;
    plugincbcall(CB_CREATETHREAD, &callbackInfo);
}

static void cbExitThread(EXIT_THREAD_DEBUG_INFO* ExitThread)
{
    PLUG_CB_EXITTHREAD callbackInfo;
    callbackInfo.ExitThread=ExitThread;
    plugincbcall(CB_EXITTHREAD, &callbackInfo);
}

static void cbSystemBreakpoint(void* ExceptionData)
{
    PLUG_CB_SYSTEMBREAKPOINT callbackInfo;
    callbackInfo.reserved=0;
    plugincbcall(CB_SYSTEMBREAKPOINT, &callbackInfo);
    //TODO: handle stuff (TLS, main entry, etc)
    //log message
    dputs("system breakpoint reached!");
    //update GUI
    DebugUpdateGui(GetContextData(UE_CIP));
    GuiSetDebugState(paused);
    //unlock
    unlock(WAITID_SYSBREAK);
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
    IMAGEHLP_MODULE64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct=sizeof(IMAGEHLP_MODULE64);
    if(SymGetModuleInfo64(fdProcessInfo->hProcess, (DWORD64)base, &modInfo))
        modload((uint)base, modInfo.ImageSize, modInfo.ImageName);
    bpenumall(0);
    char modname[256]="";
    if(modnamefromaddr((uint)base, modname, true))
        bpenumall(cbSetModuleBreakpoints, modname);

    //TODO: plugin callback
    PLUG_CB_LOADDLL callbackInfo;
    callbackInfo.LoadDll=LoadDll;
    callbackInfo.modInfo=&modInfo;
    callbackInfo.modname=modname;
    plugincbcall(CB_LOADDLL, &callbackInfo);
}

static void cbUnloadDll(UNLOAD_DLL_DEBUG_INFO* UnloadDll)
{
    //TODO: plugin callback
    PLUG_CB_UNLOADDLL callbackInfo;
    callbackInfo.UnloadDll=UnloadDll;
    plugincbcall(CB_UNLOADDLL, &callbackInfo);

    void* base=UnloadDll->lpBaseOfDll;
    char modname[256]="???";
    if(modnamefromaddr((uint)base, modname, true))
        bpenumall(cbRemoveModuleBreakpoints, modname);
    SymUnloadModule64(fdProcessInfo->hProcess, (DWORD64)base);
    dprintf("DLL Unloaded: "fhex" %s\n", base, modname);
}

static void cbOutputDebugString(OUTPUT_DEBUG_STRING_INFO* DebugString)
{
    //TODO: handle debug strings
    PLUG_CB_OUTPUTDEBUGSTRING callbackInfo;
    callbackInfo.DebugString=DebugString;
    plugincbcall(CB_OUTPUTDEBUGSTRING, &callbackInfo);
}

static void cbException(EXCEPTION_DEBUG_INFO* ExceptionData)
{
    //TODO: plugin callback
    PLUG_CB_EXCEPTION callbackInfo;
    callbackInfo.Exception=ExceptionData;
    plugincbcall(CB_EXCEPTION, &callbackInfo);

    uint addr=(uint)ExceptionData->ExceptionRecord.ExceptionAddress;
    if(ExceptionData->ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT)
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
        SetContextData(UE_CIP, (uint)ExceptionData->ExceptionRecord.ExceptionAddress);
    }

    char msg[1024]="";
    if(ExceptionData->dwFirstChance) //first chance exception
    {
        sprintf(msg, "first chance exception on "fhex" (%.8X)!", addr, ExceptionData->ExceptionRecord.ExceptionCode);
        SetNextDbgContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
    }
    else //lock the exception
    {
        sprintf(msg, "last chance exception on "fhex" (%.8X)!", addr, ExceptionData->ExceptionRecord.ExceptionCode);
        SetNextDbgContinueStatus(DBG_CONTINUE);
    }

    dputs(msg);
    DebugUpdateGui(GetContextData(UE_CIP));
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    wait(WAITID_RUN);
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
    lock(WAITID_STOP);
    strcpy(szFileName, init->exe);
    BridgeSettingSet("Recent Files", "path", szFileName);
    efree(init, "threadDebugLoop:init"); //free init struct
    varset("$hp", (uint)fdProcessInfo->hProcess, true);
    varset("$pid", fdProcessInfo->dwProcessId, true);
    ecount=0;
    //NOTE: set custom handlers
    SetCustomHandler(UE_CH_CREATEPROCESS, (void*)cbCreateProcess);
    SetCustomHandler(UE_CH_EXITPROCESS, (void*)cbExitProcess);
    SetCustomHandler(UE_CH_CREATETHREAD, (void*)cbCreateThread);
    SetCustomHandler(UE_CH_EXITTHREAD, (void*)cbExitThread);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (void*)cbSystemBreakpoint);
    SetCustomHandler(UE_CH_LOADDLL, (void*)cbLoadDll);
    SetCustomHandler(UE_CH_UNLOADDLL, (void*)cbUnloadDll);
    SetCustomHandler(UE_CH_OUTPUTDEBUGSTRING, (void*)cbOutputDebugString);
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (void*)cbException);
    //inform GUI start we started without problems
    GuiSetDebugState(initialized);
    //call plugin callback
    PLUG_CB_INITDEBUG initInfo;
    initInfo.szFileName=szFileName;
    plugincbcall(CB_INITDEBUG, &initInfo);
    //run debug loop (returns when process debugging is stopped)
    DebugLoop();
    //call plugin callback
    PLUG_CB_STOPDEBUG stopInfo;
    stopInfo.reserved=0;
    plugincbcall(CB_STOPDEBUG, &stopInfo);
    //message the user/do final stuff
    DeleteFileA("DLLLoader.exe");
    SymCleanup(fdProcessInfo->hProcess);
    dbclose();
    modclear();
    GuiSetDebugState(stopped);
    dputs("debugging stopped!");
    varset("$hp", 0, true);
    varset("$pid", 0, true);
    unlock(WAITID_STOP);
    waitclear();
    return 0;
}

CMDRESULT cbDebugInit(int argc, char* argv[])
{
    if(IsFileBeingDebugged())
    {
        dputs("already debugging!");
        return STATUS_ERROR;
    }

    char arg1[deflen]="";
    if(!argget(*argv, arg1, 0, false))
        return STATUS_ERROR;
    if(!FileExists(arg1))
    {
        dputs("file does not exsist!");
        return STATUS_ERROR;
    }

    char arg2[deflen]="";
    argget(*argv, arg2, 1, true);
    char* commandline=0;
    if(strlen(arg2))
        commandline=arg2;

    char arg3[deflen]="";
    argget(*argv, arg3, 2, true);

    char currentfolder[deflen]="";
    strcpy(currentfolder, arg1);
    int len=strlen(currentfolder);
    while(currentfolder[len]!='\\' and len!=0)
        len--;
    currentfolder[len]=0;
    if(DirExists(arg3))
        strcpy(currentfolder, arg3);
    INIT_STRUCT* init=(INIT_STRUCT*)emalloc(sizeof(INIT_STRUCT), "cbDebugInit:init");
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

CMDRESULT cbStopDebug(int argc, char* argv[])
{
    StopDebug();
    unlock(WAITID_RUN);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugRun(int argc, char* argv[])
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

CMDRESULT cbDebugSetBPXOptions(int argc, char* argv[])
{
    char argtype[deflen]="";
    uint type=0;
    if(!argget(*argv, argtype, 0, false))
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

CMDRESULT cbDebugSetBPX(int argc, char* argv[]) //bp addr [,name [,type]]
{
    char argaddr[deflen]="";
    if(!argget(*argv, argaddr, 0, false))
        return STATUS_ERROR;
    char argname[deflen]="";
    argget(*argv, argname, 1, true);
    char argtype[deflen]="";
    bool has_arg2=argget(*argv, argtype, 2, true);
    if(!has_arg2 and (scmp(argname, "ss") or scmp(argname, "long") or scmp(argname, "ud2")))
    {
        strcpy(argtype, argname);
        *argname=0;
    }
    _strlwr(argtype);
    uint addr=0;
    if(!valfromstring(argaddr, &addr, 0, 0, true, 0))
    {
        dprintf("invalid addr: \"%s\"\n", argaddr);
        return STATUS_ERROR;
    }
    if(addr==(uint)(GetPE32Data(szFileName, 0, UE_OEP)+pDebuggedDllBase))
    {
        dputs("entry breakpoint will be set automatically");
        return STATUS_ERROR;
    }
    int type=0;
    bool singleshoot=false;
    if(strstr(argtype, "ss"))
    {
        type|=UE_SINGLESHOOT;
        singleshoot=true;
    }
    else
        type|=UE_BREAKPOINT;
    if(strstr(argtype, "long"))
        type|=UE_BREAKPOINT_TYPE_LONG_INT3;
    else if(strstr(argtype, "ud2"))
        type|=UE_BREAKPOINT_TYPE_UD2;
    else if(strstr(argtype, "short"))
        type|=UE_BREAKPOINT_TYPE_INT3;
    short oldbytes;
    const char* bpname=0;
    if(*argname)
        bpname=argname;
    if(bpget(addr, BPNORMAL, bpname, 0))
    {
        dputs("breakpoint already set!");
        return STATUS_CONTINUE;
    }
    if(IsBPXEnabled(addr) or !memread(fdProcessInfo->hProcess, (void*)addr, &oldbytes, sizeof(short), 0) or !SetBPX(addr, type, (void*)cbUserBreakpoint) or !bpnew(addr, true, singleshoot, oldbytes, BPNORMAL, type, bpname))
    {
        dprintf("error setting breakpoint at "fhex"!\n", addr);
        return STATUS_ERROR;
    }
    dprintf("breakpoint at "fhex" set!\n", addr);
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

static bool cbDeleteAllBreakpoints(const BREAKPOINT* bp)
{
    if(DeleteBPX(bp->addr) and bpdel(bp->addr, BPNORMAL))
        return true;
    dprintf("delete breakpoint failed: "fhex"\n", bp->addr);
    return false;
}

CMDRESULT cbDebugDeleteBPX(int argc, char* argv[])
{
    char arg1[deflen]="";
    if(!argget(*argv, arg1, 0, true)) //delete all breakpoints
    {
        if(!bpgetcount(BPNORMAL))
        {
            dputs("no breakpoints to delete!");
            return STATUS_CONTINUE;
        }
        if(!bpenumall(cbDeleteAllBreakpoints)) //at least one deletion failed
            return STATUS_ERROR;
        dputs("all breakpoints deleted!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    if(bpget(0, BPNORMAL, arg1, &found)) //found a breakpoint with name
    {
        if(!DeleteBPX(found.addr) or !bpdel(found.addr, BPNORMAL))
        {
            dprintf("delete breakpoint failed: "fhex"\n", found.addr);
            return STATUS_ERROR;
        }
        return STATUS_CONTINUE;
    }
    uint addr=0;
    if(!valfromstring(arg1, &addr, 0, 0, true, 0) or !bpget(addr, BPNORMAL, 0, &found)) //invalid breakpoint
    {
        dprintf("no such breakpoint \"%s\"\n", arg1);
        return STATUS_ERROR;
    }
    if(!DeleteBPX(found.addr) or !bpdel(found.addr, BPNORMAL))
    {
        dprintf("delete breakpoint failed: "fhex"\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("breakpoint deleted!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

static bool cbEnableAllBreakpoints(const BREAKPOINT* bp)
{
    if(!SetBPX(bp->addr, bp->titantype, (void*)cbUserBreakpoint) or !bpenable(bp->addr, BPNORMAL, true))
    {
        dprintf("could not enable "fhex"\n", bp->addr);
        return false;
    }
    return true;
}

CMDRESULT cbDebugEnableBPX(int argc, char* argv[])
{
    puts("cbDebugEnableBPX");
    char arg1[deflen]="";
    if(!argget(*argv, arg1, 0, true)) //delete all breakpoints
    {
        if(!bpgetcount(BPNORMAL))
        {
            dputs("no breakpoints to enable!");
            return STATUS_CONTINUE;
        }
        if(!bpenumall(cbEnableAllBreakpoints)) //at least one deletion failed
            return STATUS_ERROR;
        dputs("all breakpoints enabled!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    if(bpget(0, BPNORMAL, arg1, &found)) //found a breakpoint with name
    {
        if(!SetBPX(found.addr, found.titantype, (void*)cbUserBreakpoint) or !bpenable(found.addr, BPNORMAL, true))
        {
            dprintf("could not enable "fhex"\n", found.addr);
            return STATUS_ERROR;
        }
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    uint addr=0;
    if(!valfromstring(arg1, &addr, 0, 0, true, 0) or !bpget(addr, BPNORMAL, 0, &found)) //invalid breakpoint
    {
        dprintf("no such breakpoint \"%s\"\n", arg1);
        return STATUS_ERROR;
    }
    if(found.enabled)
    {
        dputs("breakpoint already enabled!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    if(!SetBPX(found.addr, found.titantype, (void*)cbUserBreakpoint) or !bpenable(found.addr, BPNORMAL, true))
    {
        dprintf("could not enable "fhex"\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("breakpoint enabled!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

static bool cbDisableAllBreakpoints(const BREAKPOINT* bp)
{
    if(!DeleteBPX(bp->addr) or !bpenable(bp->addr, BPNORMAL, false))
    {
        dprintf("could not disable "fhex"\n", bp->addr);
        return false;
    }
    return true;
}

CMDRESULT cbDebugDisableBPX(int argc, char* argv[])
{
    char arg1[deflen]="";
    if(!argget(*argv, arg1, 0, true)) //delete all breakpoints
    {
        if(!bpgetcount(BPNORMAL))
        {
            dputs("no breakpoints to disable!");
            return STATUS_CONTINUE;
        }
        if(!bpenumall(cbDisableAllBreakpoints)) //at least one deletion failed
            return STATUS_ERROR;
        dputs("all breakpoints disabled!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    if(bpget(0, BPNORMAL, arg1, &found)) //found a breakpoint with name
    {
        if(!DeleteBPX(found.addr) or !bpenable(found.addr, BPNORMAL, false))
        {
            dprintf("could not disable "fhex"\n", found.addr);
            return STATUS_ERROR;
        }
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    uint addr=0;
    if(!valfromstring(arg1, &addr, 0, 0, true, 0) or !bpget(addr, BPNORMAL, 0, &found)) //invalid breakpoint
    {
        dprintf("no such breakpoint \"%s\"\n", arg1);
        return STATUS_ERROR;
    }
    if(!found.enabled)
    {
        dputs("breakpoint already disabled!");
        return STATUS_CONTINUE;
    }
    if(!DeleteBPX(found.addr) or !bpenable(found.addr, BPNORMAL, false))
    {
        dprintf("could not disable "fhex"\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("breakpoint enabled!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

static bool cbBreakpointList(const BREAKPOINT* bp)
{
    const char* type=0;
    if(bp->type==BPNORMAL)
    {
        if(bp->singleshoot)
            type="SS";
        else
            type="BP";
    }
    else if(bp->type==BPHARDWARE)
        type="HW";
    else if(bp->type==BPMEMORY)
        type="GP";
    bool enabled=bp->enabled;
    if(*bp->name)
        dprintf("%d:%s:"fhex":\"%s\"\n", enabled, type, bp->addr, bp->name);
    else
        dprintf("%d:%s:"fhex"\n", enabled, type, bp->addr);
    return true;
}

CMDRESULT cbDebugBplist(int argc, char* argv[])
{
    bpenumall(cbBreakpointList);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugStepInto(int argc, char* argv[])
{
    StepInto((void*)cbStep);
    isStepping=true;
    return cbDebugRun(argc, argv);
}

CMDRESULT cbDebugStepOver(int argc, char* argv[])
{
    StepOver((void*)cbStep);
    isStepping=true;
    return cbDebugRun(argc, argv);
}

CMDRESULT cbDebugSingleStep(int argc, char* argv[])
{
    char arg1[deflen]="";
    uint stepcount=1;
    if(argget(*argv, arg1, 0, true))
    {
        if(!valfromstring(arg1, &stepcount, 0, 0, true, 0))
            stepcount=1;
    }

    SingleStep((DWORD)stepcount, (void*)cbStep);
    isStepping=true;
    return cbDebugRun(argc, argv);
}

CMDRESULT cbDebugHide(int argc, char* argv[])
{
    if(HideDebugger(fdProcessInfo->hProcess, UE_HIDE_BASIC))
        dputs("debugger hidden");
    else
        dputs("something went wrong");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDisasm(int argc, char* argv[])
{
    char arg1[deflen]="";
    uint addr=GetContextData(UE_CIP);
    if(argget(*argv, arg1, 0, true))
        if(!valfromstring(arg1, &addr, 0, 0, true, 0))
            addr=GetContextData(UE_CIP);
    DebugUpdateGui(addr);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetMemoryBpx(int argc, char* argv[])
{
    char arg1[deflen]=""; //addr
    if(!argget(*argv, arg1, 0, false))
        return STATUS_ERROR;
    uint addr;
    if(!valfromstring(arg1, &addr, 0, 0, true, 0))
        return STATUS_ERROR;
    bool restore=false;
    char arg2[deflen]=""; //restore
    char arg3[deflen]=""; //type
    argget(*argv, arg3, 2, true);
    if(argget(*argv, arg2, 1, true))
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
    bool singleshoot=false;
    if(!restore)
        singleshoot=true;
    if(bpget(base, BPMEMORY, 0, 0))
    {
        dputs("hardware breakpoint already set!");
        return STATUS_CONTINUE;
    }
    if(!SetMemoryBPXEx(base, size, type, restore, (void*)cbMemoryBreakpoint) or !bpnew(base, true, singleshoot, 0, BPMEMORY, 0, 0))
    {
        dputs("error setting memory breakpoint!");
        return STATUS_ERROR;
    }
    dprintf("memory breakpoint at "fhex" set!\n", addr);
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

static bool cbDeleteAllMemoryBreakpoints(const BREAKPOINT* bp)
{
    if(!bp->enabled)
        return true;
    uint size;
    memfindbaseaddr(fdProcessInfo->hProcess, bp->addr, &size);
    if(!RemoveMemoryBPX(bp->addr, size) or !bpdel(bp->addr, BPMEMORY))
    {
        dprintf("delete memory breakpoint failed: "fhex"\n", bp->addr);
        return STATUS_ERROR;
    }
    return true;
}

CMDRESULT cbDebugDeleteMemoryBreakpoint(int argc, char* argv[])
{
    char arg1[deflen]="";
    if(!argget(*argv, arg1, 0, true)) //delete all breakpoints
    {
        if(!bpgetcount(BPMEMORY))
        {
            dputs("no memory breakpoints to delete!");
            return STATUS_CONTINUE;
        }
        if(!bpenumall(cbDeleteAllMemoryBreakpoints)) //at least one deletion failed
            return STATUS_ERROR;
        dputs("all memory breakpoints deleted!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    if(bpget(0, BPMEMORY, arg1, &found)) //found a breakpoint with name
    {
        uint size;
        memfindbaseaddr(fdProcessInfo->hProcess, found.addr, &size);
        if(!RemoveMemoryBPX(found.addr, size) or !bpdel(found.addr, BPMEMORY))
        {
            dprintf("delete memory breakpoint failed: "fhex"\n", found.addr);
            return STATUS_ERROR;
        }
        return STATUS_CONTINUE;
    }
    uint addr=0;
    if(!valfromstring(arg1, &addr, 0, 0, true, 0) or !bpget(addr, BPMEMORY, 0, &found)) //invalid breakpoint
    {
        dprintf("no such memory breakpoint \"%s\"\n", arg1);
        return STATUS_ERROR;
    }
    uint size;
    memfindbaseaddr(fdProcessInfo->hProcess, found.addr, &size);
    if(!RemoveMemoryBPX(found.addr, size) or !bpdel(found.addr, BPMEMORY))
    {
        dprintf("delete memory breakpoint failed: "fhex"\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("memory breakpoint deleted!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugRtr(int argc, char* argv[])
{
    StepOver((void*)cbRtrStep);
    cbDebugRun(argc, argv);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetHardwareBreakpoint(int argc, char* argv[])
{
    char arg1[deflen]=""; //addr
    if(!argget(*argv, arg1, 0, false))
        return STATUS_ERROR;
    uint addr;
    if(!valfromstring(arg1, &addr, 0, 0, true, 0))
        return STATUS_ERROR;
    uint type=UE_HARDWARE_EXECUTE;
    char arg2[deflen]=""; //type
    if(argget(*argv, arg2, 1, true))
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
            dputs("invalid type, assuming 'x'");
            break;
        }
    }
    char arg3[deflen]=""; //size
    uint size=UE_HARDWARE_SIZE_1;
    if(argget(*argv, arg3, 2, true))
    {
        if(!valfromstring(arg3, &size, 0, 0, true, 0))
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
        if((addr%size)!=0)
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
    int titantype=(drx<<8)|(type<<4)|size;
    //TODO: hwbp in multiple threads TEST
    if(bpget(addr, BPHARDWARE, 0, 0))
    {
        dputs("hardware breakpoint already set!");
        return STATUS_CONTINUE;
    }
    if(!SetHardwareBreakPoint(addr, drx, type, size, (void*)cbHardwareBreakpoint) or !bpnew(addr, true, false, 0, BPHARDWARE, titantype, 0))
    {
        dputs("error setting hardware breakpoint!");
        return STATUS_ERROR;
    }
    dprintf("hardware breakpoint at "fhex" set!\n", addr);
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

static bool cbDeleteAllHardwareBreakpoints(const BREAKPOINT* bp)
{
    if(!bp->enabled)
        return true;
    if(!DeleteHardwareBreakPoint((bp->titantype>>8)&0xF) or !bpdel(bp->addr, BPHARDWARE))
    {
        dprintf("delete hardware breakpoint failed: "fhex"\n", bp->addr);
        return STATUS_ERROR;
    }
    return true;
}

CMDRESULT cbDebugDeleteHardwareBreakpoint(int argc, char* argv[])
{
    char arg1[deflen]="";
    if(!argget(*argv, arg1, 0, true)) //delete all breakpoints
    {
        if(!bpgetcount(BPHARDWARE))
        {
            dputs("no hardware breakpoints to delete!");
            return STATUS_CONTINUE;
        }
        if(!bpenumall(cbDeleteAllHardwareBreakpoints)) //at least one deletion failed
            return STATUS_ERROR;
        dputs("all hardware breakpoints deleted!");
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    BREAKPOINT found;
    if(bpget(0, BPHARDWARE, arg1, &found)) //found a breakpoint with name
    {
        if(!DeleteHardwareBreakPoint((found.titantype>>8)&0xF) or !bpdel(found.addr, BPHARDWARE))
        {
            dprintf("delete hardware breakpoint failed: "fhex"\n", found.addr);
            return STATUS_ERROR;
        }
        return STATUS_CONTINUE;
    }
    uint addr=0;
    if(!valfromstring(arg1, &addr, 0, 0, true, 0) or !bpget(addr, BPHARDWARE, 0, &found)) //invalid breakpoint
    {
        dprintf("no such hardware breakpoint \"%s\"\n", arg1);
        return STATUS_ERROR;
    }
    if(!DeleteHardwareBreakPoint((found.titantype>>8)&0xF) or !bpdel(found.addr, BPHARDWARE))
    {
        dprintf("delete hardware breakpoint failed: "fhex"\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("hardware breakpoint deleted!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugAlloc(int argc, char* argv[])
{
    char arg1[deflen]=""; //size
    uint size=0x1000;
    if(argget(*argv, arg1, 0, true))
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

CMDRESULT cbDebugFree(int argc, char* argv[])
{
    uint lastalloc;
    varget("$lastalloc", &lastalloc, 0, 0);
    char arg1[deflen]=""; //addr
    uint addr=lastalloc;
    if(argget(*argv, arg1, 0, true))
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

CMDRESULT cbDebugMemset(int argc, char* argv[])
{
    char arg1[deflen]=""; //addr
    char arg2[deflen]=""; //value
    char arg3[deflen]=""; //size
    uint addr;
    uint value;
    uint size;
    if(!argget(*argv, arg1, 0, false) or !argget(*argv, arg2, 1, false))
        return STATUS_ERROR;
    if(!valfromstring(arg1, &addr, 0, 0, false, 0) or !valfromstring(arg2, &value, 0, 0, false, 0))
        return STATUS_ERROR;
    if(argget(*argv, arg3, 2, true))
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

CMDRESULT cbBenchmark(int argc, char* argv[])
{
    char arg1[deflen]="";
    if(!argget(*argv, arg1, 0, false))
        return STATUS_ERROR;
    uint addr=0;
    if(!valfromstring(arg1, &addr, 0, 0, false, 0))
        return STATUS_ERROR;
    uint ticks=GetTickCount();
    char comment[MAX_COMMENT_SIZE]="";
    commentset(addr, "benchmark");
    for(int i=0; i<100000; i++)
    {
        commentget(addr, comment);
    }
    commentdel(addr);
    dprintf("%ums\n", GetTickCount()-ticks);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugPause(int argc, char* argv[])
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

CMDRESULT cbMemWrite(int argc, char* argv[])
{
    char arg1[deflen]="";
    if(!argget(*argv, arg1, 0, false))
        return STATUS_ERROR;
    uint addr=0;
    if(!valfromstring(arg1, &addr, 0, 0, false, 0))
        return STATUS_ERROR;
    unsigned char* blub=(unsigned char*)emalloc(0x2123, "cbMemWrite:blub");
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

CMDRESULT cbStartScylla(int argc, char* argv[])
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
