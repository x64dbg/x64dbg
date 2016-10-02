#include "cmd-general-purpose.h"
#include "value.h"
#include "memory.h"
#include "variable.h"
#include "_scriptapi_stack.h"
#include "debugger.h"

static CMDRESULT ReadWriteVariable(const char* varname, const std::function<CMDRESULT(duint*, int)> & callback)
{
    duint set_value = 0;
    bool isvar;
    int varsize;
    if(!valfromstring(varname, &set_value, true, true, &varsize, &isvar))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "invalid variable \"%s\"\n"), varname);
        return STATUS_ERROR;
    }
    CMDRESULT retVal = callback(&set_value, varsize);
    if(retVal != STATUS_CONTINUE)
        return retVal;
    duint temp = 0;
    valfromstring(varname, &temp, true, true, 0, nullptr, 0); //there is no return check on this because the destination might not exist yet
    if(!isvar)
        isvar = vargettype(varname, 0);
    if(!isvar || !valtostring(varname, set_value, true))
    {
        duint value;
        if(valfromstring(varname, &value))  //if the var is a value already it's an invalid destination
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "invalid variable \"%s\"\n"), varname);
            return STATUS_ERROR;
        }
        varnew(varname, set_value, VAR_USER);
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrInc(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    return cmddirectexec(StringUtils::sprintf("%s++", argv[1]).c_str());
}

CMDRESULT cbInstrDec(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    return cmddirectexec(StringUtils::sprintf("%s--", argv[1]).c_str());
}

CMDRESULT cbInstrAdd(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    return cmddirectexec(StringUtils::sprintf("%s+=%s", argv[1], argv[2]).c_str());
}

CMDRESULT cbInstrSub(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    return cmddirectexec(StringUtils::sprintf("%s-=%s", argv[1], argv[2]).c_str());
}

CMDRESULT cbInstrMul(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    return cmddirectexec(StringUtils::sprintf("%s*=%s", argv[1], argv[2]).c_str());
}

CMDRESULT cbInstrDiv(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    return cmddirectexec(StringUtils::sprintf("%s/=%s", argv[1], argv[2]).c_str());
}

CMDRESULT cbInstrAnd(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    return cmddirectexec(StringUtils::sprintf("%s&=%s", argv[1], argv[2]).c_str());
}

CMDRESULT cbInstrOr(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    return cmddirectexec(StringUtils::sprintf("%s|=%s", argv[1], argv[2]).c_str());
}

CMDRESULT cbInstrXor(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    return cmddirectexec(StringUtils::sprintf("%s^=%s", argv[1], argv[2]).c_str());
}

CMDRESULT cbInstrNeg(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    return cmddirectexec(StringUtils::sprintf("%s=-%s", argv[1], argv[1]).c_str());
}

CMDRESULT cbInstrNot(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    return cmddirectexec(StringUtils::sprintf("%s=~%s", argv[1], argv[1]).c_str());
}

CMDRESULT cbInstrBswap(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    return ReadWriteVariable(argv[1], [](duint * value, int size)
    {
        if(size == 2)
            *value = _byteswap_ushort((uint16) * value);
        else if(size == 4)
            *value = _byteswap_ulong((uint32) * value);
#ifdef _WIN64
        else if(size == 8)
            *value = _byteswap_uint64(*value);
#endif //_WIN64
        else if(size != 1)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Variable size not supported."));
            return STATUS_ERROR;
        }
        return STATUS_CONTINUE;
    });
}

CMDRESULT cbInstrRol(int argc, char* argv[])
{
    duint value2;
    if(IsArgumentsLessThan(argc, 3) || !valfromstring(argv[2], &value2, false))
        return STATUS_ERROR;
    return ReadWriteVariable(argv[1], [value2](duint * value, int size)
    {
        if(size == 1)
            *value = _rotl8((uint8_t) * value, value2 % 8);
        else if(size == 2)
            *value = _rotl16((uint16) * value, value2 % 16);
        else if(size == 4)
            *value = _rotl((uint32) * value, value2 % 32);
#ifdef _WIN64
        else if(size == 8)
            *value = _rotl64(*value, value2 % 64);
#endif //_WIN64
        else
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Variable size not supported."));
            return STATUS_ERROR;
        }
        return STATUS_CONTINUE;
    });
}

CMDRESULT cbInstrRor(int argc, char* argv[])
{
    duint value2;
    if(IsArgumentsLessThan(argc, 3) || !valfromstring(argv[2], &value2, false))
        return STATUS_ERROR;
    return ReadWriteVariable(argv[1], [value2](duint * value, int size)
    {
        if(size == 1)
            *value = _rotr8((uint8_t) * value, value2 % 8);
        else if(size == 2)
            *value = _rotr16((uint16) * value, value2 % 16);
        else if(size == 4)
            *value = _rotr((uint32) * value, value2 % 32);
#ifdef _WIN64
        else if(size == 8)
            *value = _rotr64(*value, value2 % 64);
#endif //_WIN64
        else
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Variable size not supported."));
            return STATUS_ERROR;
        }
        return STATUS_CONTINUE;
    });
}

CMDRESULT cbInstrShl(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    //SHL and SAL have the same semantics
    return cmddirectexec(StringUtils::sprintf("%s<<=%s", argv[1], argv[2]).c_str());
}

CMDRESULT cbInstrShr(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    auto oldType = valuesignedcalc();
    valuesetsignedcalc(false); //SHR is unsigned
    auto result = cmddirectexec(StringUtils::sprintf("%s>>=%s", argv[1], argv[2]).c_str());
    valuesetsignedcalc(oldType);
    return result;
}

CMDRESULT cbInstrSar(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    auto oldType = valuesignedcalc();
    valuesetsignedcalc(true); //SAR is signed
    auto result = cmddirectexec(StringUtils::sprintf("%s>>=%s", argv[1], argv[2]).c_str());
    valuesetsignedcalc(oldType);
    return result;
}

CMDRESULT cbInstrPush(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "not enough arguments!"));
        return STATUS_ERROR;
    }
    duint value;
    if(!valfromstring(argv[1], &value))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "invalid argument \"%s\"!\n"), argv[1]);
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

CMDRESULT cbInstrTest(int argc, char* argv[])
{
    //TODO: test
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
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
    //dprintf(QT_TRANSLATE_NOOP("DBG", "$_EZ_FLAG=%d, $_BS_FLAG=%d\n"), ezflag, bsflag);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCmp(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
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
    if(valuesignedcalc())   //signed comparision
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
    //dprintf(QT_TRANSLATE_NOOP("DBG", "$_EZ_FLAG=%d, $_BS_FLAG=%d\n"), ezflag, bsflag);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrMov(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    String srcText = argv[2];
    if(srcText[0] == '#' && srcText[srcText.length() - 1] == '#')   //handle mov addr, #DATA#
    {
        //do some checks on the data
        String dataText = srcText.substr(1, srcText.length() - 2);
        int len = (int)dataText.length();
        if(len % 2)
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "invalid hex string \"%s\" (length not divisible by 2)\n"), dataText.c_str());
            return STATUS_ERROR;
        }
        for(int i = 0; i < len; i++)
        {
            if(!isxdigit(dataText[i]))
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "invalid hex string \"%s\" (contains invalid characters)\n"), dataText.c_str());
                return STATUS_ERROR;
            }
        }
        //Check the destination
        duint dest;
        if(!valfromstring(argv[1], &dest) || !MemIsValidReadPtr(dest))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "invalid destination \"%s\"\n"), argv[1]);
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
                dprintf(QT_TRANSLATE_NOOP("DBG", "invalid hex byte \"%s\"\n"), b);
                return STATUS_ERROR;
            }
            data()[j] = res;
        }
        //Move data to destination
        if(!MemWrite(dest, data(), data.size()))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "failed to write to %p\n"), dest);
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
            dprintf(QT_TRANSLATE_NOOP("DBG", "invalid src \"%s\"\n"), argv[2]);
            return STATUS_ERROR;
        }
        bool isvar = false;
        duint temp = 0;
        valfromstring(argv[1], &temp, true, true, 0, &isvar, 0); //there is no return check on this because the destination might not exist yet
        if(!isvar)
            isvar = vargettype(argv[1], 0);
        if(!isvar || !valtostring(argv[1], set_value, true))
        {
            duint value;
            if(valfromstring(argv[1], &value))  //if the var is a value already it's an invalid destination
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "invalid dest \"%s\"\n"), argv[1]);
                return STATUS_ERROR;
            }
            varnew(argv[1], set_value, VAR_USER);
        }
    }
    return STATUS_CONTINUE;
}