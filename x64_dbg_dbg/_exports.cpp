#include "_exports.h"
#include "memory.h"
#include "debugger.h"
#include "value.h"
#include "addrinfo.h"

extern "C" DLL_EXPORT duint _dbg_memfindbaseaddr(duint addr, duint* size)
{
    return memfindbaseaddr(fdProcessInfo->hProcess, addr, size);
}

extern "C" DLL_EXPORT bool _dbg_memread(duint addr, unsigned char* dest, duint size, duint* read)
{
    dbgdisablebpx();
    bool res=memread(fdProcessInfo->hProcess, (void*)addr, dest, size, read);
    dbgenablebpx();
    return res;
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
            IMAGEHLP_MODULE64 nfo;
            nfo.SizeOfStruct=sizeof(IMAGEHLP_MODULE64);
            if(SymGetModuleInfo64(fdProcessInfo->hProcess, MyAddress, &nfo))
                memcpy(curPage.mod, nfo.ModuleName, sizeof(curPage.mod));
            else
                memset(curPage.mod, 0, sizeof(curPage.mod));
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

extern "C" DLL_EXPORT void _dbg_dbgexitsignal()
{
    //TODO: handle exit signal
    cbStopDebug("");
    Sleep(200);
    DeleteFileA("DLLLoader.exe");
}

extern "C" DLL_EXPORT bool _dbg_valfromstring(const char* string, duint* value)
{
    return valfromstring(string, value, 0, 0, true, 0);
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
    if(addrinfo->flags&module) //get module
    {
        char module[64]="";
        if(modnamefromaddr(addr, module) and strlen(module)<32) //get module name
        {
            strcpy(addrinfo->module, module);
            retval=true;
        }
    }
    if(addrinfo->flags&label) //TODO: get label
    {
        //TODO: label exports
    }
    if(addrinfo->flags&comment) //TODO: get comment
    {
        //TODO: auto-comments
    }
    return retval;
}

extern "C" DLL_EXPORT bool _dbg_addrinfoset(duint addr, ADDRINFO* addrinfo)
{
    return false;
}

extern "C" DLL_EXPORT BPXTYPE _dbg_bpgettypeat(duint addr)
{
    BREAKPOINT* found=bpfind(bplist, 0, addr, 0, BPNOTYPE);
    if(!found or !found->enabled) //none found or disabled
        return bpnone;
    switch(found->type)
    {
    case BPNORMAL:
    case BPSINGLESHOOT:
        return bpnormal;
    case BPHARDWARE:
        return bphardware;
    case BPMEMORY:
        return bpmemory;
    default:
        break;
    }
    return bpnone;
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
