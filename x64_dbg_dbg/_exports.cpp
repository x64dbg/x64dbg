#include "_exports.h"
#include "memory.h"
#include "debugger.h"
#include "value.h"
#include "addrinfo.h"
#include "console.h"
#include "threading.h"
#include "breakpoint.h"
#include "disasm_helper.h"
#include "simplescript.h"
#include "symbolinfo.h"
#include "assemble.h"
#include "stackinfo.h"

extern "C" DLL_EXPORT duint _dbg_memfindbaseaddr(duint addr, duint* size)
{
    return memfindbaseaddr(fdProcessInfo->hProcess, addr, size);
}

extern "C" DLL_EXPORT bool _dbg_memread(duint addr, unsigned char* dest, duint size, duint* read)
{
    bool ret=memread(fdProcessInfo->hProcess, (void*)addr, dest, size, read);
    if(!ret)
        return false;
    bpfixmemory(addr, dest, size);
    return true;
}

extern "C" DLL_EXPORT bool _dbg_memmap(MEMMAP* memmap)
{
    memset(memmap, 0, sizeof(MEMMAP));

    MEMORY_BASIC_INFORMATION mbi;
    DWORD numBytes;
    uint MyAddress=0, newAddress=0;
    SymInitialize(fdProcessInfo->hProcess, 0, true);
    std::vector<MEMPAGE> pageVector;
    do
    {
        numBytes=VirtualQueryEx(fdProcessInfo->hProcess, (LPCVOID)MyAddress, &mbi, sizeof(mbi));
        if(mbi.State==MEM_COMMIT)
        {
            MEMPAGE curPage;
            *curPage.mod=0;
            modnamefromaddr(MyAddress, curPage.mod, true);
            memcpy(&curPage.mbi, &mbi, sizeof(mbi));
            pageVector.push_back(curPage);
            memmap->count++;
        }
        newAddress=(uint)mbi.BaseAddress+mbi.RegionSize;
        if(newAddress<=MyAddress)
            numBytes=0;
        else
            MyAddress=newAddress;
    }
    while(numBytes);

    //process vector
    int pagecount=memmap->count;
    memmap->page=(MEMPAGE*)BridgeAlloc(sizeof(MEMPAGE)*pagecount);
    memset(memmap->page, 0, sizeof(MEMPAGE)*pagecount);
    for(int i=0; i<pagecount; i++)
        memcpy(&memmap->page[i], &pageVector.at(i), sizeof(MEMPAGE));

    return true;
}

extern "C" DLL_EXPORT bool _dbg_memisvalidreadptr(duint addr)
{
    return memisvalidreadptr(fdProcessInfo->hProcess, addr);
}

extern "C" DLL_EXPORT bool _dbg_valfromstring(const char* string, duint* value)
{
    return valfromstring(string, value);
}

extern "C" DLL_EXPORT bool _dbg_isdebugging()
{
    return IsFileBeingDebugged();
}

extern "C" DLL_EXPORT bool _dbg_isjumpgoingtoexecute(duint addr)
{
    static unsigned int cacheFlags;
    static uint cacheAddr;
    static bool cacheResult;
    if(cacheAddr!=addr or cacheFlags!=GetContextData(UE_EFLAGS))
    {
        cacheFlags=GetContextData(UE_EFLAGS);
        cacheAddr=addr;
        cacheResult=IsJumpGoingToExecuteEx(fdProcessInfo->hProcess, fdProcessInfo->hThread, (ULONG_PTR)cacheAddr, cacheFlags);
    }
    return cacheResult;
}

extern "C" DLL_EXPORT bool _dbg_addrinfoget(duint addr, SEGMENTREG segment, ADDRINFO* addrinfo)
{
    bool retval=false;
    if(addrinfo->flags&flagmodule) //get module
    {
        char module[64]="";
        if(modnamefromaddr(addr, module, false) and strlen(module)<MAX_MODULE_SIZE) //get module name
        {
            strcpy(addrinfo->module, module);
            retval=true;
        }
    }
    if(addrinfo->flags&flaglabel)
    {
        if(labelget(addr, addrinfo->label))
            retval=true;
        else //no user labels
        {
            //TODO: auto-labels
            DWORD64 displacement=0;
            char buffer[sizeof(SYMBOL_INFO) + MAX_LABEL_SIZE * sizeof(char)];
            PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_LABEL_SIZE;
            if(SymFromAddr(fdProcessInfo->hProcess, (DWORD64)addr, &displacement, pSymbol) and !displacement)
            {
                //TODO: user preference
                if(!UnDecorateSymbolName(pSymbol->Name, addrinfo->label, MAX_LABEL_SIZE, UNDNAME_COMPLETE))
                    strcpy(addrinfo->label, pSymbol->Name);
                retval=true;
            }
        }
    }
    if(addrinfo->flags&flagcomment)
    {
        *addrinfo->comment=0;
        if(commentget(addr, addrinfo->comment))
            retval=true;
        else //TODO: auto-comments
        {
            DWORD dwDisplacement;
            IMAGEHLP_LINE64 line;
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            if(SymGetLineFromAddr64(fdProcessInfo->hProcess, (DWORD64)addr, &dwDisplacement, &line) and !dwDisplacement)
            {
                char filename[deflen]="";
                strcpy(filename, line.FileName);
                int len=strlen(filename);
                while(filename[len]!='\\' and len!=0)
                    len--;
                if(len)
                    len++;
                sprintf(addrinfo->comment, "%s:%u", filename+len, line.LineNumber);
                retval=true;
            }
            else //no line number
            {
                DISASM_INSTR instr;
                disasmget(addr, &instr);
                int len_left=MAX_COMMENT_SIZE;
                for(int i=0,j=0; i<instr.argcount; i++)
                {
                    char temp_string[MAX_COMMENT_SIZE*2]="";
                    ADDRINFO newinfo;
                    memset(&newinfo, 0, sizeof(ADDRINFO));
                    newinfo.flags=flaglabel;
                    char ascii[256]="";
                    char unicode[256]="";
                    STRING_TYPE strtype=str_none;
                    if(instr.arg[i].constant==instr.arg[i].value) //avoid: call <module.label> ; addr:label
                    {
                        if(instr.type==instr_branch or !disasmgetstringat(instr.arg[i].constant, &strtype, ascii, unicode, len_left) or strtype==str_none)
                            continue;
                        switch(strtype)
                        {
                        case str_none:
                            break;
                        case str_ascii:
                            len_left-=sprintf(temp_string, "%s:\"%s\"", instr.arg[i].mnemonic, ascii);
                            if(len_left<0)
                                temp_string[MAX_COMMENT_SIZE]=0;
                            break;
                        case str_unicode:
                            len_left-=sprintf(temp_string, "%s:L\"%s\"", instr.arg[i].mnemonic, unicode);
                            if(len_left<0)
                                temp_string[MAX_COMMENT_SIZE]=0;
                            break;
                        }
                    }
                    else if(instr.arg[i].memvalue and (disasmgetstringat(instr.arg[i].memvalue, &strtype, ascii, unicode, len_left) or _dbg_addrinfoget(instr.arg[i].memvalue, instr.arg[i].segment, &newinfo)))
                    {
                        switch(strtype)
                        {
                        case str_none:
                            if(*newinfo.label)
                                len_left-=sprintf(temp_string, "[%s]:%s", instr.arg[i].mnemonic, newinfo.label);
                            if(len_left<0)
                                temp_string[MAX_COMMENT_SIZE]=0;
                            break;
                        case str_ascii:
                            len_left-=sprintf(temp_string, "[%s]:\"%s\"", instr.arg[i].mnemonic, ascii);
                            if(len_left<0)
                                temp_string[MAX_COMMENT_SIZE]=0;
                            break;
                        case str_unicode:
                            len_left-=sprintf(temp_string, "[%s]:L\"%s\"", instr.arg[i].mnemonic, unicode);
                            if(len_left<0)
                                temp_string[MAX_COMMENT_SIZE]=0;
                            break;
                        }
                    }
                    else if(instr.arg[i].value and (disasmgetstringat(instr.arg[i].value, &strtype, ascii, unicode, len_left) or _dbg_addrinfoget(instr.arg[i].value, instr.arg[i].segment, &newinfo)))
                    {
                        if(instr.type!=instr_normal) //stack/jumps (eg add esp,4 or jmp 401110) cannot directly point to strings
                            strtype=str_none;
                        switch(strtype)
                        {
                        case str_none:
                            if(*newinfo.label)
                                len_left-=sprintf(temp_string, "%s:%s", instr.arg[i].mnemonic, newinfo.label);
                            if(len_left<0)
                                temp_string[MAX_COMMENT_SIZE]=0;
                            break;
                        case str_ascii:
                            len_left-=sprintf(temp_string, "%s:\"%s\"", instr.arg[i].mnemonic, ascii);
                            if(len_left<0)
                                temp_string[MAX_COMMENT_SIZE]=0;
                            break;
                        case str_unicode:
                            len_left-=sprintf(temp_string, "%s:L\"%s\"", instr.arg[i].mnemonic, unicode);
                            if(len_left<0)
                                temp_string[MAX_COMMENT_SIZE]=0;
                            break;
                        }
                    }
                    else
                        continue;
                    if(!strstr(addrinfo->comment, temp_string))
                    {
                        if(j)
                            j+=sprintf(addrinfo->comment+j, ", %s", temp_string);
                        else
                            j+=sprintf(addrinfo->comment+j, "%s", temp_string);
                        retval=true;
                    }
                }
            }
        }
    }
    if(addrinfo->flags&flagbookmark)
    {
        addrinfo->isbookmark=bookmarkget(addr);
        retval=true;
    }
    if(addrinfo->flags&flagfunction)
    {
        if(functionget(addr, &addrinfo->function.start, &addrinfo->function.end))
            retval=true;
    }
    return retval;
}

extern "C" DLL_EXPORT bool _dbg_addrinfoset(duint addr, ADDRINFO* addrinfo)
{
    bool retval=false;
    if(addrinfo->flags&flaglabel) //set label
    {
        if(labelset(addr, addrinfo->label))
            retval=true;
    }
    if(addrinfo->flags&flagcomment) //set comment
    {
        if(commentset(addr, addrinfo->comment))
            retval=true;
    }
    if(addrinfo->flags&flagbookmark) //set bookmark
    {
        if(addrinfo->isbookmark)
            retval=bookmarkset(addr);
        else
            retval=bookmarkdel(addr);
    }
    return retval;
}

extern "C" DLL_EXPORT int _dbg_bpgettypeat(duint addr)
{
    static uint cacheAddr;
    static int cacheBpCount;
    static int cacheResult;
    int bpcount=bpgetlist(0);
    if(cacheAddr!=addr or cacheBpCount!=bpcount)
    {
        BREAKPOINT bp;
        cacheAddr=addr;
        cacheResult=0;
        cacheBpCount=bpcount;
        if(bpget(addr, BPNORMAL, 0, &bp))
            if(bp.enabled)
                cacheResult|=bp_normal;
        if(bpget(addr, BPHARDWARE, 0, &bp))
            if(bp.enabled)
                cacheResult|=bp_hardware;
        if(bpget(addr, BPMEMORY, 0, &bp))
            if(bp.enabled)
                cacheResult|=bp_memory;
    }
    return cacheResult;
}

extern "C" DLL_EXPORT bool _dbg_getregdump(REGDUMP* regdump)
{
    if(!IsFileBeingDebugged())
    {
        memset(regdump, 0, sizeof(REGDUMP));
        return true;
    }
    REGDUMP r;
#ifdef _WIN64
    r.cax=GetContextData(UE_RAX);
#else
    r.cax=(duint)GetContextData(UE_EAX);
#endif // _WIN64
#ifdef _WIN64
    r.ccx=GetContextData(UE_RCX);
#else
    r.ccx=(duint)GetContextData(UE_ECX);
#endif // _WIN64
#ifdef _WIN64
    r.cdx=GetContextData(UE_RDX);
#else
    r.cdx=(duint)GetContextData(UE_EDX);
#endif // _WIN64
#ifdef _WIN64
    r.cbx=GetContextData(UE_RBX);
#else
    r.cbx=(duint)GetContextData(UE_EBX);
#endif // _WIN64
#ifdef _WIN64
    r.cbp=GetContextData(UE_RBP);
#else
    r.cbp=(duint)GetContextData(UE_EBP);
#endif // _WIN64
#ifdef _WIN64
    r.csi=GetContextData(UE_RSI);
#else
    r.csi=(duint)GetContextData(UE_ESI);
#endif // _WIN64
#ifdef _WIN64
    r.cdi=GetContextData(UE_RDI);
#else
    r.cdi=(duint)GetContextData(UE_EDI);
#endif // _WIN64
#ifdef _WIN64
    r.r8=GetContextData(UE_R8);
#endif // _WIN64
#ifdef _WIN64
    r.r9=GetContextData(UE_R9);
#endif // _WIN64
#ifdef _WIN64
    r.r10=GetContextData(UE_R10);
#endif // _WIN64
#ifdef _WIN64
    r.r11=GetContextData(UE_R11);
#endif // _WIN64
#ifdef _WIN64
    r.r12=GetContextData(UE_R12);
#endif // _WIN64
#ifdef _WIN64
    r.r13=GetContextData(UE_R13);
#endif // _WIN64
#ifdef _WIN64
    r.r14=GetContextData(UE_R14);
#endif // _WIN64
#ifdef _WIN64
    r.r15=GetContextData(UE_R15);
#endif // _WIN64
    r.csp=(duint)GetContextData(UE_CSP);
    r.cip=(duint)GetContextData(UE_CIP);
    r.eflags=(duint)GetContextData(UE_EFLAGS);
    r.gs=(unsigned short)(GetContextData(UE_SEG_GS)&0xFFFF);
    r.fs=(unsigned short)(GetContextData(UE_SEG_FS)&0xFFFF);
    r.es=(unsigned short)(GetContextData(UE_SEG_ES)&0xFFFF);
    r.ds=(unsigned short)(GetContextData(UE_SEG_DS)&0xFFFF);
    r.cs=(unsigned short)(GetContextData(UE_SEG_CS)&0xFFFF);
    r.ss=(unsigned short)(GetContextData(UE_SEG_SS)&0xFFFF);
    r.dr0=(duint)GetContextData(UE_DR0);
    r.dr1=(duint)GetContextData(UE_DR1);
    r.dr2=(duint)GetContextData(UE_DR2);
    r.dr3=(duint)GetContextData(UE_DR3);
    r.dr6=(duint)GetContextData(UE_DR6);
    r.dr7=(duint)GetContextData(UE_DR7);
    duint cflags=r.eflags;
    r.flags.c=valflagfromstring(cflags, "cf");
    r.flags.p=valflagfromstring(cflags, "pf");
    r.flags.a=valflagfromstring(cflags, "af");
    r.flags.z=valflagfromstring(cflags, "zf");
    r.flags.s=valflagfromstring(cflags, "sf");
    r.flags.t=valflagfromstring(cflags, "tf");
    r.flags.i=valflagfromstring(cflags, "if");
    r.flags.d=valflagfromstring(cflags, "df");
    r.flags.o=valflagfromstring(cflags, "of");
    memcpy(regdump, &r, sizeof(REGDUMP));
    return true;
}

extern "C" DLL_EXPORT bool _dbg_valtostring(const char* string, duint* value)
{
    return valtostring(string, value, true);
}

extern "C" DLL_EXPORT int _dbg_getbplist(BPXTYPE type, BPMAP* bpmap)
{
    if(!bpmap)
        return 0;
    BREAKPOINT* list;
    int bpcount=bpgetlist(&list);
    if(bpcount==0)
    {
        bpmap->count=0;
        return 0;
    }

    int retcount=0;
    std::vector<BRIDGEBP> bridgeList;
    BRIDGEBP curBp;
    unsigned short slot=0;
    for(int i=0; i<bpcount; i++)
    {
        memset(&curBp, 0, sizeof(BRIDGEBP));
        switch(type)
        {
        case bp_none: //all types
            break;
        case bp_normal: //normal
            if(list[i].type!=BPNORMAL)
                continue;
            break;
        case bp_hardware: //hardware
            if(list[i].type!=BPHARDWARE)
                continue;
            break;
        case bp_memory: //memory
            if(list[i].type!=BPMEMORY)
                continue;
            break;
        default:
            return 0;
        }
        switch(list[i].type)
        {
        case BPNORMAL:
            curBp.type=bp_normal;
            break;
        case BPHARDWARE:
            curBp.type=bp_hardware;
            break;
        case BPMEMORY:
            curBp.type=bp_memory;
            break;
        }
        switch(((DWORD)list[i].titantype)>>8)
        {
        case UE_DR0:
            slot=0;
            break;
        case UE_DR1:
            slot=1;
            break;
        case UE_DR2:
            slot=2;
            break;
        case UE_DR3:
            slot=3;
            break;
        }
        curBp.addr=list[i].addr;
        curBp.enabled=list[i].enabled;
        //TODO: fix this
        if(memisvalidreadptr(fdProcessInfo->hProcess, curBp.addr))
            curBp.active=true;
        strcpy(curBp.mod, list[i].mod);
        int len=strlen(curBp.mod);
        while(curBp.mod[len]!='.' and len)
            len--;
        if(len)
            curBp.mod[len]=0;
        strcpy(curBp.name, list[i].name);
        curBp.singleshoot=list[i].singleshoot;
        curBp.slot=slot;
        bridgeList.push_back(curBp);
        retcount++;
    }
    if(!retcount)
    {
        bpmap->count=retcount;
        return retcount;
    }
    bpmap->count=retcount;
    bpmap->bp=(BRIDGEBP*)BridgeAlloc(sizeof(BRIDGEBP)*retcount);
    for(int i=0; i<retcount; i++)
        memcpy(&bpmap->bp[i], &bridgeList.at(i), sizeof(BRIDGEBP));
    return retcount;
}

extern "C" DLL_EXPORT uint _dbg_getbranchdestination(uint addr)
{
    DISASM_INSTR instr;
    disasmget(addr, &instr);
    if(instr.type!=instr_branch)
        return 0;
    if(strstr(instr.instruction, "ret"))
    {
        uint atcsp=DbgValFromString("@csp");
        if(DbgMemIsValidReadPtr(atcsp))
            return atcsp;
        else
            return 0;
    }
    else if(instr.arg[0].type==arg_memory)
        return instr.arg[0].memvalue;
    else
        return instr.arg[0].value;
}

extern "C" DLL_EXPORT bool _dbg_functionoverlaps(uint start, uint end)
{
    return functionoverlaps(start, end);
}

extern "C" DLL_EXPORT uint _dbg_sendmessage(DBGMSG type, void* param1, void* param2)
{
    switch(type)
    {
    case DBG_SCRIPT_LOAD:
    {
        scriptload((const char*)param1);
    }
    break;

    case DBG_SCRIPT_UNLOAD:
    {
        scriptunload();
    }
    break;

    case DBG_SCRIPT_RUN:
    {
        scriptrun((int)(duint)param1);
    }
    break;

    case DBG_SCRIPT_STEP:
    {
        scriptstep();
    }
    break;

    case DBG_SCRIPT_BPTOGGLE:
    {
        return scriptbptoggle((int)(duint)param1);
    }
    break;

    case DBG_SCRIPT_BPGET:
    {
        return scriptbpget((int)(duint)param1);
    }
    break;

    case DBG_SCRIPT_CMDEXEC:
    {
        return scriptcmdexec((const char*)param1);
    }
    break;

    case DBG_SCRIPT_ABORT:
    {
        scriptabort();
    }
    break;

    case DBG_SCRIPT_GETLINETYPE:
    {
        return (duint)scriptgetlinetype((int)(duint)param1);
    }
    break;

    case DBG_SCRIPT_SETIP:
    {
        scriptsetip((int)(duint)param1);
    }
    break;

    case DBG_SYMBOL_ENUM:
    {
        SYMBOLCBINFO* cbInfo=(SYMBOLCBINFO*)param1;
        symbolenum(cbInfo->base, cbInfo->cbSymbolEnum, cbInfo->user);
    }
    break;

    case DBG_ASSEMBLE_AT:
    {
        return assembleat((duint)param1, (const char*)param2);
    }
    break;

    case DBG_MODBASE_FROM_NAME:
    {
        return modbasefromname((const char*)param1);
    }
    break;

    case DBG_DISASM_AT:
    {
        disasmget((uint)param1, (DISASM_INSTR*)param2);
    }
    break;

    case DBG_STACK_COMMENT_GET:
    {
        return stackcommentget((uint)param1, (STACK_COMMENT*)param2);
    }
    break;
    }
    return 0;
}
