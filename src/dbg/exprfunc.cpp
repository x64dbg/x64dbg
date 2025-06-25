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
#include "exception.h"
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

    bool ternary(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        *result = argv[0].number ? argv[1] : argv[2];
        result->string.isOwner = false;
        return true;
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

    bool memmatch(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 2);
        assert(argv[0].type == ValueTypeNumber);
        assert(argv[1].type == ValueTypeString);

        std::vector<PatternByte> pattern;
        if(!patterntransform(argv[1].string.ptr, pattern))
            return false;

        std::vector<unsigned char> data(pattern.size());
        if(!MemRead(argv[0].number, data.data(), data.size()))
            return false;

        *result = ValueNumber(patternfind(data.data(), pattern.size(), pattern) == 0);


        return true;
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
            Zydis zydis;
            if(zydis.Disassemble(addr, data))
                return zydis.IsNop();
        }
        return false;
    }

    duint disisunusual(duint addr)
    {
        unsigned char data[16];
        if(MemRead(addr, data, sizeof(data), nullptr, true))
        {
            Zydis zydis;
            if(zydis.Disassemble(addr, data))
                return zydis.IsUnusual();
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
        static auto GTC64 = (ULONGLONG(*)())GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetTickCount64");
        if(GTC64)
            return GTC64();
#endif //_WIN64
        return GetTickCount();
    }

    duint rdtsc()
    {
        return (duint)__rdtsc();
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

    duint isprocessfocused(DWORD process)
    {
        HWND foreground = GetForegroundWindow();
        if(foreground)
        {
            DWORD pid;
            DWORD tid = GetWindowThreadProcessId(foreground, &pid);
            return pid == process;
        }
        else
            return 0;
    }

    duint isdebuggerfocused()
    {
        return isprocessfocused(GetCurrentProcessId());
    }

    duint isdebuggeefocused()
    {
        if(!DbgIsDebugging())
            return 0;
        return isprocessfocused(fdProcessInfo->dwProcessId);
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
        auto lowercompare = [](char ch1, char ch2)
        {
            return StringUtils::ToLower(ch1) == StringUtils::ToLower(ch2);
        };
        auto found = std::search(
                         argv[0].string.ptr, argv[0].string.ptr + len1,
                         argv[1].string.ptr, argv[1].string.ptr + len2,
                         lowercompare
                     ) != argv[0].string.ptr + len1;

        *result = ValueNumber(found);
        return true;
    }

    bool strlen(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 1);
        assert(argv[0].type == ValueTypeString);

        *result = ValueNumber(::strlen(argv[0].string.ptr));
        return true;
    }

    template<bool Strict, typename T>
    bool readstring(std::vector<T> & temp, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc >= 1);
        assert(argv[0].type == ValueTypeNumber);

        duint addr = argv[0].number;

        auto truncate = argc > 1;
        if(truncate)
        {
            assert(argv[1].type == ValueTypeNumber);
            temp.resize(argv[1].number + 1);
        }
        else
        {
            // TODO: check for null-termination and resize accordingly
            temp.resize(MAX_STRING_SIZE + 1);
        }

        duint NumberOfBytesRead = 0;
        if(!MemRead(addr, temp.data(), temp.size() - 1, &NumberOfBytesRead) && NumberOfBytesRead == 0 && Strict)
        {
            return false;
        }

        return true;
    }

    template<bool Strict>
    bool ansi(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        std::vector<char> temp;
        if(!readstring<Strict>(temp, argc, argv, userdata))
            return false;

        *result = ValueString(StringUtils::LocalCpToUtf8(temp.data()));
        return true;
    }

    template<bool Strict>
    bool utf8(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        std::vector<char> temp;
        if(!readstring<Strict>(temp, argc, argv, userdata))
            return false;

        *result = ValueString(temp.data());
        return true;
    }

    template<bool Strict>
    bool utf16(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        std::vector<wchar_t> temp;
        if(!readstring<Strict>(temp, argc, argv, userdata))
            return false;

        auto utf8Str = StringUtils::Utf16ToUtf8(temp.data());
        if(utf8Str.empty() && wcslen(temp.data()) > 0)
            return false;

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

    bool syscall_name(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        *result = ValueString(SyscallToName((unsigned int)argv[0].number));
        return true;
    }

    bool syscall_id(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        auto id = SyscallToId(argv[0].string.ptr);
        if(id == -1)
            return false;

        *result = ValueNumber(id);
        return true;
    }

    bool strlower(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 1);
        assert(argv[0].type == ValueTypeString);

        *result = ValueString(StringUtils::ToLower(argv[0].string.ptr));
        return true;
    }

    bool strupper(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 1);
        assert(argv[0].type == ValueTypeString);

        String str = argv[0].string.ptr;
        for(auto & ch : str)
            ch = toupper(ch);

        *result = ValueString(str);
        return true;
    }

    bool strcat(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 2);
        assert(argv[0].type == ValueTypeString);
        assert(argv[1].type == ValueTypeString);

        String str = argv[0].string.ptr;
        str += argv[1].string.ptr;

        *result = ValueString(str);
        return true;
    }

    bool substr(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc >= 2 && argc <= 3);
        assert(argv[0].type == ValueTypeString);
        assert(argv[1].type == ValueTypeNumber);

        String str = argv[0].string.ptr;
        size_t start = (size_t)argv[1].number;

        if(start >= str.length())
        {
            *result = ValueString("");
            return true;
        }

        size_t length = str.length() - start;
        if(argc == 3)
        {
            assert(argv[2].type == ValueTypeNumber);
            length = std::min(length, (size_t)argv[2].number);
        }

        *result = ValueString(str.substr(start, length));
        return true;
    }

    bool strchr(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 2);
        assert(argv[0].type == ValueTypeString);
        assert(argv[1].type == ValueTypeString);

        String str = argv[0].string.ptr;
        String needle = argv[1].string.ptr;

        size_t pos = str.find(needle);
        *result = ValueNumber(pos == String::npos ? -1 : (duint)pos);
        return true;
    }

    bool strrchr(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 2);
        assert(argv[0].type == ValueTypeString);
        assert(argv[1].type == ValueTypeString);

        String str = argv[0].string.ptr;
        String needle = argv[1].string.ptr;

        size_t pos = str.rfind(needle);
        *result = ValueNumber(pos == String::npos ? -1 : (duint)pos);
        return true;
    }

    bool strreplace(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 3);
        assert(argv[0].type == ValueTypeString);
        assert(argv[1].type == ValueTypeString);
        assert(argv[2].type == ValueTypeString);

        String str = argv[0].string.ptr;
        String from = argv[1].string.ptr;
        String to = argv[2].string.ptr;

        StringUtils::ReplaceAll(str, from, to);
        *result = ValueString(str);
        return true;
    }

    bool strreplace_first(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 3);
        assert(argv[0].type == ValueTypeString);
        assert(argv[1].type == ValueTypeString);
        assert(argv[2].type == ValueTypeString);

        String str = argv[0].string.ptr;
        String from = argv[1].string.ptr;
        String to = argv[2].string.ptr;

        size_t pos = str.find(from);
        if(pos != String::npos)
            str.replace(pos, from.length(), to);

        *result = ValueString(str);
        return true;
    }

    bool strreplace_last(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 3);
        assert(argv[0].type == ValueTypeString);
        assert(argv[1].type == ValueTypeString);
        assert(argv[2].type == ValueTypeString);

        String str = argv[0].string.ptr;
        String from = argv[1].string.ptr;
        String to = argv[2].string.ptr;

        size_t pos = str.rfind(from);
        if(pos != String::npos)
            str.replace(pos, from.length(), to);

        *result = ValueString(str);
        return true;
    }

    bool streval(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 1);
        assert(argv[0].type == ValueTypeString);

        duint value;
        if(!valfromstring(argv[0].string.ptr, &value))
            return false;

        *result = ValueNumber(value);
        return true;
    }

    bool strtrim(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        assert(argc == 1);
        assert(argv[0].type == ValueTypeString);

        *result = ValueString(StringUtils::Trim(argv[0].string.ptr));
        return true;
    }
}
