#include "cmd-undocumented.h"
#include "console.h"
#include "function.h"
#include "bookmark.h"
#include "label.h"
#include "comment.h"
#include "debugger.h"
#include "variable.h"
#include "loop.h"
#include "zydis_wrapper.h"
#include "mnemonichelp.h"
#include "value.h"
#include "symbolinfo.h"
#include "argument.h"
#include "expressionparser.h"

bool cbBadCmd(int argc, char* argv[])
{
    duint value = 0;
    int valsize = 0;
    bool isvar = false;
    bool hexonly = false;
    bool silent = false;
    bool baseonly = false;
    bool allowassign = true;
    ExpressionParser parser(argv[0]);
    ExpressionParser::EvalValue evalue(0);
    if(parser.Calculate(evalue, valuesignedcalc(), allowassign, silent, baseonly, &valsize, &isvar, &hexonly))
    {
        if(evalue.isString)
        {
            varset("$ans", evalue.data.c_str(), true);
            dprintf_untranslated("\"%s\"\n", StringUtils::Escape(evalue.data).c_str());
        }
        else if(evalue.DoEvaluate(value, silent, baseonly, &valsize, &isvar, &hexonly))
        {
            varset("$ans", value, true);
            if(valsize)
                valsize *= 2;
            else
                valsize = 1;
            char format_str[deflen] = "";
            auto symbolic = SymGetSymbolicName(value, false);
            if(symbolic.length())
                symbolic = " " + symbolic;
            if(isvar)  // and *cmd!='.' and *cmd!='x') //prevent stupid 0=0 stuff
            {
                if(value > 9 && !hexonly)
                {
                    if(!valuesignedcalc())  //signed numbers
#ifdef _WIN64
                        sprintf_s(format_str, "%%s=%%.%dllX (%%llud)%%s\n", valsize); // TODO: This and the following statements use "%llX" for a "int"-typed variable. Maybe we can use "%X" everywhere?
#else //x86
                        sprintf_s(format_str, "%%s=%%.%dX (%%ud)%%s\n", valsize);
#endif //_WIN64
                    else
#ifdef _WIN64
                        sprintf_s(format_str, "%%s=%%.%dllX (%%lld)%%s\n", valsize);
#else //x86
                        sprintf_s(format_str, "%%s=%%.%dX (%%d)%%s\n", valsize);
#endif //_WIN64
                    dprintf_untranslated(format_str, *argv, value, value, symbolic.c_str());
                }
                else
                {
                    sprintf_s(format_str, "%%s=%%.%dX%%s\n", valsize);
                    dprintf_untranslated(format_str, *argv, value, symbolic.c_str());
                }
            }
            else
            {
                if(value > 9 && !hexonly)
                {
                    if(!valuesignedcalc())  //signed numbers
#ifdef _WIN64
                        sprintf_s(format_str, "%%s=%%.%dllX (%%llud)%%s\n", valsize);
#else //x86
                        sprintf_s(format_str, "%%s=%%.%dX (%%ud)%%s\n", valsize);
#endif //_WIN64
                    else
#ifdef _WIN64
                        sprintf_s(format_str, "%%s=%%.%dllX (%%lld)%%s\n", valsize);
#else //x86
                        sprintf_s(format_str, "%%s=%%.%dX (%%d)%%s\n", valsize);
#endif //_WIN64
#ifdef _WIN64
                    sprintf_s(format_str, "%%.%dllX (%%llud)%%s\n", valsize);
#else //x86
                    sprintf_s(format_str, "%%.%dX (%%ud)%%s\n", valsize);
#endif //_WIN64
                    dprintf_untranslated(format_str, value, value, symbolic.c_str());
                }
                else
                {
#ifdef _WIN64
                    sprintf_s(format_str, "%%.%dllX%%s\n", valsize);
#else //x86
                    sprintf_s(format_str, "%%.%dX%%s\n", valsize);
#endif //_WIN64
                    dprintf_untranslated(format_str, value, symbolic.c_str());
                }
            }
        }
        else //unknown command
        {
            dprintf_untranslated("Unknown command/expression: \"%s\"\n", *argv);
            return false;
        }
    }
    return true;
}

bool cbDebugBenchmark(int argc, char* argv[])
{
    duint addr = MemFindBaseAddr(GetContextDataEx(hActiveThread, UE_CIP), 0);
    DWORD ticks = GetTickCount();
    for(duint i = addr; i < addr + 100000; i++)
    {
        CommentSet(i, "test", false);
        LabelSet(i, "test", false);
        BookmarkSet(i, false);
        FunctionAdd(i, i, false);
        ArgumentAdd(i, i, false);
        LoopAdd(i, i, false);
    }
    dprintf_untranslated("%ums\n", GetTickCount() - ticks);
    return true;
}

bool cbInstrSetstr(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    varnew(argv[1], 0, VAR_USER);
    if(!vargettype(argv[1], 0))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such variable \"%s\"!\n"), argv[1]);
        return false;
    }
    if(!varset(argv[1], argv[2], false))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to set variable \"%s\"!\n"), argv[1]);
        return false;
    }
    cmddirectexec(StringUtils::sprintf("getstr \"%s\"", argv[1]).c_str());
    return true;
}

bool cbInstrGetstr(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    VAR_VALUE_TYPE valtype;
    if(!vargettype(argv[1], 0, &valtype))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such variable \"%s\"!\n"), argv[1]);
        return false;
    }
    if(valtype != VAR_STRING)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Variable \"%s\" is not a string!\n"), argv[1]);
        return false;
    }
    int size;
    if(!varget(argv[1], (char*)0, &size, 0) || !size)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to get variable size \"%s\"!\n"), argv[1]);
        return false;
    }
    Memory<char*> string(size + 1, "cbInstrGetstr:string");
    if(!varget(argv[1], string(), &size, 0))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to get variable data \"%s\"!\n"), argv[1]);
        return false;
    }
    dprintf_untranslated("%s=\"%s\"\n", argv[1], string());
    return true;
}

bool cbInstrCopystr(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    VAR_VALUE_TYPE valtype;
    if(!vargettype(argv[2], 0, &valtype))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "No such variable \"%s\"!\n"), argv[2]);
        return false;
    }
    if(valtype != VAR_STRING)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Variable \"%s\" is not a string!\n"), argv[2]);
        return false;
    }
    int size;
    if(!varget(argv[2], (char*)0, &size, 0) || !size)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to get variable size \"%s\"!\n"), argv[2]);
        return false;
    }
    Memory<char*> string(size + 1, "cbInstrGetstr:string");
    if(!varget(argv[2], string(), &size, 0))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to get variable data \"%s\"!\n"), argv[2]);
        return false;
    }
    duint addr;
    if(!valfromstring(argv[1], &addr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid address \"%s\"!\n"), argv[1]);
        return false;
    }
    if(!MemPatch(addr, string(), strlen(string())))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "MemPatch failed!"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "String written!"));
    GuiUpdateAllViews();
    GuiUpdatePatches();
    return true;
}

bool cbInstrZydis(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;

    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !MemIsValidReadPtr(addr))
    {
        dprintf_untranslated("Invalid address \"%s\"\n", argv[1]);
        return false;
    }

    unsigned char data[16];
    if(!MemRead(addr, data, sizeof(data)))
    {
        dprintf_untranslated("Could not read memory at %p\n", addr);
        return false;
    }

    if(argc > 2)
        if(!valfromstring(argv[2], &addr, false))
            return false;

    Zydis zydis;
    if(!zydis.Disassemble(addr, data))
    {
        dputs_untranslated("Failed to disassemble!\n");
        return false;
    }

    auto instr = zydis.GetInstr();
    int argcount = zydis.OpCount();
    dputs_untranslated(zydis.InstructionText(true).c_str());
    dprintf_untranslated("prefix size: %d\n", instr->info.raw.prefix_count);
    if(instr->info.attributes & ZYDIS_ATTRIB_HAS_REX)
    {
        auto rexdata = data[instr->info.raw.rex.offset];
        dprintf_untranslated("rex.W: %d, rex.R: %d, rex.X: %d, rex.B: %d, rex.data: %02x\n", instr->info.raw.rex.W, instr->info.raw.rex.R, instr->info.raw.rex.X, instr->info.raw.rex.B, rexdata);
    }
    dprintf_untranslated("disp.offset: %d, disp.size: %d\n", instr->info.raw.disp.offset, instr->info.raw.disp.size);
    dprintf_untranslated("imm[0].offset: %d, imm[0].size: %d\n", instr->info.raw.imm[0].offset, instr->info.raw.imm[0].size);
    dprintf_untranslated("imm[1].offset: %d, imm[1].size: %d\n", instr->info.raw.imm[1].offset, instr->info.raw.imm[1].size);
    dprintf_untranslated("size: %d, id: %d, opcount: %d\n", zydis.Size(), zydis.GetId(), instr->info.operand_count);
    auto rwstr = [](uint8_t action)
    {
        switch(action)
        {
        case ZYDIS_OPERAND_ACTION_READ:
        case ZYDIS_OPERAND_ACTION_CONDREAD:
            return "read";
        case ZYDIS_OPERAND_ACTION_WRITE:
        case ZYDIS_OPERAND_ACTION_CONDWRITE:
            return "write";
        case ZYDIS_OPERAND_ACTION_READWRITE:
        case ZYDIS_OPERAND_ACTION_READ_CONDWRITE:
        case ZYDIS_OPERAND_ACTION_CONDREAD_WRITE:
            return "read+write";
        default:
            return "???";
        }
    };
    auto vis = [](uint8_t visibility)
    {
        switch(visibility)
        {
        case ZYDIS_OPERAND_VISIBILITY_INVALID:
            return "invalid";
        case ZYDIS_OPERAND_VISIBILITY_EXPLICIT:
            return "explicit";
        case ZYDIS_OPERAND_VISIBILITY_IMPLICIT:
            return "implicit";
        case ZYDIS_OPERAND_VISIBILITY_HIDDEN:
            return "hidden";
        default:
            return "???";
        }
    };
    for(int i = 0; i < argcount; i++)
    {
        const auto & op = instr->operands[i];
        dprintf("operand %d (size: %d, access: %s, visibility: %s) \"%s\", ", i + 1, op.size, rwstr(op.actions), vis(op.visibility), zydis.OperandText(i).c_str());
        switch(op.type)
        {
        case ZYDIS_OPERAND_TYPE_REGISTER:
            dprintf_untranslated("register: %s\n", zydis.RegName(op.reg.value));
            break;
        case ZYDIS_OPERAND_TYPE_IMMEDIATE:
            dprintf_untranslated("immediate: 0x%p\n", op.imm.value.u);
            break;
        case ZYDIS_OPERAND_TYPE_MEMORY:
        {
            //[base + index * scale +/- disp]
            const auto & mem = op.mem;
            dprintf_untranslated("memory segment: %s, base: %s, index: %s, scale: %d, displacement: 0x%p\n",
                                 zydis.RegName(mem.segment),
                                 zydis.RegName(mem.base),
                                 zydis.RegName(mem.index),
                                 mem.scale,
                                 mem.disp.value);
        }
        break;
        case ZYDIS_OPERAND_TYPE_POINTER:
            dprintf_untranslated("pointer: %X:%p\n", op.ptr.segment, op.ptr.offset);
            break;
        default:
            break;
        }
    }

    return true;
}

bool cbInstrVisualize(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint start;
    duint maxaddr;
    if(!valfromstring(argv[1], &start) || !valfromstring(argv[2], &maxaddr))
    {
        dputs_untranslated("Invalid arguments!");
        return false;
    }
    //actual algorithm
    //make sure to set these options in the INI (rest the default theme of x64dbg):
    //DisassemblyBookmarkBackgroundColor = #00FFFF
    //DisassemblyBookmarkColor = #000000
    //DisassemblyHardwareBreakpointBackgroundColor = #00FF00
    //DisassemblyHardwareBreakpointColor = #000000
    //DisassemblyBreakpointBackgroundColor = #FF0000
    //DisassemblyBreakpointColor = #000000
    {
        //initialize
        Zydis zydis;
        duint _base = start;
        duint _size = maxaddr - start;
        Memory<unsigned char*> _data(_size);
        MemRead(_base, _data(), _size);
        FunctionClear();

        //linear search with some trickery
        duint end = 0;
        duint jumpback = 0;
        for(duint addr = start, fardest = 0; addr < maxaddr;)
        {
            //update GUI
            BpClear();
            BookmarkClear();
            LabelClear();
            SetContextDataEx(hActiveThread, UE_CIP, addr);
            if(end)
                BpNew(end, true, false, 0, BPNORMAL, 0, nullptr);
            if(jumpback)
                BookmarkSet(jumpback, false);
            if(fardest)
                BpNew(fardest, true, false, 0, BPHARDWARE, 0, nullptr);
            DebugUpdateGuiAsync(addr, false);
            Sleep(300);

            //continue algorithm
            const unsigned char* curData = (addr >= _base && addr < _base + _size) ? _data() + (addr - _base) : nullptr;
            if(zydis.Disassemble(addr, curData, MAX_DISASM_BUFFER))
            {
                if(addr + zydis.Size() > maxaddr) //we went past the maximum allowed address
                    break;

                if((zydis.IsJump() || zydis.IsLoop()) && zydis.OpCount() && zydis[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE) //jump
                {
                    duint dest = (duint)zydis[0].imm.value.u;

                    if(dest >= maxaddr) //jump across function boundaries
                    {
                        //currently unused
                    }
                    else if(dest > addr && dest > fardest) //save the farthest JXX destination forward
                    {
                        fardest = dest;
                    }
                    else if(end && dest < end && zydis.GetId() == ZYDIS_MNEMONIC_JMP) //save the last JMP backwards
                    {
                        jumpback = addr;
                    }
                }
                else if(zydis.IsRet()) //possible function end?
                {
                    end = addr;
                    if(fardest < addr) //we stop if the farthest JXX destination forward is before this RET
                        break;
                }

                addr += zydis.Size();
            }
            else
                addr++;
        }
        end = end < jumpback ? jumpback : end;

        //update GUI
        FunctionAdd(start, end, false);
        BpClear();
        BookmarkClear();
        SetContextDataEx(hActiveThread, UE_CIP, start);
        DebugUpdateGuiAsync(start, false);
    }
    return true;
}

bool cbInstrMeminfo(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs_untranslated("Usage: meminfo a/r, addr[, size]");
        return false;
    }
    duint addr;
    if(!valfromstring(argv[2], &addr))
    {
        dputs_untranslated("Invalid argument");
        return false;
    }
    if(argv[1][0] == 'a')
    {
        duint size = 1;
        if(argc > 3 && !valfromstring(argv[3], &size))
        {
            dputs_untranslated("Invalid argument");
            return false;
        }
        std::vector<uint8_t> buf;
        buf.resize(size);
        SIZE_T NumberOfBytesRead = 0;
        ReadProcessMemory(fdProcessInfo->hProcess, (const void*)addr, buf.data(), buf.size(), &NumberOfBytesRead);
        dprintf_untranslated("Data: %s\n", StringUtils::ToHex(buf.data(), NumberOfBytesRead).c_str());
    }
    else if(argv[1][0] == 'r')
    {
        MemUpdateMap();
        GuiUpdateMemoryView();
        dputs_untranslated("Memory map updated!");
    }
    return true;
}

bool cbInstrBriefcheck(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    duint size;
    auto base = DbgMemFindBaseAddr(addr, &size);
    if(!base)
        return false;
    Memory<unsigned char*> buffer(size + 16);
    DbgMemRead(base, buffer(), size);
    Zydis zydis;
    std::unordered_set<String> reported;
    for(duint i = 0; i < size;)
    {
        if(!zydis.Disassemble(base + i, buffer() + i, 16))
        {
            i++;
            continue;
        }
        i += zydis.Size();
        auto mnem = StringUtils::ToLower(zydis.Mnemonic());
        auto brief = MnemonicHelp::getBriefDescription(mnem.c_str());
        if(brief.length() || reported.count(mnem))
            continue;
        reported.insert(mnem);
        dprintf_untranslated("%p: %s\n", zydis.Address(), mnem.c_str());
    }
    return true;
}

bool cbInstrFocusinfo(int argc, char* argv[])
{
    ACTIVEVIEW activeView;
    GuiGetActiveView(&activeView);
    dprintf_untranslated("activeTitle: %s, activeClass: %s\n", activeView.title, activeView.className);
    return true;
}

bool cbInstrFlushlog(int argc, char* argv[])
{
    GuiFlushLog();
    return true;
}

extern char animate_command[deflen];

bool cbInstrAnimateWait(int argc, char* argv[])
{
    while(DbgIsDebugging() && dbgisrunning() && animate_command[0] != 0) //while not locked (NOTE: possible deadlock)
    {
        Sleep(1);
    }
    return true;
}

#include <lz4/lz4file.h>

bool cbInstrDbdecompress(int argc, char* argv[])
{
    if(argc < 2)
    {
        dprintf_untranslated("Usage: dbdecompress \"c:\\path\\to\\db\"\n");
        return false;
    }
    auto dbFile = StringUtils::Utf8ToUtf16(argv[1]);
    if(LZ4_decompress_fileW(dbFile.c_str(), dbFile.c_str()) != LZ4_SUCCESS)
    {
        dprintf_untranslated("Failed to decompress '%s'\n", argv[1]);
        return false;
    }
    dprintf_untranslated("Decompressed '%s'\n", argv[1]);
    return true;
}

bool cbInstrDebugFlags(int argc, char* argv[])
{
    if(argc < 2)
    {
        dprintf_untranslated("Usage: DebugFlags 0xFFFFFFFF\n");
        return false;
    }
    auto debugFlags = (DWORD)DbgValFromString(argv[1]);
    dbgsetdebugflags(debugFlags);
    dprintf_untranslated("DebugFlags = 0x%08X\n", debugFlags);
    return true;
}

bool cbInstrLabelRuntimeFunctions(int argc, char* argv[])
{
#ifdef _WIN64
    if(argc < 2)
    {
        dputs_untranslated("Usage: LabelRuntimeFunctions modaddr");
        return false;
    }
    auto modaddr = DbgValFromString(argv[1]);
    SHARED_ACQUIRE(LockModules);
    auto info = ModInfoFromAddr(modaddr);
    if(info)
    {
        std::vector<COMMENTSINFO> comments;
        CommentGetList(comments);
        for(const auto & comment : comments)
        {
            if(comment.modhash == info->hash)
            {
                if(!comment.manual && comment.text.find("RUNTIME_FUNCTION") == 0)
                {
                    CommentDelete(comment.addr + info->base);
                }
            }
        }
        for(const auto & runtimeFunction : info->runtimeFunctions)
        {
            auto setComment = [info](duint addr, const char* prefix)
            {
                char comment[MAX_COMMENT_SIZE] = "";
                if(!CommentGet(addr, comment))
                    strncpy_s(comment, "RUNTIME_FUNCTION", _TRUNCATE);
                strncat_s(comment, " ", _TRUNCATE);
                strncat_s(comment, prefix, _TRUNCATE);
                CommentSet(addr, comment, false);
            };
            setComment(info->base + runtimeFunction.BeginAddress, "BeginAddress");
            setComment(info->base + runtimeFunction.EndAddress, "EndAddress");
        }
        GuiUpdateAllViews();
    }
    else
    {
        dprintf_untranslated("No module found at %p\n", modaddr);
    }
    return true;
#else
    return false;
#endif // _WIN64
}

bool cbInstrCmdTest(int argc, char* argv[])
{
    for(int i = 0; i < argc; i++)
        dprintf_untranslated("argv[%d]:%s\n", i, argv[i]);
    return true;
}
