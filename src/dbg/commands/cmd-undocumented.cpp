#include "cmd-undocumented.h"
#include "console.h"
#include "function.h"
#include "bookmark.h"
#include "label.h"
#include "comment.h"
#include "debugger.h"
#include "variable.h"
#include "loop.h"
#include "capstone_wrapper.h"
#include "mnemonichelp.h"
#include "value.h"
#include "symbolinfo.h"

bool cbBadCmd(int argc, char* argv[])
{
    duint value = 0;
    int valsize = 0;
    bool isvar = false;
    bool hexonly = false;
    if(valfromstring(*argv, &value, false, false, &valsize, &isvar, &hexonly, true))   //dump variable/value/register/etc
    {
        varset("$ans", value, true);
        if(valsize)
            valsize *= 2;
        else
            valsize = 1;
        char format_str[deflen] = "";
        auto symbolic = SymGetSymbolicName(value);
        if(symbolic.length())
            symbolic = " " + symbolic;
        if(isvar)  // and *cmd!='.' and *cmd!='x') //prevent stupid 0=0 stuff
        {
            if(value > 9 && !hexonly)
            {
                if(!valuesignedcalc())   //signed numbers
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
                if(!valuesignedcalc())   //signed numbers
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
        dprintf(QT_TRANSLATE_NOOP("DBG", "unknown command/expression: \"%s\"\n"), *argv);
        return false;
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
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "%ums\n"), GetTickCount() - ticks);
    return true;
}

bool cbInstrSetstr(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    varnew(argv[1], 0, VAR_USER);
    if(!vargettype(argv[1], 0))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "no such variable \"%s\"!\n"), argv[1]);
        return false;
    }
    if(!varset(argv[1], argv[2], false))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "failed to set variable \"%s\"!\n"), argv[1]);
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
        dprintf(QT_TRANSLATE_NOOP("DBG", "no such variable \"%s\"!\n"), argv[1]);
        return false;
    }
    if(valtype != VAR_STRING)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "variable \"%s\" is not a string!\n"), argv[1]);
        return false;
    }
    int size;
    if(!varget(argv[1], (char*)0, &size, 0) || !size)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "failed to get variable size \"%s\"!\n"), argv[1]);
        return false;
    }
    Memory<char*> string(size + 1, "cbInstrGetstr:string");
    if(!varget(argv[1], string(), &size, 0))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "failed to get variable data \"%s\"!\n"), argv[1]);
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
        dprintf(QT_TRANSLATE_NOOP("DBG", "no such variable \"%s\"!\n"), argv[2]);
        return false;
    }
    if(valtype != VAR_STRING)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "variable \"%s\" is not a string!\n"), argv[2]);
        return false;
    }
    int size;
    if(!varget(argv[2], (char*)0, &size, 0) || !size)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "failed to get variable size \"%s\"!\n"), argv[2]);
        return false;
    }
    Memory<char*> string(size + 1, "cbInstrGetstr:string");
    if(!varget(argv[2], string(), &size, 0))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "failed to get variable data \"%s\"!\n"), argv[2]);
        return false;
    }
    duint addr;
    if(!valfromstring(argv[1], &addr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "invalid address \"%s\"!\n"), argv[1]);
        return false;
    }
    if(!MemPatch(addr, string(), strlen(string())))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "memwrite failed!"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "string written!"));
    GuiUpdateAllViews();
    GuiUpdatePatches();
    return true;
}

bool cbInstrLoopList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Loops")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Start")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "End")));
    GuiReferenceAddColumn(64, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Disassembly (Start)")));
    GuiReferenceAddColumn(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Label/Comment")));
    GuiReferenceSetRowCount(0);
    GuiReferenceReloadData();
    size_t cbsize;
    LoopEnum(0, &cbsize);
    if(!cbsize)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "no loops"));
        return true;
    }
    Memory<LOOPSINFO*> loops(cbsize, "cbInstrLoopList:loops");
    LoopEnum(loops(), 0);
    int count = (int)(cbsize / sizeof(LOOPSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf_s(addrText, "%p", loops()[i].start);
        GuiReferenceSetCellContent(i, 0, addrText);
        sprintf_s(addrText, "%p", loops()[i].end);
        GuiReferenceSetCellContent(i, 1, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(loops()[i].start, disassembly))
            GuiReferenceSetCellContent(i, 2, disassembly);
        char label[MAX_LABEL_SIZE] = "";
        if(LabelGet(loops()[i].start, label))
            GuiReferenceSetCellContent(i, 3, label);
        else
        {
            char comment[MAX_COMMENT_SIZE] = "";
            if(CommentGet(loops()[i].start, comment))
                GuiReferenceSetCellContent(i, 3, comment);
        }
    }
    varset("$result", count, false);
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d loop(s) listed\n"), count);
    GuiReferenceReloadData();
    return true;
}

bool cbInstrCapstone(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;

    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !MemIsValidReadPtr(addr))
    {
        dprintf("invalid address \"%s\"\n", argv[1]);
        return false;
    }

    unsigned char data[16];
    if(!MemRead(addr, data, sizeof(data)))
    {
        dprintf("could not read memory at %p\n", addr);
        return false;
    }

    if(argc > 2)
        if(!valfromstring(argv[2], &addr, false))
            return false;

    Capstone cp;
    if(!cp.Disassemble(addr, data))
    {
        dputs("failed to disassemble!\n");
        return false;
    }

    const cs_insn* instr = cp.GetInstr();
    const cs_x86 & x86 = cp.x86();
    int argcount = x86.op_count;
    dprintf("%s %s\n", instr->mnemonic, instr->op_str);
    dprintf("size: %d, id: %d, opcount: %d\n", cp.Size(), cp.GetId(), cp.OpCount());
    for(int i = 0; i < argcount; i++)
    {
        const cs_x86_op & op = x86.operands[i];
        dprintf("operand \"%s\" %d, ", cp.OperandText(i).c_str(), i + 1);
        switch(op.type)
        {
        case X86_OP_REG:
            dprintf("register: %s\n", cp.RegName((x86_reg)op.reg));
            break;
        case X86_OP_IMM:
            dprintf("immediate: 0x%p\n", op.imm);
            break;
        case X86_OP_MEM:
        {
            //[base + index * scale +/- disp]
            const x86_op_mem & mem = op.mem;
            dprintf("memory segment: %s, base: %s, index: %s, scale: %d, displacement: 0x%p\n",
                    cp.RegName((x86_reg)mem.segment),
                    cp.RegName((x86_reg)mem.base),
                    cp.RegName((x86_reg)mem.index),
                    mem.scale,
                    mem.disp);
        }
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
        dputs("invalid arguments!");
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
        Capstone _cp;
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
            SetContextDataEx(fdProcessInfo->hThread, UE_CIP, addr);
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
            if(_cp.Disassemble(addr, curData, MAX_DISASM_BUFFER))
            {
                if(addr + _cp.Size() > maxaddr)      //we went past the maximum allowed address
                    break;

                const cs_x86_op & operand = _cp.x86().operands[0];
                if((_cp.InGroup(CS_GRP_JUMP) || _cp.IsLoop()) && operand.type == X86_OP_IMM)      //jump
                {
                    duint dest = (duint)operand.imm;

                    if(dest >= maxaddr)      //jump across function boundaries
                    {
                        //currently unused
                    }
                    else if(dest > addr && dest > fardest)      //save the farthest JXX destination forward
                    {
                        fardest = dest;
                    }
                    else if(end && dest < end && _cp.GetId() == X86_INS_JMP)      //save the last JMP backwards
                    {
                        jumpback = addr;
                    }
                }
                else if(_cp.InGroup(CS_GRP_RET))      //possible function end?
                {
                    end = addr;
                    if(fardest < addr)     //we stop if the farthest JXX destination forward is before this RET
                        break;
                }

                addr += _cp.Size();
            }
            else
                addr++;
        }
        end = end < jumpback ? jumpback : end;

        //update GUI
        FunctionAdd(start, end, false);
        BpClear();
        BookmarkClear();
        SetContextDataEx(fdProcessInfo->hThread, UE_CIP, start);
        DebugUpdateGuiAsync(start, false);
    }
    return true;
}

bool cbInstrMeminfo(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "usage: meminfo a/r, addr"));
        return false;
    }
    duint addr;
    if(!valfromstring(argv[2], &addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "invalid argument"));
        return false;
    }
    if(argv[1][0] == 'a')
    {
        unsigned char buf = 0;
        if(!ReadProcessMemory(fdProcessInfo->hProcess, (void*)addr, &buf, sizeof(buf), nullptr))
            dputs(QT_TRANSLATE_NOOP("DBG", "ReadProcessMemory failed!"));
        else
            dprintf(QT_TRANSLATE_NOOP("DBG", "data: %02X\n"), buf);
    }
    else if(argv[1][0] == 'r')
    {
        MemUpdateMap();
        GuiUpdateMemoryView();
        dputs(QT_TRANSLATE_NOOP("DBG", "memory map updated!"));
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
    Capstone cp;
    std::unordered_set<String> reported;
    for(duint i = 0; i < size;)
    {
        if(!cp.Disassemble(base + i, buffer() + i, 16))
        {
            i++;
            continue;
        }
        i += cp.Size();
        auto mnem = StringUtils::ToLower(cp.MnemonicId());
        auto brief = MnemonicHelp::getBriefDescription(mnem.c_str());
        if(brief.length() || reported.count(mnem))
            continue;
        reported.insert(mnem);
        dprintf("%p: %s\n", cp.Address(), mnem.c_str());
    }
    return true;
}

bool cbInstrFocusinfo(int argc, char* argv[])
{
    ACTIVEVIEW activeView;
    GuiGetActiveView(&activeView);
    dprintf("activeTitle: %s, activeClass: %s\n", activeView.title, activeView.className);
    return true;
}
