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
#include "thread.h"
#include "disasm_fast.h"
#include "plugin_loader.h"
#include "_dbgfunctions.h"

static bool bOnlyCipAutoComments = false;

extern "C" DLL_EXPORT duint _dbg_memfindbaseaddr(duint addr, duint* size)
{
    return memfindbaseaddr(addr, size);
}

extern "C" DLL_EXPORT bool _dbg_memread(duint addr, unsigned char* dest, duint size, duint* read)
{
    return memread(fdProcessInfo->hProcess, (void*)addr, dest, size, read);
}

extern "C" DLL_EXPORT bool _dbg_memwrite(duint addr, const unsigned char* src, duint size, duint* written)
{
    return memwrite(fdProcessInfo->hProcess, (void*)addr, src, size, written);
}

extern "C" DLL_EXPORT bool _dbg_memmap(MEMMAP* memmap)
{
    CriticalSectionLocker locker(LockMemoryPages);
    int pagecount = (int)memoryPages.size();
    memset(memmap, 0, sizeof(MEMMAP));
    memmap->count = pagecount;
    if(!pagecount)
        return true;
    memmap->page = (MEMPAGE*)BridgeAlloc(sizeof(MEMPAGE) * pagecount);
    memset(memmap->page, 0, sizeof(MEMPAGE)*pagecount);
    int j = 0;
    for(MemoryMap::iterator i = memoryPages.begin(); i != memoryPages.end(); ++i, j++)
        memcpy(&memmap->page[j], &i->second, sizeof(MEMPAGE));
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
    if(IsFileBeingDebugged())
        return true;
    return false;
}

extern "C" DLL_EXPORT bool _dbg_isjumpgoingtoexecute(duint addr)
{
    static uint cacheFlags;
    static uint cacheAddr;
    static bool cacheResult;
    if(cacheAddr != addr or cacheFlags != GetContextDataEx(hActiveThread, UE_EFLAGS))
    {
        cacheFlags = GetContextDataEx(hActiveThread, UE_EFLAGS);
        cacheAddr = addr;
        cacheResult = IsJumpGoingToExecuteEx(fdProcessInfo->hProcess, fdProcessInfo->hThread, (ULONG_PTR)cacheAddr, cacheFlags);
    }
    return cacheResult;
}

extern "C" DLL_EXPORT bool _dbg_addrinfoget(duint addr, SEGMENTREG segment, ADDRINFO* addrinfo)
{
    if(!DbgIsDebugging())
        return false;
    bool retval = false;
    if(addrinfo->flags & flagmodule) //get module
    {
        if(modnamefromaddr(addr, addrinfo->module, false)) //get module name
            retval = true;
    }
    if(addrinfo->flags & flaglabel)
    {
        if(labelget(addr, addrinfo->label))
            retval = true;
        else //no user labels
        {
            DWORD64 displacement = 0;
            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)];
            PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_LABEL_SIZE;
            if(SymFromAddr(fdProcessInfo->hProcess, (DWORD64)addr, &displacement, pSymbol) and !displacement)
            {
                pSymbol->Name[pSymbol->MaxNameLen - 1] = '\0';
                if(!bUndecorateSymbolNames or !UnDecorateSymbolName(pSymbol->Name, addrinfo->label, MAX_LABEL_SIZE, UNDNAME_COMPLETE))
                    strcpy_s(addrinfo->label, pSymbol->Name);
                retval = true;
            }
            if(!retval) //search for CALL <jmp.&user32.MessageBoxA>
            {
                BASIC_INSTRUCTION_INFO basicinfo;
                memset(&basicinfo, 0, sizeof(BASIC_INSTRUCTION_INFO));
                if(disasmfast(addr, &basicinfo) && basicinfo.branch && !basicinfo.call && basicinfo.memory.value) //thing is a JMP
                {
                    uint val = 0;
                    if(memread(fdProcessInfo->hProcess, (const void*)basicinfo.memory.value, &val, sizeof(val), 0))
                    {
                        if(SymFromAddr(fdProcessInfo->hProcess, (DWORD64)val, &displacement, pSymbol) and !displacement)
                        {
                            pSymbol->Name[pSymbol->MaxNameLen - 1] = '\0';
                            if(!bUndecorateSymbolNames or !UnDecorateSymbolName(pSymbol->Name, addrinfo->label, MAX_LABEL_SIZE, UNDNAME_COMPLETE))
                                sprintf_s(addrinfo->label, "JMP.&%s", pSymbol->Name);
                            retval = true;
                        }
                    }
                }
            }
        }
    }
    if(addrinfo->flags & flagbookmark)
    {
        addrinfo->isbookmark = bookmarkget(addr);
        retval = true;
    }
    if(addrinfo->flags & flagfunction)
    {
        if(functionget(addr, &addrinfo->function.start, &addrinfo->function.end))
            retval = true;
    }
    if(addrinfo->flags & flagloop)
    {
        if(loopget(addrinfo->loop.depth, addr, &addrinfo->loop.start, &addrinfo->loop.end))
            retval = true;
    }
    if(addrinfo->flags & flagcomment)
    {
        *addrinfo->comment = 0;
        if(commentget(addr, addrinfo->comment))
            retval = true;
        else
        {
            DWORD dwDisplacement;
            IMAGEHLP_LINE64 line;
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            if(SymGetLineFromAddr64(fdProcessInfo->hProcess, (DWORD64)addr, &dwDisplacement, &line) and !dwDisplacement)
            {
                char filename[deflen] = "";
                strcpy(filename, line.FileName);
                int len = (int)strlen(filename);
                while(filename[len] != '\\' and len != 0)
                    len--;
                if(len)
                    len++;
                sprintf_s(addrinfo->comment, "\1%s:%u", filename + len, line.LineNumber);
                retval = true;
            }
            else if(!bOnlyCipAutoComments || addr == GetContextDataEx(hActiveThread, UE_CIP)) //no line number
            {
                DISASM_INSTR instr;
                String temp_string;
                String comment;
                ADDRINFO newinfo;
                char ascii[256 * 2] = "";
                char unicode[256 * 2] = "";

                memset(&instr, 0, sizeof(DISASM_INSTR));
                disasmget(addr, &instr);
                int len_left = MAX_COMMENT_SIZE;
                for(int i = 0; i < instr.argcount; i++)
                {
                    memset(&newinfo, 0, sizeof(ADDRINFO));
                    newinfo.flags = flaglabel;

                    STRING_TYPE strtype = str_none;

                    if(instr.arg[i].constant == instr.arg[i].value) //avoid: call <module.label> ; addr:label
                    {
                        if(instr.type == instr_branch or !disasmgetstringat(instr.arg[i].constant, &strtype, ascii, unicode, len_left) or strtype == str_none)
                            continue;
                        switch(strtype)
                        {
                        case str_none:
                            break;
                        case str_ascii:
                            temp_string = instr.arg[i].mnemonic;
                            temp_string.append(":\"");
                            temp_string.append(ascii);
                            temp_string.append("\"");
                            break;
                        case str_unicode:
                            temp_string = instr.arg[i].mnemonic;
                            temp_string.append(":L\"");
                            temp_string.append(unicode);
                            temp_string.append("\"");
                            break;
                        }
                    }
                    else if(instr.arg[i].memvalue and (disasmgetstringat(instr.arg[i].memvalue, &strtype, ascii, unicode, len_left) or _dbg_addrinfoget(instr.arg[i].memvalue, instr.arg[i].segment, &newinfo)))
                    {
                        switch(strtype)
                        {
                        case str_none:
                            if(*newinfo.label)
                            {
                                temp_string = "[";
                                temp_string.append(instr.arg[i].mnemonic);
                                temp_string.append("]:");
                                temp_string.append(newinfo.label);
                            }
                            break;
                        case str_ascii:
                            temp_string = "[";
                            temp_string.append(instr.arg[i].mnemonic);
                            temp_string.append("]:");
                            temp_string.append(ascii);
                            break;
                        case str_unicode:
                            temp_string = "[";
                            temp_string.append(instr.arg[i].mnemonic);
                            temp_string.append("]:");
                            temp_string.append(unicode);
                            break;
                        }
                    }
                    else if(instr.arg[i].value and (disasmgetstringat(instr.arg[i].value, &strtype, ascii, unicode, len_left) or _dbg_addrinfoget(instr.arg[i].value, instr.arg[i].segment, &newinfo)))
                    {
                        if(instr.type != instr_normal) //stack/jumps (eg add esp,4 or jmp 401110) cannot directly point to strings
                            strtype = str_none;
                        switch(strtype)
                        {
                        case str_none:
                            if(*newinfo.label)
                            {
                                temp_string = instr.arg[i].mnemonic;
                                temp_string.append(":");
                                temp_string.append(newinfo.label);
                            }
                            break;
                        case str_ascii:
                            temp_string = instr.arg[i].mnemonic;
                            temp_string.append(":\"");
                            temp_string.append(ascii);
                            temp_string.append("\"");
                            break;
                        case str_unicode:
                            temp_string = instr.arg[i].mnemonic;
                            temp_string.append(":L\"");
                            temp_string.append(unicode);
                            temp_string.append("\"");
                            break;
                        }
                    }
                    else
                        continue;

                    if(!strstr(comment.c_str(), temp_string.c_str()))
                    {
                        if(comment.length())
                            comment.append(", ");
                        comment.append(temp_string);
                        retval = true;
                    }
                }
                comment.resize(MAX_COMMENT_SIZE - 2);
                String fullComment = "\1";
                fullComment += comment;
                strcpy_s(addrinfo->comment, fullComment.c_str());
            }
        }
    }
    return retval;
}

extern "C" DLL_EXPORT bool _dbg_addrinfoset(duint addr, ADDRINFO* addrinfo)
{
    bool retval = false;
    if(addrinfo->flags & flaglabel) //set label
    {
        if(labelset(addr, addrinfo->label, true))
            retval = true;
    }
    if(addrinfo->flags & flagcomment) //set comment
    {
        if(commentset(addr, addrinfo->comment, true))
            retval = true;
    }
    if(addrinfo->flags & flagbookmark) //set bookmark
    {
        if(addrinfo->isbookmark)
            retval = bookmarkset(addr, true);
        else
            retval = bookmarkdel(addr);
    }
    return retval;
}

extern "C" DLL_EXPORT int _dbg_bpgettypeat(duint addr)
{
    static uint cacheAddr;
    static int cacheBpCount;
    static int cacheResult;
    int bpcount = bpgetlist(0);
    if(cacheAddr != addr or cacheBpCount != bpcount)
    {
        BREAKPOINT bp;
        cacheAddr = addr;
        cacheResult = 0;
        cacheBpCount = bpcount;
        if(bpget(addr, BPNORMAL, 0, &bp))
            if(bp.enabled)
                cacheResult |= bp_normal;
        if(bpget(addr, BPHARDWARE, 0, &bp))
            if(bp.enabled)
                cacheResult |= bp_hardware;
        if(bpget(addr, BPMEMORY, 0, &bp))
            if(bp.enabled)
                cacheResult |= bp_memory;
    }
    return cacheResult;
}

static void GetMxCsrFields(MXCSRFIELDS* MxCsrFields, DWORD MxCsr)
{
    MxCsrFields->DAZ = valmxcsrflagfromstring(MxCsr, "DAZ");
    MxCsrFields->DE = valmxcsrflagfromstring(MxCsr, "DE");
    MxCsrFields->FZ = valmxcsrflagfromstring(MxCsr, "FZ");
    MxCsrFields->IE = valmxcsrflagfromstring(MxCsr, "IE");
    MxCsrFields->IM = valmxcsrflagfromstring(MxCsr, "IM");
    MxCsrFields->DM = valmxcsrflagfromstring(MxCsr, "DM");
    MxCsrFields->OE = valmxcsrflagfromstring(MxCsr, "OE");
    MxCsrFields->OM = valmxcsrflagfromstring(MxCsr, "OM");
    MxCsrFields->PE = valmxcsrflagfromstring(MxCsr, "PE");
    MxCsrFields->PM = valmxcsrflagfromstring(MxCsr, "PM");
    MxCsrFields->UE = valmxcsrflagfromstring(MxCsr, "UE");
    MxCsrFields->UM = valmxcsrflagfromstring(MxCsr, "UM");
    MxCsrFields->ZE = valmxcsrflagfromstring(MxCsr, "ZE");
    MxCsrFields->ZM = valmxcsrflagfromstring(MxCsr, "ZM");

    MxCsrFields->RC = valmxcsrfieldfromstring(MxCsr, "RC");
}

static void Getx87ControlWordFields(X87CONTROLWORDFIELDS* x87ControlWordFields, WORD ControlWord)
{
    x87ControlWordFields->DM = valx87controlwordflagfromstring(ControlWord, "DM");
    x87ControlWordFields->IC = valx87controlwordflagfromstring(ControlWord, "IC");
    x87ControlWordFields->IEM = valx87controlwordflagfromstring(ControlWord, "IEM");
    x87ControlWordFields->IM = valx87controlwordflagfromstring(ControlWord, "IM");
    x87ControlWordFields->OM = valx87controlwordflagfromstring(ControlWord, "OM");
    x87ControlWordFields->PM = valx87controlwordflagfromstring(ControlWord, "PM");
    x87ControlWordFields->UM = valx87controlwordflagfromstring(ControlWord, "UM");
    x87ControlWordFields->ZM = valx87controlwordflagfromstring(ControlWord, "ZM");

    x87ControlWordFields->RC = valx87controlwordfieldfromstring(ControlWord, "RC");
    x87ControlWordFields->PC = valx87controlwordfieldfromstring(ControlWord, "PC");
}

static void Getx87StatusWordFields(X87STATUSWORDFIELDS* x87StatusWordFields, WORD StatusWord)
{
    x87StatusWordFields->B = valx87statuswordflagfromstring(StatusWord, "B");
    x87StatusWordFields->C0 = valx87statuswordflagfromstring(StatusWord, "C0");
    x87StatusWordFields->C1 = valx87statuswordflagfromstring(StatusWord, "C1");
    x87StatusWordFields->C2 = valx87statuswordflagfromstring(StatusWord, "C2");
    x87StatusWordFields->C3 = valx87statuswordflagfromstring(StatusWord, "C3");
    x87StatusWordFields->D = valx87statuswordflagfromstring(StatusWord, "D");
    x87StatusWordFields->I = valx87statuswordflagfromstring(StatusWord, "I");
    x87StatusWordFields->IR = valx87statuswordflagfromstring(StatusWord, "IR");
    x87StatusWordFields->O = valx87statuswordflagfromstring(StatusWord, "O");
    x87StatusWordFields->P = valx87statuswordflagfromstring(StatusWord, "P");
    x87StatusWordFields->SF = valx87statuswordflagfromstring(StatusWord, "SF");
    x87StatusWordFields->U = valx87statuswordflagfromstring(StatusWord, "U");
    x87StatusWordFields->Z = valx87statuswordflagfromstring(StatusWord, "Z");

    x87StatusWordFields->TOP = valx87statuswordfieldfromstring(StatusWord, "TOP");
}

static void TranslateTitanFpu(const x87FPU_t* titanfpu, X87FPU* fpu)
{
    fpu->ControlWord = titanfpu->ControlWord;
    fpu->StatusWord = titanfpu->StatusWord;
    fpu->TagWord = titanfpu->TagWord;
    fpu->ErrorOffset = titanfpu->ErrorOffset;
    fpu->ErrorSelector = titanfpu->ErrorSelector;
    fpu->DataOffset = titanfpu->DataOffset;
    fpu->DataSelector = titanfpu->DataSelector;
    fpu->Cr0NpxState = titanfpu->Cr0NpxState;
}

static void TranslateTitanContextToRegContext(const TITAN_ENGINE_CONTEXT_t* titcontext, REGISTERCONTEXT* regcontext)
{
    regcontext->cax = titcontext->cax;
    regcontext->ccx = titcontext->ccx;
    regcontext->cdx = titcontext->cdx;
    regcontext->cbx = titcontext->cbx;
    regcontext->csp = titcontext->csp;
    regcontext->cbp = titcontext->cbp;
    regcontext->csi = titcontext->csi;
    regcontext->cdi = titcontext->cdi;
#ifdef _WIN64
    regcontext->r8 = titcontext->r8;
    regcontext->r9 = titcontext->r9;
    regcontext->r10 = titcontext->r10;
    regcontext->r11 = titcontext->r11;
    regcontext->r12 = titcontext->r12;
    regcontext->r13 = titcontext->r13;
    regcontext->r14 = titcontext->r14;
    regcontext->r15 = titcontext->r15;
#endif //_WIN64
    regcontext->cip = titcontext->cip;
    regcontext->eflags = titcontext->eflags;
    regcontext->gs = titcontext->gs;
    regcontext->fs = titcontext->fs;
    regcontext->es = titcontext->es;
    regcontext->ds = titcontext->ds;
    regcontext->cs = titcontext->cs;
    regcontext->ss = titcontext->ss;
    regcontext->dr0 = titcontext->dr0;
    regcontext->dr1 = titcontext->dr1;
    regcontext->dr2 = titcontext->dr2;
    regcontext->dr3 = titcontext->dr3;
    regcontext->dr6 = titcontext->dr6;
    regcontext->dr7 = titcontext->dr7;
    memcpy(regcontext->RegisterArea, titcontext->RegisterArea, sizeof(regcontext->RegisterArea));
    TranslateTitanFpu(&titcontext->x87fpu, &regcontext->x87fpu);
    regcontext->MxCsr = titcontext->MxCsr;
    memcpy(regcontext->XmmRegisters, titcontext->XmmRegisters, sizeof(regcontext->XmmRegisters));
    memcpy(regcontext->YmmRegisters, titcontext->YmmRegisters, sizeof(regcontext->YmmRegisters));
}

static void TranslateTitanFpuRegister(const x87FPURegister_t* titanReg, X87FPUREGISTER* reg)
{
    memcpy(reg->data, titanReg->data, sizeof(reg->data));
    reg->st_value = titanReg->st_value;
    reg->tag = titanReg->tag;
}

static void TranslateTitanFpuRegisters(const x87FPURegister_t titanFpu[8], X87FPUREGISTER fpu[8])
{
    for(int i = 0; i < 8; i++)
        TranslateTitanFpuRegister(&titanFpu[i], &fpu[i]);
}

extern "C" DLL_EXPORT bool _dbg_getregdump(REGDUMP* regdump)
{
    if(!DbgIsDebugging())
    {
        memset(regdump, 0, sizeof(REGDUMP));
        return true;
    }

    TITAN_ENGINE_CONTEXT_t titcontext;
    if(!GetFullContextDataEx(hActiveThread, &titcontext))
        return false;
    TranslateTitanContextToRegContext(&titcontext, &regdump->regcontext);

    duint cflags = regdump->regcontext.eflags;
    regdump->flags.c = valflagfromstring(cflags, "cf");
    regdump->flags.p = valflagfromstring(cflags, "pf");
    regdump->flags.a = valflagfromstring(cflags, "af");
    regdump->flags.z = valflagfromstring(cflags, "zf");
    regdump->flags.s = valflagfromstring(cflags, "sf");
    regdump->flags.t = valflagfromstring(cflags, "tf");
    regdump->flags.i = valflagfromstring(cflags, "if");
    regdump->flags.d = valflagfromstring(cflags, "df");
    regdump->flags.o = valflagfromstring(cflags, "of");

    x87FPURegister_t x87FPURegisters[8];
    Getx87FPURegisters(x87FPURegisters,  &titcontext);
    TranslateTitanFpuRegisters(x87FPURegisters, regdump->x87FPURegisters);

    GetMMXRegisters(regdump->mmx,  &titcontext);
    GetMxCsrFields(& (regdump->MxCsrFields), regdump->regcontext.MxCsr);
    Getx87ControlWordFields(& (regdump->x87ControlWordFields), regdump->regcontext.x87fpu.ControlWord);
    Getx87StatusWordFields(& (regdump->x87StatusWordFields), regdump->regcontext.x87fpu.StatusWord);

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
    std::vector<BREAKPOINT> list;
    int bpcount = bpgetlist(&list);
    if(bpcount == 0)
    {
        bpmap->count = 0;
        return 0;
    }

    int retcount = 0;
    std::vector<BRIDGEBP> bridgeList;
    BRIDGEBP curBp;
    unsigned short slot = 0;
    for(int i = 0; i < bpcount; i++)
    {
        memset(&curBp, 0, sizeof(BRIDGEBP));
        switch(type)
        {
        case bp_none: //all types
            break;
        case bp_normal: //normal
            if(list[i].type != BPNORMAL)
                continue;
            break;
        case bp_hardware: //hardware
            if(list[i].type != BPHARDWARE)
                continue;
            break;
        case bp_memory: //memory
            if(list[i].type != BPMEMORY)
                continue;
            break;
        default:
            return 0;
        }
        switch(list[i].type)
        {
        case BPNORMAL:
            curBp.type = bp_normal;
            break;
        case BPHARDWARE:
            curBp.type = bp_hardware;
            break;
        case BPMEMORY:
            curBp.type = bp_memory;
            break;
        }
        switch(((DWORD)list[i].titantype) >> 8)
        {
        case UE_DR0:
            slot = 0;
            break;
        case UE_DR1:
            slot = 1;
            break;
        case UE_DR2:
            slot = 2;
            break;
        case UE_DR3:
            slot = 3;
            break;
        }
        curBp.addr = list[i].addr;
        curBp.enabled = list[i].enabled;
        //TODO: fix this
        if(memisvalidreadptr(fdProcessInfo->hProcess, curBp.addr))
            curBp.active = true;
        strcpy(curBp.mod, list[i].mod);
        strcpy(curBp.name, list[i].name);
        curBp.singleshoot = list[i].singleshoot;
        curBp.slot = slot;
        if(curBp.active)
        {
            bridgeList.push_back(curBp);
            retcount++;
        }
    }
    if(!retcount)
    {
        bpmap->count = retcount;
        return retcount;
    }
    bpmap->count = retcount;
    bpmap->bp = (BRIDGEBP*)BridgeAlloc(sizeof(BRIDGEBP) * retcount);
    for(int i = 0; i < retcount; i++)
        memcpy(&bpmap->bp[i], &bridgeList.at(i), sizeof(BRIDGEBP));
    return retcount;
}

extern "C" DLL_EXPORT uint _dbg_getbranchdestination(uint addr)
{
    DISASM_INSTR instr;
    memset(&instr, 0, sizeof(instr));
    disasmget(addr, &instr);
    if(instr.type != instr_branch)
        return 0;
    if(strstr(instr.instruction, "ret"))
    {
        uint atcsp = DbgValFromString("@csp");
        if(DbgMemIsValidReadPtr(atcsp))
            return atcsp;
        else
            return 0;
    }
    else if(instr.arg[0].type == arg_memory)
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

    case DBG_SCRIPT_GETBRANCHINFO:
    {
        return (duint)scriptgetbranchinfo((int)(duint)param1, (SCRIPTBRANCH*)param2);
    }
    break;

    case DBG_SYMBOL_ENUM:
    {
        SYMBOLCBINFO* cbInfo = (SYMBOLCBINFO*)param1;
        symenum(cbInfo->base, cbInfo->cbSymbolEnum, cbInfo->user);
    }
    break;

    case DBG_ASSEMBLE_AT:
    {
        return assembleat((duint)param1, (const char*)param2, 0, 0, false);
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

    case DBG_GET_THREAD_LIST:
    {
        threadgetlist((THREADLIST*)param1);
    }
    break;

    case DBG_SETTINGS_UPDATED:
    {
        valuesetsignedcalc(!settingboolget("Engine", "CalculationType")); //0:signed, 1:unsigned
        SetEngineVariable(UE_ENGINE_SET_DEBUG_PRIVILEGE, settingboolget("Engine", "EnableDebugPrivilege"));
        bOnlyCipAutoComments = settingboolget("Disassembler", "OnlyCipAutoComments");
        bListAllPages = settingboolget("Engine", "ListAllPages");
        bUndecorateSymbolNames = settingboolget("Engine", "UndecorateSymbolNames");

        uint setting;
        if(BridgeSettingGetUint("Engine", "BreakpointType", &setting))
        {
            switch(setting)
            {
            case 0: //break_int3short
                SetBPXOptions(UE_BREAKPOINT_INT3);
                break;
            case 1: //break_int3long
                SetBPXOptions(UE_BREAKPOINT_LONG_INT3);
                break;
            case 2: //break_ud2
                SetBPXOptions(UE_BREAKPOINT_UD2);
                break;
            }
        }

        char exceptionRange[MAX_SETTING_SIZE] = "";
        dbgclearignoredexceptions();
        if(BridgeSettingGet("Exceptions", "IgnoreRange", exceptionRange))
        {
            char* entry = strtok(exceptionRange, ",");
            while(entry)
            {
                unsigned long start;
                unsigned long end;
                if(sscanf(entry, "%08X-%08X", &start, &end) == 2 && start <= end)
                {
                    ExceptionRange range;
                    range.start = start;
                    range.end = end;
                    dbgaddignoredexception(range);
                }
                entry = strtok(0, ",");
            }
        }
    }
    break;

    case DBG_DISASM_FAST_AT:
    {
        if(!param1 or !param2)
            return 0;
        unsigned char data[16];
        if(!memread(fdProcessInfo->hProcess, param1, data, sizeof(data), 0))
            return 0;
        DISASM disasm;
        memset(&disasm, 0, sizeof(disasm));
#ifdef _WIN64
        disasm.Archi = 64;
#endif // _WIN64
        disasm.EIP = (UIntPtr)data;
        disasm.VirtualAddr = (UInt64)param1;
        int len = Disasm(&disasm);
        if(len == UNKNOWN_OPCODE)
            len = 1;
        uint i = 0;
        BASIC_INSTRUCTION_INFO* basicinfo = (BASIC_INSTRUCTION_INFO*)param2;
        fillbasicinfo(&disasm, basicinfo);
        basicinfo->size = len;
    }
    break;

    case DBG_MENU_ENTRY_CLICKED:
    {
        int hEntry = (int)(uint)param1;
        pluginmenucall(hEntry);
    }
    break;

    case DBG_FUNCTION_GET:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (uint)functionget(info->addr, &info->start, &info->end);
    }
    break;

    case DBG_FUNCTION_OVERLAPS:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (uint)functionoverlaps(info->start, info->end);
    }
    break;

    case DBG_FUNCTION_ADD:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (uint)functionadd(info->start, info->end, info->manual);
    }
    break;

    case DBG_FUNCTION_DEL:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (uint)functiondel(info->addr);
    }
    break;

    case DBG_LOOP_GET:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (uint)loopget(info->depth, info->addr, &info->start, &info->end);
    }
    break;

    case DBG_LOOP_OVERLAPS:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (uint)loopoverlaps(info->depth, info->start, info->end, 0);
    }
    break;

    case DBG_LOOP_ADD:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (uint)loopadd(info->start, info->end, info->manual);
    }
    break;

    case DBG_LOOP_DEL:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (uint)loopdel(info->depth, info->addr);
    }
    break;

    case DBG_IS_RUN_LOCKED:
    {
        return (uint)waitislocked(WAITID_RUN);
    }
    break;

    case DBG_IS_BP_DISABLED:
    {
        BREAKPOINT bp;
        if(bpget((uint)param1, BPNORMAL, 0, &bp))
            return !(uint)bp.enabled;
        return (uint)false;
    }
    break;

    case DBG_SET_AUTO_COMMENT_AT:
    {
        return (uint)commentset((uint)param1, (const char*)param2, false);
    }
    break;

    case DBG_DELETE_AUTO_COMMENT_RANGE:
    {
        commentdelrange((uint)param1, (uint)param2);
    }
    break;

    case DBG_SET_AUTO_LABEL_AT:
    {
        return (uint)labelset((uint)param1, (const char*)param2, false);
    }
    break;

    case DBG_DELETE_AUTO_LABEL_RANGE:
    {
        labeldelrange((uint)param1, (uint)param2);
    }
    break;

    case DBG_SET_AUTO_BOOKMARK_AT:
    {
        return (uint)bookmarkset((uint)param1, false);
    }
    break;

    case DBG_DELETE_AUTO_BOOKMARK_RANGE:
    {
        bookmarkdelrange((uint)param1, (uint)param2);
    }
    break;

    case DBG_SET_AUTO_FUNCTION_AT:
    {
        return (uint)functionadd((uint)param1, (uint)param2, false);
    }
    break;

    case DBG_DELETE_AUTO_FUNCTION_RANGE:
    {
        functiondelrange((uint)param1, (uint)param2);
    }
    break;

    case DBG_GET_STRING_AT:
    {
        STRING_TYPE strtype;
        char string[512] = "";
        if(disasmgetstringat((uint)param1, &strtype, string, string, 500))
        {
            if(strtype == str_ascii)
                sprintf((char*)param2, "\"%s\"", string);
            else //unicode
                sprintf((char*)param2, "L\"%s\"", string);
            return true;
        }
        return false;
    }
    break;

    case DBG_GET_FUNCTIONS:
    {
        return (uint)dbgfunctionsget();
    }
    break;

    case DBG_WIN_EVENT:
    {
        return (uint)pluginwinevent((MSG*)param1, (long*)param2);
    }
    break;

    case DBG_WIN_EVENT_GLOBAL:
    {
        return (uint)pluginwineventglobal((MSG*)param1);
    }
    break;
    }
    return 0;
}
