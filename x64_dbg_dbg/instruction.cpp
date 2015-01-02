#include "instruction.h"
#include "argument.h"
#include "variable.h"
#include "console.h"
#include "value.h"
#include "command.h"
#include "addrinfo.h"
#include "assemble.h"
#include "debugger.h"
#include "memory.h"
#include "x64_dbg.h"
#include "disasm_fast.h"
#include "reference.h"
#include "disasm_helper.h"

static bool bRefinit = false;

CMDRESULT cbBadCmd(int argc, char* argv[])
{
    uint value = 0;
    int valsize = 0;
    bool isvar = false;
    bool hexonly = false;
    if(valfromstring(*argv, &value, false, false, &valsize, &isvar, &hexonly)) //dump variable/value/register/etc
    {
        //dprintf("[DEBUG] valsize: %d\n", valsize);
        if(valsize)
            valsize *= 2;
        else
            valsize = 1;
        char format_str[deflen] = "";
        if(isvar)// and *cmd!='.' and *cmd!='x') //prevent stupid 0=0 stuff
        {
            if(value > 15 and !hexonly)
            {
                if(!valuesignedcalc()) //signed numbers
                    sprintf(format_str, "%%s=%%.%d"fext"X (%%"fext"ud)\n", valsize);
                else
                    sprintf(format_str, "%%s=%%.%d"fext"X (%%"fext"d)\n", valsize);
                dprintf(format_str, *argv, value, value);
            }
            else
            {
                sprintf(format_str, "%%s=%%.%d"fext"X\n", valsize);
                dprintf(format_str, *argv, value);
            }
        }
        else
        {
            if(value > 15 and !hexonly)
            {
                if(!valuesignedcalc()) //signed numbers
                    sprintf(format_str, "%%s=%%.%d"fext"X (%%"fext"ud)\n", valsize);
                else
                    sprintf(format_str, "%%s=%%.%d"fext"X (%%"fext"d)\n", valsize);
                sprintf(format_str, "%%.%d"fext"X (%%"fext"ud)\n", valsize);
                dprintf(format_str, value, value);
            }
            else
            {
                sprintf(format_str, "%%.%d"fext"X\n", valsize);
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
    char arg2[deflen] = "";
    argget(*argv, arg2, 1, true); //var value (optional)
    uint value = 0;
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
            dprintf("%s=%"fext"X (%"fext"ud)\n", argv[1], value, value);
        else
            dprintf("%s=%"fext"X\n", argv[1], value);
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
    if(srcText[0] == '#' && srcText[srcText.length() - 1] == '#') //handle mov addr, #DATA#
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
        uint dest;
        if(!valfromstring(argv[1], &dest) || !memisvalidreadptr(fdProcessInfo->hProcess, dest))
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
            sscanf_s(b, "%X", &res);
            data[j] = res;
        }
        //Move data to destination
        if(!memwrite(fdProcessInfo->hProcess, (void*)dest, data, data.size(), 0))
        {
            dprintf("failed to write to "fhex"\n", dest);
            return STATUS_ERROR;
        }
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return STATUS_CONTINUE;
    }
    else
    {
        uint set_value = 0;
        if(!valfromstring(srcText.c_str(), &set_value))
        {
            dprintf("invalid src \"%s\"\n", argv[2]);
            return STATUS_ERROR;
        }
        bool isvar = false;
        uint temp = 0;
        valfromstring(argv[1], &temp, true, false, 0, &isvar, 0);
        if(!isvar)
            isvar = vargettype(argv[1], 0);
        if(!isvar or !valtostring(argv[1], &set_value, true))
        {
            uint value;
            if(valfromstring(argv[1], &value)) //if the var is a value already it's an invalid destination
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
    char arg1[deflen] = "";
    argget(*argv, arg1, 0, true);
    int filter = 0;
    if(!_stricmp(arg1, "USER"))
        filter = VAR_USER;
    else if(!_stricmp(arg1, "READONLY"))
        filter = VAR_READONLY;
    else if(!_stricmp(arg1, "SYSTEM"))
        filter = VAR_SYSTEM;

    size_t cbsize = 0;
    if(!varenum(0, &cbsize))
    {
        dputs("no variables!");
        return STATUS_CONTINUE;
    }
    Memory<VAR*> variables(cbsize, "cbInstrVarList:variables");
    if(!varenum(variables, 0))
    {
        dputs("error listing variables!");
        return STATUS_ERROR;
    }

    int varcount = (int)cbsize / sizeof(VAR);
    for(int i = 0; i < varcount; i++)
    {
        if(variables[i].alias.length())
            continue;
        char name[deflen] = "";
        strcpy(name, variables[i].name.c_str());
        uint value = (uint)variables[i].value.u.value;
        if(variables[i].type != VAR_HIDDEN)
        {
            if(filter)
            {
                if(variables[i].type == filter)
                {
                    if(value > 15)
                        dprintf("%s=%"fext"X (%"fext"ud)\n", name, value, value);
                    else
                        dprintf("%s=%"fext"X\n", name, value);
                }
            }
            else
            {
                if(value > 15)
                    dprintf("%s=%"fext"X (%"fext"ud)\n", name, value, value);
                else
                    dprintf("%s=%"fext"X\n", name, value);
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
    uint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!commentset(addr, argv[2], true))
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
    uint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!commentdel(addr))
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
    uint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!labelset(addr, argv[2], true))
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
    uint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!labeldel(addr))
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
    uint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!bookmarkset(addr, true))
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
    uint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!bookmarkdel(addr))
    {
        dputs("failed to delete bookmark!");
        return STATUS_ERROR;
    }
    dputs("bookmark deleted!");
    return STATUS_CONTINUE;
}

CMDRESULT cbLoaddb(int argc, char* argv[])
{
    dbload();
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbSavedb(int argc, char* argv[])
{
    dbsave();
    return STATUS_CONTINUE;
}

CMDRESULT cbAssemble(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint addr = 0;
    if(!valfromstring(argv[1], &addr))
    {
        dprintf("invalid expression: \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!DbgMemIsValidReadPtr(addr))
    {
        dprintf("invalid address: "fhex"!\n", addr);
        return STATUS_ERROR;
    }
    bool fillnop = false;
    if(argc > 3)
        fillnop = true;
    char error[256] = "";
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

CMDRESULT cbFunctionAdd(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint start = 0;
    uint end = 0;
    if(!valfromstring(argv[1], &start, false) or !valfromstring(argv[2], &end, false))
        return STATUS_ERROR;
    if(!functionadd(start, end, true))
    {
        dputs("failed to add function");
        return STATUS_ERROR;
    }
    dputs("function added!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbFunctionDel(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!functiondel(addr))
    {
        dputs("failed to delete function");
        return STATUS_ERROR;
    }
    dputs("function deleted!");
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCmp(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint arg1 = 0;
    if(!valfromstring(argv[1], &arg1, false))
        return STATUS_ERROR;
    uint arg2 = 0;
    if(!valfromstring(argv[2], &arg2, false))
        return STATUS_ERROR;
    uint ezflag;
    uint bsflag;
    if(arg1 == arg2)
        ezflag = 1;
    else
        ezflag = 0;
    if(valuesignedcalc()) //signed comparision
    {
        if((sint)arg1 < (sint)arg2)
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
    uint result = 0;
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
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s+%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrAnd(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s&%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrDec(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s-1", argv[1], argv[1]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrDiv(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s/%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrInc(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s+1", argv[1], argv[1]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrMul(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s*%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrNeg(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s*-1", argv[1], argv[1]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrNot(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,~%s", argv[1], argv[1]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrOr(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s|%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrRol(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s<%s", argv[1], argv[1], argv[2]);
    bool signedcalc = valuesignedcalc();
    valuesetsignedcalc(true); //rol = signed
    CMDRESULT res = cmddirectexec(dbggetcommandlist(), newcmd);
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
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s>%s", argv[1], argv[1], argv[2]);
    bool signedcalc = valuesignedcalc();
    valuesetsignedcalc(true); //ror = signed
    CMDRESULT res = cmddirectexec(dbggetcommandlist(), newcmd);
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
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s<%s", argv[1], argv[1], argv[2]);
    bool signedcalc = valuesignedcalc();
    valuesetsignedcalc(false); //shl = unsigned
    CMDRESULT res = cmddirectexec(dbggetcommandlist(), newcmd);
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
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s>%s", argv[1], argv[1], argv[2]);
    bool signedcalc = valuesignedcalc();
    valuesetsignedcalc(false); //shr = unsigned
    CMDRESULT res = cmddirectexec(dbggetcommandlist(), newcmd);
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
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s-%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrTest(int argc, char* argv[])
{
    //TODO: test
    if(argc < 3)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    uint arg1 = 0;
    if(!valfromstring(argv[1], &arg1, false))
        return STATUS_ERROR;
    uint arg2 = 0;
    if(!valfromstring(argv[2], &arg2, false))
        return STATUS_ERROR;
    uint ezflag;
    uint bsflag = 0;
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
    char newcmd[deflen] = "";
    sprintf(newcmd, "mov %s,%s^%s", argv[1], argv[1], argv[2]);
    return cmddirectexec(dbggetcommandlist(), newcmd);
}

CMDRESULT cbInstrRefinit(int argc, char* argv[])
{
    GuiReferenceInitialize("Script");
    GuiReferenceAddColumn(sizeof(uint) * 2, "Address");
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
    uint addr = 0;
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
    uint start;
    uint end;
};

//reffind value[,page]
static bool cbRefFind(DISASM* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo)
{
    if(!disasm && !basicinfo) //initialize
    {
        GuiReferenceInitialize(refinfo->name);
        GuiReferenceAddColumn(2 * sizeof(uint), "Address");
        GuiReferenceAddColumn(0, "Disassembly");
        GuiReferenceReloadData();
        return true;
    }
    bool found = false;
    VALUERANGE* range = (VALUERANGE*)refinfo->userinfo;
    uint start = range->start;
    uint end = range->end;
    if((basicinfo->type & TYPE_VALUE) == TYPE_VALUE)
    {
        uint value = basicinfo->value.value;
        if(value >= start && value <= end)
            found = true;
    }
    if((basicinfo->type & TYPE_MEMORY) == TYPE_MEMORY)
    {
        uint value = basicinfo->memory.value;
        if(value >= start && value <= end)
            found = true;
    }
    if((basicinfo->type & TYPE_ADDR) == TYPE_ADDR)
    {
        uint value = basicinfo->addr;
        if(value >= start && value <= end)
            found = true;
    }
    if(found)
    {
        char addrText[20] = "";
        sprintf(addrText, "%p", disasm->VirtualAddr);
        GuiReferenceSetRowCount(refinfo->refcount + 1);
        GuiReferenceSetCellContent(refinfo->refcount, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly((duint)disasm->VirtualAddr, disassembly))
            GuiReferenceSetCellContent(refinfo->refcount, 1, disassembly);
        else
            GuiReferenceSetCellContent(refinfo->refcount, 1, disasm->CompleteInstr);
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
    return cmddirectexec(dbggetcommandlist(), newCommand.c_str());
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
    if(argc < 3 or !valfromstring(argv[2], &range.end, false))
        range.end = range.start;
    uint addr = 0;
    if(argc < 4 or !valfromstring(argv[3], &addr))
        addr = GetContextDataEx(hActiveThread, UE_CIP);
    uint size = 0;
    if(argc >= 5)
        if(!valfromstring(argv[4], &size))
            size = 0;
    uint ticks = GetTickCount();
    int found = reffind(addr, size, cbRefFind, &range, false, "Constant");
    dprintf("%u reference(s) in %ums\n", found, GetTickCount() - ticks);
    varset("$result", found, false);
    return STATUS_CONTINUE;
}

//refstr [page]
bool cbRefStr(DISASM* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo)
{
    if(!disasm && !basicinfo) //initialize
    {
        GuiReferenceInitialize(refinfo->name);
        GuiReferenceAddColumn(2 * sizeof(uint), "Address");
        GuiReferenceAddColumn(64, "Disassembly");
        GuiReferenceAddColumn(500, "String");
        GuiReferenceSetSearchStartCol(2); //only search the strings
        GuiReferenceReloadData();
        return true;
    }
    bool found = false;
    STRING_TYPE strtype;
    char string[1024] = "";
    if(basicinfo->branch) //branches have no strings (jmp dword [401000])
        return false;
    if((basicinfo->type & TYPE_VALUE) == TYPE_VALUE)
    {
        if(disasmgetstringat(basicinfo->value.value, &strtype, string, string, 500))
            found = true;
    }
    if((basicinfo->type & TYPE_MEMORY) == TYPE_MEMORY)
    {
        if(!found and disasmgetstringat(basicinfo->memory.value, &strtype, string, string, 500))
            found = true;
    }
    if(found)
    {
        char addrText[20] = "";
        sprintf(addrText, "%p", disasm->VirtualAddr);
        GuiReferenceSetRowCount(refinfo->refcount + 1);
        GuiReferenceSetCellContent(refinfo->refcount, 0, addrText);
        char disassembly[4096] = "";
        if(GuiGetDisassembly((duint)disasm->VirtualAddr, disassembly))
            GuiReferenceSetCellContent(refinfo->refcount, 1, disassembly);
        else
            GuiReferenceSetCellContent(refinfo->refcount, 1, disasm->CompleteInstr);
        char dispString[1024] = "";
        if(strtype == str_ascii)
            sprintf(dispString, "\"%s\"", string);
        else
            sprintf(dispString, "L\"%s\"", string);
        GuiReferenceSetCellContent(refinfo->refcount, 2, dispString);
    }
    return found;
}

CMDRESULT cbInstrRefStr(int argc, char* argv[])
{
    uint addr;
    if(argc < 2 or !valfromstring(argv[1], &addr, true))
        addr = GetContextDataEx(hActiveThread, UE_CIP);
    uint size = 0;
    if(argc >= 3)
        if(!valfromstring(argv[2], &size, true))
            size = 0;
    uint ticks = GetTickCount();
    int found = reffind(addr, size, cbRefStr, 0, false, "Strings");
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
    char cmd[deflen] = "";
    sprintf(cmd, "getstr \"%s\"", argv[1]);
    cmddirectexec(dbggetcommandlist(), cmd);
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
    if(!varget(argv[1], (char*)0, &size, 0) or !size)
    {
        dprintf("failed to get variable size \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    Memory<char*> string(size + 1, "cbInstrGetstr:string");
    memset(string, 0, size + 1);
    if(!varget(argv[1], (char*)string, &size, 0))
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
    if(!varget(argv[2], (char*)0, &size, 0) or !size)
    {
        dprintf("failed to get variable size \"%s\"!\n", argv[2]);
        return STATUS_ERROR;
    }
    Memory<char*> string(size + 1, "cbInstrGetstr:string");
    memset(string, 0, size + 1);
    if(!varget(argv[2], (char*)string, &size, 0))
    {
        dprintf("failed to get variable data \"%s\"!\n", argv[2]);
        return STATUS_ERROR;
    }
    uint addr;
    if(!valfromstring(argv[1], &addr))
    {
        dprintf("invalid address \"%s\"!\n", argv[1]);
        return STATUS_ERROR;
    }
    if(!mempatch(fdProcessInfo->hProcess, (void*)addr, string, strlen(string), 0))
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
    uint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    char pattern[deflen] = "";
    //remove # from the start and end of the pattern (ODBGScript support)
    if(argv[2][0] == '#')
        strcpy(pattern, argv[2] + 1);
    else
        strcpy(pattern, argv[2]);
    int len = (int)strlen(pattern);
    if(pattern[len - 1] == '#')
        pattern[len - 1] = '\0';
    uint size = 0;
    uint base = memfindbaseaddr(addr, &size, true);
    if(!base)
    {
        dprintf("invalid memory address "fhex"!\n", addr);
        return STATUS_ERROR;
    }
    Memory<unsigned char*> data(size, "cbInstrFind:data");
    if(!memread(fdProcessInfo->hProcess, (const void*)base, data, size, 0))
    {
        dputs("failed to read memory!");
        return STATUS_ERROR;
    }
    uint start = addr - base;
    uint find_size = 0;
    if(argc >= 4)
    {
        if(!valfromstring(argv[3], &find_size))
            find_size = size - start;
        if(find_size > (size - start))
            find_size = size - start;
    }
    else
        find_size = size - start;
    uint foundoffset = memfindpattern(data + start, find_size, pattern);
    uint result = 0;
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
    uint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;

    char pattern[deflen] = "";
    //remove # from the start and end of the pattern (ODBGScript support)
    if(argv[2][0] == '#')
        strcpy(pattern, argv[2] + 1);
    else
        strcpy(pattern, argv[2]);
    int len = (int)strlen(pattern);
    if(pattern[len - 1] == '#')
        pattern[len - 1] = '\0';
    uint size = 0;
    uint base = memfindbaseaddr(addr, &size, true);
    if(!base)
    {
        dprintf("invalid memory address "fhex"!\n", addr);
        return STATUS_ERROR;
    }
    Memory<unsigned char*> data(size, "cbInstrFindAll:data");
    if(!memread(fdProcessInfo->hProcess, (const void*)base, data, size, 0))
    {
        dputs("failed to read memory!");
        return STATUS_ERROR;
    }
    uint start = addr - base;
    uint find_size = 0;
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
    GuiReferenceInitialize("Occurrences");
    GuiReferenceAddColumn(2 * sizeof(uint), "Address");
    if(findData)
        GuiReferenceAddColumn(0, "&Data&");
    else
        GuiReferenceAddColumn(0, "Disassembly");
    GuiReferenceReloadData();
    DWORD ticks = GetTickCount();
    int refCount = 0;
    uint i = 0;
    uint result = 0;
    while(refCount < 5000)
    {
        int patternsize = 0;
        uint foundoffset = memfindpattern(data + start + i, find_size - i, pattern, &patternsize);
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
            Memory<unsigned char*> printData(patternsize, "cbInstrFindAll:printData");
            memread(fdProcessInfo->hProcess, (const void*)result, printData, patternsize, 0);
            for(int j = 0, k = 0; j < patternsize; j++)
            {
                if(j)
                    k += sprintf(msg + k, " ");
                k += sprintf(msg + k, "%.2X", printData[j]);
            }
        }
        else
            GuiGetDisassembly(result, msg);
        GuiReferenceSetCellContent(refCount, 1, msg);
        result++;
        refCount++;
    }
    GuiReferenceReloadData();
    dprintf("%d occurrences found in %ums\n", refCount, GetTickCount() - ticks);
    varset("$result", refCount, false);
    return STATUS_CONTINUE;
}

//modcallfind [page]
static bool cbModCallFind(DISASM* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo)
{
    if(!disasm && !basicinfo) //initialize
    {
        GuiReferenceInitialize(refinfo->name);
        GuiReferenceAddColumn(2 * sizeof(uint), "Address");
        GuiReferenceAddColumn(0, "Disassembly");
        GuiReferenceReloadData();
        return true;
    }
    bool found = false;
    if(basicinfo->call) //we are looking for calls
    {
        uint ptr = basicinfo->addr > 0 ? basicinfo->addr : basicinfo->memory.value;
        char label[MAX_LABEL_SIZE] = "";
        found = DbgGetLabelAt(ptr, SEG_DEFAULT, label) && !labelget(ptr, label); //a non-user label
    }
    if(found)
    {
        char addrText[20] = "";
        sprintf(addrText, "%p", disasm->VirtualAddr);
        GuiReferenceSetRowCount(refinfo->refcount + 1);
        GuiReferenceSetCellContent(refinfo->refcount, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly((duint)disasm->VirtualAddr, disassembly))
            GuiReferenceSetCellContent(refinfo->refcount, 1, disassembly);
        else
            GuiReferenceSetCellContent(refinfo->refcount, 1, disasm->CompleteInstr);
    }
    return found;
}

CMDRESULT cbInstrModCallFind(int argc, char* argv[])
{
    uint addr;
    if(argc < 2 or !valfromstring(argv[1], &addr, true))
        addr = GetContextDataEx(hActiveThread, UE_CIP);
    uint size = 0;
    if(argc >= 3)
        if(!valfromstring(argv[2], &size, true))
            size = 0;
    uint ticks = GetTickCount();
    int found = reffind(addr, size, cbModCallFind, 0, false, "Calls");
    dprintf("%u call(s) in %ums\n", found, GetTickCount() - ticks);
    varset("$result", found, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCommentList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize("Comments");
    GuiReferenceAddColumn(2 * sizeof(uint), "Address");
    GuiReferenceAddColumn(64, "Disassembly");
    GuiReferenceAddColumn(0, "Comment");
    GuiReferenceReloadData();
    size_t cbsize;
    commentenum(0, &cbsize);
    if(!cbsize)
    {
        dputs("no comments");
        return STATUS_CONTINUE;
    }
    Memory<COMMENTSINFO*> comments(cbsize, "cbInstrCommentList:comments");
    commentenum(comments, 0);
    int count = (int)(cbsize / sizeof(COMMENTSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf(addrText, "%p", comments[i].addr);
        GuiReferenceSetCellContent(i, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(comments[i].addr, disassembly))
            GuiReferenceSetCellContent(i, 1, disassembly);
        GuiReferenceSetCellContent(i, 2, comments[i].text);
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
    GuiReferenceAddColumn(2 * sizeof(uint), "Address");
    GuiReferenceAddColumn(64, "Disassembly");
    GuiReferenceAddColumn(0, "Label");
    GuiReferenceReloadData();
    size_t cbsize;
    labelenum(0, &cbsize);
    if(!cbsize)
    {
        dputs("no labels");
        return STATUS_CONTINUE;
    }
    Memory<LABELSINFO*> labels(cbsize, "cbInstrLabelList:labels");
    labelenum(labels, 0);
    int count = (int)(cbsize / sizeof(LABELSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf(addrText, "%p", labels[i].addr);
        GuiReferenceSetCellContent(i, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(labels[i].addr, disassembly))
            GuiReferenceSetCellContent(i, 1, disassembly);
        GuiReferenceSetCellContent(i, 2, labels[i].text);
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
    GuiReferenceAddColumn(2 * sizeof(uint), "Address");
    GuiReferenceAddColumn(0, "Disassembly");
    GuiReferenceReloadData();
    size_t cbsize;
    bookmarkenum(0, &cbsize);
    if(!cbsize)
    {
        dputs("no bookmarks");
        return STATUS_CONTINUE;
    }
    Memory<BOOKMARKSINFO*> bookmarks(cbsize, "cbInstrBookmarkList:bookmarks");
    bookmarkenum(bookmarks, 0);
    int count = (int)(cbsize / sizeof(BOOKMARKSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf(addrText, "%p", bookmarks[i].addr);
        GuiReferenceSetCellContent(i, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(bookmarks[i].addr, disassembly))
            GuiReferenceSetCellContent(i, 1, disassembly);
    }
    varset("$result", count, false);
    dprintf("%d bookmark(s) listed in Reference View\n", count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFunctionList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize("Functions");
    GuiReferenceAddColumn(2 * sizeof(uint), "Start");
    GuiReferenceAddColumn(2 * sizeof(uint), "End");
    GuiReferenceAddColumn(64, "Disassembly (Start)");
    GuiReferenceAddColumn(0, "Label/Comment");
    GuiReferenceReloadData();
    size_t cbsize;
    functionenum(0, &cbsize);
    if(!cbsize)
    {
        dputs("no functions");
        return STATUS_CONTINUE;
    }
    Memory<FUNCTIONSINFO*> functions(cbsize, "cbInstrFunctionList:functions");
    functionenum(functions, 0);
    int count = (int)(cbsize / sizeof(FUNCTIONSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf(addrText, "%p", functions[i].start);
        GuiReferenceSetCellContent(i, 0, addrText);
        sprintf(addrText, "%p", functions[i].end);
        GuiReferenceSetCellContent(i, 1, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(functions[i].start, disassembly))
            GuiReferenceSetCellContent(i, 2, disassembly);
        char label[MAX_LABEL_SIZE] = "";
        if(labelget(functions[i].start, label))
            GuiReferenceSetCellContent(i, 3, label);
        else
        {
            char comment[MAX_COMMENT_SIZE] = "";
            if(commentget(functions[i].start, comment))
                GuiReferenceSetCellContent(i, 3, comment);
        }
    }
    varset("$result", count, false);
    dprintf("%d function(s) listed in Reference View\n", count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLoopList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize("Loops");
    GuiReferenceAddColumn(2 * sizeof(uint), "Start");
    GuiReferenceAddColumn(2 * sizeof(uint), "End");
    GuiReferenceAddColumn(64, "Disassembly (Start)");
    GuiReferenceAddColumn(0, "Label/Comment");
    GuiReferenceReloadData();
    size_t cbsize;
    loopenum(0, &cbsize);
    if(!cbsize)
    {
        dputs("no loops");
        return STATUS_CONTINUE;
    }
    Memory<LOOPSINFO*> loops(cbsize, "cbInstrLoopList:loops");
    loopenum(loops, 0);
    int count = (int)(cbsize / sizeof(LOOPSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf(addrText, "%p", loops[i].start);
        GuiReferenceSetCellContent(i, 0, addrText);
        sprintf(addrText, "%p", loops[i].end);
        GuiReferenceSetCellContent(i, 1, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(loops[i].start, disassembly))
            GuiReferenceSetCellContent(i, 2, disassembly);
        char label[MAX_LABEL_SIZE] = "";
        if(labelget(loops[i].start, label))
            GuiReferenceSetCellContent(i, 3, label);
        else
        {
            char comment[MAX_COMMENT_SIZE] = "";
            if(commentget(loops[i].start, comment))
                GuiReferenceSetCellContent(i, 3, comment);
        }
    }
    varset("$result", count, false);
    dprintf("%d loop(s) listed in Reference View\n", count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrSleep(int argc, char* argv[])
{
    uint ms = 100;
    if(argc > 1)
        if(!valfromstring(argv[1], &ms, false))
            return STATUS_ERROR;
    if(ms >= 0xFFFFFFFF)
        ms = 100;
    Sleep((DWORD)ms);
    return STATUS_CONTINUE;
}

//reffindasm value[,page]
static bool cbFindAsm(DISASM* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo)
{
    if(!disasm && !basicinfo) //initialize
    {
        GuiReferenceInitialize(refinfo->name);
        GuiReferenceAddColumn(2 * sizeof(uint), "Address");
        GuiReferenceAddColumn(0, "Disassembly");
        GuiReferenceReloadData();
        return true;
    }
    const char* instruction = (const char*)refinfo->userinfo;
    bool found = !_stricmp(instruction, basicinfo->instruction);
    if(found)
    {
        char addrText[20] = "";
        sprintf(addrText, "%p", disasm->VirtualAddr);
        GuiReferenceSetRowCount(refinfo->refcount + 1);
        GuiReferenceSetCellContent(refinfo->refcount, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly((duint)disasm->VirtualAddr, disassembly))
            GuiReferenceSetCellContent(refinfo->refcount, 1, disassembly);
        else
            GuiReferenceSetCellContent(refinfo->refcount, 1, disasm->CompleteInstr);
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

    uint addr = 0;
    if(argc < 3 or !valfromstring(argv[2], &addr))
        addr = GetContextDataEx(hActiveThread, UE_CIP);
    uint size = 0;
    if(argc >= 4)
        if(!valfromstring(argv[3], &size))
            size = 0;

    unsigned char dest[16];
    int asmsize = 0;
    char error[256] = "";
    if(!assemble(addr + size / 2, dest, &asmsize, argv[1], error))
    {
        dprintf("failed to assemble \"%s\" (%s)!\n", argv[1], error);
        return STATUS_ERROR;
    }
    BASIC_INSTRUCTION_INFO basicinfo;
    memset(&basicinfo, 0, sizeof(BASIC_INSTRUCTION_INFO));
    disasmfast(dest, addr + size / 2, &basicinfo);

    uint ticks = GetTickCount();
    int found = reffind(addr, size, cbFindAsm, (void*)&basicinfo.instruction[0], false, "Command");
    dprintf("%u result(s) in %ums\n", found, GetTickCount() - ticks);
    varset("$result", found, false);
    return STATUS_CONTINUE;
}
