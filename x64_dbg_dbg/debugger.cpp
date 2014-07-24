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
#include "x64_dbg.h"
#include "disasm_helper.h"
#include "symbolinfo.h"
#include "thread.h"
#include "disasm_fast.h"
#include "simplescript.h"

#include "BeaEngine\BeaEngine.h"
#include "DeviceNameResolver\DeviceNameResolver.h"

static PROCESS_INFORMATION g_pi= {0,0,0,0};
static char szFileName[MAX_PATH]="";
static char szBaseFileName[MAX_PATH]="";
static bool bFileIsDll=false;
static uint pDebuggedBase=0;
static uint pDebuggedEntry=0;
static bool isStepping=false;
static bool isPausedByUser=false;
static bool isDetachedByUser=false;
static bool bScyllaLoaded=false;
static bool bIsAttached=false;
static bool bSkipExceptions=false;
static bool bBreakOnNextDll=false;
static int ecount=0;
static std::vector<ExceptionRange> ignoredExceptionRange;
static std::map<unsigned int, const char*> exceptionNames;

//Superglobal variables
char sqlitedb[deflen]="";
PROCESS_INFORMATION* fdProcessInfo=&g_pi;

//static functions
static void cbStep();
static void cbSystemBreakpoint(void* ExceptionData);
static void cbUserBreakpoint();

void dbginit()
{
    exceptionNames.insert(std::make_pair(0x40000005, "STATUS_SEGMENT_NOTIFICATION"));
    exceptionNames.insert(std::make_pair(0x4000001C, "STATUS_WX86_UNSIMULATE"));
    exceptionNames.insert(std::make_pair(0x4000001D, "STATUS_WX86_CONTINUE"));
    exceptionNames.insert(std::make_pair(0x4000001E, "STATUS_WX86_SINGLE_STEP"));
    exceptionNames.insert(std::make_pair(0x4000001F, "STATUS_WX86_BREAKPOINT"));
    exceptionNames.insert(std::make_pair(0x40000020, "STATUS_WX86_EXCEPTION_CONTINUE"));
    exceptionNames.insert(std::make_pair(0x40000021, "STATUS_WX86_EXCEPTION_LASTCHANCE"));
    exceptionNames.insert(std::make_pair(0x40000022, "STATUS_WX86_EXCEPTION_CHAIN"));
    exceptionNames.insert(std::make_pair(0x40000028, "STATUS_WX86_CREATEWX86TIB"));
    exceptionNames.insert(std::make_pair(0x40010003, "DBG_TERMINATE_THREAD"));
    exceptionNames.insert(std::make_pair(0x40010004, "DBG_TERMINATE_PROCESS"));
    exceptionNames.insert(std::make_pair(0x40010005, "DBG_CONTROL_C"));
    exceptionNames.insert(std::make_pair(0x40010006, "DBG_PRINTEXCEPTION_C"));
    exceptionNames.insert(std::make_pair(0x40010007, "DBG_RIPEXCEPTION"));
    exceptionNames.insert(std::make_pair(0x40010008, "DBG_CONTROL_BREAK"));
    exceptionNames.insert(std::make_pair(0x40010009, "DBG_COMMAND_EXCEPTION"));
    exceptionNames.insert(std::make_pair(0x80000001, "EXCEPTION_GUARD_PAGE"));
    exceptionNames.insert(std::make_pair(0x80000002, "EXCEPTION_DATATYPE_MISALIGNMENT"));
    exceptionNames.insert(std::make_pair(0x80000003, "EXCEPTION_BREAKPOINT"));
    exceptionNames.insert(std::make_pair(0x80000004, "EXCEPTION_SINGLE_STEP"));
    exceptionNames.insert(std::make_pair(0x80000026, "STATUS_LONGJUMP"));
    exceptionNames.insert(std::make_pair(0x80000029, "STATUS_UNWIND_CONSOLIDATE"));
    exceptionNames.insert(std::make_pair(0x80010001, "DBG_EXCEPTION_NOT_HANDLED"));
    exceptionNames.insert(std::make_pair(0xC0000005, "EXCEPTION_ACCESS_VIOLATION"));
    exceptionNames.insert(std::make_pair(0xC0000006, "EXCEPTION_IN_PAGE_ERROR"));
    exceptionNames.insert(std::make_pair(0xC0000008, "EXCEPTION_INVALID_HANDLE"));
    exceptionNames.insert(std::make_pair(0xC000000D, "STATUS_INVALID_PARAMETER"));
    exceptionNames.insert(std::make_pair(0xC0000017, "STATUS_NO_MEMORY"));
    exceptionNames.insert(std::make_pair(0xC000001D, "EXCEPTION_ILLEGAL_INSTRUCTION"));
    exceptionNames.insert(std::make_pair(0xC0000025, "EXCEPTION_NONCONTINUABLE_EXCEPTION"));
    exceptionNames.insert(std::make_pair(0xC0000026, "EXCEPTION_INVALID_DISPOSITION"));
    exceptionNames.insert(std::make_pair(0xC000008C, "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"));
    exceptionNames.insert(std::make_pair(0xC000008D, "EXCEPTION_FLT_DENORMAL_OPERAND"));
    exceptionNames.insert(std::make_pair(0xC000008E, "EXCEPTION_FLT_DIVIDE_BY_ZERO"));
    exceptionNames.insert(std::make_pair(0xC000008F, "EXCEPTION_FLT_INEXACT_RESULT"));
    exceptionNames.insert(std::make_pair(0xC0000090, "EXCEPTION_FLT_INVALID_OPERATION"));
    exceptionNames.insert(std::make_pair(0xC0000091, "EXCEPTION_FLT_OVERFLOW"));
    exceptionNames.insert(std::make_pair(0xC0000092, "EXCEPTION_FLT_STACK_CHECK"));
    exceptionNames.insert(std::make_pair(0xC0000093, "EXCEPTION_FLT_UNDERFLOW"));
    exceptionNames.insert(std::make_pair(0xC0000094, "EXCEPTION_INT_DIVIDE_BY_ZERO"));
    exceptionNames.insert(std::make_pair(0xC0000095, "EXCEPTION_INT_OVERFLOW"));
    exceptionNames.insert(std::make_pair(0xC0000096, "EXCEPTION_PRIV_INSTRUCTION"));
    exceptionNames.insert(std::make_pair(0xC00000FD, "EXCEPTION_STACK_OVERFLOW"));
    exceptionNames.insert(std::make_pair(0xC0000135, "STATUS_DLL_NOT_FOUND"));
    exceptionNames.insert(std::make_pair(0xC0000138, "STATUS_ORDINAL_NOT_FOUND"));
    exceptionNames.insert(std::make_pair(0xC0000139, "STATUS_ENTRYPOINT_NOT_FOUND"));
    exceptionNames.insert(std::make_pair(0xC000013A, "STATUS_CONTROL_C_EXIT"));
    exceptionNames.insert(std::make_pair(0xC0000142, "STATUS_DLL_INIT_FAILED"));
    exceptionNames.insert(std::make_pair(0xC000014A, "STATUS_ILLEGAL_FLOAT_CONTEXT"));
    exceptionNames.insert(std::make_pair(0xC0000194, "EXCEPTION_POSSIBLE_DEADLOCK"));
    exceptionNames.insert(std::make_pair(0xC00002B4, "STATUS_FLOAT_MULTIPLE_FAULTS"));
    exceptionNames.insert(std::make_pair(0xC00002B5, "STATUS_FLOAT_MULTIPLE_TRAPS"));
    exceptionNames.insert(std::make_pair(0xC00002C5, "STATUS_DATATYPE_MISALIGNMENT_ERROR"));
    exceptionNames.insert(std::make_pair(0xC00002C9, "STATUS_REG_NAT_CONSUMPTION"));
    exceptionNames.insert(std::make_pair(0xC0000409, "STATUS_STACK_BUFFER_OVERRUN"));
    exceptionNames.insert(std::make_pair(0xC0000417, "STATUS_INVALID_CRUNTIME_PARAMETER"));
    exceptionNames.insert(std::make_pair(0xC0000420, "STATUS_ASSERTION_FAILURE"));
    exceptionNames.insert(std::make_pair(0x04242420, "CLRDBG_NOTIFICATION_EXCEPTION_CODE"));
    exceptionNames.insert(std::make_pair(0xE0434352, "CLR_EXCEPTION"));
    exceptionNames.insert(std::make_pair(0xE06D7363, "CPP_EH_EXCEPTION"));
}

void dbgdisablebpx()
{
    std::vector<BREAKPOINT> list;
    int bpcount=bpgetlist(&list);
    for(int i=0; i<bpcount; i++)
    {
        if(list[i].type==BPNORMAL and IsBPXEnabled(list[i].addr))
            DeleteBPX(list[i].addr);
    }
}

void dbgenablebpx()
{
    std::vector<BREAKPOINT> list;
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

void dbgsetskipexceptions(bool skip)
{
    bSkipExceptions=skip;
}

void dbgclearignoredexceptions()
{
    std::vector<ExceptionRange>().swap(ignoredExceptionRange);
}

void dbgaddignoredexception(ExceptionRange range)
{
    ignoredExceptionRange.push_back(range);
}

bool dbgisignoredexception(unsigned int exception)
{
    for(unsigned int i=0; i<ignoredExceptionRange.size(); i++)
    {
        unsigned int curStart=ignoredExceptionRange.at(i).start;
        unsigned int curEnd=ignoredExceptionRange.at(i).end;
        if(exception>=curStart && exception<=curEnd)
            return true;
    }
    return false;
}

bool dbgcmdnew(const char* name, CBCOMMAND cbCommand, bool debugonly)
{
    if(!cmdnew(dbggetcommandlist(), name, cbCommand, debugonly))
        return false;
    GuiAutoCompleteAddCmd(name);
    return true;
}

bool dbgcmddel(const char* name)
{
    if(!cmddel(dbggetcommandlist(), name))
        return false;
    GuiAutoCompleteDelCmd(name);
    return true;
}

DWORD WINAPI updateCallStackThread(void* ptr)
{
    GuiUpdateCallStack();
    return 0;
}

void DebugUpdateGui(uint disasm_addr, bool stack)
{
    uint cip=GetContextData(UE_CIP);
    if(memisvalidreadptr(fdProcessInfo->hProcess, disasm_addr))
        GuiDisasmAt(disasm_addr, cip);
    uint csp=GetContextData(UE_CSP);
    if(stack)
        GuiStackDumpAt(csp, csp);
    static uint cacheCsp=0;
    if(csp!=cacheCsp)
    {
        cacheCsp=csp;
        CloseHandle(CreateThread(0, 0, updateCallStackThread, 0, 0, 0));
    }
    char modname[MAX_MODULE_SIZE]="";
    char modtext[MAX_MODULE_SIZE*2]="";
    if(!modnamefromaddr(disasm_addr, modname, true))
        *modname=0;
    else
        sprintf(modtext, "Module: %s - ", modname);
    char title[1024]="";
    sprintf(title, "File: %s - PID: %X - %sThread: %X", szBaseFileName, fdProcessInfo->dwProcessId, modtext, ((DEBUG_EVENT*)GetDebugData())->dwThreadId);
    GuiUpdateWindowTitle(title);
    GuiUpdateAllViews();
}

static void cbUserBreakpoint()
{
    BREAKPOINT bp;
    BRIDGEBP pluginBp;
    PLUG_CB_BREAKPOINT bpInfo;
    bpInfo.breakpoint=0;
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
        const char* symbolicname=symgetsymbolicname(bp.addr);
        if(symbolicname)
        {
            if(*bp.name)
                dprintf("%s breakpoint \"%s\" at %s ("fhex")!\n", bptype, bp.name, symbolicname, bp.addr);
            else
                dprintf("%s breakpoint at %s ("fhex")!\n", bptype, symbolicname, bp.addr);
        }
        else
        {
            if(*bp.name)
                dprintf("%s breakpoint \"%s\" at "fhex"!\n", bptype, bp.name, bp.addr);
            else
                dprintf("%s breakpoint at "fhex"!\n", bptype, bp.addr);
        }
        if(bp.singleshoot)
            bpdel(bp.addr, BPNORMAL);
        bptobridge(&bp, &pluginBp);
        bpInfo.breakpoint=&pluginBp;
    }
    DebugUpdateGui(GetContextData(UE_CIP), true);
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions=false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved=0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    plugincbcall(CB_BREAKPOINT, &bpInfo);
    wait(WAITID_RUN);
}

static void cbHardwareBreakpoint(void* ExceptionAddress)
{
    uint cip=GetContextData(UE_CIP);
    BREAKPOINT bp;
    BRIDGEBP pluginBp;
    PLUG_CB_BREAKPOINT bpInfo;
    bpInfo.breakpoint=0;
    if(!bpget((uint)ExceptionAddress, BPHARDWARE, 0, &bp))
        dputs("hardware breakpoint reached not in list!");
    else
    {
        const char* bpsize="";
        switch(bp.titantype&0xF) //size
        {
        case UE_HARDWARE_SIZE_1:
            bpsize="byte, ";
            break;
        case UE_HARDWARE_SIZE_2:
            bpsize="word, ";
            break;
        case UE_HARDWARE_SIZE_4:
            bpsize="dword, ";
            break;
#ifdef _WIN64
        case UE_HARDWARE_SIZE_8:
            bpsize="qword, ";
            break;
#endif //_WIN64
        }
        const char* bptype="";
        switch((bp.titantype>>4)&0xF) //type
        {
        case UE_HARDWARE_EXECUTE:
            bptype="execute";
            bpsize="";
            break;
        case UE_HARDWARE_READWRITE:
            bptype="read/write";
            break;
        case UE_HARDWARE_WRITE:
            bptype="write";
            break;
        }
        const char* symbolicname=symgetsymbolicname(bp.addr);
        if(symbolicname)
        {
            if(*bp.name)
                dprintf("hardware breakpoint (%s%s) \"%s\" at %s ("fhex")!\n", bpsize, bptype, bp.name, symbolicname, bp.addr);
            else
                dprintf("hardware breakpoint (%s%s) at %s ("fhex")!\n", bpsize, bptype, symbolicname, bp.addr);
        }
        else
        {
            if(*bp.name)
                dprintf("hardware breakpoint (%s%s) \"%s\" at "fhex"!\n", bpsize, bptype, bp.name, bp.addr);
            else
                dprintf("hardware breakpoint (%s%s) at "fhex"!\n", bpsize, bptype, bp.addr);
        }
        bptobridge(&bp, &pluginBp);
        bpInfo.breakpoint=&pluginBp;
    }
    DebugUpdateGui(cip, true);
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions=false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved=0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    plugincbcall(CB_BREAKPOINT, &bpInfo);
    wait(WAITID_RUN);
}

static void cbMemoryBreakpoint(void* ExceptionAddress)
{
    uint cip=GetContextData(UE_CIP);
    uint size;
    uint base=memfindbaseaddr((uint)ExceptionAddress, &size, true);
    BREAKPOINT bp;
    BRIDGEBP pluginBp;
    PLUG_CB_BREAKPOINT bpInfo;
    bpInfo.breakpoint=0;
    if(!bpget(base, BPMEMORY, 0, &bp))
        dputs("memory breakpoint reached not in list!");
    else
    {
        const char* bptype="";
        switch(bp.titantype)
        {
        case UE_MEMORY_READ:
            bptype=" (read)";
            break;
        case UE_MEMORY_WRITE:
            bptype=" (write)";
            break;
        case UE_MEMORY_EXECUTE:
            bptype=" (execute)";
            break;
        case UE_MEMORY:
            bptype=" (read/write/execute)";
            break;
        }
        const char* symbolicname=symgetsymbolicname(bp.addr);
        if(symbolicname)
        {
            if(*bp.name)
                dprintf("memory breakpoint%s \"%s\" at %s ("fhex", "fhex")!\n", bptype, bp.name, symbolicname, bp.addr, ExceptionAddress);
            else
                dprintf("memory breakpoint%s at %s ("fhex", "fhex")!\n", bptype, symbolicname, bp.addr, ExceptionAddress);
        }
        else
        {
            if(*bp.name)
                dprintf("memory breakpoint%s \"%s\" at "fhex" ("fhex")!\n", bptype, bp.name, bp.addr, ExceptionAddress);
            else
                dprintf("memory breakpoint%s at "fhex" ("fhex")!\n", bptype, bp.addr, ExceptionAddress);
        }
        bptobridge(&bp, &pluginBp);
        bpInfo.breakpoint=&pluginBp;
    }
    if(bp.singleshoot)
        bpdel(bp.addr, BPMEMORY); //delete from breakpoint list
    DebugUpdateGui(cip, true);
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions=false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved=0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    plugincbcall(CB_BREAKPOINT, &bpInfo);
    wait(WAITID_RUN);
}

static void cbLibrarianBreakpoint(void* lpData)
{
    bBreakOnNextDll=true;
}

static BOOL CALLBACK SymRegisterCallbackProc64(HANDLE hProcess, ULONG ActionCode, ULONG64 CallbackData, ULONG64 UserContext)
{
    UNREFERENCED_PARAMETER(hProcess);
    UNREFERENCED_PARAMETER(UserContext);
    PIMAGEHLP_CBA_EVENT evt;
    switch (ActionCode)
    {
    case CBA_EVENT:
    {
        evt=(PIMAGEHLP_CBA_EVENT)CallbackData;
        const char* text=(const char*)evt->desc;
        int len=(int)strlen(text);
        bool suspress=false;
        for(int i=0; i<len; i++)
            if(text[i]==0x08)
            {
                suspress=true;
                break;
            }
        int percent=0;
        static bool zerobar=false;
        if(zerobar)
        {
            zerobar=false;
            GuiSymbolSetProgress(0);
        }
        if(strstr(text, " bytes -  "))
        {
            char* newtext=(char*)emalloc(len+1, "SymRegisterCallbackProc64:newtext");
            strcpy(newtext, text);
            strstr(newtext, " bytes -  ")[8]=0;
            GuiSymbolLogAdd(newtext);
            efree(newtext, "SymRegisterCallbackProc64:newtext");
            suspress=true;
        }
        else if(strstr(text, " copied         "))
        {
            GuiSymbolSetProgress(100);
            GuiSymbolLogAdd(" downloaded!\n");
            suspress=true;
            zerobar=true;
        }
        else if(sscanf(text, "%*s %d percent", &percent)==1 or sscanf(text, "%d percent", &percent)==1)
        {
            GuiSymbolSetProgress(percent);
            suspress=true;
        }

        if(!suspress)
            GuiSymbolLogAdd(text);
    }
    break;

    default:
    {
        return FALSE;
    }
    }
    return TRUE;
}

static bool cbSetModuleBreakpoints(const BREAKPOINT* bp)
{
    if(!bp->enabled)
        return true;
    switch(bp->type)
    {
    case BPNORMAL:
    {
        if(!SetBPX(bp->addr, bp->titantype, (void*)cbUserBreakpoint))
            dprintf("could not set breakpoint "fhex"!\n", bp->addr);
    }
    break;

    case BPMEMORY:
    {
        uint size=0;
        memfindbaseaddr(bp->addr, &size);
        bool restore=false;
        if(!bp->singleshoot)
            restore=true;
        if(!SetMemoryBPXEx(bp->addr, size, bp->titantype, restore, (void*)cbMemoryBreakpoint))
            dprintf("could not set memory breakpoint "fhex"!\n", bp->addr);
    }
    break;

    case BPHARDWARE:
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
        if(bp->enabled)
            RemoveMemoryBPX(bp->addr, 0);
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
    DebugUpdateGui(GetContextData(UE_CIP), true);
    GuiSetDebugState(paused);
    PLUG_CB_STEPPED stepInfo;
    stepInfo.reserved=0;
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions=false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved=0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    plugincbcall(CB_STEPPED, &stepInfo);
    wait(WAITID_RUN);
}

static void cbRtrFinalStep()
{
    DebugUpdateGui(GetContextData(UE_CIP), true);
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions=false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved=0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    wait(WAITID_RUN);
}

static unsigned char getCIPch()
{
    unsigned char ch=0x90;
    uint cip=GetContextData(UE_CIP);
    memread(fdProcessInfo->hProcess, (void*)cip, &ch, 1, 0);
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
    if(!GetFileNameFromHandle(CreateProcessInfo->hFile, DebugFileName))
    {
        if(!DevicePathFromFileHandleA(CreateProcessInfo->hFile, DebugFileName, deflen))
            strcpy(DebugFileName, "??? (GetFileNameFromHandle failed!)");
    }
    dprintf("Process Started: "fhex" %s\n", base, DebugFileName);

    //init program database
    int len=(int)strlen(szFileName);
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
    sprintf(dbpath, "%s\\%s", dbbasepath, sqlitedb);
    dprintf("Database file: %s\n", dbpath);
    dbload();
    SymSetOptions(SYMOPT_DEBUG|SYMOPT_LOAD_LINES);
    GuiSymbolLogClear();
    SymInitialize(fdProcessInfo->hProcess, 0, false); //initialize symbols
    SymRegisterCallback64(fdProcessInfo->hProcess, SymRegisterCallbackProc64, 0);
    SymLoadModuleEx(fdProcessInfo->hProcess, CreateProcessInfo->hFile, DebugFileName, 0, (DWORD64)base, 0, 0, 0);
    IMAGEHLP_MODULE64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct=sizeof(modInfo);
    if(SymGetModuleInfo64(fdProcessInfo->hProcess, (DWORD64)base, &modInfo))
        modload((uint)base, modInfo.ImageSize, modInfo.ImageName);
    memupdatemap(fdProcessInfo->hProcess); //update memory map
    char modname[256]="";
    if(modnamefromaddr((uint)base, modname, true))
        bpenumall(cbSetModuleBreakpoints, modname);
    GuiUpdateBreakpointsView();
    if(!bFileIsDll and !bIsAttached) //Set entry breakpoint
    {
        pDebuggedBase=(uint)CreateProcessInfo->lpBaseOfImage; //debugged base = executable
        char command[256]="";

        if(settingboolget("Events", "TlsCallbacks"))
        {
            DWORD NumberOfCallBacks=0;
            TLSGrabCallBackData(DebugFileName, 0, &NumberOfCallBacks);
            if(NumberOfCallBacks)
            {
                dprintf("TLS Callbacks: %d\n", NumberOfCallBacks);
                uint* TLSCallBacks=(uint*)emalloc(NumberOfCallBacks*sizeof(uint), "cbCreateProcess:TLSCallBacks");
                if(!TLSGrabCallBackData(DebugFileName, TLSCallBacks, &NumberOfCallBacks))
                    dputs("failed to get TLS callback addresses!");
                else
                {
                    for(unsigned int i=0; i<NumberOfCallBacks; i++)
                    {
                        sprintf(command, "bp "fhex",\"TLS Callback %d\",ss", TLSCallBacks[i], i+1);
                        cmddirectexec(dbggetcommandlist(), command);
                    }
                }
                efree(TLSCallBacks, "cbCreateProcess:TLSCallBacks");
            }
        }

        if(settingboolget("Events", "EntryBreakpoint"))
        {
            sprintf(command, "bp "fhex",\"entry breakpoint\",ss", CreateProcessInfo->lpStartAddress);
            cmddirectexec(dbggetcommandlist(), command);
        }
    }
    GuiUpdateBreakpointsView();

    //call plugin callback
    PLUG_CB_CREATEPROCESS callbackInfo;
    callbackInfo.CreateProcessInfo=CreateProcessInfo;
    callbackInfo.modInfo=&modInfo;
    callbackInfo.DebugFileName=DebugFileName;
    callbackInfo.fdProcessInfo=fdProcessInfo;
    plugincbcall(CB_CREATEPROCESS, &callbackInfo);

    //update thread list
    CREATE_THREAD_DEBUG_INFO threadInfo;
    threadInfo.hThread=CreateProcessInfo->hThread;
    threadInfo.lpStartAddress=CreateProcessInfo->lpStartAddress;
    threadInfo.lpThreadLocalBase=CreateProcessInfo->lpThreadLocalBase;
    threadcreate(&threadInfo);
}

static void cbExitProcess(EXIT_PROCESS_DEBUG_INFO* ExitProcess)
{
    PLUG_CB_EXITPROCESS callbackInfo;
    callbackInfo.ExitProcess=ExitProcess;
    plugincbcall(CB_EXITPROCESS, &callbackInfo);
    //Cleanup
    SymCleanup(fdProcessInfo->hProcess);
}

static void cbCreateThread(CREATE_THREAD_DEBUG_INFO* CreateThread)
{
    threadcreate(CreateThread); //update thread list
    DWORD dwThreadId=((DEBUG_EVENT*)GetDebugData())->dwThreadId;

    if(settingboolget("Events", "ThreadEntry"))
    {
        char command[256]="";
        sprintf(command, "bp "fhex",\"Thread %X\",ss", CreateThread->lpStartAddress, dwThreadId);
        cmddirectexec(dbggetcommandlist(), command);
    }

    PLUG_CB_CREATETHREAD callbackInfo;
    callbackInfo.CreateThread=CreateThread;
    callbackInfo.dwThreadId=dwThreadId;
    plugincbcall(CB_CREATETHREAD, &callbackInfo);

    dprintf("Thread %X created\n", dwThreadId);

    if(settingboolget("Events", "ThreadStart"))
    {
        memupdatemap(fdProcessInfo->hProcess); //update memory map
        //update GUI
        DebugUpdateGui(GetContextData(UE_CIP), true);
        GuiSetDebugState(paused);
        //lock
        lock(WAITID_RUN);
        SetForegroundWindow(GuiGetWindowHandle());
        PLUG_CB_PAUSEDEBUG pauseInfo;
        pauseInfo.reserved=0;
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        wait(WAITID_RUN);
    }
}

static void cbExitThread(EXIT_THREAD_DEBUG_INFO* ExitThread)
{
    DWORD dwThreadId=((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    PLUG_CB_EXITTHREAD callbackInfo;
    callbackInfo.ExitThread=ExitThread;
    callbackInfo.dwThreadId=dwThreadId;
    plugincbcall(CB_EXITTHREAD, &callbackInfo);
    threadexit(dwThreadId);
    dprintf("Thread %X exit\n", dwThreadId);

    if(settingboolget("Events", "ThreadEnd"))
    {
        //update GUI
        DebugUpdateGui(GetContextData(UE_CIP), true);
        GuiSetDebugState(paused);
        //lock
        lock(WAITID_RUN);
        SetForegroundWindow(GuiGetWindowHandle());
        PLUG_CB_PAUSEDEBUG pauseInfo;
        pauseInfo.reserved=0;
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        wait(WAITID_RUN);
    }
}

static void cbSystemBreakpoint(void* ExceptionData)
{
    //log message
    dputs("system breakpoint reached!");
    bSkipExceptions=false; //we are not skipping first-chance exceptions
    uint cip=GetContextData(UE_CIP);
    GuiDumpAt(memfindbaseaddr(cip, 0, true)); //dump somewhere

    //plugin callbacks
    PLUG_CB_SYSTEMBREAKPOINT callbackInfo;
    callbackInfo.reserved=0;
    plugincbcall(CB_SYSTEMBREAKPOINT, &callbackInfo);

    if(settingboolget("Events", "SystemBreakpoint"))
    {
        //update GUI
        DebugUpdateGui(cip, true);
        GuiSetDebugState(paused);
        //lock
        lock(WAITID_RUN);
        SetForegroundWindow(GuiGetWindowHandle());
        PLUG_CB_PAUSEDEBUG pauseInfo;
        pauseInfo.reserved=0;
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        wait(WAITID_RUN);
    }
}

static void cbLoadDll(LOAD_DLL_DEBUG_INFO* LoadDll)
{
    void* base=LoadDll->lpBaseOfDll;
    char DLLDebugFileName[deflen]="";
    if(!GetFileNameFromHandle(LoadDll->hFile, DLLDebugFileName))
    {
        if(!DevicePathFromFileHandleA(LoadDll->hFile, DLLDebugFileName, deflen))
            strcpy(DLLDebugFileName, "??? (GetFileNameFromHandle failed!)");
    }
    SymLoadModuleEx(fdProcessInfo->hProcess, LoadDll->hFile, DLLDebugFileName, 0, (DWORD64)base, 0, 0, 0);
    IMAGEHLP_MODULE64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct=sizeof(IMAGEHLP_MODULE64);
    if(SymGetModuleInfo64(fdProcessInfo->hProcess, (DWORD64)base, &modInfo))
        modload((uint)base, modInfo.ImageSize, modInfo.ImageName);
    memupdatemap(fdProcessInfo->hProcess); //update memory map
    char modname[256]="";
    if(modnamefromaddr((uint)base, modname, true))
        bpenumall(cbSetModuleBreakpoints, modname);
    GuiUpdateBreakpointsView();
    bool bAlreadySetEntry=false;
    if(bFileIsDll and !_stricmp(DLLDebugFileName, szFileName) and !bIsAttached) //Set entry breakpoint
    {
        pDebuggedBase=(uint)base;
        char command[256]="";
        if(settingboolget("Events", "EntryBreakpoint"))
        {
            bAlreadySetEntry=true;
            sprintf(command, "bp "fhex",\"entry breakpoint\",ss", pDebuggedBase+pDebuggedEntry);
            cmddirectexec(dbggetcommandlist(), command);
        }
    }
    GuiUpdateBreakpointsView();

    if((bBreakOnNextDll || settingboolget("Events", "DllEntry")) && !bAlreadySetEntry)
    {
        uint oep=GetPE32Data(DLLDebugFileName, 0, UE_OEP);
        if(oep)
        {
            char command[256]="";
            sprintf(command, "bp "fhex",\"DllMain (%s)\",ss", oep+(uint)base, modname);
            cmddirectexec(dbggetcommandlist(), command);
        }
    }

    dprintf("DLL Loaded: "fhex" %s\n", base, DLLDebugFileName);

    //plugin callback
    PLUG_CB_LOADDLL callbackInfo;
    callbackInfo.LoadDll=LoadDll;
    callbackInfo.modInfo=&modInfo;
    callbackInfo.modname=modname;
    plugincbcall(CB_LOADDLL, &callbackInfo);

    if(bBreakOnNextDll || settingboolget("Events", "DllLoad"))
    {
        bBreakOnNextDll=false;
        //update GUI
        DebugUpdateGui(GetContextData(UE_CIP), true);
        GuiSetDebugState(paused);
        //lock
        lock(WAITID_RUN);
        SetForegroundWindow(GuiGetWindowHandle());
        PLUG_CB_PAUSEDEBUG pauseInfo;
        pauseInfo.reserved=0;
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        wait(WAITID_RUN);
    }
}

static void cbUnloadDll(UNLOAD_DLL_DEBUG_INFO* UnloadDll)
{
    PLUG_CB_UNLOADDLL callbackInfo;
    callbackInfo.UnloadDll=UnloadDll;
    plugincbcall(CB_UNLOADDLL, &callbackInfo);

    void* base=UnloadDll->lpBaseOfDll;
    char modname[256]="???";
    if(modnamefromaddr((uint)base, modname, true))
        bpenumall(cbRemoveModuleBreakpoints, modname);
    GuiUpdateBreakpointsView();
    SymUnloadModule64(fdProcessInfo->hProcess, (DWORD64)base);
    dprintf("DLL Unloaded: "fhex" %s\n", base, modname);

    if(bBreakOnNextDll || settingboolget("Events", "DllÙnload"))
    {
        bBreakOnNextDll=false;
        //update GUI
        DebugUpdateGui(GetContextData(UE_CIP), true);
        GuiSetDebugState(paused);
        //lock
        lock(WAITID_RUN);
        SetForegroundWindow(GuiGetWindowHandle());
        PLUG_CB_PAUSEDEBUG pauseInfo;
        pauseInfo.reserved=0;
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        wait(WAITID_RUN);
    }

    modunload((uint)base);
}

static void cbOutputDebugString(OUTPUT_DEBUG_STRING_INFO* DebugString)
{
    PLUG_CB_OUTPUTDEBUGSTRING callbackInfo;
    callbackInfo.DebugString=DebugString;
    plugincbcall(CB_OUTPUTDEBUGSTRING, &callbackInfo);

    if(!DebugString->fUnicode) //ASCII
    {
        char* DebugText=(char*)emalloc(DebugString->nDebugStringLength+1, "cbOutputDebugString:DebugText");
        memset(DebugText, 0, DebugString->nDebugStringLength+1);
        if(memread(fdProcessInfo->hProcess, DebugString->lpDebugStringData, DebugText, DebugString->nDebugStringLength, 0))
        {
            int len=(int)strlen(DebugText);
            int escape_count=0;
            for(int i=0; i<len; i++)
                if(DebugText[i]=='\\' or DebugText[i]=='\"' or !isprint(DebugText[i]))
                    escape_count++;
            char* DebugTextEscaped=(char*)emalloc(len+escape_count*3+1, "cbOutputDebugString:DebugTextEscaped");
            memset(DebugTextEscaped, 0, len+escape_count*3+1);
            for(int i=0,j=0; i<len; i++)
            {
                switch(DebugText[i])
                {
                case '\t':
                    j+=sprintf(DebugTextEscaped+j, "\\t");
                    break;
                case '\f':
                    j+=sprintf(DebugTextEscaped+j, "\\f");
                    break;
                case '\v':
                    j+=sprintf(DebugTextEscaped+j, "\\v");
                    break;
                case '\n':
                    j+=sprintf(DebugTextEscaped+j, "\\n");
                    break;
                case '\r':
                    j+=sprintf(DebugTextEscaped+j, "\\r");
                    break;
                case '\\':
                    j+=sprintf(DebugTextEscaped+j, "\\\\");
                    break;
                case '\"':
                    j+=sprintf(DebugTextEscaped+j, "\\\"");
                    break;
                default:
                    if(!isprint(DebugText[i])) //unknown unprintable character
                        j+=sprintf(DebugTextEscaped+j, "\\%.2x", DebugText[i]);
                    else
                        j+=sprintf(DebugTextEscaped+j, "%c", DebugText[i]);
                    break;
                }
            }
            dprintf("DebugString: \"%s\"\n", DebugTextEscaped);
            efree(DebugTextEscaped, "cbOutputDebugString:DebugTextEscaped");
        }
        efree(DebugText, "cbOutputDebugString:DebugText");
    }

    if(settingboolget("Events", "DebugStrings"))
    {
        //update GUI
        DebugUpdateGui(GetContextData(UE_CIP), true);
        GuiSetDebugState(paused);
        //lock
        lock(WAITID_RUN);
        SetForegroundWindow(GuiGetWindowHandle());
        PLUG_CB_PAUSEDEBUG pauseInfo;
        pauseInfo.reserved=0;
        plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
        wait(WAITID_RUN);
    }
}

static void cbException(EXCEPTION_DEBUG_INFO* ExceptionData)
{
    PLUG_CB_EXCEPTION callbackInfo;
    callbackInfo.Exception=ExceptionData;
    unsigned int ExceptionCode=ExceptionData->ExceptionRecord.ExceptionCode;
    GuiSetLastException(ExceptionCode);

    uint addr=(uint)ExceptionData->ExceptionRecord.ExceptionAddress;
    if(ExceptionData->ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT)
    {
        if(isDetachedByUser)
        {
            PLUG_CB_DETACH detachInfo;
            detachInfo.fdProcessInfo=fdProcessInfo;
            plugincbcall(CB_DETACH, &detachInfo);
            if(!DetachDebuggerEx(fdProcessInfo->dwProcessId))
                dputs("DetachDebuggerEx failed...");
            else
                dputs("detached!");
            isDetachedByUser=false;
            return;
        }
        else if(isPausedByUser)
        {
            dputs("paused!");
            SetNextDbgContinueStatus(DBG_CONTINUE);
            DebugUpdateGui(GetContextData(UE_CIP), true);
            GuiSetDebugState(paused);
            //lock
            lock(WAITID_RUN);
            SetForegroundWindow(GuiGetWindowHandle());
            bSkipExceptions=false;
            PLUG_CB_PAUSEDEBUG pauseInfo;
            pauseInfo.reserved=0;
            plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
            plugincbcall(CB_EXCEPTION, &callbackInfo);
            wait(WAITID_RUN);
            return;
        }
        SetContextData(UE_CIP, (uint)ExceptionData->ExceptionRecord.ExceptionAddress);
    }
    else if(ExceptionData->ExceptionRecord.ExceptionCode==0x406D1388) //SetThreadName exception
    {
        if(ExceptionData->ExceptionRecord.NumberParameters==sizeof(THREADNAME_INFO)/sizeof(uint))
        {
            THREADNAME_INFO nameInfo;
            memcpy(&nameInfo, ExceptionData->ExceptionRecord.ExceptionInformation, sizeof(THREADNAME_INFO));
            if(nameInfo.dwThreadID==-1) //current thread
                nameInfo.dwThreadID=((DEBUG_EVENT*)GetDebugData())->dwThreadId;
            if(nameInfo.dwType==0x1000 and nameInfo.dwFlags==0 and threadisvalid(nameInfo.dwThreadID)) //passed basic checks
            {
                char* ThreadName=(char*)emalloc(MAX_THREAD_NAME_SIZE, "cbException:ThreadName");
                memset(ThreadName, 0, MAX_THREAD_NAME_SIZE);
                if(memread(fdProcessInfo->hProcess, nameInfo.szName, ThreadName, MAX_THREAD_NAME_SIZE-1, 0))
                {
                    int len=(int)strlen(ThreadName);
                    int escape_count=0;
                    for(int i=0; i<len; i++)
                        if(ThreadName[i]=='\\' or ThreadName[i]=='\"' or !isprint(ThreadName[i]))
                            escape_count++;
                    char* ThreadNameEscaped=(char*)emalloc(len+escape_count*3+1, "cbException:ThreadNameEscaped");
                    memset(ThreadNameEscaped, 0, len+escape_count*3+1);
                    for(int i=0,j=0; i<len; i++)
                    {
                        switch(ThreadName[i])
                        {
                        case '\t':
                            j+=sprintf(ThreadNameEscaped+j, "\\t");
                            break;
                        case '\f':
                            j+=sprintf(ThreadNameEscaped+j, "\\f");
                            break;
                        case '\v':
                            j+=sprintf(ThreadNameEscaped+j, "\\v");
                            break;
                        case '\n':
                            j+=sprintf(ThreadNameEscaped+j, "\\n");
                            break;
                        case '\r':
                            j+=sprintf(ThreadNameEscaped+j, "\\r");
                            break;
                        case '\\':
                            j+=sprintf(ThreadNameEscaped+j, "\\\\");
                            break;
                        case '\"':
                            j+=sprintf(ThreadNameEscaped+j, "\\\"");
                            break;
                        default:
                            if(!isprint(ThreadName[i])) //unknown unprintable character
                                j+=sprintf(ThreadNameEscaped+j, "\\%.2x", ThreadName[i]);
                            else
                                j+=sprintf(ThreadNameEscaped+j, "%c", ThreadName[i]);
                            break;
                        }
                    }
                    dprintf("SetThreadName(%X, \"%s\")\n", nameInfo.dwThreadID, ThreadNameEscaped);
                    threadsetname(nameInfo.dwThreadID, ThreadNameEscaped);
                    efree(ThreadNameEscaped, "cbException:ThreadNameEscaped");
                }
                efree(ThreadName, "cbException:ThreadName");
            }
        }
    }
    const char* exceptionName = 0;
    if(exceptionNames.count(ExceptionCode))
        exceptionName = exceptionNames[ExceptionCode];
    if(ExceptionData->dwFirstChance) //first chance exception
    {
        if(exceptionName)
            dprintf("first chance exception on "fhex" (%.8X, %s)!\n", addr, ExceptionCode, exceptionName);
        else
            dprintf("first chance exception on "fhex" (%.8X)!\n", addr, ExceptionCode);
        SetNextDbgContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
        if(bSkipExceptions || dbgisignoredexception(ExceptionCode))
            return;
    }
    else //lock the exception
    {
        if(exceptionName)
            dprintf("last chance exception on "fhex" (%.8X, %s)!\n", addr, ExceptionCode, exceptionName);
        else
            dprintf("last chance exception on "fhex" (%.8X)!\n", addr, ExceptionCode);
        SetNextDbgContinueStatus(DBG_CONTINUE);
    }

    DebugUpdateGui(GetContextData(UE_CIP), true);
    GuiSetDebugState(paused);
    //lock
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    bSkipExceptions=false;
    PLUG_CB_PAUSEDEBUG pauseInfo;
    pauseInfo.reserved=0;
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    plugincbcall(CB_EXCEPTION, &callbackInfo);
    wait(WAITID_RUN);
}

static void cbDebugEvent(DEBUG_EVENT* DebugEvent)
{
    PLUG_CB_DEBUGEVENT debugEventInfo;
    debugEventInfo.DebugEvent=DebugEvent;
    plugincbcall(CB_DEBUGEVENT, &debugEventInfo);
}

static DWORD WINAPI threadDebugLoop(void* lpParameter)
{
    lock(WAITID_STOP); //we are running
    //initialize
    bIsAttached=false;
    bSkipExceptions=false;
    bBreakOnNextDll=false;
    INIT_STRUCT* init=(INIT_STRUCT*)lpParameter;
    bFileIsDll=IsFileDLL(init->exe, 0);
    pDebuggedEntry=GetPE32Data(init->exe, 0, UE_OEP);
    strcpy(szFileName, init->exe);
    if(bFileIsDll)
        fdProcessInfo=(PROCESS_INFORMATION*)InitDLLDebug(init->exe, false, init->commandline, init->currentfolder, 0);
    else
        fdProcessInfo=(PROCESS_INFORMATION*)InitDebug(init->exe, init->commandline, init->currentfolder);
    efree(init, "threadDebugLoop:init"); //free init struct
    if(!fdProcessInfo)
    {
        fdProcessInfo=&g_pi;
        dputs("error starting process (invalid pe?)!");
        unlock(WAITID_STOP);
        return 0;
    }
    BOOL wow64=false, mewow64=false;
    if(!IsWow64Process(fdProcessInfo->hProcess, &wow64) or !IsWow64Process(GetCurrentProcess(), &mewow64))
    {
        dputs("IsWow64Process failed!");
        StopDebug();
        unlock(WAITID_STOP);
        return 0;
    }
    if((mewow64 and !wow64) or (!mewow64 and wow64))
    {
#ifdef _WIN64
        dputs("Use x32_dbg to debug this process!");
#else
        dputs("Use x64_dbg to debug this process!");
#endif // _WIN64
        unlock(WAITID_STOP);
        return 0;
    }
    GuiAddRecentFile(szFileName);
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
    SetCustomHandler(UE_CH_DEBUGEVENT, (void*)cbDebugEvent);
    //inform GUI we started without problems
    GuiSetDebugState(initialized);
    //set GUI title
    strcpy(szBaseFileName, szFileName);
    int len=(int)strlen(szBaseFileName);
    while(szBaseFileName[len]!='\\' and len)
        len--;
    if(len)
        strcpy(szBaseFileName, szBaseFileName+len+1);
    GuiUpdateWindowTitle(szBaseFileName);
    //call plugin callback
    PLUG_CB_INITDEBUG initInfo;
    initInfo.szFileName=szFileName;
    plugincbcall(CB_INITDEBUG, &initInfo);
    //run debug loop (returns when process debugging is stopped)
    DebugLoop();
    isDetachedByUser=false;
    //call plugin callback
    PLUG_CB_STOPDEBUG stopInfo;
    stopInfo.reserved=0;
    plugincbcall(CB_STOPDEBUG, &stopInfo);
    //message the user/do final stuff
    RemoveAllBreakPoints(UE_OPTION_REMOVEALL); //remove all breakpoints
    //cleanup
    dbclose();
    modclear();
    threadclear();
    GuiSetDebugState(stopped);
    dputs("debugging stopped!");
    varset("$hp", (uint)0, true);
    varset("$pid", (uint)0, true);
    unlock(WAITID_STOP); //we are done
    return 0;
}

CMDRESULT cbDebugInit(int argc, char* argv[])
{
    if(DbgIsDebugging())
    {
        dputs("already debugging!");
        return STATUS_ERROR;
    }

    static char arg1[deflen]="";
    if(!argget(*argv, arg1, 0, false))
        return STATUS_ERROR;
    if(!FileExists(arg1))
    {
        dputs("file does not exist!");
        return STATUS_ERROR;
    }
    HANDLE hFile=CreateFileA(arg1, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hFile==INVALID_HANDLE_VALUE)
    {
        dputs("could not open file!");
        return STATUS_ERROR;
    }
    GetFileNameFromHandle(hFile, arg1); //get full path of the file
    CloseHandle(hFile);

    static char arg2[deflen]="";
    argget(*argv, arg2, 1, true);
    char* commandline=0;
    if(strlen(arg2))
        commandline=arg2;

    char arg3[deflen]="";
    argget(*argv, arg3, 2, true);

    static char currentfolder[deflen]="";
    strcpy(currentfolder, arg1);
    int len=(int)strlen(currentfolder);
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
    wait(WAITID_STOP); //wait for the debugger to stop
    waitclear(); //clear waiting flags NOTE: thread-unsafe
    CloseHandle(CreateThread(0, 0, threadDebugLoop, init, 0, 0));
    return STATUS_CONTINUE;
}

CMDRESULT cbStopDebug(int argc, char* argv[])
{
    scriptreset(); //reset the currently-loaded script
    StopDebug();
    unlock(WAITID_RUN);
    wait(WAITID_STOP);
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
    PLUG_CB_RESUMEDEBUG callbackInfo;
    callbackInfo.reserved=0;
    plugincbcall(CB_RESUMEDEBUG, &callbackInfo);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugErun(int argc, char* argv[])
{
    if(waitislocked(WAITID_RUN))
        bSkipExceptions=true;
    return cbDebugRun(argc, argv);
}

CMDRESULT cbDebugSetBPXOptions(int argc, char* argv[])
{
    char argtype[deflen]="";
    DWORD type=0;
    if(!argget(*argv, argtype, 0, false))
        return STATUS_ERROR;
    const char* a=0;
    uint setting_type;
    if(strstr(argtype, "long"))
    {
        setting_type=1; //break_int3long
        a="TYPE_LONG_INT3";
        type=UE_BREAKPOINT_LONG_INT3;
    }
    else if(strstr(argtype, "ud2"))
    {
        setting_type=2; //break_ud2
        a="TYPE_UD2";
        type=UE_BREAKPOINT_UD2;
    }
    else if(strstr(argtype, "short"))
    {
        setting_type=0; //break_int3short
        a="TYPE_INT3";
        type=UE_BREAKPOINT_INT3;
    }
    else
    {
        dputs("invalid type specified!");
        return STATUS_ERROR;
    }
    SetBPXOptions(type);
    BridgeSettingSetUint("Engine", "BreakpointType", setting_type);
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
    if(!valfromstring(argaddr, &addr))
    {
        dprintf("invalid addr: \"%s\"\n", argaddr);
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
    if(IsBPXEnabled(addr))
    {
        dprintf("error setting breakpoint at "fhex"!\n (IsBPXEnabled)", addr);
        return STATUS_ERROR;
    }
    else if(!memread(fdProcessInfo->hProcess, (void*)addr, &oldbytes, sizeof(short), 0))
    {
        dprintf("error setting breakpoint at "fhex"!\n (memread)", addr);
        return STATUS_ERROR;
    }
    else if(!bpnew(addr, true, singleshoot, oldbytes, BPNORMAL, type, bpname))
    {
        dprintf("error setting breakpoint at "fhex"!\n (bpnew)", addr);
        return STATUS_ERROR;
    }
    else if(!SetBPX(addr, type, (void*)cbUserBreakpoint))
    {
        dprintf("error setting breakpoint at "fhex"! (SetBPX)\n", addr);
        return STATUS_ERROR;
    }
    dprintf("breakpoint at "fhex" set!\n", addr);
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

static bool cbDeleteAllBreakpoints(const BREAKPOINT* bp)
{
    if(bpdel(bp->addr, BPNORMAL) and (!bp->enabled or DeleteBPX(bp->addr)))
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
        if(!bpdel(found.addr, BPNORMAL))
        {
            dprintf("delete breakpoint failed (bpdel): "fhex"\n", found.addr);
            return STATUS_ERROR;
        }
        else if(found.enabled && !DeleteBPX(found.addr))
        {
            dprintf("delete breakpoint failed (DeleteBPX): "fhex"\n", found.addr);
            GuiUpdateAllViews();
            return STATUS_ERROR;
        }
        return STATUS_CONTINUE;
    }
    uint addr=0;
    if(!valfromstring(arg1, &addr) or !bpget(addr, BPNORMAL, 0, &found)) //invalid breakpoint
    {
        dprintf("no such breakpoint \"%s\"\n", arg1);
        return STATUS_ERROR;
    }
    if(!bpdel(found.addr, BPNORMAL))
    {
        dprintf("delete breakpoint failed (bpdel): "fhex"\n", found.addr);
        return STATUS_ERROR;
    }
    else if(found.enabled && !DeleteBPX(found.addr))
    {
        dprintf("delete breakpoint failed (DeleteBPX): "fhex"\n", found.addr);
        GuiUpdateAllViews();
        return STATUS_ERROR;
    }
    dputs("breakpoint deleted!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

static bool cbEnableAllBreakpoints(const BREAKPOINT* bp)
{
    if(!bpenable(bp->addr, BPNORMAL, true) or !SetBPX(bp->addr, bp->titantype, (void*)cbUserBreakpoint))
    {
        dprintf("could not enable "fhex"\n", bp->addr);
        return false;
    }
    return true;
}

CMDRESULT cbDebugEnableBPX(int argc, char* argv[])
{
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
        if(!bpenable(found.addr, BPNORMAL, true) or !SetBPX(found.addr, found.titantype, (void*)cbUserBreakpoint))
        {
            dprintf("could not enable "fhex"\n", found.addr);
            return STATUS_ERROR;
        }
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    uint addr=0;
    if(!valfromstring(arg1, &addr) or !bpget(addr, BPNORMAL, 0, &found)) //invalid breakpoint
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
    if(!bpenable(found.addr, BPNORMAL, true) or !SetBPX(found.addr, found.titantype, (void*)cbUserBreakpoint))
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
    if(!bpenable(bp->addr, BPNORMAL, false) or !DeleteBPX(bp->addr))
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
        if(!bpenable(found.addr, BPNORMAL, false) or !DeleteBPX(found.addr))
        {
            dprintf("could not disable "fhex"\n", found.addr);
            return STATUS_ERROR;
        }
        GuiUpdateAllViews();
        return STATUS_CONTINUE;
    }
    uint addr=0;
    if(!valfromstring(arg1, &addr) or !bpget(addr, BPNORMAL, 0, &found)) //invalid breakpoint
    {
        dprintf("no such breakpoint \"%s\"\n", arg1);
        return STATUS_ERROR;
    }
    if(!found.enabled)
    {
        dputs("breakpoint already disabled!");
        return STATUS_CONTINUE;
    }
    if(!bpenable(found.addr, BPNORMAL, false) or !DeleteBPX(found.addr))
    {
        dprintf("could not disable "fhex"\n", found.addr);
        return STATUS_ERROR;
    }
    dputs("breakpoint disabled!");
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

CMDRESULT cbDebugeStepInto(int argc, char* argv[])
{
    bSkipExceptions=true;
    return cbDebugStepInto(argc, argv);
}

CMDRESULT cbDebugStepOver(int argc, char* argv[])
{
    StepOver((void*)cbStep);
    isStepping=true;
    return cbDebugRun(argc, argv);
}

CMDRESULT cbDebugeStepOver(int argc, char* argv[])
{
    bSkipExceptions=true;
    return cbDebugStepOver(argc, argv);
}

CMDRESULT cbDebugSingleStep(int argc, char* argv[])
{
    char arg1[deflen]="";
    uint stepcount=1;
    if(argget(*argv, arg1, 0, true))
    {
        if(!valfromstring(arg1, &stepcount))
            stepcount=1;
    }
    SingleStep((DWORD)stepcount, (void*)cbStep);
    isStepping=true;
    return cbDebugRun(argc, argv);
}

CMDRESULT cbDebugeSingleStep(int argc, char* argv[])
{
    bSkipExceptions=true;
    return cbDebugSingleStep(argc, argv);
}

CMDRESULT cbDebugHide(int argc, char* argv[])
{
    if(HideDebugger(fdProcessInfo->hProcess, UE_HIDE_PEBONLY))
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
        if(!valfromstring(arg1, &addr))
            addr=GetContextData(UE_CIP);
    if(!memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return STATUS_CONTINUE;
    DebugUpdateGui(addr, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetMemoryBpx(int argc, char* argv[])
{
    char arg1[deflen]=""; //addr
    if(!argget(*argv, arg1, 0, false))
        return STATUS_ERROR;
    uint addr;
    if(!valfromstring(arg1, &addr))
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
    DWORD type=UE_MEMORY;
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
    uint base=memfindbaseaddr(addr, &size, true);
    bool singleshoot=false;
    if(!restore)
        singleshoot=true;
    if(bpget(base, BPMEMORY, 0, 0))
    {
        dputs("hardware breakpoint already set!");
        return STATUS_CONTINUE;
    }
    if(!bpnew(base, true, singleshoot, 0, BPMEMORY, type, 0) or !SetMemoryBPXEx(base, size, type, restore, (void*)cbMemoryBreakpoint))
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
    memfindbaseaddr(bp->addr, &size);
    if(!bpdel(bp->addr, BPMEMORY) or !RemoveMemoryBPX(bp->addr, size))
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
        memfindbaseaddr(found.addr, &size);
        if(!bpdel(found.addr, BPMEMORY) or !RemoveMemoryBPX(found.addr, size))
        {
            dprintf("delete memory breakpoint failed: "fhex"\n", found.addr);
            return STATUS_ERROR;
        }
        return STATUS_CONTINUE;
    }
    uint addr=0;
    if(!valfromstring(arg1, &addr) or !bpget(addr, BPMEMORY, 0, &found)) //invalid breakpoint
    {
        dprintf("no such memory breakpoint \"%s\"\n", arg1);
        return STATUS_ERROR;
    }
    uint size;
    memfindbaseaddr(found.addr, &size);
    if(!bpdel(found.addr, BPMEMORY) or !RemoveMemoryBPX(found.addr, size))
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

CMDRESULT cbDebugeRtr(int argc, char* argv[])
{
    bSkipExceptions=true;
    return cbDebugRtr(argc, argv);
}

CMDRESULT cbDebugSetHardwareBreakpoint(int argc, char* argv[])
{
    char arg1[deflen]=""; //addr
    if(!argget(*argv, arg1, 0, false))
        return STATUS_ERROR;
    uint addr;
    if(!valfromstring(arg1, &addr))
        return STATUS_ERROR;
    DWORD type=UE_HARDWARE_EXECUTE;
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
        if(!valfromstring(arg3, &size))
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
    int titantype=(drx<<8)|(type<<4)|(DWORD)size;
    //TODO: hwbp in multiple threads TEST
    if(bpget(addr, BPHARDWARE, 0, 0))
    {
        dputs("hardware breakpoint already set!");
        return STATUS_CONTINUE;
    }
    if(!bpnew(addr, true, false, 0, BPHARDWARE, titantype, 0) or !SetHardwareBreakPoint(addr, drx, type, (DWORD)size, (void*)cbHardwareBreakpoint))
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
    if(!bpdel(bp->addr, BPHARDWARE) or !DeleteHardwareBreakPoint((bp->titantype>>8)&0xF))
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
        if(!bpdel(found.addr, BPHARDWARE) or !DeleteHardwareBreakPoint((found.titantype>>8)&0xF))
        {
            dprintf("delete hardware breakpoint failed: "fhex"\n", found.addr);
            return STATUS_ERROR;
        }
        return STATUS_CONTINUE;
    }
    uint addr=0;
    if(!valfromstring(arg1, &addr) or !bpget(addr, BPHARDWARE, 0, &found)) //invalid breakpoint
    {
        dprintf("no such hardware breakpoint \"%s\"\n", arg1);
        return STATUS_ERROR;
    }
    if(!bpdel(found.addr, BPHARDWARE) or !DeleteHardwareBreakPoint((found.titantype>>8)&0xF))
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
        if(!valfromstring(arg1, &size, false))
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
        if(!valfromstring(arg1, &addr, false))
            return STATUS_ERROR;
    }
    else if(!lastalloc)
    {
        dputs("lastalloc is zero, provide a page address");
        return STATUS_ERROR;
    }
    if(addr==lastalloc)
        varset("$lastalloc", (uint)0, true);
    bool ok=!!VirtualFreeEx(fdProcessInfo->hProcess, (void*)addr, 0, MEM_RELEASE);
    if(!ok)
        dputs("VirtualFreeEx failed");
    varset("$res", ok, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugMemset(int argc, char* argv[])
{
    char arg3[deflen]=""; //size
    uint addr;
    uint value;
    uint size;
    if(argc<3)
    {
        dputs("not enough arguments");
        return STATUS_ERROR;
    }
    if(!valfromstring(argv[1], &addr, false) or !valfromstring(argv[2], &value, false))
        return STATUS_ERROR;
    if(argget(*argv, arg3, 2, true))
    {
        if(!valfromstring(arg3, &size, false))
            return STATUS_ERROR;
    }
    else
    {
        uint base=memfindbaseaddr(addr, &size, true);
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
    uint addr=memfindbaseaddr(GetContextData(UE_CIP), 0);
    DWORD ticks=GetTickCount();
    char comment[MAX_COMMENT_SIZE]="";
    for(uint i=addr; i<addr+100000; i++)
    {
        commentset(i, "test", false);
        labelset(i, "test", false);
        bookmarkset(i, false);
        functionadd(i, i, false);
    }
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
    if(argc<2)
    {
        dputs("not enough arguments");
        return STATUS_ERROR;
    }
    uint addr=0;
    if(!valfromstring(argv[1], &addr, false))
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
        ScyllaStartGui(fdProcessInfo->dwProcessId, (HINSTANCE)pDebuggedBase);
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
    CloseHandle(CreateThread(0, 0, scyllaThread, 0, 0, 0));
    return STATUS_CONTINUE;
}

static void cbAttachDebugger()
{
    varset("$hp", (uint)fdProcessInfo->hProcess, true);
    varset("$pid", fdProcessInfo->dwProcessId, true);
}

static DWORD WINAPI threadAttachLoop(void* lpParameter)
{
    lock(WAITID_STOP);
    bIsAttached=true;
    bSkipExceptions=false;
    DWORD pid=(DWORD)lpParameter;
    static PROCESS_INFORMATION pi_attached;
    fdProcessInfo=&pi_attached;
    //do some init stuff
    bFileIsDll=IsFileDLL(szFileName, 0);
    GuiAddRecentFile(szFileName);
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
    SetCustomHandler(UE_CH_DEBUGEVENT, (void*)cbDebugEvent);
    //inform GUI start we started without problems
    GuiSetDebugState(initialized);
    //set GUI title
    strcpy(szBaseFileName, szFileName);
    int len=(int)strlen(szBaseFileName);
    while(szBaseFileName[len]!='\\' and len)
        len--;
    if(len)
        strcpy(szBaseFileName, szBaseFileName+len+1);
    GuiUpdateWindowTitle(szBaseFileName);
    //call plugin callback (init)
    PLUG_CB_INITDEBUG initInfo;
    initInfo.szFileName=szFileName;
    plugincbcall(CB_INITDEBUG, &initInfo);
    //call plugin callback (attach)
    PLUG_CB_ATTACH attachInfo;
    attachInfo.dwProcessId=(DWORD)pid;
    plugincbcall(CB_ATTACH, &attachInfo);
    //run debug loop (returns when process debugging is stopped)
    AttachDebugger(pid, true, fdProcessInfo, (void*)cbAttachDebugger);
    isDetachedByUser=false;
    //call plugin callback
    PLUG_CB_STOPDEBUG stopInfo;
    stopInfo.reserved=0;
    plugincbcall(CB_STOPDEBUG, &stopInfo);
    //message the user/do final stuff
    RemoveAllBreakPoints(UE_OPTION_REMOVEALL); //remove all breakpoints
    dbclose();
    modclear();
    GuiSetDebugState(stopped);
    dputs("debugging stopped!");
    varset("$hp", (uint)0, true);
    varset("$pid", (uint)0, true);
    unlock(WAITID_STOP);
    waitclear();
    return 0;
}

CMDRESULT cbDebugAttach(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint pid=0;
    if(!valfromstring(argv[1], &pid))
    {
        dprintf("invalid expression \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    if(DbgIsDebugging())
    {
        //TODO: do stuff
        dputs("terminate the current session!");
        return STATUS_ERROR;
    }
    HANDLE hProcess=TitanOpenProcess(PROCESS_ALL_ACCESS, false, (DWORD)pid);
    if(!hProcess)
    {
        dprintf("could not open process %X!\n", pid);
        return STATUS_ERROR;
    }
    BOOL wow64=false, mewow64=false;
    if(!IsWow64Process(hProcess, &wow64) or !IsWow64Process(GetCurrentProcess(), &mewow64))
    {
        dputs("IsWow64Process failed!");
        CloseHandle(hProcess);
        return STATUS_ERROR;
    }
    if((mewow64 and !wow64) or (!mewow64 and wow64))
    {
#ifdef _WIN64
        dputs("Use x32_dbg to debug this process!");
#else
        dputs("Use x64_dbg to debug this process!");
#endif // _WIN64
        CloseHandle(hProcess);
        return STATUS_ERROR;
    }
    if(!GetModuleFileNameExA(hProcess, 0, szFileName, sizeof(szFileName)))
    {
        dprintf("could not get module filename %X!\n", pid);
        CloseHandle(hProcess);
        return STATUS_ERROR;
    }
    CloseHandle(hProcess);
    CloseHandle(CreateThread(0, 0, threadAttachLoop, (void*)pid, 0, 0));
    return STATUS_CONTINUE;
}

static void cbDetach()
{
    if(!isDetachedByUser)
        return;
    PLUG_CB_DETACH detachInfo;
    detachInfo.fdProcessInfo=fdProcessInfo;
    plugincbcall(CB_DETACH, &detachInfo);
    if(!DetachDebuggerEx(fdProcessInfo->dwProcessId))
        dputs("DetachDebuggerEx failed...");
    else
        dputs("detached!");
    return;
}

CMDRESULT cbDebugDetach(int argc, char* argv[])
{
    unlock(WAITID_RUN); //run
    isDetachedByUser=true; //detach when paused
    StepInto((void*)cbDetach);
    DebugBreakProcess(fdProcessInfo->hProcess);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDump(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr=0;
    if(!valfromstring(argv[1], &addr))
    {
        dprintf("invalid address \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    GuiDumpAt(addr);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugStackDump(int argc, char* argv[])
{
    duint addr=0;
    if(argc<2)
        addr=GetContextData(UE_CSP);
    else if(!valfromstring(argv[1], &addr))
    {
        dprintf("invalid address \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    duint csp=GetContextData(UE_CSP);
    duint size=0;
    duint base=memfindbaseaddr(csp, &size);
    if(base && addr>=base && addr<(base+size))
        GuiStackDumpAt(addr, csp);
    else
        dputs("invalid stack address!");
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugContinue(int argc, char* argv[])
{
    if(argc<2)
    {
        SetNextDbgContinueStatus(DBG_CONTINUE);
        dputs("exception will be swallowed");
    }
    else
    {
        SetNextDbgContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
        dputs("exception will be thrown in the program");
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbBpDll(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    DWORD type=UE_ON_LIB_ALL;
    if(argc>2)
    {
        switch(*argv[2])
        {
        case 'l':
            type=UE_ON_LIB_LOAD;
            break;
        case 'u':
            type=UE_ON_LIB_UNLOAD;
            break;
        }
    }
    bool singleshoot=true;
    if(argc>3)
        singleshoot=false;
    LibrarianSetBreakPoint(argv[1], type, singleshoot, (void*)cbLibrarianBreakpoint);
    dprintf("dll breakpoint set on \"%s\"!\n", argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbBcDll(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments");
        return STATUS_ERROR;
    }
    if(!LibrarianRemoveBreakPoint(argv[1], UE_ON_LIB_ALL))
    {
        dputs("failed to remove dll breakpoint...");
        return STATUS_ERROR;
    }
    dputs("dll breakpoint removed!");
    return STATUS_CONTINUE;
}
