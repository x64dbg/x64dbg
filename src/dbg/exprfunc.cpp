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
#include <vector>
#include <regex>
#include <string>
#include <cctype>

/// <summary>
/// Creates an owning ExpressionValue string
/// </summary>
static ExpressionValue ValueString(const char* str)
{
    auto len = ::strlen(str);
    auto buf = (char*)BridgeAlloc(len + 1);
    memcpy(buf, str, len);

    ExpressionValue value = {};
    value.type = ValueTypeString;
    value.string.ptr = buf;
    value.string.isOwner = true;
    return value;
}

/// <summary>
/// Creates an owning ExpressionValue string
/// </summary>
static ExpressionValue ValueString(const String & str)
{
    auto len = str.length();
    auto buf = (char*)BridgeAlloc(len + 1);
    memcpy(buf, str.c_str(), len);

    ExpressionValue value = {};
    value.type = ValueTypeString;
    value.string.ptr = buf;
    value.string.isOwner = true;
    return value;
}

/// <summary>
/// Creates an owning ExpressionValue number
/// </summary>
static ExpressionValue ValueNumber(duint number)
{
    ExpressionValue value = {};
    value.type = ValueTypeNumber;
    value.number = number;
    return value;
}

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
        duint disp;
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
        SHARED_ACQUIRE(LockModules);
        auto info = ModInfoFromAddr(addr);
        if(info)
            return info->party == mod_system;
        else
            return 0;
    }

    duint moduser(duint addr)
    {
        SHARED_ACQUIRE(LockModules);
        auto info = ModInfoFromAddr(addr);
        if(info)
            return info->party == mod_user;
        else
            return 0;
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

    duint modisexport(duint addr)
    {
        SHARED_ACQUIRE(LockModules);
        auto info = ModInfoFromAddr(addr);
        if(info)
        {
            duint rva = addr - info->base;
            return info->findExport(rva) ? 1 : 0;
        }
        return 0;
    }

    bool modbasefromname(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 1);
        assert(argv[0].type == ValueTypeString);

        *result = ValueNumber(ModBaseFromName(argv[0].string.ptr));
        return true;
    }

    static duint selstart(GUISELECTIONTYPE hWindow)
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

    duint kusd()
    {
        return duint(SharedUserData);
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
        return info.branch && !info.call && !::strstr(info.instruction, "jmp");
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
        return ::strstr(info.instruction, "ret") != nullptr;
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
        return info.branch && !::strstr(info.instruction, "jmp") ? addr + info.size : 0;
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

    duint disiscallsystem(duint addr)
    {
        duint dest = disbranchdest(addr);
        return dest && (modsystem(dest) || modsystem(disbranchdest(dest)));
    }

    bool dismnemonic(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 1);
        assert(argv[0].type == ValueTypeNumber);

        String mnemonic = "???";
        auto addr = argv[0].number;
        unsigned char data[MAX_DISASM_BUFFER];
        if(MemRead(addr, data, sizeof(data)))
        {
            Zydis dis;
            if(dis.Disassemble(addr, data))
            {
                mnemonic = dis.Mnemonic();
            }
        }

        *result = ValueString(mnemonic);
        return true;
    }

    bool distext(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 1);
        assert(argv[0].type == ValueTypeNumber);

        std::string text = "???";
        auto addr = argv[0].number;
        unsigned char data[MAX_DISASM_BUFFER];
        if(MemRead(addr, data, sizeof(data)))
        {
            Zydis dis;
            if(dis.Disassemble(addr, data))
            {
                text = dis.InstructionText();
            }
        }

        *result = ValueString(text);
        return true;
    }

    bool dismatch(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 2);
        assert(argv[0].type == ValueTypeNumber);
        assert(argv[1].type == ValueTypeString);

        bool matched = false;
        auto addr = argv[0].number;
        unsigned char data[MAX_DISASM_BUFFER];
        if(MemRead(addr, data, sizeof(data)))
        {
            Zydis dis;
            if(dis.Disassemble(addr, data))
            {
                auto text = dis.InstructionText();
                std::smatch m;
                std::regex re(argv[1].string.ptr);
                matched = std::regex_search(text, m, re);
            }
        }

        *result = ValueNumber(matched);
        return true;
    }

    duint trenabled(duint addr)
    {
        return TraceRecord.getTraceRecordType(addr) != TraceRecordManager::TraceRecordNone;
    }

    duint trhitcount(duint addr)
    {
        return trenabled(addr) ? TraceRecord.getHitCount(addr) : 0;
    }

    duint trisrecording()
    {
        return TraceRecord.isTraceRecordingEnabled() ? 1 : 0;
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
        duint value = 0;
        valfromstring(argExpr(index).c_str(), &value);
        return value;
    }

    duint argset(duint index, duint value)
    {
        auto expr = argExpr(index);
        duint oldvalue = 0;
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

    duint exfirstchance()
    {
        return getLastExceptionInfo().dwFirstChance;
    }

    duint exaddr()
    {
        return (duint)getLastExceptionInfo().ExceptionRecord.ExceptionAddress;
    }

    duint excode()
    {
        return getLastExceptionInfo().ExceptionRecord.ExceptionCode;
    }

    duint exflags()
    {
        return getLastExceptionInfo().ExceptionRecord.ExceptionFlags;
    }

    duint exinfocount()
    {
        return getLastExceptionInfo().ExceptionRecord.NumberParameters;
    }

    duint exinfo(duint index)
    {
        if(index >= EXCEPTION_MAXIMUM_PARAMETERS)
            return 0;
        return getLastExceptionInfo().ExceptionRecord.ExceptionInformation[index];
    }

    duint isdebuggerfocused()
    {
        return GuiIsDebuggerFocused();
    }

    bool streq(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 2);
        assert(argv[0].type == ValueTypeString);
        assert(argv[1].type == ValueTypeString);

        *result = ValueNumber(::strcmp(argv[0].string.ptr, argv[1].string.ptr) == 0);
        return true;
    }

    bool strieq(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 2);
        assert(argv[0].type == ValueTypeString);
        assert(argv[1].type == ValueTypeString);

        *result = ValueNumber(::_stricmp(argv[0].string.ptr, argv[1].string.ptr) == 0);
        return true;
    }

    bool strstr(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 2);
        assert(argv[0].type == ValueTypeString);
        assert(argv[1].type == ValueTypeString);

        *result = ValueNumber(::strstr(argv[0].string.ptr, argv[1].string.ptr) != nullptr);
        return true;
    }

    bool stristr(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 2);
        assert(argv[0].type == ValueTypeString);
        assert(argv[1].type == ValueTypeString);

        size_t len1 = ::strlen(argv[0].string.ptr);
        size_t len2 = ::strlen(argv[1].string.ptr);
        auto it = std::search(
                      argv[0].string.ptr, argv[0].string.ptr + len1,
                      argv[1].string.ptr, argv[1].string.ptr + len2,
        [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
                  );

        *result = ValueNumber(it != argv[0].string.ptr + len1);
        return true;
    }

    bool strlen(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 1);
        assert(argv[0].type == ValueTypeString);

        *result = ValueNumber(::strlen(argv[0].string.ptr));
        return true;
    }

    template<bool Strict>
    bool ansi(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 1);
        assert(argv[0].type == ValueTypeNumber);

        duint addr = argv[0].number;

        std::vector<char> tempStr(MAX_STRING_SIZE + 1);
        duint NumberOfBytesRead = 0;
        if(!MemRead(addr, tempStr.data(), tempStr.size() - 1, &NumberOfBytesRead) && NumberOfBytesRead == 0 && Strict)
        {
            return false;
        }

        *result = ValueString(StringUtils::LocalCpToUtf8(tempStr.data()));
        return true;
    }

    template<bool Strict>
    bool utf8(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 1);
        assert(argv[0].type == ValueTypeNumber);

        duint addr = argv[0].number;

        std::vector<char> tempStr(MAX_STRING_SIZE + 1);
        duint NumberOfBytesRead = 0;
        if(!MemRead(addr, tempStr.data(), tempStr.size() - 1, &NumberOfBytesRead) && NumberOfBytesRead == 0 && Strict)
        {
            return false;
        }

        *result = ValueString(tempStr.data());
        return true;
    }

    template<bool Strict>
    bool utf16(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 1);
        assert(argv[0].type == ValueTypeNumber);

        duint addr = argv[0].number;

        std::vector<wchar_t> tempStr(MAX_STRING_SIZE + 1);
        duint NumberOfBytesRead = 0;
        if(!MemRead(addr, tempStr.data(), sizeof(wchar_t) * (tempStr.size() - 1), &NumberOfBytesRead) && NumberOfBytesRead == 0 && Strict)
        {
            return false;
        }

        auto utf8Str = StringUtils::Utf16ToUtf8(tempStr.data());
        if(utf8Str.empty() && wcslen(tempStr.data()) > 0)
        {
            return false;
        }

        *result = ValueString(utf8Str);

        return true;
    }

    bool ansi(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        return ansi<false>(result, argc, argv, userdata);
    }

    bool ansi_strict(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        return ansi<true>(result, argc, argv, userdata);
    }

    bool utf8(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        return utf8<false>(result, argc, argv, userdata);
    }

    bool utf8_strict(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        return utf8<true>(result, argc, argv, userdata);
    }

    bool utf16(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        return utf16<false>(result, argc, argv, userdata);
    }

    bool utf16_strict(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        return utf16<true>(result, argc, argv, userdata);
    }
}
