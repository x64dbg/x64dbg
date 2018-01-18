#include "cmd-general-purpose.h"
#include "value.h"
#include "memory.h"
#include "variable.h"
#include "_scriptapi_stack.h"
#include "debugger.h"
#include <intrin.h>

static bool ReadWriteVariable(const char* varname, const std::function<bool(duint*, int)> & callback)
{
    duint set_value = 0;
    bool isvar;
    int varsize;
    if(!valfromstring(varname, &set_value, true, true, &varsize, &isvar))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid variable \"%s\"\n"), varname);
        return false;
    }
    bool retVal = callback(&set_value, varsize);
    if(retVal != true)
        return retVal;
    duint temp = 0;
    valfromstring(varname, &temp, true, true, 0, nullptr, 0); //there is no return check on this because the destination might not exist yet
    if(!isvar)
        isvar = vargettype(varname, 0);
    if(!isvar || !valtostring(varname, set_value, true))
    {
        duint value;
        if(valfromstring(varname, &value)) //if the var is a value already it's an invalid destination
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid variable \"%s\"\n"), varname);
            return false;
        }
        varnew(varname, set_value, VAR_USER);
    }
    return true;
}

bool cbInstrInc(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    return cmddirectexec(StringUtils::sprintf("%s++", argv[1]).c_str());
}

bool cbInstrDec(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    return cmddirectexec(StringUtils::sprintf("%s--", argv[1]).c_str());
}

bool cbInstrAdd(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    return cmddirectexec(StringUtils::sprintf("%s+=%s", argv[1], argv[2]).c_str());
}

bool cbInstrSub(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    return cmddirectexec(StringUtils::sprintf("%s-=%s", argv[1], argv[2]).c_str());
}

bool cbInstrMul(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    return cmddirectexec(StringUtils::sprintf("%s*=%s", argv[1], argv[2]).c_str());
}

bool cbInstrDiv(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    return cmddirectexec(StringUtils::sprintf("%s/=%s", argv[1], argv[2]).c_str());
}

bool cbInstrAnd(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    return cmddirectexec(StringUtils::sprintf("%s&=%s", argv[1], argv[2]).c_str());
}

bool cbInstrOr(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    return cmddirectexec(StringUtils::sprintf("%s|=%s", argv[1], argv[2]).c_str());
}

bool cbInstrXor(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    return cmddirectexec(StringUtils::sprintf("%s^=%s", argv[1], argv[2]).c_str());
}

bool cbInstrNeg(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    return cmddirectexec(StringUtils::sprintf("%s=-%s", argv[1], argv[1]).c_str());
}

bool cbInstrNot(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    return cmddirectexec(StringUtils::sprintf("%s=~%s", argv[1], argv[1]).c_str());
}

bool cbInstrBswap(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
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
            return false;
        }
        return true;
    });
}

bool cbInstrRol(int argc, char* argv[])
{
    duint value2;
    if(IsArgumentsLessThan(argc, 3) || !valfromstring(argv[2], &value2, false))
        return false;
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
            return false;
        }
        return true;
    });
}

bool cbInstrRor(int argc, char* argv[])
{
    duint value2;
    if(IsArgumentsLessThan(argc, 3) || !valfromstring(argv[2], &value2, false))
        return false;
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
            return false;
        }
        return true;
    });
}

bool cbInstrShl(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    //SHL and SAL have the same semantics
    return cmddirectexec(StringUtils::sprintf("%s<<=%s", argv[1], argv[2]).c_str());
}

bool cbInstrShr(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    auto oldType = valuesignedcalc();
    valuesetsignedcalc(false); //SHR is unsigned
    auto result = cmddirectexec(StringUtils::sprintf("%s>>=%s", argv[1], argv[2]).c_str());
    valuesetsignedcalc(oldType);
    return result;
}

bool cbInstrSar(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    auto oldType = valuesignedcalc();
    valuesetsignedcalc(true); //SAR is signed
    auto result = cmddirectexec(StringUtils::sprintf("%s>>=%s", argv[1], argv[2]).c_str());
    valuesetsignedcalc(oldType);
    return result;
}

bool cbInstrPush(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint value;
    if(!valfromstring(argv[1], &value))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "invalid argument \"%s\"!\n"), argv[1]);
        return false;
    }
    Script::Stack::Push(value);
    duint csp = GetContextDataEx(hActiveThread, UE_CSP);
    DebugUpdateStack(csp, csp);
    GuiUpdateRegisterView();
    return true;
}

bool cbInstrPop(int argc, char* argv[])
{
    duint value = Script::Stack::Pop();
    duint csp = GetContextDataEx(hActiveThread, UE_CSP);
    DebugUpdateStack(csp, csp);
    GuiUpdateRegisterView();
    if(argc > 1)
    {
        if(!valtostring(argv[1], value, false))
            return false;
    }
    return true;
}

bool cbInstrTest(int argc, char* argv[])
{
    //TODO: test
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint arg1 = 0;
    if(!valfromstring(argv[1], &arg1, false))
        return false;
    duint arg2 = 0;
    if(!valfromstring(argv[2], &arg2, false))
        return false;
    duint ezflag;
    duint bsflag = 0;
    if(!(arg1 & arg2))
        ezflag = 1;
    else
        ezflag = 0;
    varset("$_EZ_FLAG", ezflag, true);
    varset("$_BS_FLAG", bsflag, true);
    //dprintf(QT_TRANSLATE_NOOP("DBG", "$_EZ_FLAG=%d, $_BS_FLAG=%d\n"), ezflag, bsflag);
    return true;
}

bool cbInstrCmp(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint arg1 = 0;
    if(!valfromstring(argv[1], &arg1, false))
        return false;
    duint arg2 = 0;
    if(!valfromstring(argv[2], &arg2, false))
        return false;
    duint ezflag;
    duint bsflag;
    if(arg1 == arg2)
        ezflag = 1;
    else
        ezflag = 0;
    if(valuesignedcalc()) //signed comparision
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
    return true;
}

bool cbInstrMov(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    String srcText = argv[2];
    if(srcText[0] == '#' && srcText[srcText.length() - 1] == '#') //handle mov addr, #DATA#
    {
        //do some checks on the data
        String dataText = srcText.substr(1, srcText.length() - 2);
        std::vector<unsigned char> data;
        if(!StringUtils::FromCompressedHex(dataText, data))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid hex string \"%s\"\n"), dataText.c_str());
            return false;
        }
        //Check the destination
        duint dest;
        if(!valfromstring(argv[1], &dest) || !MemIsValidReadPtr(dest))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid destination \"%s\"\n"), argv[1]);
            return false;
        }
        //Move data to destination
        if(!MemPatch(dest, data.data(), data.size()))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to write to %p\n"), dest);
            return false;
        }
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else
    {
        duint set_value = 0;
        if(!valfromstring(srcText.c_str(), &set_value))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid src \"%s\"\n"), argv[2]);
            return false;
        }
        bool isvar = false;
        duint temp = 0;
        valfromstring(argv[1], &temp, true, true, 0, &isvar, 0); //there is no return check on this because the destination might not exist yet
        if(!isvar)
            isvar = vargettype(argv[1], 0);
        if(!isvar || !valtostring(argv[1], set_value, true))
        {
            duint value;
            if(valfromstring(argv[1], &value)) //if the var is a value already it's an invalid destination
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid dest \"%s\"\n"), argv[1]);
                return false;
            }
            varnew(argv[1], set_value, VAR_USER);
        }
    }
    return true;
}