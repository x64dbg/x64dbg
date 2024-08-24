/**
 @file _exports.cpp

 @brief Implements the exports class.
 */

#include "_exports.h"
#include "memory.h"
#include "debugger.h"
#include "value.h"
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
#include "module.h"
#include "comment.h"
#include "label.h"
#include "bookmark.h"
#include "function.h"
#include "loop.h"
#include "exception.h"
#include "x64dbg.h"
#include "xrefs.h"
#include "encodemap.h"
#include "argument.h"
#include "watch.h"
#include "animate.h"
#include "TraceRecord.h"
#include "recursiveanalysis.h"
#include "dbghelp_safe.h"
#include "symbolinfo.h"

static bool bOnlyCipAutoComments = false;
static bool bNoSourceLineAutoComments = false;
static TITAN_ENGINE_CONTEXT_t lastContext;

extern "C" DLL_EXPORT duint _dbg_memfindbaseaddr(duint addr, duint* size)
{
    return MemFindBaseAddr(addr, size);
}

extern "C" DLL_EXPORT bool _dbg_memread(duint addr, unsigned char* dest, duint size, duint* read)
{
    return MemRead(addr, dest, size, read);
}

extern "C" DLL_EXPORT bool _dbg_memwrite(duint addr, const unsigned char* src, duint size, duint* written)
{
    return MemWrite(addr, src, size, written);
}

extern "C" DLL_EXPORT bool _dbg_memmap(MEMMAP* memmap)
{
    SHARED_ACQUIRE(LockMemoryPages);

    int pagecount = (int)memoryPages.size();
    memmap->count = pagecount;
    memmap->page = nullptr;
    if(!pagecount)
        return true;

    // Allocate memory that is already zeroed
    memmap->page = (MEMPAGE*)BridgeAlloc(sizeof(MEMPAGE) * pagecount);

    // Copy all elements over
    int i = 0;

    for(auto & itr : memoryPages)
        memcpy(&memmap->page[i++], &itr.second, sizeof(MEMPAGE));

    // Done
    return true;
}

extern "C" DLL_EXPORT bool _dbg_memisvalidreadptr(duint addr)
{
    return MemIsValidReadPtr(addr, true);
}

extern "C" DLL_EXPORT bool _dbg_valfromstring(const char* string, duint* value)
{
    return valfromstring(string, value);
}

extern "C" DLL_EXPORT bool _dbg_isdebugging()
{
    return hDebugLoopThread && IsFileBeingDebugged();
}

extern "C" DLL_EXPORT bool _dbg_isjumpgoingtoexecute(duint addr)
{
    if(!hActiveThread)
        return false;

    unsigned char data[16];
    if(MemRead(addr, data, sizeof(data), nullptr, true))
    {
        Zydis zydis;
        if(zydis.Disassemble(addr, data))
        {
            CONTEXT ctx;
            memset(&ctx, 0, sizeof(ctx));
            ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
            GetThreadContext(hActiveThread, &ctx);
#ifdef _WIN64
            auto cflags = ctx.EFlags;
            auto ccx = ctx.Rcx;
#else
            auto cflags = ctx.EFlags;
            auto ccx = ctx.Ecx;
#endif //_WIN64
            return zydis.IsBranchGoingToExecute(cflags, ccx);
        }
    }
    return false;
}

static bool shouldFilterSymbol(const char* name)
{
    if(!name)
        return true;
    if(strstr(name, "`string'"))
        return true;
    if(strstr(name, "__imp_") == name || strstr(name, "_imp_") == name)
        return true;

    PLUG_CB_FILTERSYMBOL filterInfo = { name, false };
    plugincbcall(CB_FILTERSYMBOL, &filterInfo);
    return filterInfo.retval;
}

// https://github.com/llvm-mirror/llvm/blob/2ae7de27f7d9276e7bada445ea7576bbc4c83ae6/lib/DebugInfo/Symbolize/Symbolize.cpp#L427
// https://github.com/x64dbg/x64dbg/pull/1478
// Undo these various manglings for Win32 extern "C" functions:
// cdecl       - _foo
// stdcall     - _foo@12
// fastcall    - @foo@12
// vectorcall  - foo@@12
// These are all different linkage names for 'foo'.
static char* demanglePE32ExternCFunc(char* SymbolName)
{
    // Only do this for Win32
#ifdef _WIN64
    return SymbolName;
#endif //_WIN64

    // Don't try to demangle C++ names
    char Front = SymbolName[0];
    if(Front == '?')
        return SymbolName;

    // Remove any '_' or '@' prefix.
    if(Front == '_' || Front == '@')
        SymbolName++;

    // Remove any '@[0-9]+' suffix.
    auto AtPos = strrchr(SymbolName, '@');
    if(AtPos)
    {
        auto p = AtPos + 1;
        while(*p && isdigit(*p))
            p++;

        // All characters after '@' were digits
        if(!*p)
            *AtPos = '\0';
    }

    // Remove any ending '@' for vectorcall.
    auto len = strlen(SymbolName);
    if(len && SymbolName[len - 1] == '@')
        SymbolName[len - 1] = '\0';

    return SymbolName;
}

static bool getLabel(duint addr, char* label, bool noFuncOffset)
{
    bool retval = false;
    label[0] = 0;
    if(LabelGet(addr, label))
        return true;
    else //no user labels
    {
        DWORD64 displacement = 0;
        {
            SYMBOLINFOCPP symInfo;

            bool res;
            if(noFuncOffset)
                res = SymbolFromAddressExact(addr, &symInfo);
            else
                res = SymbolFromAddressExactOrLower(addr, &symInfo);

            if(res)
            {
                displacement = (int32_t)(addr - symInfo.addr);

                //auto name = demanglePE32ExternCFunc(symInfo.decoratedName.c_str());
                if(bUndecorateSymbolNames && *symInfo.undecoratedSymbol != '\0')
                    strncpy_s(label, MAX_LABEL_SIZE, symInfo.undecoratedSymbol, _TRUNCATE);
                else
                    strncpy_s(label, MAX_LABEL_SIZE, symInfo.decoratedSymbol, _TRUNCATE);

                retval = !shouldFilterSymbol(label);
                if(retval && displacement != 0)
                {
                    char temp[32];
                    sprintf_s(temp, "+%llX", displacement);
                    strncat_s(label, MAX_LABEL_SIZE, temp, _TRUNCATE);
                }
            }
        }

        if(!retval)  //search for CALL <jmp.&user32.MessageBoxA>
        {
            BASIC_INSTRUCTION_INFO basicinfo;
            memset(&basicinfo, 0, sizeof(BASIC_INSTRUCTION_INFO));
            if(disasmfast(addr, &basicinfo, true) && basicinfo.branch && !basicinfo.call && basicinfo.memory.value)  //thing is a JMP
            {
                duint val = 0;
                if(MemRead(basicinfo.memory.value, &val, sizeof(val), nullptr, true))
                {
                    SYMBOLINFOCPP symInfo;
                    bool res;
                    if(noFuncOffset)
                        res = SymbolFromAddressExact(val, &symInfo);
                    else
                        res = SymbolFromAddressExactOrLower(val, &symInfo);

                    if(res)
                    {
                        //pSymbol->Name[pSymbol->MaxNameLen - 1] = '\0';

                        //auto name = demanglePE32ExternCFunc(pSymbol->Name);
                        if(bUndecorateSymbolNames && *symInfo.undecoratedSymbol != '\0')
                            _snprintf_s(label, MAX_LABEL_SIZE, _TRUNCATE, "JMP.&%s", symInfo.undecoratedSymbol);
                        else
                            _snprintf_s(label, MAX_LABEL_SIZE, _TRUNCATE, "JMP.&%s", symInfo.decoratedSymbol);
                        retval = !shouldFilterSymbol(label);
                        if(retval && displacement)
                        {
                            char temp[32];
                            sprintf_s(temp, "+%llX", displacement);
                            strncat_s(label, MAX_LABEL_SIZE, temp, _TRUNCATE);
                        }
                    }
                }
            }
        }
        if(!retval)  //search for module entry
        {
            if(addr != 0 && ModEntryFromAddr(addr) == addr)
            {
                strcpy_s(label, MAX_LABEL_SIZE, "EntryPoint");
                return true;
            }
            duint start;
            if(FunctionGet(addr, &start, nullptr))
            {
                duint rva = addr - start;
                if(rva == 0)
                {
#ifdef _WIN64
                    sprintf_s(label, MAX_LABEL_SIZE, "sub_%llX", start);
#else //x86
                    sprintf_s(label, MAX_LABEL_SIZE, "sub_%X", start);
#endif //_WIN64
                    return true;
                }
                if(noFuncOffset)
                    return false;
                getLabel(start, label, false);
                char temp[32];
#ifdef _WIN64
                sprintf_s(temp, "+%llX", rva);
#else //x86
                sprintf_s(temp, "+%X", rva);
#endif //_WIN64
                strncat_s(label, MAX_LABEL_SIZE, temp, _TRUNCATE);
                return true;
            }
        }
    }
    return retval;
}

static bool getAutoComment(duint addr, String & comment)
{
    bool retval = false;
    duint disp;
    char fileName[MAX_STRING_SIZE] = {};
    int lineNumber = 0;
    if(!bNoSourceLineAutoComments && SymGetSourceLine(addr, fileName, &lineNumber, &disp) && !disp)
    {
        char* actualName = fileName;
        char* l = strrchr(fileName, '\\');
        if(l)
            actualName = l + 1;

        comment = StringUtils::sprintf("%s:%u", actualName, lineNumber);
        retval = true;
    }
    else
    {
        SHARED_ACQUIRE(LockModules);
        auto modInfo = ModInfoFromAddr(addr);
        if(modInfo != nullptr)
        {
            auto exportInfo = modInfo->findExport(addr - modInfo->base);
            if(exportInfo != nullptr && exportInfo->forwarded)
            {
                comment = StringUtils::sprintf("-> %s", exportInfo->forwardName.c_str());
                retval = true;
                dputs(comment.c_str());
            }
        }
    }

    DISASM_INSTR instr;
    String temp_string;
    BRIDGE_ADDRINFO newinfo;
    char string_text[MAX_STRING_SIZE] = "";

    Zydis zydis;
    auto getregs = !bOnlyCipAutoComments || addr == lastContext.cip;
    disasmget(zydis, addr, &instr, getregs);
    // Some nop variants have 'operands' that should be ignored
    if(zydis.Success() && !zydis.IsNop())
    {
        //Ignore register values when not on CIP and OnlyCipAutoComments is enabled: https://github.com/x64dbg/x64dbg/issues/1383
        if(!getregs)
        {
            for(int i = 0; i < instr.argcount; i++)
                instr.arg[i].value = instr.arg[i].constant;
        }

        if(addr == lastContext.cip && (zydis.GetId() == ZYDIS_MNEMONIC_SYSCALL || (zydis.GetId() == ZYDIS_MNEMONIC_INT && zydis[0].imm.value.u == 0x2e)))
        {
            auto syscallName = SyscallToName((unsigned int)lastContext.cax);
            if(!syscallName.empty())
            {
                if(!comment.empty())
                {
                    comment.push_back(',');
                    comment.push_back(' ');
                }
                comment.append(syscallName);
                retval = true;
            }
        }

        for(int i = 0; i < instr.argcount; i++)
        {
            memset(&newinfo, 0, sizeof(BRIDGE_ADDRINFO));
            newinfo.flags = flaglabel;

            STRING_TYPE strtype = str_none;

            if(instr.arg[i].constant == instr.arg[i].value)  //avoid: call <module.label> ; addr:label
            {
                auto constant = instr.arg[i].constant;
                if(instr.arg[i].type == arg_normal && instr.arg[i].value == addr + instr.instr_size && zydis.IsCall())
                    temp_string.assign("call $0");
                else if(instr.arg[i].type == arg_normal && instr.arg[i].value == addr + instr.instr_size && zydis.IsJump())
                    temp_string.assign("jmp $0");
                else if(instr.type == instr_branch)
                    continue;
                else if(instr.arg[i].type == arg_normal && constant < 256 && (isprint(int(constant)) || isspace(int(constant))) && (strstr(instr.instruction, "cmp") || strstr(instr.instruction, "mov")))
                {
                    temp_string.assign(instr.arg[i].mnemonic);
                    temp_string.push_back(':');
                    temp_string.push_back('\'');
                    temp_string.append(StringUtils::Escape((unsigned char)constant));
                    temp_string.push_back('\'');
                }
                else if(DbgGetStringAt(instr.arg[i].constant, string_text))
                {
                    temp_string.assign(instr.arg[i].mnemonic);
                    temp_string.push_back(':');
                    temp_string.append(string_text);
                }
            }
            else if(instr.arg[i].memvalue && (DbgGetStringAt(instr.arg[i].memvalue, string_text) || _dbg_addrinfoget(instr.arg[i].memvalue, instr.arg[i].segment, &newinfo)))
            {
                if(*string_text)
                {
                    temp_string.assign("[");
                    temp_string.append(instr.arg[i].mnemonic);
                    temp_string.push_back(']');
                    temp_string.push_back(':');
                    temp_string.append(string_text);
                }
                else if(*newinfo.label)
                {
                    temp_string.assign("[");
                    temp_string.append(instr.arg[i].mnemonic);
                    temp_string.push_back(']');
                    temp_string.push_back(':');
                    temp_string.append(newinfo.label);
                }
            }
            else if(instr.arg[i].value && (DbgGetStringAt(instr.arg[i].value, string_text) || _dbg_addrinfoget(instr.arg[i].value, instr.arg[i].segment, &newinfo)))
            {
                if(instr.type != instr_normal)  //stack/jumps (eg add esp, 4 or jmp 401110) cannot directly point to strings
                {
                    if(*newinfo.label)
                    {
                        temp_string = instr.arg[i].mnemonic;
                        temp_string.push_back(':');
                        temp_string.append(newinfo.label);
                    }
                }
                else if(*string_text)
                {
                    temp_string = instr.arg[i].mnemonic;
                    temp_string.push_back(':');
                    temp_string.append(string_text);
                }
                else if(*newinfo.label)
                {
                    temp_string = instr.arg[i].mnemonic;
                    temp_string.push_back(':');
                    temp_string.append(newinfo.label);
                }
            }
            else
                continue;

            if(!strstr(comment.c_str(), temp_string.c_str()))  //avoid duplicate comments
            {
                if(!comment.empty())
                {
                    comment.push_back(',');
                    comment.push_back(' ');
                }
                comment.append(temp_string);
                retval = true;
            }
        }
    }
    BREAKPOINT bp;
    // Add autocomment for breakpoints with BreakpointsView format because there's usually something useful
    if(BpGet(addr, BPNORMAL, nullptr, &bp) || BpGet(addr, BPHARDWARE, nullptr, &bp))
    {
        temp_string.clear();
        auto next = [&temp_string]()
        {
            if(!temp_string.empty())
                temp_string += ", ";
        };
        if(!bp.breakCondition.empty())
        {
            next();
            temp_string += GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "breakif"));
            temp_string += "(";
            temp_string += bp.breakCondition;
            temp_string += ")";
        }

        if(bp.fastResume)
        {
            next();
            temp_string += GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "fastresume()"));
        }
        else //fast resume skips all other steps
        {
            if(!bp.logText.empty())
            {
                next();
                if(!bp.logCondition.empty())
                {
                    temp_string += GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "logif"));
                    temp_string += "(";
                    temp_string += bp.logCondition;
                    temp_string += ", ";
                }
                else
                {
                    temp_string += GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "log"));
                    temp_string += "(";
                }
                temp_string += bp.logText;
                temp_string += ")";
            }

            if(!bp.commandText.empty())
            {
                next();
                if(!bp.commandCondition.empty())
                {
                    temp_string += GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "cmdif"));
                    temp_string += "(";
                    temp_string += bp.commandCondition;
                    temp_string += ", ";
                }
                else
                {
                    temp_string += GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "cmd"));
                    temp_string += "(";
                }
                temp_string += bp.commandText;
                temp_string += ")";
            }
        }
        if(!temp_string.empty())
        {
            if(!comment.empty())
            {
                comment.push_back(',');
                comment.push_back(' ');
            }
            comment.append(temp_string);
            retval = true;
        }
    }
    StringUtils::ReplaceAll(comment, "{", "{{");
    StringUtils::ReplaceAll(comment, "}", "}}");
    return retval;
}

extern "C" DLL_EXPORT bool _dbg_addrinfoget(duint addr, SEGMENTREG segment, BRIDGE_ADDRINFO* addrinfo)
{
    if(!DbgIsDebugging())
        return false;
    bool retval = false;
    if(addrinfo->flags & flagmodule) //get module
    {
        if(ModNameFromAddr(addr, addrinfo->module, false)) //get module name
            retval = true;
    }
    if(addrinfo->flags & flaglabel)
    {
        retval = getLabel(addr, addrinfo->label, (addrinfo->flags & flagNoFuncOffset) != 0);
    }
    if(addrinfo->flags & flagbookmark)
    {
        addrinfo->isbookmark = BookmarkGet(addr);
        retval = true;
    }
    if(addrinfo->flags & flagfunction)
    {
        if(FunctionGet(addr, &addrinfo->function.start, &addrinfo->function.end, &addrinfo->function.instrcount))
            retval = true;
    }
    if(addrinfo->flags & flagloop)
    {
        if(LoopGet(addrinfo->loop.depth, addr, &addrinfo->loop.start, &addrinfo->loop.end, &addrinfo->loop.instrcount))
            retval = true;
    }
    if(addrinfo->flags & flagargs)
    {
        if(ArgumentGet(addr, &addrinfo->args.start, &addrinfo->args.end, &addrinfo->function.instrcount))
            retval = true;
    }
    if(addrinfo->flags & flagcomment)
    {
        *addrinfo->comment = 0;
        if(CommentGet(addr, addrinfo->comment))
        {
            retval = true;
        }
        else
        {
            String comment;
            retval = getAutoComment(addr, comment);
            strcpy_s(addrinfo->comment, "\1");
            strncat_s(addrinfo->comment, comment.c_str(), _TRUNCATE);
        }
    }
    PLUG_CB_ADDRINFO info;
    info.addr = addr;
    info.addrinfo = addrinfo;
    info.retval = retval;
    plugincbcall(CB_ADDRINFO, &info);
    return info.retval;
}

extern "C" DLL_EXPORT bool _dbg_addrinfoset(duint addr, BRIDGE_ADDRINFO* addrinfo)
{
    bool retval = false;
    if(addrinfo->flags & flaglabel) //set label
    {
        if(LabelSet(addr, addrinfo->label, true))
            retval = true;
    }
    if(addrinfo->flags & flagcomment) //set comment
    {
        if(CommentSet(addr, addrinfo->comment, true))
            retval = true;
    }
    if(addrinfo->flags & flagbookmark) //set bookmark
    {
        if(addrinfo->isbookmark)
            retval = BookmarkSet(addr, true);
        else
            retval = BookmarkDelete(addr);
    }
    return retval;
}

extern "C" DLL_EXPORT bool _dbg_encodetypeset(duint addr, duint size, ENCODETYPE type)
{
    return EncodeMapSetType(addr, size, type);
}

extern "C" DLL_EXPORT PROCESS_INFORMATION* _dbg_getProcessInformation()
{
    return fdProcessInfo;
}

extern "C" DLL_EXPORT int _dbg_bpgettypeat(duint addr)
{
    static duint cacheAddr;
    static int cacheBpCount;
    static int cacheResult;
    int bpcount = BpGetList(nullptr);
    if(cacheAddr != addr || cacheBpCount != bpcount)
    {
        BREAKPOINT bp;
        cacheAddr = addr;
        cacheResult = 0;
        cacheBpCount = bpcount;
        if(BpGet(addr, BPNORMAL, 0, &bp))
            if(bp.enabled)
                cacheResult |= bp_normal;
        if(BpGet(addr, BPHARDWARE, 0, &bp))
            if(bp.enabled)
                cacheResult |= bp_hardware;
        if(BpGet(addr, BPMEMORY, 0, &bp))
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
    x87StatusWordFields->ES = valx87statuswordflagfromstring(StatusWord, "ES");
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

    // NOTE: this is not thread-safe, but that's fine because lastContext is only used for GUI-related operations
    memcpy(&lastContext, &titcontext, sizeof(titcontext));

    TranslateTitanContextToRegContext(&titcontext, &regdump->regcontext);

    duint cflags = regdump->regcontext.eflags;
    regdump->flags.c = (cflags & (1 << 0)) != 0;
    regdump->flags.p = (cflags & (1 << 2)) != 0;
    regdump->flags.a = (cflags & (1 << 4)) != 0;
    regdump->flags.z = (cflags & (1 << 6)) != 0;
    regdump->flags.s = (cflags & (1 << 7)) != 0;
    regdump->flags.t = (cflags & (1 << 8)) != 0;
    regdump->flags.i = (cflags & (1 << 9)) != 0;
    regdump->flags.d = (cflags & (1 << 10)) != 0;
    regdump->flags.o = (cflags & (1 << 11)) != 0;

    x87FPURegister_t x87FPURegisters[8];
    Getx87FPURegisters(x87FPURegisters,  &titcontext);
    TranslateTitanFpuRegisters(x87FPURegisters, regdump->x87FPURegisters);

    GetMMXRegisters(regdump->mmx,  &titcontext);
    GetMxCsrFields(& (regdump->MxCsrFields), regdump->regcontext.MxCsr);
    Getx87ControlWordFields(& (regdump->x87ControlWordFields), regdump->regcontext.x87fpu.ControlWord);
    Getx87StatusWordFields(& (regdump->x87StatusWordFields), regdump->regcontext.x87fpu.StatusWord);

    LASTERROR lastError;
    memset(&lastError.name, 0, sizeof(lastError.name));
    lastError.code = ThreadGetLastError(ThreadGetId(hActiveThread));
    strncpy_s(lastError.name, ErrorCodeToName(lastError.code).c_str(), _TRUNCATE);
    regdump->lastError = lastError;

    LASTSTATUS lastStatus;
    memset(&lastStatus.name, 0, sizeof(lastStatus.name));
    lastStatus.code = ThreadGetLastStatus(ThreadGetId(hActiveThread));
    strncpy_s(lastStatus.name, NtStatusCodeToName(lastStatus.code).c_str(), _TRUNCATE);
    regdump->lastStatus = lastStatus;

    return true;
}

extern "C" DLL_EXPORT bool _dbg_valtostring(const char* string, duint value)
{
    return valtostring(string, value, true);
}

extern "C" DLL_EXPORT int _dbg_getbplist(BPXTYPE type, BPMAP* bpmap)
{
    if(!bpmap)
        return 0;

    bpmap->count = 0;
    bpmap->bp = nullptr;

    std::vector<BREAKPOINT> list;
    int bpcount = BpGetList(&list);
    if(bpcount == 0)
        return 0;

    int retcount = 0;
    std::vector<BRIDGEBP> bridgeList;
    BRIDGEBP curBp;
    BP_TYPE currentBpType;
    switch(type)
    {
    case bp_none:
        currentBpType = BP_TYPE(-1);
        break;
    case bp_normal:
        currentBpType = BPNORMAL;
        break;
    case bp_hardware:
        currentBpType = BPHARDWARE;
        break;
    case bp_memory:
        currentBpType = BPMEMORY;
        break;
    case bp_dll:
        currentBpType = BPDLL;
        break;
    case bp_exception:
        currentBpType = BPEXCEPTION;
        break;
    default:
        return 0;
    }
    unsigned short slot = 0;
    for(int i = 0; i < bpcount; i++)
    {
        if(currentBpType != -1 && list[i].type != currentBpType)
            continue;
        BpToBridge(&list[i], &curBp);
        bridgeList.push_back(curBp);
        retcount++;
    }
    if(!retcount)
        return 0;
    bpmap->count = retcount;
    bpmap->bp = (BRIDGEBP*)BridgeAlloc(sizeof(BRIDGEBP) * retcount);
    for(int i = 0; i < retcount; i++)
        memcpy(&bpmap->bp[i], &bridgeList.at(i), sizeof(BRIDGEBP));
    return retcount;
}

extern "C" DLL_EXPORT duint _dbg_getbranchdestination(duint addr)
{
    unsigned char data[MAX_DISASM_BUFFER];
    if(!MemIsValidReadPtr(addr, true) || !MemRead(addr, data, sizeof(data)))
        return 0;
    Zydis zydis;
    if(!zydis.Disassemble(addr, data))
        return 0;
    if(zydis.IsBranchType(Zydis::BTJmp | Zydis::BTCall | Zydis::BTLoop | Zydis::BTXbegin))
    {
        auto opValue = (duint)zydis.ResolveOpValue(0, [](ZydisRegister reg) -> uint64_t
        {
            switch(reg)
            {
#ifndef _WIN64 //x32
            case ZYDIS_REGISTER_EAX:
                return lastContext.cax;
            case ZYDIS_REGISTER_EBX:
                return lastContext.cbx;
            case ZYDIS_REGISTER_ECX:
                return lastContext.ccx;
            case ZYDIS_REGISTER_EDX:
                return lastContext.cdx;
            case ZYDIS_REGISTER_EBP:
                return lastContext.cbp;
            case ZYDIS_REGISTER_ESP:
                return lastContext.csp;
            case ZYDIS_REGISTER_ESI:
                return lastContext.csi;
            case ZYDIS_REGISTER_EDI:
                return lastContext.cdi;
            case ZYDIS_REGISTER_EIP:
                return lastContext.cip;
#else //x64
            case ZYDIS_REGISTER_RAX:
                return lastContext.cax;
            case ZYDIS_REGISTER_RBX:
                return lastContext.cbx;
            case ZYDIS_REGISTER_RCX:
                return lastContext.ccx;
            case ZYDIS_REGISTER_RDX:
                return lastContext.cdx;
            case ZYDIS_REGISTER_RBP:
                return lastContext.cbp;
            case ZYDIS_REGISTER_RSP:
                return lastContext.csp;
            case ZYDIS_REGISTER_RSI:
                return lastContext.csi;
            case ZYDIS_REGISTER_RDI:
                return lastContext.cdi;
            case ZYDIS_REGISTER_RIP:
                return lastContext.cip;
            case ZYDIS_REGISTER_R8:
                return lastContext.r8;
            case ZYDIS_REGISTER_R9:
                return lastContext.r9;
            case ZYDIS_REGISTER_R10:
                return lastContext.r10;
            case ZYDIS_REGISTER_R11:
                return lastContext.r11;
            case ZYDIS_REGISTER_R12:
                return lastContext.r12;
            case ZYDIS_REGISTER_R13:
                return lastContext.r13;
            case ZYDIS_REGISTER_R14:
                return lastContext.r14;
            case ZYDIS_REGISTER_R15:
                return lastContext.r15;
#endif //_WIN64
            default:
                return 0;
            }
        });
        if(zydis.OpCount() && zydis[0].type == ZYDIS_OPERAND_TYPE_MEMORY)
        {
            auto const tebseg = ArchValue(ZYDIS_REGISTER_FS, ZYDIS_REGISTER_GS);
            if(zydis[0].mem.segment == tebseg)
                opValue += duint(GetTEBLocation(hActiveThread));
            if(MemRead(opValue, &opValue, sizeof(opValue)))
                return opValue;
        }
        else
            return opValue;
    }
    if(zydis.IsRet())
    {
        auto csp = lastContext.csp;
        duint dest = 0;
        if(MemRead(csp, &dest, sizeof(dest)))
            return dest;
    }
    return 0;
}

extern "C" DLL_EXPORT bool _dbg_functionoverlaps(duint start, duint end)
{
    return FunctionOverlaps(start, end);
}

extern "C" DLL_EXPORT duint _dbg_sendmessage(DBGMSG type, void* param1, void* param2)
{
    if(dbgisstopped())
    {
        switch(type) //ignore win events
        {
        //these functions are safe to call when we did not initialize yet
        case DBG_DEINITIALIZE_LOCKS:
        case DBG_INITIALIZE_LOCKS:
        case DBG_GET_FUNCTIONS:
        case DBG_SETTINGS_UPDATED:
        case DBG_GET_THREAD_LIST:
        case DBG_WIN_EVENT:
        case DBG_WIN_EVENT_GLOBAL:
        case DBG_RELEASE_ENCODE_TYPE_BUFFER:
        case DBG_GET_TIME_WASTED_COUNTER:
        case DBG_GET_DEBUG_ENGINE:
            break;
        //the rest is unsafe -> throw an exception when people try to call them
        default:
            __debugbreak(); //we cannot process messages when the debugger is stopped, this must be a bug
        }
    }
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
    case DBG_SYMBOL_ENUM_FROMCACHE:
    {
        SYMBOLCBINFO* cbInfo = (SYMBOLCBINFO*)param1;
        if(cbInfo->base == -1)
        {
            SHARED_ACQUIRE(LockModules);
            auto info = ModInfoFromAddr(cbInfo->start);
            if(info != nullptr && cbInfo->end >= info->base && cbInfo->end < info->base + info->size)
            {
                auto beginRva = cbInfo->start - info->base;
                auto endRva = cbInfo->end - info->base;
                if(beginRva > endRva)
                {
                    return false;
                }
                return SymEnum(info->base, cbInfo->cbSymbolEnum, cbInfo->user, beginRva, endRva, cbInfo->symbolMask);
            }
            else
            {
                return false;
            }
        }
        else
        {
            return SymEnum(cbInfo->base, cbInfo->cbSymbolEnum, cbInfo->user, 0, -1, SYMBOL_MASK_ALL);
        }
    }
    break;

    case DBG_ASSEMBLE_AT:
    {
        return assembleat((duint)param1, (const char*)param2, 0, 0, false);
    }
    break;

    case DBG_MODBASE_FROM_NAME:
    {
        return ModBaseFromName((const char*)param1);
    }
    break;

    case DBG_DISASM_AT:
    {
        disasmget((duint)param1, (DISASM_INSTR*)param2);
    }
    break;

    case DBG_STACK_COMMENT_GET:
    {
        return stackcommentget((duint)param1, (STACK_COMMENT*)param2);
    }
    break;

    case DBG_GET_THREAD_LIST:
    {
        ThreadGetList((THREADLIST*)param1);
    }
    break;

    case DBG_SETTINGS_UPDATED:
    {
        valuesetsignedcalc(!settingboolget("Engine", "CalculationType")); //0:signed, 1:unsigned
        SetEngineVariable(UE_ENGINE_SET_DEBUG_PRIVILEGE, settingboolget("Engine", "EnableDebugPrivilege"));
        SetEngineVariable(UE_ENGINE_SAFE_ATTACH, settingboolget("Engine", "SafeAttach"));
        SetEngineVariable(UE_ENGINE_MEMBP_ALT, settingboolget("Engine", "MembpAlt"));
        SetEngineVariable(UE_ENGINE_DISABLE_ASLR, settingboolget("Engine", "DisableAslr"));
        bOnlyCipAutoComments = settingboolget("Disassembler", "OnlyCipAutoComments");
        bNoSourceLineAutoComments = settingboolget("Disassembler", "NoSourceLineAutoComments");
        bListAllPages = settingboolget("Engine", "ListAllPages");
        bUndecorateSymbolNames = settingboolget("Engine", "UndecorateSymbolNames");
        bEnableSourceDebugging = settingboolget("Engine", "EnableSourceDebugging");
        bSkipInt3Stepping = settingboolget("Engine", "SkipInt3Stepping");
        bIgnoreInconsistentBreakpoints = settingboolget("Engine", "IgnoreInconsistentBreakpoints");
        bNoForegroundWindow = settingboolget("Gui", "NoForegroundWindow");
        bVerboseExceptionLogging = settingboolget("Engine", "VerboseExceptionLogging");
        bNoWow64SingleStepWorkaround = settingboolget("Engine", "NoWow64SingleStepWorkaround");
        bQueryWorkingSet = settingboolget("Misc", "QueryWorkingSet");
        bForceLoadSymbols = settingboolget("Misc", "ForceLoadSymbols");
        bTruncateBreakpointLogs = settingboolget("Engine", "TruncateBreakpointLogs");
        stackupdatesettings();

        duint setting;
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
        if(BridgeSettingGetUint("Engine", "Assembler", &setting))
            assemblerEngine = AssemblerEngine(setting);
        else
        {
            assemblerEngine = AssemblerEngine::asmjit;
            BridgeSettingSetUint("Engine", "Assembler", duint(assemblerEngine));
        }

        std::vector<char> settingText(MAX_SETTING_SIZE + 1, '\0');
        bool unknownExceptionsFilterAdded = false;
        dbgclearexceptionfilters();
        if(BridgeSettingGet("Exceptions", "IgnoreRange", settingText.data()))
        {
            char* context = nullptr;
            auto entry = strtok_s(settingText.data(), ",", &context);
            while(entry)
            {
                unsigned long start;
                unsigned long end;
                if(strstr(entry, "debug") == nullptr && // check for old ignore format
                        sscanf_s(entry, "%08X-%08X", &start, &end) == 2 && start <= end)
                {
                    ExceptionFilter filter;
                    filter.range.start = start;
                    filter.range.end = end;
                    // Default settings for an ignore entry
                    filter.breakOn = ExceptionBreakOn::SecondChance;
                    filter.logException = true;
                    filter.handledBy = ExceptionHandledBy::Debuggee;
                    dbgaddexceptionfilter(filter);
                }
                else if(strstr(entry, "debug") != nullptr && // new filter format
                        sscanf_s(entry, "%08X-%08X", &start, &end) == 2 && start <= end)
                {
                    ExceptionFilter filter;
                    filter.range.start = start;
                    filter.range.end = end;
                    filter.breakOn = strstr(entry, "first") != nullptr ? ExceptionBreakOn::FirstChance : strstr(entry, "second") != nullptr ? ExceptionBreakOn::SecondChance : ExceptionBreakOn::DoNotBreak;
                    filter.logException = strstr(entry, "nolog") == nullptr;
                    filter.handledBy = strstr(entry, "debugger") != nullptr ? ExceptionHandledBy::Debugger : ExceptionHandledBy::Debuggee;
                    dbgaddexceptionfilter(filter);
                    if(filter.range.start == 0 && filter.range.start == filter.range.end)
                        unknownExceptionsFilterAdded = true;
                }
                entry = strtok_s(nullptr, ",", &context);
            }
        }
        if(!unknownExceptionsFilterAdded) // add a default filter for unknown exceptions if it was not yet present in settings
        {
            ExceptionFilter unknownExceptionsFilter;
            unknownExceptionsFilter.range.start = unknownExceptionsFilter.range.end = 0;
            unknownExceptionsFilter.breakOn = ExceptionBreakOn::FirstChance;
            unknownExceptionsFilter.logException = true;
            unknownExceptionsFilter.handledBy = ExceptionHandledBy::Debuggee;
            dbgaddexceptionfilter(unknownExceptionsFilter);
        }

        // check if we need to change the main window title
        bool bNewWindowLongPath = settingboolget("Gui", "WindowLongPath");
        if(bWindowLongPath != bNewWindowLongPath)
        {
            bWindowLongPath = bNewWindowLongPath;
            if(DbgIsDebugging())
            {
                duint addr = 0;
                SELECTIONDATA selection;
                if(GuiSelectionGet(GUI_DISASSEMBLY, &selection))
                    addr = selection.start;
                else
                    addr = GetContextDataEx(hActiveThread, UE_CIP);
                DebugUpdateTitleAsync(addr, false);
            }
        }

        if(BridgeSettingGet("Symbols", "CachePath", settingText.data()))
        {
            // Trim the buffer to fit inside MAX_PATH
            strncpy_s(szSymbolCachePath, settingText.data(), _TRUNCATE);
        }

        duint animateInterval;
        if(BridgeSettingGetUint("Engine", "AnimateInterval", &animateInterval))
            _dbg_setanimateinterval((unsigned int)animateInterval);
        else
            _dbg_setanimateinterval(50); // 20 commands per second

        if(BridgeSettingGetUint("Engine", "MaxSkipExceptionCount", &setting))
            maxSkipExceptionCount = setting;
        else
            BridgeSettingSetUint("Engine", "MaxSkipExceptionCount", maxSkipExceptionCount);

        duint newStringAlgorithm = 0;
        if(!BridgeSettingGetUint("Engine", "NewStringAlgorithm", &newStringAlgorithm))
        {
            auto acp = GetACP();
            newStringAlgorithm = acp == 932 || acp == 936 || acp == 949 || acp == 950 || acp == 951 || acp == 1251;
        }
        bNewStringAlgorithm = !!newStringAlgorithm;
    }
    break;

    case DBG_DISASM_FAST_AT:
    {
        if(!param1 || !param2)
            return 0;
        BASIC_INSTRUCTION_INFO* basicinfo = (BASIC_INSTRUCTION_INFO*)param2;
        if(!disasmfast((duint)param1, basicinfo))
            basicinfo->size = 1;
        return 0;
    }
    break;

    case DBG_MENU_ENTRY_CLICKED:
    {
        int hEntry = (int)(duint)param1;
        pluginmenucall(hEntry);
    }
    break;

    case DBG_FUNCTION_GET:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (duint)FunctionGet(info->addr, &info->start, &info->end);
    }
    break;

    case DBG_FUNCTION_OVERLAPS:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (duint)FunctionOverlaps(info->start, info->end);
    }
    break;

    case DBG_FUNCTION_ADD:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (duint)FunctionAdd(info->start, info->end, info->manual);
    }
    break;

    case DBG_FUNCTION_DEL:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (duint)FunctionDelete(info->addr);
    }
    break;

    case DBG_ARGUMENT_GET:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (duint)ArgumentGet(info->addr, &info->start, &info->end);
    }
    break;

    case DBG_ARGUMENT_OVERLAPS:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (duint)ArgumentOverlaps(info->start, info->end);
    }
    break;

    case DBG_ARGUMENT_ADD:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (duint)ArgumentAdd(info->start, info->end, info->manual);
    }
    break;

    case DBG_ARGUMENT_DEL:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (duint)ArgumentDelete(info->addr);
    }
    break;

    case DBG_LOOP_GET:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (duint)LoopGet(info->depth, info->addr, &info->start, &info->end);
    }
    break;

    case DBG_LOOP_OVERLAPS:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (duint)LoopOverlaps(info->depth, info->start, info->end, 0);
    }
    break;

    case DBG_LOOP_ADD:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (duint)LoopAdd(info->start, info->end, info->manual);
    }
    break;

    case DBG_LOOP_DEL:
    {
        FUNCTION_LOOP_INFO* info = (FUNCTION_LOOP_INFO*)param1;
        return (duint)LoopDelete(info->depth, info->addr);
    }
    break;

    case DBG_IS_RUN_LOCKED:
    {
        return (duint)waitislocked(WAITID_RUN);
    }
    break;

    case DBG_IS_BP_DISABLED:
    {
        BREAKPOINT bp;
        if(BpGet((duint)param1, BPNORMAL, 0, &bp))
            return (duint)!bp.enabled;
        return (duint)false;
    }
    break;

    case DBG_SET_AUTO_COMMENT_AT:
    {
        return (duint)CommentSet((duint)param1, (const char*)param2, false);
    }
    break;

    case DBG_DELETE_AUTO_COMMENT_RANGE:
    {
        CommentDelRange((duint)param1, (duint)param2, false);
    }
    break;

    case DBG_SET_AUTO_LABEL_AT:
    {
        return (duint)LabelSet((duint)param1, (const char*)param2, false);
    }
    break;

    case DBG_DELETE_AUTO_LABEL_RANGE:
    {
        LabelDelRange((duint)param1, (duint)param2, false);
    }
    break;

    case DBG_SET_AUTO_BOOKMARK_AT:
    {
        return (duint)BookmarkSet((duint)param1, false);
    }
    break;

    case DBG_DELETE_AUTO_BOOKMARK_RANGE:
    {
        BookmarkDelRange((duint)param1, (duint)param2, false);
    }
    break;

    case DBG_SET_AUTO_FUNCTION_AT:
    {
        return (duint)FunctionAdd((duint)param1, (duint)param2, false);
    }
    break;

    case DBG_DELETE_AUTO_FUNCTION_RANGE:
    {
        FunctionDelRange((duint)param1, (duint)param2, false);
    }
    break;

    case DBG_GET_XREF_COUNT_AT:
    {
        return XrefGetCount((duint)param1);
    }
    break;

    case DBG_XREF_GET:
    {
        if(!param2)
            return false;
        XREF_INFO* info = (XREF_INFO*)param2;
        duint address = (duint)param1;
        info->refcount = XrefGetCount(address);

        if(info->refcount == 0)
        {
            info->references = nullptr;
            return false;
        }
        else
        {
            info->references = (XREF_RECORD*)BridgeAlloc(sizeof(XREF_RECORD) * info->refcount);
            return XrefGet(address, info);
        }
    }
    break;

    case DBG_XREF_ADD:
    {
        return XrefAdd((duint)param1, (duint)param2);
    }
    break;

    case DBG_XREF_DEL_ALL:
    {
        return XrefDeleteAll((duint)param1);
    }
    break;

    case DBG_GET_XREF_TYPE_AT:
    {
        return XrefGetType((duint)param1);
    }
    break;

    case DBG_GET_ENCODE_TYPE_BUFFER:
    {
        return (duint)EncodeMapGetBuffer((duint)param1, (duint*)param2);
    }
    break;

    case DBG_ENCODE_TYPE_GET:
    {
        return EncodeMapGetType((duint)param1, (duint)param2);
    }
    break;

    case DBG_ENCODE_SIZE_GET:
    {
        return EncodeMapGetSize((duint)param1, (duint)param2);
    }
    break;

    case DBG_DELETE_ENCODE_TYPE_RANGE:
    {
        EncodeMapDelRange((duint)param1, (duint)param2);
    }
    break;

    case DBG_DELETE_ENCODE_TYPE_SEG:
    {
        EncodeMapDelSegment((duint)param1);
    }
    break;

    case DBG_RELEASE_ENCODE_TYPE_BUFFER:
    {
        EncodeMapReleaseBuffer(param1);
    }
    break;

    case DBG_GET_STRING_AT:
    {
        return disasmgetstringatwrapper(duint(param1), (char*)param2, true);
    }
    break;

    case DBG_GET_FUNCTIONS:
    {
        return (duint)dbgfunctionsget();
    }
    break;

    case DBG_WIN_EVENT:
    {
        return (duint)pluginwinevent((MSG*)param1, (long*)param2);
    }
    break;

    case DBG_WIN_EVENT_GLOBAL:
    {
        return (duint)pluginwineventglobal((MSG*)param1);
    }
    break;

    case DBG_INITIALIZE_LOCKS:
    {
        SectionLockerGlobal::Initialize();
    }
    break;

    case DBG_DEINITIALIZE_LOCKS:
    {
        SectionLockerGlobal::Deinitialize();
    }
    break;

    case DBG_GET_TIME_WASTED_COUNTER:
        return dbggettimewastedcounter();

    case DBG_DELETE_COMMENT_RANGE:
    {
        CommentDelRange((duint)param1, (duint)param2, true);
    }
    break;

    case DBG_DELETE_LABEL_RANGE:
    {
        LabelDelRange((duint)param1, (duint)param2, true);
    }
    break;

    case DBG_DELETE_BOOKMARK_RANGE:
    {
        BookmarkDelRange((duint)param1, (duint)param2, true);
    }
    break;

    case DBG_GET_WATCH_LIST:
    {
        BridgeList<WATCHINFO>::CopyData((ListInfo*)param1, WatchGetList());
    }
    break;

    case DBG_SELCHANGED:
    {
        PLUG_CB_SELCHANGED plugSelChanged;
        plugSelChanged.hWindow = (int)param1;
        plugSelChanged.VA = (duint)param2;
        plugincbcall(CB_SELCHANGED, &plugSelChanged);
    }
    break;

    case DBG_GET_PROCESS_HANDLE:
    {
        return duint(fdProcessInfo->hProcess);
    }
    break;

    case DBG_GET_THREAD_HANDLE:
    {
        return duint(hActiveThread);
    }
    break;

    case DBG_GET_PROCESS_ID:
    {
        return duint(fdProcessInfo->dwProcessId);
    }
    break;

    case DBG_GET_THREAD_ID:
    {
        return duint(ThreadGetId(hActiveThread));
    }
    break;

    case DBG_GET_PEB_ADDRESS:
    {
        auto ProcessId = DWORD(param1);
        if(ProcessId == fdProcessInfo->dwProcessId)
            return (duint)GetPEBLocation(fdProcessInfo->hProcess);
        auto hProcess = TitanOpenProcess(PROCESS_QUERY_INFORMATION, false, ProcessId);
        duint pebAddress = 0;
        if(hProcess)
        {
            pebAddress = (duint)GetPEBLocation(hProcess);
            CloseHandle(hProcess);
        }
        return pebAddress;
    }
    break;

    case DBG_GET_TEB_ADDRESS:
    {
        auto ThreadId = DWORD(param1);
        auto tebAddress = ThreadGetLocalBase(ThreadId);
        if(tebAddress)
            return tebAddress;
        HANDLE hThread = TitanOpenThread(THREAD_QUERY_INFORMATION, FALSE, ThreadId);
        if(hThread)
        {
            tebAddress = (duint)GetTEBLocation(hThread);
            CloseHandle(hThread);
        }
        return tebAddress;
    }
    break;

    case DBG_ANALYZE_FUNCTION:
    {
        auto entry = duint(param1);
        duint size;
        auto base = MemFindBaseAddr(entry, &size);
        if(!base || !MemIsValidReadPtr(entry))
            return false;
        auto modbase = ModBaseFromAddr(base);
        if(modbase)
            base = modbase, size = ModSizeFromAddr(modbase);
        RecursiveAnalysis analysis(base, size, entry, true);
        analysis.Analyse();
        auto graph = analysis.GetFunctionGraph(entry);
        if(!graph)
            return false;
        *(BridgeCFGraphList*)param2 = graph->ToGraphList();
        return true;
    }
    break;

    case DBG_MENU_PREPARE:
    {
        PLUG_CB_MENUPREPARE info;
        info.hMenu = GUIMENUTYPE(duint(param1));
        plugincbcall(CB_MENUPREPARE, &info);
    }
    break;

    case DBG_GET_SYMBOL_INFO:
    {
        auto symbolptr = (const SYMBOLPTR*)param1;
        ((const SymbolInfoGui*)symbolptr->symbol)->convertToGuiSymbol(symbolptr->modbase, (SYMBOLINFO*)param2);
    }
    break;

    case DBG_GET_DEBUG_ENGINE:
    {
        static auto debugEngine = []
        {
            duint setting = DebugEngineTitanEngine;
            if(!BridgeSettingGetUint("Engine", "DebugEngine", &setting))
            {
                BridgeSettingSetUint("Engine", "DebugEngine", setting);
            }
            return (DEBUG_ENGINE)setting;
        }();
        return debugEngine;
    }
    break;

    case DBG_GET_SYMBOL_INFO_AT:
    {
        return SymbolFromAddressExact((duint)param1, (SYMBOLINFO*)param2);
    }
    break;

    case DBG_XREF_ADD_MULTI:
    {
        return XrefAddMulti((const XREF_EDGE*)param1, (duint)param2);
    }
    break;
    }
    return 0;
}
