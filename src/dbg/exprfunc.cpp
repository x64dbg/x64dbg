#include "exprfunc.h"
#include "symbolinfo.h"
#include "module.h"
#include "debugger.h"
#include "thread.h"
#include "memory.h"
#include "disasm_fast.h"
#include "TraceRecord.h"
#include "disasm_helper.h"
#include "function.h"
#include "value.h"
#include "TraceRecord.h"
#include "exhandlerinfo.h"
#include "threading.h"

namespace Exprfunc
{
    duint srcline(duint addr)
    {
        int line = 0;
        if(!SymGetSourceLine(addr, nullptr, &line, nullptr))
            return 0;
        return line;
    }

    duint srcdisp(duint addr)
    {
        DWORD disp;
        if(!SymGetSourceLine(addr, nullptr, nullptr, &disp))
            return 0;
        return disp;
    }

    duint modparty(duint addr)
    {
        return ModGetParty(addr);
    }

    duint modsystem(duint addr)
    {
        return ModGetParty(addr) == 1;
    }

    duint moduser(duint addr)
    {
        return ModGetParty(addr) == 0;
    }

    duint modrva(duint addr)
    {
        auto base = ModBaseFromAddr(addr);
        return base ? addr - base : 0;
    }

    duint modheaderva(duint addr)
    {
        SHARED_ACQUIRE(LockModules);
        auto info = ModInfoFromAddr(addr);
        if(info)
            return (addr - info->base) + info->headerImageBase;
        else
            return 0;
    }

    static duint selstart(int hWindow)
    {
        SELECTIONDATA selection;
        GuiSelectionGet(hWindow, &selection);
        return selection.start;
    }

    duint disasmsel()
    {
        return selstart(GUI_DISASSEMBLY);
    }

    duint dumpsel()
    {
        return selstart(GUI_DUMP);
    }

    duint stacksel()
    {
        return selstart(GUI_STACK);
    }

    duint peb()
    {
        return duint(GetPEBLocation(fdProcessInfo->hProcess));
    }

    duint teb()
    {
        return duint(GetTEBLocation(hActiveThread));
    }

    duint tid()
    {
        return duint(ThreadGetId(hActiveThread));
    }

    duint bswap(duint value)
    {
        duint result = 0;
        for(size_t i = 0; i < sizeof(value); i++)
            ((unsigned char*)&result)[sizeof(value) - i - 1] = ((unsigned char*)&value)[i];
        return result;
    }

    duint ternary(duint condition, duint value1, duint value2)
    {
        return condition ? value1 : value2;
    }

    duint memvalid(duint addr)
    {
        return MemIsValidReadPtr(addr, true);
    }

    duint membase(duint addr)
    {
        return MemFindBaseAddr(addr, nullptr);
    }

    duint memsize(duint addr)
    {
        duint size;
        return MemFindBaseAddr(addr, &size) ? size : 0;
    }

    duint memiscode(duint addr)
    {
        return MemIsCodePage(addr, false);
    }

    duint memisstring(duint addr)
    {
        STRING_TYPE strType;
        disasmispossiblestring(addr, &strType);
        if(strType != STRING_TYPE::str_none)
            return strType == STRING_TYPE::str_unicode ? 2 : 1;
        else
            return 0;
    }

    duint memdecodepointer(duint ptr)
    {
        auto decoded = ptr;
        return MemDecodePointer(&decoded, IsVistaOrLater()) ? decoded : ptr;
    }

    duint dislen(duint addr)
    {
        BASIC_INSTRUCTION_INFO info;
        return disasmfast(addr, &info, true) ? info.size : 0;
    }

    duint disiscond(duint addr)
    {
        BASIC_INSTRUCTION_INFO info;
        if(!disasmfast(addr, &info, true))
            return 0;
        return info.branch && !info.call && !strstr(info.instruction, "jmp");
    }

    duint disisbranch(duint addr)
    {
        BASIC_INSTRUCTION_INFO info;
        if(!disasmfast(addr, &info, true))
            return 0;
        return info.branch;
    }

    duint disisret(duint addr)
    {
        BASIC_INSTRUCTION_INFO info;
        if(!disasmfast(addr, &info, true))
            return 0;
        return strstr(info.instruction, "ret") != nullptr;
    }

    duint disiscall(duint addr)
    {
        BASIC_INSTRUCTION_INFO info;
        if(!disasmfast(addr, &info, true))
            return 0;
        return info.call;
    }

    duint disismem(duint addr)
    {
        BASIC_INSTRUCTION_INFO info;
        if(!disasmfast(addr, &info, true))
            return 0;
        return (info.type & TYPE_MEMORY) == TYPE_MEMORY;
    }

    duint disisnop(duint addr)
    {
        unsigned char data[16];
        if(MemRead(addr, data, sizeof(data), nullptr, true))
        {
            Zydis cp;
            if(cp.Disassemble(addr, data))
                return cp.IsNop();
        }
        return false;
    }

    duint disisunusual(duint addr)
    {
        unsigned char data[16];
        if(MemRead(addr, data, sizeof(data), nullptr, true))
        {
            Zydis cp;
            if(cp.Disassemble(addr, data))
                return cp.IsUnusual();
        }
        return false;
    }

    duint disbranchdest(duint addr)
    {
        return DbgGetBranchDestination(addr);
    }

    duint disbranchexec(duint addr)
    {
        return DbgIsJumpGoingToExecute(addr);
    }

    duint disimm(duint addr)
    {
        BASIC_INSTRUCTION_INFO info;
        if(!disasmfast(addr, &info, true))
            return 0;
        return info.value.value;
    }

    duint disbrtrue(duint addr)
    {
        BASIC_INSTRUCTION_INFO info;
        if(!disasmfast(addr, &info, true))
            return 0;
        return info.branch ? info.addr : 0;
    }

    duint disbrfalse(duint addr)
    {
        BASIC_INSTRUCTION_INFO info;
        if(!disasmfast(addr, &info, true))
            return 0;
        return info.branch && !strstr(info.instruction, "jmp") ? addr + info.size : 0;
    }

    duint disnext(duint addr)
    {
        BASIC_INSTRUCTION_INFO info;
        if(!disasmfast(addr, &info, true))
            return 0;
        return addr + info.size;
    }

    duint disprev(duint addr)
    {
        duint size = 0;
        duint base = MemFindBaseAddr(addr, &size);
        duint readStart = addr - 16 * 4;
        if(readStart < base)
            readStart = base;
        unsigned char disasmData[256];
        MemRead(readStart, disasmData, sizeof(disasmData));
        return readStart + disasmback(disasmData, 0, sizeof(disasmData), addr - readStart, 1);
    }

    duint trenabled(duint addr)
    {
        return TraceRecord.getTraceRecordType(addr) != TraceRecordManager::TraceRecordNone;
    }

    duint trhitcount(duint addr)
    {
        return trenabled(addr) ? TraceRecord.getHitCount(addr) : 0;
    }

    duint trisruntraceenabled()
    {
        return _dbg_dbgisRunTraceEnabled() ? 1 : 0;
    }

    duint gettickcount()
    {
#ifdef _WIN64
        static auto GTC64 = (duint(*)())GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetTickCount64");
        if(GTC64)
            return GTC64();
#endif //_WIN64
        return GetTickCount();
    }

    static duint readMem(duint addr, duint size)
    {
        duint value = 0;
        return MemRead(addr, &value, size) ? value : 0;
    }

    duint readbyte(duint addr)
    {
        return readMem(addr, 1);
    }

    duint readword(duint addr)
    {
        return readMem(addr, 2);
    }

    duint readdword(duint addr)
    {
        return readMem(addr, 4);
    }

    duint readqword(duint addr)
    {
        return readMem(addr, 8);
    }

    duint readptr(duint addr)
    {
        return readMem(addr, sizeof(duint));
    }

    duint funcstart(duint addr)
    {
        duint start;
        return FunctionGet(addr, &start) ? start : 0;
    }

    duint funcend(duint addr)
    {
        duint end;
        return FunctionGet(addr, nullptr, &end) ? end : 0;
    }

    duint refcount()
    {
        return GuiReferenceGetRowCount();
    }

    duint refaddr(duint row)
    {
        auto content = GuiReferenceGetCellContent(int(row), 0);
        duint addr = 0;
        valfromstring(content, &addr, false);
        BridgeFree(content);
        return addr;
    }

    duint refsearchcount()
    {
        return GuiReferenceSearchGetRowCount();
    }

    duint refsearchaddr(duint row)
    {
        auto content = GuiReferenceSearchGetCellContent(int(row), 0);
        duint addr = 0;
        valfromstring(content, &addr, false);
        BridgeFree(content);
        return addr;
    }

    static String argExpr(duint index)
    {
#ifdef _WIN64
        //http://msdn.microsoft.com/en-us/library/windows/hardware/ff561499(v=vs.85).aspx
        switch(index)
        {
        case 0:
            return "rcx";
        case 1:
            return "rdx";
        case 2:
            return "r8";
        case 3:
            return "r9";
        default:
            break;
        }
#endif
        return StringUtils::sprintf("[csp+%X]", int32_t(++index * sizeof(duint)));
    }

    duint argget(duint index)
    {
        duint value;
        valfromstring(argExpr(index).c_str(), &value);
        return value;
    }

    duint argset(duint index, duint value)
    {
        auto expr = argExpr(index);
        duint oldvalue;
        valfromstring(expr.c_str(), &oldvalue);
        valtostring(expr.c_str(), value, true);
        return oldvalue;
    }

    duint bpgoto(duint cip)
    {
        //This is a function to sets CIP without calling DebugUpdateGui. This is a workaround for "bpgoto".
        SetContextDataEx(hActiveThread, UE_CIP, cip);
        return cip;
    }
}
