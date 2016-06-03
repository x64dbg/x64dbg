/**
@file instruction.cpp

@brief Implements the instruction class.
*/

#include "instruction.h"
#include "variable.h"
#include "console.h"
#include "value.h"
#include "command.h"
#include "database.h"
#include "addrinfo.h"
#include "assemble.h"
#include "debugger.h"
#include "memory.h"
#include "x64_dbg.h"
#include "disasm_fast.h"
#include "reference.h"
#include "disasm_helper.h"
#include "comment.h"
#include "label.h"
#include "bookmark.h"
#include "function.h"
#include "loop.h"
#include "patternfind.h"
#include "module.h"
#include "stringformat.h"
#include "filehelper.h"
#include "linearanalysis.h"
#include "controlflowanalysis.h"
#include "analysis_nukem.h"
#include "exceptiondirectoryanalysis.h"
#include "_scriptapi_stack.h"
#include "threading.h"
#include "mnemonichelp.h"
#include "error.h"
#include "recursiveanalysis.h"
#include "xrefsanalysis.h"

static bool bRefinit = false;
static int maxFindResults = 5000;

CMDRESULT cbBadCmd(int argc, char* argv[])
{
    duint value = 0;
    int valsize = 0;
    bool isvar = false;
    bool hexonly = false;
    if(valfromstring(*argv, &value, false, false, &valsize, &isvar, &hexonly))  //dump variable/value/register/etc
    {
        //dprintf("[DEBUG] valsize: %d\n", valsize);
        if(valsize)
            valsize *= 2;
        else
            valsize = 1;
        char format_str[deflen] = "";
        if(isvar) // and *cmd!='.' and *cmd!='x') //prevent stupid 0=0 stuff
        {
            if(value > 15 && !hexonly)
            {
                if(!valuesignedcalc())  //signed numbers
                    sprintf(format_str, "%%s=%%.%d" fext "X (%%" fext "ud)\n", valsize);
                else
                    sprintf(format_str, "%%s=%%.%d" fext "X (%%" fext "d)\n", valsize);
                dprintf(format_str, *argv, value, value);
            }
            else
            {
                sprintf(format_str, "%%s=%%.%d" fext "X\n", valsize);
                dprintf(format_str, *argv, value);
            }
        }
        else
        {
            if(value > 15 && !hexonly)
            {
                if(!valuesignedcalc())  //signed numbers
                    sprintf(format_str, "%%s=%%.%d" fext "X (%%" fext "ud)\n", valsize);
                else
                    sprintf(format_str, "%%s=%%.%d" fext "X (%%" fext "d)\n", valsize);
                sprintf(format_str, "%%.%d" fext "X (%%" fext "ud)\n", valsize);
                dprintf(format_str, value, value);
            }
            else
            {
                sprintf(format_str, "%%.%d" fext "X\n", valsize);
                dprintf(format_str, value);
            }
        }
    }
    else //unknown command
    {
        dprintf("unknown command/expression: \"%s\"\n", *argv);
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrVar(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char arg2[deflen] = ""; //var value (optional)
    if(argc > 2)
        strcpy_s(arg2, argv[2]);
    duint value = 0;
    int add = 0;
    if(*argv[1] == '$')
        add++;
    if(valfromstring(argv[1] + add, &value))
    {
        dprintf("invalid variable name \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!valfromstring(arg2, &value))
    {
        dprintf("invalid value \"%s\"\n", arg2);
        return STATUS_ERROR;
    }
    if(!varnew(argv[1], value, VAR_USER))
    {
        dprintf("error creating variable \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }
    else
    {
        if(value > 15)
            dprintf("%s=%" fext "X (%" fext "ud)\n", argv[1], value, value);
        else
            dprintf("%s=%" fext "X\n", argv[1], value);
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrVarDel(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    if(!vardel(argv[1], false))
        dprintf("could not delete variable \"%s\"\n", argv[1]);
    else
        dprintf("deleted variable \"%s\"\n", argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrMov(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments");
        return STATUS_ERROR;
    }

    String srcText = argv[2];
    if(srcText[0] == '#' && srcText[srcText.length() - 1] == '#')  //handle mov addr, #DATA#
    {
        //do some checks on the data
        String dataText = srcText.substr(1, srcText.length() - 2);
        int len = (int)dataText.length();
        if(len % 2)
        {
            dprintf("invalid hex string \"%s\" (length not divisible by 2)\n");
            return STATUS_ERROR;
        }
        for(int i = 0; i < len; i++)
        {
            if(!isxdigit(dataText[i]))
            {
                dprintf("invalid hex string \"%s\" (contains invalid characters)\n", dataText.c_str());
                return STATUS_ERROR;
            }
        }
        //Check the destination
        duint dest;
        if(!valfromstring(argv[1], &dest) || !MemIsValidReadPtr(dest))
        {
            dprintf("invalid destination \"%s\"\n", argv[1]);
            return STATUS_ERROR;
        }
        //Convert text to byte array (very ugly)
        Memory<unsigned char*> data(len / 2);
        for(int i = 0, j = 0; i < len; i += 2, j++)
        {
            char b[3] = "";
            b[0] = dataText[i];
            b[1] = dataText[i + 1];
            int res = 0;
            if(sscanf_s(b, "%X", &res) != 1)
            {
                dprintf("invalid hex byte \"%s\"\n", b);
                return STATUS_ERROR;
            }
            data()[j] = res;
        }
        //Move data to destination
        if(!MemWrite(dest, data(), data.size()))
        {
            dprintf("failed to write to " fhex "\n", dest);
            return STATUS_ERROR;
        }
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return STATUS_CONTINUE;
    }
    else
    {
        duint set_value = 0;
        if(!valfromstring(srcText.c_str(), &set_value))
        {
            dprintf("invalid src \"%s\"\n", argv[2]);
            return STATUS_ERROR;
        }
        bool isvar = false;
        duint temp = 0;
        valfromstring(argv[1], &temp, true, false, 0, &isvar, 0); //there is no return check on this because the destination might not exist yet
        if(!isvar)
            isvar = vargettype(argv[1], 0);
        if(!isvar || !valtostring(argv[1], set_value, true))
        {
            duint value;
            if(valfromstring(argv[1], &value))  //if the var is a value already it's an invalid destination
            {
                dprintf("invalid dest \"%s\"\n", argv[1]);
                return STATUS_ERROR;
            }
            varnew(argv[1], set_value, VAR_USER);
        }
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrVarList(int argc, char* argv[])
{
    int filter = 0;
    if(argc > 1)
    {
        if(!_stricmp(argv[1], "USER"))
            filter = VAR_USER;
        else if(!_stricmp(argv[1], "READONLY"))
            filter = VAR_READONLY;
        else if(!_stricmp(argv[1], "SYSTEM"))
            filter = VAR_SYSTEM;
    }

    size_t cbsize = 0;
    if(!varenum(0, &cbsize))
    {
        dputs("no variables!");
        return STATUS_CONTINUE;
    }
    Memory<VAR*> variables(cbsize, "cbInstrVarList:variables");
    if(!varenum(variables(), 0))
    {
        dputs("error listing variables!");
        return STATUS_ERROR;
    }

    int varcount = (int)cbsize / sizeof(VAR);
    for(int i = 0; i < varcount; i++)
    {
        if(variables()[i].alias.length())
            continue;
        char name[deflen] = "";
        strcpy_s(name, variables()[i].name.c_str());
        duint value = (duint)variables()[i].value.u.value;
        if(variables()[i].type != VAR_HIDDEN)
        {
            if(filter)
            {
                if(variables()[i].type == filter)
                {
                    if(value > 15)
                        dprintf("%s=%" fext "X (%" fext "ud)\n", name, value, value);
                    else
                        dprintf("%s=%" fext "X\n", name, value);
                }
            }
            else
            {
                if(value > 15)
                    dprintf("%s=%" fext "X (%" fext "ud)\n", name, value, value);
                else
                    dprintf("%s=%" fext "X\n", name, value);
            }
        }
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrChd(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    if(!DirExists(argv[1]))
    {
        dputs("directory doesn't exist");
        return STATUS_ERROR;
    }
    SetCurrentDirectoryW(StringUtils::Utf8ToUtf16(argv[1]).c_str());
    dputs("current directory changed!");
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCmt(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!CommentSet(addr, argv[2], true))
    {
        dputs("error setting comment");
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCmtdel(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!CommentDelete(addr))
    {
        dputs("error deleting comment");
        return STATUS_ERROR;
    }
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLbl(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!LabelSet(addr, argv[2], true))
    {
        dputs("error setting label");
        return STATUS_ERROR;
    }
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLbldel(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!LabelDelete(addr))
    {
        dputs("error deleting label");
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrBookmarkSet(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!BookmarkSet(addr, true))
    {
        dputs("failed to set bookmark!");
        return STATUS_ERROR;
    }
    dputs("bookmark set!");
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrBookmarkDel(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!BookmarkDelete(addr))
    {
        dputs("failed to delete bookmark!");
        return STATUS_ERROR;
    }
    dputs("bookmark deleted!");
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLoaddb(int argc, char* argv[])
{
    DbLoad(DbLoadSaveType::All);
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrSavedb(int argc, char* argv[])
{
    DbSave(DbLoadSaveType::All);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAssemble(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr))
    {
        dprintf("invalid expression: \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!DbgMemIsValidReadPtr(addr))
    {
        dprintf("invalid address: " fhex "!\n", addr);
        return STATUS_ERROR;
    }
    bool fillnop = false;
    if(argc > 3)
        fillnop = true;
    char error[MAX_ERROR_SIZE] = "";
    int size = 0;
    if(!assembleat(addr, argv[2], &size, error, fillnop))
    {
        varset("$result", size, false);
        dprintf("failed to assemble \"%s\" (%s)\n", argv[2], error);
        return STATUS_ERROR;
    }
    varset("$result", size, false);
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFunctionAdd(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint start = 0;
    duint end = 0;
    if(!valfromstring(argv[1], &start, false) || !valfromstring(argv[2], &end, false))
        return STATUS_ERROR;
    if(!FunctionAdd(start, end, true))
    {
        dputs("failed to add function");
        return STATUS_ERROR;
    }
    dputs("function added!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFunctionDel(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!FunctionDelete(addr))
    {
        dputs("failed to delete function");
        return STATUS_ERROR;
    }
    dputs("function deleted!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFunctionClear(int argc, char* argv[])
{
    FunctionClear();
    GuiUpdateAllViews();
    dputs("all functions deleted!");
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCmp(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint arg1 = 0;
    if(!valfromstring(argv[1], &arg1, false))
        return STATUS_ERROR;
    duint arg2 = 0;
    if(!valfromstring(argv[2], &arg2, false))
        return STATUS_ERROR;
    duint ezflag;
    duint bsflag;
    if(arg1 == arg2)
        ezflag = 1;
    else
        ezflag = 0;
    if(valuesignedcalc())  //signed comparision
    {
        if((dsint)arg1 < (dsint)arg2)
            bsflag = 0;
        else
            bsflag = 1;
    }
    else //unsigned comparision
    {
        if(arg1 > arg2)
            bsflag = 1;
        else
            bsflag = 0;
    }
    varset("$_EZ_FLAG", ezflag, true);
    varset("$_BS_FLAG", bsflag, true);
    //dprintf("$_EZ_FLAG=%d, $_BS_FLAG=%d\n", ezflag, bsflag);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrGpa(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen] = "";
    if(argc >= 3)
        sprintf(newcmd, "%s:%s", argv[2], argv[1]);
    else
        sprintf(newcmd, "%s", argv[1]);
    duint result = 0;
    if(!valfromstring(newcmd, &result, false))
        return STATUS_ERROR;
    varset("$RESULT", result, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAdd(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    return cmddirectexec("mov %s,%s+%s", argv[1], argv[1], argv[2]);
}

CMDRESULT cbInstrAnd(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    return cmddirectexec("mov %s,%s&%s", argv[1], argv[1], argv[2]);
}

CMDRESULT cbInstrDec(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    return cmddirectexec("mov %s,%s-1", argv[1], argv[1]);
}

CMDRESULT cbInstrDiv(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    return cmddirectexec("mov %s,%s/%s", argv[1], argv[1], argv[2]);
}

CMDRESULT cbInstrInc(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    return cmddirectexec("mov %s,%s+1", argv[1], argv[1]);
}

CMDRESULT cbInstrMul(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    return cmddirectexec("mov %s,%s*%s", argv[1], argv[1], argv[2]);
}

CMDRESULT cbInstrNeg(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    return cmddirectexec("mov %s,%s*-1", argv[1], argv[1]);
}

CMDRESULT cbInstrNot(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    return cmddirectexec("mov %s,~%s", argv[1], argv[1]);
}

CMDRESULT cbInstrOr(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    return cmddirectexec("mov %s,%s|%s", argv[1], argv[1], argv[2]);
}

CMDRESULT cbInstrRol(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    bool signedcalc = valuesignedcalc();
    valuesetsignedcalc(true); //rol = signed
    CMDRESULT res = cmddirectexec("mov %s,%s<%s", argv[1], argv[1], argv[2]);
    valuesetsignedcalc(signedcalc);
    return res;
}

CMDRESULT cbInstrRor(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    bool signedcalc = valuesignedcalc();
    valuesetsignedcalc(true); //ror = signed
    CMDRESULT res = cmddirectexec("mov %s,%s>%s", argv[1], argv[1], argv[2]);
    valuesetsignedcalc(signedcalc);
    return res;
}

CMDRESULT cbInstrShl(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    bool signedcalc = valuesignedcalc();
    valuesetsignedcalc(false); //shl = unsigned
    CMDRESULT res = cmddirectexec("mov %s,%s<%s", argv[1], argv[1], argv[2]);
    valuesetsignedcalc(signedcalc);
    return res;
}

CMDRESULT cbInstrShr(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    bool signedcalc = valuesignedcalc();
    valuesetsignedcalc(false); //shr = unsigned
    CMDRESULT res = cmddirectexec("mov %s,%s>%s", argv[1], argv[1], argv[2]);
    valuesetsignedcalc(signedcalc);
    return res;
}

CMDRESULT cbInstrSub(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    return cmddirectexec("mov %s,%s-%s", argv[1], argv[1], argv[2]);
}

CMDRESULT cbInstrTest(int argc, char* argv[])
{
    //TODO: test
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint arg1 = 0;
    if(!valfromstring(argv[1], &arg1, false))
        return STATUS_ERROR;
    duint arg2 = 0;
    if(!valfromstring(argv[2], &arg2, false))
        return STATUS_ERROR;
    duint ezflag;
    duint bsflag = 0;
    if(!(arg1 & arg2))
        ezflag = 1;
    else
        ezflag = 0;
    varset("$_EZ_FLAG", ezflag, true);
    varset("$_BS_FLAG", bsflag, true);
    //dprintf("$_EZ_FLAG=%d, $_BS_FLAG=%d\n", ezflag, bsflag);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrXor(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    return cmddirectexec("mov %s,%s^%s", argv[1], argv[1], argv[2]);
}

CMDRESULT cbInstrPush(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint value;
    if(!valfromstring(argv[1], &value))
    {
        dprintf("invalid argument \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    Script::Stack::Push(value);
    duint csp = GetContextDataEx(hActiveThread, UE_CSP);
    DebugUpdateStack(csp, csp);
    GuiUpdateRegisterView();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrPop(int argc, char* argv[])
{
    duint value = Script::Stack::Pop();
    duint csp = GetContextDataEx(hActiveThread, UE_CSP);
    DebugUpdateStack(csp, csp);
    GuiUpdateRegisterView();
    if(argc > 1)
    {
        if(!valtostring(argv[1], value, false))
            return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrRefinit(int argc, char* argv[])
{
    GuiReferenceInitialize("Script");
    GuiReferenceAddColumn(sizeof(duint) * 2, "Address");
    GuiReferenceAddColumn(0, "Data");
    GuiReferenceSetRowCount(0);
    GuiReferenceReloadData();
    bRefinit = true;
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrRefadd(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!bRefinit)
        cbInstrRefinit(argc, argv);
    int index = GuiReferenceGetRowCount();
    GuiReferenceSetRowCount(index + 1);
    char addr_text[deflen] = "";
    sprintf(addr_text, fhex, addr);
    GuiReferenceSetCellContent(index, 0, addr_text);
    GuiReferenceSetCellContent(index, 1, argv[2]);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

struct VALUERANGE
{
    duint start;
    duint end;
};

static bool cbRefFind(Capstone* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo)
{
    if(!disasm || !basicinfo)  //initialize
    {
        GuiReferenceInitialize(refinfo->name);
        GuiReferenceAddColumn(2 * sizeof(duint), "Address");
        GuiReferenceAddColumn(0, "Disassembly");
        GuiReferenceSetRowCount(0);
        GuiReferenceReloadData();
        return true;
    }
    bool found = false;
    VALUERANGE* range = (VALUERANGE*)refinfo->userinfo;
    duint start = range->start;
    duint end = range->end;
    if((basicinfo->type & TYPE_VALUE) == TYPE_VALUE)
    {
        duint value = basicinfo->value.value;
        if(value >= start && value <= end)
            found = true;
    }
    if((basicinfo->type & TYPE_MEMORY) == TYPE_MEMORY)
    {
        duint value = basicinfo->memory.value;
        if(value >= start && value <= end)
            found = true;
    }
    if((basicinfo->type & TYPE_ADDR) == TYPE_ADDR)
    {
        duint value = basicinfo->addr;
        if(value >= start && value <= end)
            found = true;
    }
    if(found)
    {
        char addrText[20] = "";
        sprintf(addrText, "%p", disasm->Address());
        GuiReferenceSetRowCount(refinfo->refcount + 1);
        GuiReferenceSetCellContent(refinfo->refcount, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly((duint)disasm->Address(), disassembly))
            GuiReferenceSetCellContent(refinfo->refcount, 1, disassembly);
        else
            GuiReferenceSetCellContent(refinfo->refcount, 1, disasm->InstructionText().c_str());
    }
    return found;
}

CMDRESULT cbInstrRefFind(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    std::string newCommand = std::string("reffindrange ") + argv[1] + std::string(",") + argv[1];
    if(argc > 2)
        newCommand += std::string(",") + argv[2];
    if(argc > 3)
        newCommand += std::string(",") + argv[3];
    if(argc > 4)
        newCommand += std::string(",") + argv[4];
    return cmddirectexec(newCommand.c_str());
}

CMDRESULT cbInstrRefFindRange(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    VALUERANGE range;
    if(!valfromstring(argv[1], &range.start, false))
        return STATUS_ERROR;
    if(argc < 3 || !valfromstring(argv[2], &range.end, false))
        range.end = range.start;
    duint addr = 0;
    if(argc < 4 || !valfromstring(argv[3], &addr))
        addr = GetContextDataEx(hActiveThread, UE_CIP);
    duint size = 0;
    if(argc >= 5)
        if(!valfromstring(argv[4], &size))
            size = 0;
    duint ticks = GetTickCount();
    char title[256] = "";
    if(range.start == range.end)
        sprintf_s(title, "Constant: %" fext "X", range.start);
    else
        sprintf_s(title, "Range: %" fext "X-%" fext "X", range.start, range.end);

    duint refFindType = CURRENT_REGION;
    if(argc >= 6 && valfromstring(argv[5], &refFindType, true))
        if(refFindType != CURRENT_REGION && refFindType != CURRENT_MODULE && refFindType != ALL_MODULES)
            refFindType = CURRENT_REGION;

    int found = RefFind(addr, size, cbRefFind, &range, false, title, (REFFINDTYPE)refFindType);
    dprintf("%u reference(s) in %ums\n", found, GetTickCount() - ticks);
    varset("$result", found, false);
    return STATUS_CONTINUE;
}

bool cbRefStr(Capstone* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo)
{
    if(!disasm || !basicinfo)  //initialize
    {
        GuiReferenceInitialize(refinfo->name);
        GuiReferenceAddColumn(2 * sizeof(duint), "Address");
        GuiReferenceAddColumn(64, "Disassembly");
        GuiReferenceAddColumn(500, "String");
        GuiReferenceSetSearchStartCol(2); //only search the strings
        GuiReferenceReloadData();
        return true;
    }
    bool found = false;
    char string[MAX_STRING_SIZE] = "";
    if(basicinfo->branch)  //branches have no strings (jmp dword [401000])
        return false;
    if((basicinfo->type & TYPE_VALUE) == TYPE_VALUE)
    {
        if(DbgGetStringAt(basicinfo->value.value, string))
            found = true;
    }
    if((basicinfo->type & TYPE_MEMORY) == TYPE_MEMORY)
    {
        if(DbgGetStringAt(basicinfo->memory.value, string))
            found = true;
    }
    if(found)
    {
        char addrText[20] = "";
        sprintf(addrText, fhex, disasm->Address());
        GuiReferenceSetRowCount(refinfo->refcount + 1);
        GuiReferenceSetCellContent(refinfo->refcount, 0, addrText);
        char disassembly[4096] = "";
        if(GuiGetDisassembly((duint)disasm->Address(), disassembly))
            GuiReferenceSetCellContent(refinfo->refcount, 1, disassembly);
        else
            GuiReferenceSetCellContent(refinfo->refcount, 1, disasm->InstructionText().c_str());
        GuiReferenceSetCellContent(refinfo->refcount, 2, string);
    }
    return found;
}

CMDRESULT cbInstrRefStr(int argc, char* argv[])
{
    duint addr;
    duint size = 0;

    // If not specified, assume CURRENT_REGION by default
    if(argc < 2 || !valfromstring(argv[1], &addr, true))
        addr = GetContextDataEx(hActiveThread, UE_CIP);
    if(argc >= 3)
        if(!valfromstring(argv[2], &size, true))
            size = 0;

    duint refFindType = CURRENT_REGION;
    if(argc >= 4 && valfromstring(argv[3], &refFindType, true))
        if(refFindType != CURRENT_REGION && refFindType != CURRENT_MODULE && refFindType != ALL_MODULES)
            refFindType = CURRENT_REGION;

    duint ticks = GetTickCount();
    int found = RefFind(addr, size, cbRefStr, 0, false, "Strings", (REFFINDTYPE)refFindType);
    dprintf("%u string(s) in %ums\n", found, GetTickCount() - ticks);
    varset("$result", found, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrSetstr(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    varnew(argv[1], 0, VAR_USER);
    if(!vargettype(argv[1], 0))
    {
        dprintf("no such variable \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!varset(argv[1], argv[2], false))
    {
        dprintf("failed to set variable \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    cmddirectexec("getstr \"%s\"", argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrGetstr(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    VAR_VALUE_TYPE valtype;
    if(!vargettype(argv[1], 0, &valtype))
    {
        dprintf("no such variable \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    if(valtype != VAR_STRING)
    {
        dprintf("variable \"%s\" is not a string!\n", argv[1]);
        return STATUS_ERROR;
    }
    int size;
    if(!varget(argv[1], (char*)0, &size, 0) || !size)
    {
        dprintf("failed to get variable size \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    Memory<char*> string(size + 1, "cbInstrGetstr:string");
    if(!varget(argv[1], string(), &size, 0))
    {
        dprintf("failed to get variable data \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    dprintf("%s=\"%s\"\n", argv[1], string());
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCopystr(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    VAR_VALUE_TYPE valtype;
    if(!vargettype(argv[2], 0, &valtype))
    {
        dprintf("no such variable \"%s\"!\n", argv[2]);
        return STATUS_ERROR;
    }
    if(valtype != VAR_STRING)
    {
        dprintf("variable \"%s\" is not a string!\n", argv[2]);
        return STATUS_ERROR;
    }
    int size;
    if(!varget(argv[2], (char*)0, &size, 0) || !size)
    {
        dprintf("failed to get variable size \"%s\"!\n", argv[2]);
        return STATUS_ERROR;
    }
    Memory<char*> string(size + 1, "cbInstrGetstr:string");
    if(!varget(argv[2], string(), &size, 0))
    {
        dprintf("failed to get variable data \"%s\"!\n", argv[2]);
        return STATUS_ERROR;
    }
    duint addr;
    if(!valfromstring(argv[1], &addr))
    {
        dprintf("invalid address \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!MemPatch(addr, string(), strlen(string())))
    {
        dputs("memwrite failed!");
        return STATUS_ERROR;
    }
    dputs("string written!");
    GuiUpdateAllViews();
    GuiUpdatePatches();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFind(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    char pattern[deflen] = "";
    //remove # from the start and end of the pattern (ODBGScript support)
    if(argv[2][0] == '#')
        strcpy_s(pattern, argv[2] + 1);
    else
        strcpy_s(pattern, argv[2]);
    int len = (int)strlen(pattern);
    if(pattern[len - 1] == '#')
        pattern[len - 1] = '\0';
    duint size = 0;
    duint base = MemFindBaseAddr(addr, &size, true);
    if(!base)
    {
        dprintf("invalid memory address " fhex "!\n", addr);
        return STATUS_ERROR;
    }
    Memory<unsigned char*> data(size, "cbInstrFind:data");
    if(!MemRead(base, data(), size))
    {
        dputs("failed to read memory!");
        return STATUS_ERROR;
    }
    duint start = addr - base;
    duint find_size = 0;
    if(argc >= 4)
    {
        if(!valfromstring(argv[3], &find_size))
            find_size = size - start;
        if(find_size > (size - start))
            find_size = size - start;
    }
    else
        find_size = size - start;
    duint foundoffset = patternfind(data() + start, find_size, pattern);
    duint result = 0;
    if(foundoffset != -1)
        result = addr + foundoffset;
    varset("$result", result, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFindAll(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;

    char pattern[deflen] = "";
    //remove # from the start and end of the pattern (ODBGScript support)
    if(argv[2][0] == '#')
        strcpy_s(pattern, argv[2] + 1);
    else
        strcpy_s(pattern, argv[2]);
    int len = (int)strlen(pattern);
    if(pattern[len - 1] == '#')
        pattern[len - 1] = '\0';
    duint size = 0;
    duint base = MemFindBaseAddr(addr, &size, true);
    if(!base)
    {
        dprintf("invalid memory address " fhex "!\n", addr);
        return STATUS_ERROR;
    }
    Memory<unsigned char*> data(size, "cbInstrFindAll:data");
    if(!MemRead(base, data(), size))
    {
        dputs("failed to read memory!");
        return STATUS_ERROR;
    }
    duint start = addr - base;
    duint find_size = 0;
    bool findData = false;
    if(argc >= 4)
    {
        if(!_stricmp(argv[3], "&data&"))
        {
            find_size = size - start;
            findData = true;
        }
        else if(!valfromstring(argv[3], &find_size))
            find_size = size - start;
        else if(find_size > (size - start))
            find_size = size - start;
    }
    else
        find_size = size - start;
    //setup reference view
    char patternshort[256] = "";
    strncpy_s(patternshort, pattern, min(16, len));
    if(len > 16)
        strcat_s(patternshort, "...");
    char patterntitle[256] = "";
    sprintf_s(patterntitle, "Pattern: %s", patternshort);
    GuiReferenceInitialize(patterntitle);
    GuiReferenceAddColumn(2 * sizeof(duint), "Address");
    if(findData)
        GuiReferenceAddColumn(0, "&Data&");
    else
        GuiReferenceAddColumn(0, "Disassembly");
    GuiReferenceReloadData();
    DWORD ticks = GetTickCount();
    int refCount = 0;
    duint i = 0;
    duint result = 0;
    std::vector<PatternByte> searchpattern;
    if(!patterntransform(pattern, searchpattern))
    {
        dputs("failed to transform pattern!");
        return STATUS_ERROR;
    }
    while(refCount < maxFindResults)
    {
        duint foundoffset = patternfind(data() + start + i, find_size - i, searchpattern);
        if(foundoffset == -1)
            break;
        i += foundoffset + 1;
        result = addr + i - 1;
        char msg[deflen] = "";
        sprintf(msg, fhex, result);
        GuiReferenceSetRowCount(refCount + 1);
        GuiReferenceSetCellContent(refCount, 0, msg);
        if(findData)
        {
            Memory<unsigned char*> printData(searchpattern.size(), "cbInstrFindAll:printData");
            MemRead(result, printData(), printData.size());
            for(size_t j = 0, k = 0; j < printData.size(); j++)
            {
                if(j)
                    k += sprintf(msg + k, " ");
                k += sprintf(msg + k, "%.2X", printData()[j]);
            }
        }
        else
        {
            if(!GuiGetDisassembly(result, msg))
                strcpy_s(msg, "[Error disassembling]");
        }
        GuiReferenceSetCellContent(refCount, 1, msg);
        result++;
        refCount++;
    }
    GuiReferenceReloadData();
    dprintf("%d occurrences found in %ums\n", refCount, GetTickCount() - ticks);
    varset("$result", refCount, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFindMemAll(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;

    char pattern[deflen] = "";
    //remove # from the start and end of the pattern (ODBGScript support)
    if(argv[2][0] == '#')
        strcpy_s(pattern, argv[2] + 1);
    else
        strcpy_s(pattern, argv[2]);
    int len = (int)strlen(pattern);
    if(pattern[len - 1] == '#')
        pattern[len - 1] = '\0';
    std::vector<PatternByte> searchpattern;
    if(!patterntransform(pattern, searchpattern))
    {
        dputs("failed to transform pattern!");
        return STATUS_ERROR;
    }

    duint endAddr = -1;
    bool findData = false;
    if(argc >= 4)
    {
        if(!_stricmp(argv[3], "&data&"))
            findData = true;
        else if(!valfromstring(argv[3], &endAddr))
            findData = false;
    }

    SHARED_ACQUIRE(LockMemoryPages);
    std::vector<SimplePage> searchPages;
    for(auto & itr : memoryPages)
    {
        if(itr.second.mbi.State != MEM_COMMIT)
            continue;
        SimplePage page(duint(itr.second.mbi.BaseAddress), itr.second.mbi.RegionSize);
        if(page.address >= addr && page.address + page.size <= endAddr)
            searchPages.push_back(page);
    }
    SHARED_RELEASE();

    DWORD ticks = GetTickCount();

    std::vector<duint> results;
    if(!MemFindInMap(searchPages, searchpattern, results, maxFindResults))
    {
        dputs("MemFindInMap failed!");
        return STATUS_ERROR;
    }

    //setup reference view
    char patternshort[256] = "";
    strncpy_s(patternshort, pattern, min(16, len));
    if(len > 16)
        strcat_s(patternshort, "...");
    char patterntitle[256] = "";
    sprintf_s(patterntitle, "Pattern: %s", patternshort);
    GuiReferenceInitialize(patterntitle);
    GuiReferenceAddColumn(2 * sizeof(duint), "Address");
    if(findData)
        GuiReferenceAddColumn(0, "&Data&");
    else
        GuiReferenceAddColumn(0, "Disassembly");
    GuiReferenceReloadData();

    int refCount = 0;
    for(duint result : results)
    {
        char msg[deflen] = "";
        sprintf(msg, fhex, result);
        GuiReferenceSetRowCount(refCount + 1);
        GuiReferenceSetCellContent(refCount, 0, msg);
        if(findData)
        {
            Memory<unsigned char*> printData(searchpattern.size(), "cbInstrFindAll:printData");
            MemRead(result, printData(), printData.size());
            for(size_t j = 0, k = 0; j < printData.size(); j++)
            {
                if(j)
                    k += sprintf(msg + k, " ");
                k += sprintf(msg + k, "%.2X", printData()[j]);
            }
        }
        else
        {
            if(!GuiGetDisassembly(result, msg))
                strcpy_s(msg, "[Error disassembling]");
        }
        GuiReferenceSetCellContent(refCount, 1, msg);
        refCount++;
    }

    GuiReferenceReloadData();
    dprintf("%d occurrences found in %ums\n", refCount, GetTickCount() - ticks);
    varset("$result", refCount, false);

    return STATUS_CONTINUE;
}

static bool cbModCallFind(Capstone* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo)
{
    if(!disasm || !basicinfo)  //initialize
    {
        GuiReferenceInitialize(refinfo->name);
        GuiReferenceAddColumn(2 * sizeof(duint), "Address");
        GuiReferenceAddColumn(20, "Disassembly");
        GuiReferenceAddColumn(MAX_LABEL_SIZE, "Destination");
        GuiReferenceReloadData();
        return true;
    }
    bool found = false;
    char label[MAX_LABEL_SIZE] = "";
    char module[MAX_MODULE_SIZE] = "";
    if(basicinfo->call)  //we are looking for calls
    {
        duint ptr = basicinfo->addr > 0 ? basicinfo->addr : basicinfo->memory.value;
        found = DbgGetLabelAt(ptr, SEG_DEFAULT, label) && !LabelGet(ptr, label) && DbgGetModuleAt(ptr, module); //a non-user label
    }
    if(found)
    {
        char addrText[20] = "";
        char moduleTargetText[256] = "";
        sprintf(addrText, "%p", disasm->Address());
        sprintf(moduleTargetText, "%s.%s", module, label);
        GuiReferenceSetRowCount(refinfo->refcount + 1);
        GuiReferenceSetCellContent(refinfo->refcount, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly((duint)disasm->Address(), disassembly))
        {
            GuiReferenceSetCellContent(refinfo->refcount, 1, disassembly);
            GuiReferenceSetCellContent(refinfo->refcount, 2, moduleTargetText);
        }
        else
        {
            GuiReferenceSetCellContent(refinfo->refcount, 1, disasm->InstructionText().c_str());
            GuiReferenceSetCellContent(refinfo->refcount, 2, moduleTargetText);
        }
    }
    return found;
}

CMDRESULT cbInstrModCallFind(int argc, char* argv[])
{
    duint addr;
    if(argc < 2 || !valfromstring(argv[1], &addr, true))
        addr = GetContextDataEx(hActiveThread, UE_CIP);
    duint size = 0;
    if(argc >= 3)
        if(!valfromstring(argv[2], &size, true))
            size = 0;

    duint refFindType = CURRENT_REGION;
    if(argc >= 4 && valfromstring(argv[3], &refFindType, true))
        if(refFindType != CURRENT_REGION && refFindType != CURRENT_MODULE && refFindType != ALL_MODULES)
            refFindType = CURRENT_REGION;

    duint ticks = GetTickCount();
    int found = RefFind(addr, size, cbModCallFind, 0, false, "Calls", (REFFINDTYPE)refFindType);
    dprintf("%u call(s) in %ums\n", found, GetTickCount() - ticks);
    varset("$result", found, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCommentList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize("Comments");
    GuiReferenceAddColumn(2 * sizeof(duint), "Address");
    GuiReferenceAddColumn(64, "Disassembly");
    GuiReferenceAddColumn(0, "Comment");
    GuiReferenceReloadData();
    size_t cbsize;
    CommentEnum(0, &cbsize);
    if(!cbsize)
    {
        dputs("no comments");
        return STATUS_CONTINUE;
    }
    Memory<COMMENTSINFO*> comments(cbsize, "cbInstrCommentList:comments");
    CommentEnum(comments(), 0);
    int count = (int)(cbsize / sizeof(COMMENTSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf(addrText, "%p", comments()[i].addr);
        GuiReferenceSetCellContent(i, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(comments()[i].addr, disassembly))
            GuiReferenceSetCellContent(i, 1, disassembly);
        GuiReferenceSetCellContent(i, 2, comments()[i].text);
    }
    varset("$result", count, false);
    dprintf("%d comment(s) listed in Reference View\n", count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLabelList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize("Labels");
    GuiReferenceAddColumn(2 * sizeof(duint), "Address");
    GuiReferenceAddColumn(64, "Disassembly");
    GuiReferenceAddColumn(0, "Label");
    GuiReferenceReloadData();
    size_t cbsize;
    LabelEnum(0, &cbsize);
    if(!cbsize)
    {
        dputs("no labels");
        return STATUS_CONTINUE;
    }
    Memory<LABELSINFO*> labels(cbsize, "cbInstrLabelList:labels");
    LabelEnum(labels(), 0);
    int count = (int)(cbsize / sizeof(LABELSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf(addrText, "%p", labels()[i].addr);
        GuiReferenceSetCellContent(i, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(labels()[i].addr, disassembly))
            GuiReferenceSetCellContent(i, 1, disassembly);
        GuiReferenceSetCellContent(i, 2, labels()[i].text);
    }
    varset("$result", count, false);
    dprintf("%d label(s) listed in Reference View\n", count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrBookmarkList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize("Bookmarks");
    GuiReferenceAddColumn(2 * sizeof(duint), "Address");
    GuiReferenceAddColumn(0, "Disassembly");
    GuiReferenceReloadData();
    size_t cbsize;
    BookmarkEnum(0, &cbsize);
    if(!cbsize)
    {
        dputs("No bookmarks found");
        return STATUS_CONTINUE;
    }
    Memory<BOOKMARKSINFO*> bookmarks(cbsize, "cbInstrBookmarkList:bookmarks");
    BookmarkEnum(bookmarks(), 0);
    int count = (int)(cbsize / sizeof(BOOKMARKSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf(addrText, "%p", bookmarks()[i].addr);
        GuiReferenceSetCellContent(i, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(bookmarks()[i].addr, disassembly))
            GuiReferenceSetCellContent(i, 1, disassembly);
    }
    varset("$result", count, false);
    dprintf("%d bookmark(s) listed\n", count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFunctionList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize("Functions");
    GuiReferenceAddColumn(2 * sizeof(duint), "Start");
    GuiReferenceAddColumn(2 * sizeof(duint), "End");
    GuiReferenceAddColumn(64, "Disassembly (Start)");
    GuiReferenceAddColumn(0, "Label/Comment");
    GuiReferenceReloadData();
    size_t cbsize;
    FunctionEnum(0, &cbsize);
    if(!cbsize)
    {
        dputs("No functions");
        return STATUS_CONTINUE;
    }
    Memory<FUNCTIONSINFO*> functions(cbsize, "cbInstrFunctionList:functions");
    FunctionEnum(functions(), 0);
    int count = (int)(cbsize / sizeof(FUNCTIONSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf(addrText, "%p", functions()[i].start);
        GuiReferenceSetCellContent(i, 0, addrText);
        sprintf(addrText, "%p", functions()[i].end);
        GuiReferenceSetCellContent(i, 1, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(functions()[i].start, disassembly))
            GuiReferenceSetCellContent(i, 2, disassembly);
        char label[MAX_LABEL_SIZE] = "";
        if(LabelGet(functions()[i].start, label))
            GuiReferenceSetCellContent(i, 3, label);
        else
        {
            char comment[MAX_COMMENT_SIZE] = "";
            if(CommentGet(functions()[i].start, comment))
                GuiReferenceSetCellContent(i, 3, comment);
        }
    }
    varset("$result", count, false);
    dprintf("%d function(s) listed\n", count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLoopList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize("Loops");
    GuiReferenceAddColumn(2 * sizeof(duint), "Start");
    GuiReferenceAddColumn(2 * sizeof(duint), "End");
    GuiReferenceAddColumn(64, "Disassembly (Start)");
    GuiReferenceAddColumn(0, "Label/Comment");
    GuiReferenceReloadData();
    size_t cbsize;
    LoopEnum(0, &cbsize);
    if(!cbsize)
    {
        dputs("no loops");
        return STATUS_CONTINUE;
    }
    Memory<LOOPSINFO*> loops(cbsize, "cbInstrLoopList:loops");
    LoopEnum(loops(), 0);
    int count = (int)(cbsize / sizeof(LOOPSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf(addrText, "%p", loops()[i].start);
        GuiReferenceSetCellContent(i, 0, addrText);
        sprintf(addrText, "%p", loops()[i].end);
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
    dprintf("%d loop(s) listed\n", count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrSleep(int argc, char* argv[])
{
    duint ms = 100;
    if(argc > 1)
        if(!valfromstring(argv[1], &ms, false))
            return STATUS_ERROR;
    if(ms >= 0xFFFFFFFF)
        ms = 100;
    Sleep((DWORD)ms);
    return STATUS_CONTINUE;
}

static bool cbFindAsm(Capstone* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo)
{
    if(!disasm || !basicinfo)  //initialize
    {
        GuiReferenceInitialize(refinfo->name);
        GuiReferenceAddColumn(2 * sizeof(duint), "Address");
        GuiReferenceAddColumn(0, "Disassembly");
        GuiReferenceReloadData();
        return true;
    }
    const char* instruction = (const char*)refinfo->userinfo;
    bool found = !_stricmp(instruction, basicinfo->instruction);
    if(found)
    {
        char addrText[20] = "";
        sprintf(addrText, fhex, disasm->Address());
        GuiReferenceSetRowCount(refinfo->refcount + 1);
        GuiReferenceSetCellContent(refinfo->refcount, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly((duint)disasm->Address(), disassembly))
            GuiReferenceSetCellContent(refinfo->refcount, 1, disassembly);
        else
            GuiReferenceSetCellContent(refinfo->refcount, 1, disasm->InstructionText().c_str());
    }
    return found;
}

CMDRESULT cbInstrFindAsm(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }

    duint addr = 0;
    if(argc < 3 || !valfromstring(argv[2], &addr))
        addr = GetContextDataEx(hActiveThread, UE_CIP);
    duint size = 0;
    if(argc >= 4)
        if(!valfromstring(argv[3], &size))
            size = 0;

    duint refFindType = CURRENT_REGION;
    if(argc >= 5 && valfromstring(argv[4], &refFindType, true))
        if(refFindType != CURRENT_REGION && refFindType != CURRENT_MODULE && refFindType != ALL_MODULES)
            refFindType = CURRENT_REGION;

    unsigned char dest[16];
    int asmsize = 0;
    char error[MAX_ERROR_SIZE] = "";
    if(!assemble(addr + size / 2, dest, &asmsize, argv[1], error))
    {
        dprintf("failed to assemble \"%s\" (%s)!\n", argv[1], error);
        return STATUS_ERROR;
    }
    BASIC_INSTRUCTION_INFO basicinfo;
    memset(&basicinfo, 0, sizeof(BASIC_INSTRUCTION_INFO));
    disasmfast(dest, addr + size / 2, &basicinfo);

    duint ticks = GetTickCount();
    char title[256] = "";
    sprintf_s(title, "Command: \"%s\"", basicinfo.instruction);
    int found = RefFind(addr, size, cbFindAsm, (void*)&basicinfo.instruction[0], false, title, (REFFINDTYPE)refFindType);
    dprintf("%u result(s) in %ums\n", found, GetTickCount() - ticks);
    varset("$result", found, false);
    return STATUS_CONTINUE;
}

static void yaraCompilerCallback(int error_level, const char* file_name, int line_number, const char* message, void* user_data)
{
    switch(error_level)
    {
    case YARA_ERROR_LEVEL_ERROR:
        dprintf("[YARA ERROR] ");
        break;
    case YARA_ERROR_LEVEL_WARNING:
        dprintf("[YARA WARNING] ");
        break;
    }
    dprintf("File: \"%s\", Line: %d, Message: \"%s\"\n", file_name, line_number, message);
}

static String yara_print_string(const uint8_t* data, int length)
{
    String result = "\"";
    const char* str = (const char*)data;
    for(int i = 0; i < length; i++)
    {
        char cur[16] = "";
        if(str[i] >= 32 && str[i] <= 126)
            sprintf_s(cur, "%c", str[i]);
        else
            sprintf_s(cur, "\\x%02X", (uint8_t)str[i]);
        result += cur;
    }
    result += "\"";
    return result;
}

static String yara_print_hex_string(const uint8_t* data, int length)
{
    String result = "";
    for(int i = 0; i < length; i++)
    {
        if(i)
            result += " ";
        char cur[16] = "";
        sprintf_s(cur, "%02X", (uint8_t)data[i]);
        result += cur;
    }
    return result;
}

struct YaraScanInfo
{
    duint base;
    int index;
    bool rawFile;
    const char* modname;

    YaraScanInfo(duint base, bool rawFile, const char* modname)
        : base(base), index(0), rawFile(rawFile), modname(modname)
    {
    }
};

static int yaraScanCallback(int message, void* message_data, void* user_data)
{
    YaraScanInfo* scanInfo = (YaraScanInfo*)user_data;
    switch(message)
    {
    case CALLBACK_MSG_RULE_MATCHING:
    {
        duint base = scanInfo->base;
        YR_RULE* yrRule = (YR_RULE*)message_data;
        auto addReference = [scanInfo, yrRule](duint addr, const char* identifier, const std::string & pattern)
        {
            auto index = scanInfo->index;
            GuiReferenceSetRowCount(index + 1);
            scanInfo->index++;

            char addr_text[deflen] = "";
            sprintf(addr_text, fhex, addr);
            GuiReferenceSetCellContent(index, 0, addr_text); //Address
            String ruleFullName = "";
            ruleFullName += yrRule->identifier;
            if(identifier)
            {
                ruleFullName += ".";
                ruleFullName += identifier;
            }
            GuiReferenceSetCellContent(index, 1, ruleFullName.c_str()); //Rule
            GuiReferenceSetCellContent(index, 2, pattern.c_str()); //Data
        };

        if(STRING_IS_NULL(yrRule->strings))
        {
            dprintf("[YARA] Global rule \"%s\' matched!\n", yrRule->identifier);
            GuiReferenceSetRowCount(1);
            addReference(base, nullptr, "");
        }
        else
        {
            dprintf("[YARA] Rule \"%s\" matched:\n", yrRule->identifier);
            YR_STRING* string;
            yr_rule_strings_foreach(yrRule, string)
            {
                YR_MATCH* match;
                yr_string_matches_foreach(string, match)
                {
                    String pattern;
                    if(STRING_IS_HEX(string))
                        pattern = yara_print_hex_string(match->data, match->length);
                    else
                        pattern = yara_print_string(match->data, match->length);
                    auto offset = duint(match->base + match->offset);
                    duint addr;
                    if(scanInfo->rawFile)  //convert raw offset to virtual offset
                        addr = valfileoffsettova(scanInfo->modname, offset);
                    else
                        addr = base + offset;

                    dprintf("[YARA] String \"%s\" : %s on 0x%" fext "X\n", string->identifier, pattern.c_str(), addr);

                    addReference(addr, string->identifier, pattern);
                }
            }
        }
    }
    break;

    case CALLBACK_MSG_RULE_NOT_MATCHING:
    {
        YR_RULE* yrRule = (YR_RULE*)message_data;
        dprintf("[YARA] Rule \"%s\" did not match!\n", yrRule->identifier);
    }
    break;

    case CALLBACK_MSG_SCAN_FINISHED:
    {
        dputs("[YARA] Scan finished!");
    }
    break;

    case CALLBACK_MSG_IMPORT_MODULE:
    {
        YR_MODULE_IMPORT* yrModuleImport = (YR_MODULE_IMPORT*)message_data;
        dprintf("[YARA] Imported module \"%s\"!\n", yrModuleImport->module_name);
    }
    break;
    }
    return ERROR_SUCCESS; //nicely undocumented what this should be
}

CMDRESULT cbInstrYara(int argc, char* argv[])
{
    if(argc < 2)  //yara rulesFile, addr_of_mempage, size_of_scan
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint addr = 0;
    SELECTIONDATA sel;
    GuiSelectionGet(GUI_DISASSEMBLY, &sel);
    addr = sel.start;

    duint base = 0;
    duint size = 0;
    duint mod = ModBaseFromName(argv[2]);
    bool rawFile = false;
    if(mod)
    {
        base = mod;
        size = ModSizeFromAddr(base);
        rawFile = argc > 3 && *argv[3] == '1';
    }
    else
    {
        if(!valfromstring(argv[2], &addr))
        {
            dprintf("invalid value \"%s\"!\n", argv[2]);
            return STATUS_ERROR;
        }

        size = 0;
        if(argc >= 4)
            if(!valfromstring(argv[3], &size))
                size = 0;
        if(!size)
            addr = MemFindBaseAddr(addr, &size);
        base = addr;
    }
    std::vector<unsigned char> rawFileData;
    if(rawFile)  //read the file from disk
    {
        char modPath[MAX_PATH] = "";
        if(!ModPathFromAddr(base, modPath, MAX_PATH))
        {
            dprintf("failed to get module path for " fhex "!\n", base);
            return STATUS_ERROR;
        }
        if(!FileHelper::ReadAllData(modPath, rawFileData))
        {
            dprintf("failed to read file \"%s\"!\n", modPath);
            return STATUS_ERROR;
        }
        size = rawFileData.size();
    }
    Memory<uint8_t*> data(size);
    if(rawFile)
        memcpy(data(), rawFileData.data(), size);
    else if(!MemRead(base, data(), size))
    {
        dprintf("failed to read memory page %p[%X]!\n", base, size);
        return STATUS_ERROR;
    }

    String rulesContent;
    if(!FileHelper::ReadAllText(argv[1], rulesContent))
    {
        dprintf("Failed to read the rules file \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }

    bool bSuccess = false;
    YR_COMPILER* yrCompiler;
    if(yr_compiler_create(&yrCompiler) == ERROR_SUCCESS)
    {
        yr_compiler_set_callback(yrCompiler, yaraCompilerCallback, 0);
        if(yr_compiler_add_string(yrCompiler, rulesContent.c_str(), nullptr) == 0)   //no errors found
        {
            YR_RULES* yrRules;
            if(yr_compiler_get_rules(yrCompiler, &yrRules) == ERROR_SUCCESS)
            {
                //initialize new reference tab
                char modname[MAX_MODULE_SIZE] = "";
                if(!ModNameFromAddr(base, modname, true))
                    sprintf_s(modname, "%p", base);
                String fullName;
                const char* fileName = strrchr(argv[1], '\\');
                if(fileName)
                    fullName = fileName + 1;
                else
                    fullName = argv[1];
                fullName += " (";
                fullName += modname;
                fullName += ")"; //nanana, very ugly code (long live open source)
                GuiReferenceInitialize(fullName.c_str());
                GuiReferenceAddColumn(sizeof(duint) * 2, "Address");
                GuiReferenceAddColumn(48, "Rule");
                GuiReferenceAddColumn(0, "Data");
                GuiReferenceSetRowCount(0);
                GuiReferenceReloadData();
                YaraScanInfo scanInfo(base, rawFile, argv[2]);
                duint ticks = GetTickCount();
                dputs("[YARA] Scan started...");
                int err = yr_rules_scan_mem(yrRules, data(), size, 0, yaraScanCallback, &scanInfo, 0);
                GuiReferenceReloadData();
                switch(err)
                {
                case ERROR_SUCCESS:
                    dprintf("%u scan results in %ums...\n", scanInfo.index, GetTickCount() - ticks);
                    bSuccess = true;
                    break;
                case ERROR_TOO_MANY_MATCHES:
                    dputs("too many matches!");
                    break;
                default:
                    dputs("error while scanning memory!");
                    break;
                }
                yr_rules_destroy(yrRules);
            }
            else
                dputs("error while getting the rules!");
        }
        else
            dputs("errors in the rules file!");
        yr_compiler_destroy(yrCompiler);
    }
    else
        dputs("yr_compiler_create failed!");
    return bSuccess ? STATUS_CONTINUE : STATUS_ERROR;
}

CMDRESULT cbInstrYaramod(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    if(!ModBaseFromName(argv[2]))
    {
        dprintf("invalid module \"%s\"!\n", argv[2]);
        return STATUS_ERROR;
    }
    return cmddirectexec("yara \"%s\",%s,%s", argv[1], argv[2], argc > 3 && *argv[3] == '1' ? "1" : "0");
}

CMDRESULT cbInstrLog(int argc, char* argv[])
{
    //log "format {0} string",arg1, arg2, argN
    if(argc == 1)  //just log newline
    {
        dprintf("\n");
        return STATUS_CONTINUE;
    }
    FormatValueVector formatArgs;
    for(int i = 2; i < argc; i++)
        formatArgs.push_back(argv[i]);
    String logString = stringformat(argv[1], formatArgs);
    dputs(logString.c_str());
    return STATUS_CONTINUE;
}

#include <capstone_wrapper.h>

CMDRESULT cbInstrCapstone(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments...");
        return STATUS_ERROR;
    }

    duint addr = 0;
    if(!valfromstring(argv[1], &addr) || !MemIsValidReadPtr(addr))
    {
        dprintf("invalid address \"%s\"\n", argv[1]);
        return STATUS_ERROR;
    }

    unsigned char data[16];
    if(!MemRead(addr, data, sizeof(data)))
    {
        dprintf("could not read memory at %p\n", addr);
        return STATUS_ERROR;
    }

    if(argc > 2)
        if(!valfromstring(argv[2], &addr, false))
            return STATUS_ERROR;

    Capstone cp;
    if(!cp.Disassemble(addr, data))
    {
        dprintf("failed to disassemble!");
        return STATUS_ERROR;
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
        case X86_OP_FP:
            dprintf("float: %f\n", op.fp);
            break;
        }
    }

    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAnalyseNukem(int argc, char* argv[])
{
    SELECTIONDATA sel;
    GuiSelectionGet(GUI_DISASSEMBLY, &sel);
    duint size = 0;
    duint base = MemFindBaseAddr(sel.start, &size);
    Analyse_nukem(base, size);
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAnalyse(int argc, char* argv[])
{
    SELECTIONDATA sel;
    GuiSelectionGet(GUI_DISASSEMBLY, &sel);
    duint size = 0;
    duint base = MemFindBaseAddr(sel.start, &size);
    LinearAnalysis anal(base, size);
    anal.Analyse();
    anal.SetMarkers();
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCfanalyse(int argc, char* argv[])
{
    bool exceptionDirectory = false;
    if(argc > 1)
        exceptionDirectory = true;
    SELECTIONDATA sel;
    GuiSelectionGet(GUI_DISASSEMBLY, &sel);
    duint size = 0;
    duint base = MemFindBaseAddr(sel.start, &size);
    ControlFlowAnalysis anal(base, size, exceptionDirectory);
    anal.Analyse();
    anal.SetMarkers();
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrExanalyse(int argc, char* argv[])
{
    SELECTIONDATA sel;
    GuiSelectionGet(GUI_DISASSEMBLY, &sel);
    duint size = 0;
    duint base = MemFindBaseAddr(sel.start, &size);
    ExceptionDirectoryAnalysis anal(base, size);
    anal.Analyse();
    anal.SetMarkers();
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAnalrecur(int argc, char* argv[])
{
    if(argc < 2)
        return STATUS_ERROR;
    duint entry;
    if(!valfromstring(argv[1], &entry, false))
        return STATUS_ERROR;
    duint size;
    auto base = MemFindBaseAddr(entry, &size);
    if(!base)
        return STATUS_ERROR;
    RecursiveAnalysis analysis(base, size, entry, 0);
    analysis.Analyse();
    analysis.SetMarkers();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAnalxrefs(int argc, char* argv[])
{
    SELECTIONDATA sel;
    GuiSelectionGet(GUI_DISASSEMBLY, &sel);
    duint size = 0;
    auto base = MemFindBaseAddr(sel.start, &size);
    XrefsAnalysis anal(base, size);
    anal.Analyse();
    anal.SetMarkers();
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrVirtualmod(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("Not enough arguments!");
        return STATUS_ERROR;
    }
    duint base;
    if(!valfromstring(argv[2], &base))
    {
        dputs("Invalid parameter [base]!");
        return STATUS_ERROR;
    }
    if(!MemIsValidReadPtr(base))
    {
        dputs("Invalid memory address!");
        return STATUS_ERROR;
    }
    duint size;
    if(argc < 4)
        base = MemFindBaseAddr(base, &size);
    else if(!valfromstring(argv[3], &size))
    {
        dputs("Invalid parameter [size]");
        return STATUS_ERROR;
    }
    auto name = String("virtual:\\") + (argv[1]);
    if(!ModLoad(base, size, name.c_str()))
    {
        dputs("Failed to load module (ModLoad)...");
        return STATUS_ERROR;
    }

    char modname[256] = "";
    if(ModNameFromAddr(base, modname, true))
        BpEnumAll(cbSetModuleBreakpoints, modname);

    dprintf("Virtual module \"%s\" loaded on %p[%p]!\n", argv[1], base, size);
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrVisualize(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint start;
    duint maxaddr;
    if(!valfromstring(argv[1], &start) || !valfromstring(argv[2], &maxaddr))
    {
        dputs("invalid arguments!");
        return STATUS_ERROR;
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
            DebugUpdateGui(addr, false);
            Sleep(300);

            //continue algorithm
            const unsigned char* curData = (addr >= _base && addr < _base + _size) ? _data() + (addr - _base) : nullptr;
            if(_cp.Disassemble(addr, curData, MAX_DISASM_BUFFER))
            {
                if(addr + _cp.Size() > maxaddr)     //we went past the maximum allowed address
                    break;

                const cs_x86_op & operand = _cp.x86().operands[0];
                if((_cp.InGroup(CS_GRP_JUMP) || _cp.IsLoop()) && operand.type == X86_OP_IMM)     //jump
                {
                    duint dest = (duint)operand.imm;

                    if(dest >= maxaddr)     //jump across function boundaries
                    {
                        //currently unused
                    }
                    else if(dest > addr && dest > fardest)     //save the farthest JXX destination forward
                    {
                        fardest = dest;
                    }
                    else if(end && dest < end && _cp.GetId() == X86_INS_JMP)     //save the last JMP backwards
                    {
                        jumpback = addr;
                    }
                }
                else if(_cp.InGroup(CS_GRP_RET))     //possible function end?
                {
                    end = addr;
                    if(fardest < addr)    //we stop if the farthest JXX destination forward is before this RET
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
        DebugUpdateGui(start, false);
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrMeminfo(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("usage: meminfo a/r, addr");
        return STATUS_ERROR;
    }
    duint addr;
    if(!valfromstring(argv[2], &addr))
    {
        dputs("invalid argument");
        return STATUS_ERROR;
    }
    if(argv[1][0] == 'a')
    {
        unsigned char buf = 0;
        if(!ReadProcessMemory(fdProcessInfo->hProcess, (void*)addr, &buf, sizeof(buf), nullptr))
            dputs("ReadProcessMemory failed!");
        else
            dprintf("data: %02X\n", buf);
    }
    else if(argv[1][0] == 'r')
    {
        MemUpdateMap();
        GuiUpdateMemoryView();
        dputs("memory map updated!");
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrSetMaxFindResult(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments!");
        return STATUS_ERROR;
    }
    duint num;
    if(!valfromstring(argv[1], &num))
    {
        dprintf("Invalid expression: \"%s\"", argv[1]);
        return STATUS_ERROR;
    }
    maxFindResults = int(num & 0x7FFFFFFF);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrSavedata(int argc, char* argv[])
{
    if(argc < 3)  //savedata filename,addr,size
    {
        dputs("Not enough arguments...");
        return STATUS_ERROR;
    }
    duint addr, size;
    if(!valfromstring(argv[2], &addr, false) || !valfromstring(argv[3], &size, false))
        return STATUS_ERROR;

    Memory<unsigned char*> data(size);
    if(!MemRead(addr, data(), data.size()))
    {
        dputs("Failed to read memory...");
        return STATUS_ERROR;
    }

    if(!FileHelper::WriteAllData(argv[1], data(), data.size()))
    {
        dputs("Failed to write file...");
        return STATUS_ERROR;
    }

    dprintf("%p[% " fext "X] written to \"%s\" !\n", addr, size, argv[1]);

    return STATUS_CONTINUE;
}

CMDRESULT cbInstrMnemonichelp(int argc, char* argv[])
{
    if(argc < 2)
        return STATUS_ERROR;
    auto description = MnemonicHelp::getDescription(argv[1]);
    if(!description.length())
        dputs("no description or empty description");
    else
    {
        auto padding = "================================================================";
        auto logText = StringUtils::sprintf("%s%s%s\n", padding, description.c_str(), padding);
        GuiAddLogMessage(logText.c_str());
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrMnemonicbrief(int argc, char* argv[])
{
    if(argc < 2)
        return STATUS_ERROR;
    dputs(MnemonicHelp::getBriefDescription(argv[1]).c_str());
    return STATUS_CONTINUE;
}

CMDRESULT cbGetPrivilegeState(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments");
        return STATUS_ERROR;
    }
    DWORD returnLength;
    LUID luid;
    if(LookupPrivilegeValueW(nullptr, StringUtils::Utf8ToUtf16(argv[1]).c_str(), &luid) == 0)
    {
        varset("$result", (duint)0, false);
        return STATUS_CONTINUE;
    }
    Memory <TOKEN_PRIVILEGES*> Privileges(64 * 16 + 8, "_dbg_getprivilegestate");
    if(GetTokenInformation(hProcessToken, TokenPrivileges, Privileges(), 64 * 16 + 8, &returnLength) == 0)
    {
        if(returnLength > 4 * 1024 * 1024)
        {
            varset("$result", (duint)0, false);
            return STATUS_CONTINUE;
        }
        Privileges.realloc(returnLength, "_dbg_getprivilegestate");
        if(GetTokenInformation(hProcessToken, TokenPrivileges, Privileges(), returnLength, &returnLength) == 0)
            return STATUS_ERROR;
    }
    for(unsigned int i = 0; i < Privileges()->PrivilegeCount; i++)
    {
        if(4 + sizeof(LUID_AND_ATTRIBUTES) * i > returnLength)
            return STATUS_ERROR;
        if(memcmp(&Privileges()->Privileges[i].Luid, &luid, sizeof(LUID)) == 0)
        {
            varset("$result", (duint)(Privileges()->Privileges[i].Attributes + 1), false); // 2=enabled, 3=default, 1=disabled
            return STATUS_CONTINUE;
        }
    }
    varset("$result", (duint)0, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbEnablePrivilege(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments");
        return STATUS_ERROR;
    }
    LUID luid;
    if(LookupPrivilegeValueW(nullptr, StringUtils::Utf8ToUtf16(argv[1]).c_str(), &luid) == 0)
    {
        dprintf("Could not find the specified privilege: %s\n", argv[1]);
        return STATUS_ERROR;
    }
    TOKEN_PRIVILEGES Privilege;
    Privilege.PrivilegeCount = 1;
    Privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    Privilege.Privileges[0].Luid = luid;
    bool ret = AdjustTokenPrivileges(hProcessToken, FALSE, &Privilege, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr) != NO_ERROR;
    return ret ? STATUS_CONTINUE : STATUS_ERROR;
}

CMDRESULT cbDisablePrivilege(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments");
        return STATUS_ERROR;
    }
    LUID luid;
    if(LookupPrivilegeValueW(nullptr, StringUtils::Utf8ToUtf16(argv[1]).c_str(), &luid) == 0)
    {
        dprintf("Could not find the specified privilege: %s\n", argv[1]);
        return STATUS_ERROR;
    }
    TOKEN_PRIVILEGES Privilege;
    Privilege.PrivilegeCount = 1;
    Privilege.Privileges[0].Attributes = 0;
    Privilege.Privileges[0].Luid = luid;
    bool ret = AdjustTokenPrivileges(hProcessToken, FALSE, &Privilege, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr) != NO_ERROR;
    return ret ? STATUS_CONTINUE : STATUS_ERROR;
}

CMDRESULT cbHandleClose(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("Not enough arguments");
        return STATUS_ERROR;
    }
    duint handle;
    if(!valfromstring(argv[1], &handle, false))
        return STATUS_ERROR;
    if(!DuplicateHandle(fdProcessInfo->hProcess, HANDLE(handle), NULL, NULL, 0, FALSE, DUPLICATE_CLOSE_SOURCE))
    {
        dprintf("DuplicateHandle failed: %s\n", ErrorCodeToName(GetLastError()));
        return STATUS_ERROR;
    }
    dprintf("Handle %" fext "X closed!\n", handle);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrBriefcheck(int argc, char* argv[])
{
    if(argc < 2)
        return STATUS_ERROR;
    duint addr;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    duint size;
    auto base = DbgMemFindBaseAddr(addr, &size);
    if(!base)
        return STATUS_ERROR;
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
        dprintf(fhex ": %s\n", cp.Address(), mnem.c_str());
    }
    return STATUS_CONTINUE;
}
