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

bool cbInstrMulhi(int argc, char* argv[])
{
    duint value2;
    if(IsArgumentsLessThan(argc, 3) || !valfromstring(argv[2], &value2, false))
        return false;

    return ReadWriteVariable(argv[1], [value2](duint * value, int size)
    {
#ifdef _WIN64
        unsigned __int64 res;
        _umul128(*value, value2, &res);
        *value = res;
#else //x86
        *value = (((unsigned long long)value2) * (*value)) >> 32;
#endif
        return true;
    });
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

bool cbInstrPopcnt(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint arg = 0;
    if(!valfromstring(argv[2], &arg, false))
        return false;
    duint ezflag;
    duint bsflag = 0;
    if(arg == 0)
    {
        ezflag = 1;
    }
    else
    {
        ezflag = 0;
#ifdef _WIN64
        arg = __popcnt64(arg);
#else //x86
        arg = __popcnt(arg);
#endif
        bool isvar = false;
        duint temp = 0;
        valfromstring(argv[1], &temp, true, true, 0, &isvar, 0); //there is no return check on this because the destination might not exist yet
        if(!isvar)
            isvar = vargettype(argv[1], 0);
        if(!isvar || !valtostring(argv[1], arg, true))
        {
            duint value;
            if(valfromstring(argv[1], &value))  //if the var is a value already it's an invalid destination
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid dest \"%s\"\n"), argv[1]);
                return false;
            }
            varnew(argv[1], arg, VAR_USER);
        }
    }
    varset("$_EZ_FLAG", ezflag, true);
    varset("$_BS_FLAG", bsflag, true);
    return true;
}

bool cbInstrLzcnt(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint arg = 0;
    if(!valfromstring(argv[2], &arg, false))
        return false;
    duint ezflag;
    duint bsflag = 0;
    if(arg == 0)
    {
        ezflag = 1;
        arg = sizeof(duint);
    }
    else
    {
        ezflag = 0;
#ifdef _WIN64
        arg = __lzcnt64(arg);
#else //x86
        arg = __lzcnt(arg);
#endif
        bool isvar = false;
        duint temp = 0;
        valfromstring(argv[1], &temp, true, true, 0, &isvar, 0); //there is no return check on this because the destination might not exist yet
        if(!isvar)
            isvar = vargettype(argv[1], 0);
        if(!isvar || !valtostring(argv[1], arg, true))
        {
            duint value;
            if(valfromstring(argv[1], &value))  //if the var is a value already it's an invalid destination
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid dest \"%s\"\n"), argv[1]);
                return false;
            }
            varnew(argv[1], arg, VAR_USER);
        }
    }
    varset("$_EZ_FLAG", ezflag, true);
    varset("$_BS_FLAG", bsflag, true);
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

bool cbInstrMovdqu(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    String dstText = argv[1];
    String srcText = argv[2];
    duint address = 0;
    DWORD registerindex = 0;
    if(srcText[0] == '[' && srcText[srcText.length() - 1] == ']' && _memicmp(dstText.c_str(), "xmm", 3) == 0)
    {
        char newValue[16];
        // movdqu xmm0, [address]
        dstText = dstText.substr(3);
        srcText = srcText.substr(1, srcText.size() - 2);
        DWORD registerindex;
        bool found = true;
        registerindex = atoi(dstText.c_str());
        if(registerindex < ArchValue(8, 16))
        {
            registerindex += UE_XMM0;
        }
        else
        {
            goto InvalidDest;
        }
        if(!valfromstring(srcText.c_str(), &address))
        {
            goto InvalidSrc;
        }
        if(!MemRead(address, newValue, 16))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read (all) memory..."));
            return false;
        }
        SetContextDataEx(hActiveThread, registerindex, (ULONG_PTR)newValue);
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else if(dstText[0] == '[' && dstText[dstText.length() - 1] == ']' && _memicmp(srcText.c_str(), "xmm", 3) == 0)
    {
        // movdqu [address], xmm0
        srcText = srcText.substr(3);
        dstText = dstText.substr(1, dstText.size() - 2);
        DWORD registerindex;
        bool found = true;
        registerindex = atoi(srcText.c_str());
        if(registerindex >= ArchValue(8, 16))
        {
            goto InvalidSrc;
        }
        if(!valfromstring(dstText.c_str(), &address) || !MemIsValidReadPtr(address))
        {
            goto InvalidDest;
        }
        REGDUMP_AVX512 registers;
        if(!DbgGetRegDumpEx(&registers, sizeof(registers)))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        if(!MemWrite(address, &registers.regcontext.ZmmRegisters[registerindex].Low.Low, 16))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to write to %p\n"), address);
            return false;
        }
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else if(_memicmp(srcText.c_str(), "xmm", 3) == 0 && _memicmp(dstText.c_str(), "xmm", 3) == 0)
    {
        // movdqu xmm0, xmm1
        srcText = srcText.substr(3);
        dstText = dstText.substr(3);
        DWORD registerindex[2];
        bool found = true;
        registerindex[0] = atoi(srcText.c_str());
        if(registerindex[0] >= ArchValue(8, 16))
        {
            goto InvalidSrc;
        }
        registerindex[1] = atoi(dstText.c_str());
        if(registerindex[1] < ArchValue(8, 16))
        {
            registerindex[1] += UE_XMM0;
        }
        else
        {
            goto InvalidDest;
        }
        REGDUMP_AVX512 registers;
        if(!DbgGetRegDumpEx(&registers, sizeof(registers)))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        SetContextDataEx(hActiveThread, registerindex[1], (ULONG_PTR)&registers.regcontext.ZmmRegisters[registerindex[0]].Low.Low);
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Usage: movdqu xmm0, [address] / movdqu [address], xmm0 / movdqu xmm0, xmm1"));
        return false;
    }
InvalidSrc:
    dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid src \"%s\"\n"), argv[2]);
    return false;
InvalidDest:
    dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid dest \"%s\"\n"), argv[1]);
    return false;
}

bool cbInstrVmovdqu(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    String dstText = argv[1];
    String srcText = argv[2];
    duint address = 0;
    DWORD registerindex = 0;
    // AVX
    if(srcText[0] == '[' && srcText[srcText.length() - 1] == ']' && _memicmp(dstText.c_str(), "ymm", 3) == 0)
    {
        YmmRegister_t newValue;
        // vmovdqu ymm0, [address]
        dstText = dstText.substr(3);
        srcText = srcText.substr(1, srcText.size() - 2);
        DWORD registerindex;
        bool found = true;
        registerindex = atoi(dstText.c_str());
        if(registerindex >= ArchValue(8, 32))
        {
            goto InvalidDest;
        }
        if(!valfromstring(srcText.c_str(), &address))
        {
            goto InvalidSrc;
        }
        if(!MemRead(address, &newValue, sizeof(newValue)))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read (all) memory..."));
            return false;
        }
        TITAN_ENGINE_CONTEXT_t context;
        if(!GetAVXContext(hActiveThread, &context))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        context.YmmRegisters[registerindex] = newValue;
        SetAVXContext(hActiveThread, &context);
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else if(dstText[0] == '[' && dstText[dstText.length() - 1] == ']' && _memicmp(srcText.c_str(), "ymm", 3) == 0)
    {
        // vmovdqu [address], ymm0
        srcText = srcText.substr(3);
        dstText = dstText.substr(1, dstText.size() - 2);
        DWORD registerindex;
        bool found = true;
        registerindex = atoi(srcText.c_str());
        if(registerindex >= ArchValue(8, 32))
        {
            goto InvalidSrc;
        }
        if(!valfromstring(dstText.c_str(), &address) || !MemIsValidReadPtr(address))
        {
            goto InvalidDest;
        }
        TITAN_ENGINE_CONTEXT_t context;
        if(!GetAVXContext(hActiveThread, &context))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        if(!MemWrite(address, &context.YmmRegisters[registerindex], sizeof(ZmmRegister_t)))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to write to %p\n"), address);
            return false;
        }
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else if(_memicmp(srcText.c_str(), "ymm", 3) == 0 && _memicmp(dstText.c_str(), "ymm", 3) == 0)
    {
        // vmovdqu ymm0, ymm1
        srcText = srcText.substr(3);
        dstText = dstText.substr(3);
        DWORD registerindex[2];
        bool found = true;
        registerindex[0] = atoi(srcText.c_str());
        if(registerindex[0] >= ArchValue(8, 32))
        {
            goto InvalidSrc;
        }
        registerindex[1] = atoi(dstText.c_str());
        if(registerindex[1] >= ArchValue(8, 32))
        {
            goto InvalidDest;
        }
        TITAN_ENGINE_CONTEXT_t context;
        if(!GetAVXContext(hActiveThread, &context))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        context.YmmRegisters[registerindex[1]] = context.YmmRegisters[registerindex[0]];
        SetAVXContext(hActiveThread, &context);
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    // AVX-512
    else if(srcText[0] == '[' && srcText[srcText.length() - 1] == ']' && _memicmp(dstText.c_str(), "zmm", 3) == 0)
    {
        ZmmRegister_t newValue;
        // vmovdqu zmm0, [address]
        dstText = dstText.substr(3);
        srcText = srcText.substr(1, srcText.size() - 2);
        DWORD registerindex;
        bool found = true;
        registerindex = atoi(dstText.c_str());
        if(registerindex >= ArchValue(8, 32))
        {
            goto InvalidDest;
        }
        if(!valfromstring(srcText.c_str(), &address))
        {
            goto InvalidSrc;
        }
        if(!MemRead(address, &newValue, sizeof(newValue)))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read (all) memory..."));
            return false;
        }
        TITAN_ENGINE_CONTEXT_AVX512_t context;
        if(!GetAVX512Context(hActiveThread, &context))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        context.ZmmRegisters[registerindex] = newValue;
        SetAVX512Context(hActiveThread, &context);
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else if(dstText[0] == '[' && dstText[dstText.length() - 1] == ']' && _memicmp(srcText.c_str(), "zmm", 3) == 0)
    {
        // vmovdqu [address], zmm0
        srcText = srcText.substr(3);
        dstText = dstText.substr(1, dstText.size() - 2);
        DWORD registerindex;
        bool found = true;
        registerindex = atoi(srcText.c_str());
        if(registerindex >= ArchValue(8, 32))
        {
            goto InvalidSrc;
        }
        if(!valfromstring(dstText.c_str(), &address) || !MemIsValidReadPtr(address))
        {
            goto InvalidDest;
        }
        TITAN_ENGINE_CONTEXT_AVX512_t context;
        if(!GetAVX512Context(hActiveThread, &context))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        if(!MemWrite(address, &context.ZmmRegisters[registerindex], sizeof(ZmmRegister_t)))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to write to %p\n"), address);
            return false;
        }
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else if(_memicmp(srcText.c_str(), "zmm", 3) == 0 && _memicmp(dstText.c_str(), "zmm", 3) == 0)
    {
        // vmovdqu zmm0, zmm1
        srcText = srcText.substr(3);
        dstText = dstText.substr(3);
        DWORD registerindex[2];
        bool found = true;
        registerindex[0] = atoi(srcText.c_str());
        if(registerindex[0] >= ArchValue(8, 32))
        {
            goto InvalidSrc;
        }
        registerindex[1] = atoi(dstText.c_str());
        if(registerindex[1] >= ArchValue(8, 32))
        {
            goto InvalidDest;
        }
        TITAN_ENGINE_CONTEXT_AVX512_t context;
        if(!GetAVX512Context(hActiveThread, &context))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        context.ZmmRegisters[registerindex[1]] = context.ZmmRegisters[registerindex[0]];
        SetAVX512Context(hActiveThread, &context);
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Usage: vmovdqu zmm0 (or ymm0), [address] / vmovdqu [address], zmm0 (or ymm0) / vmovdqu zmm0 (or ymm0), zmm1 (or ymm1)"));
        return false;
    }
InvalidSrc:
    dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid src \"%s\"\n"), argv[2]);
    return false;
InvalidDest:
    dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid dest \"%s\"\n"), argv[1]);
    return false;
}

static bool isOpmaskRegister(const String & name)
{
    return name.size() == 2 && (name[0] == 'k' || name[0] == 'K') && (name[1] >= '0' && name[1] <= '7');
}

bool cbInstrKmovq(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    String dstText = argv[1];
    String srcText = argv[2];
    srcText = StringUtils::Trim(srcText);
    dstText = StringUtils::Trim(dstText);
    const bool isKregisters[2] = { isOpmaskRegister(srcText), isOpmaskRegister(dstText) };
    duint address = 0;
    if(isKregisters[0] && isKregisters[1])
    {
        // kmovq k1, k2
        srcText = srcText.substr(1);
        dstText = dstText.substr(1);
        int registerindex[2];
        bool found = true;
        registerindex[0] = atoi(srcText.c_str());
        if(registerindex[0] >= 8)
        {
            goto InvalidSrc;
        }
        registerindex[1] = atoi(dstText.c_str());
        if(registerindex[1] >= 8)
        {
            goto InvalidDest;
        }
        TITAN_ENGINE_CONTEXT_AVX512_t context;
        if(!GetAVX512Context(hActiveThread, &context))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        context.Opmask[registerindex[1]] = context.Opmask[registerindex[0]];
        SetAVX512Context(hActiveThread, &context);
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else if(isKregisters[1])
    {
        ULONGLONG newValue;
        // kmovq k1, [address]
#ifdef _WIN64
        // 64-bit, can also read from registers
        if(!valfromstring(srcText.c_str(), &newValue))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read (all) memory..."));
            return false;
        }
#else // x86
        // 32-bit, memory location only
        if(srcText[0] == '[' && srcText[srcText.length() - 1] == ']')
        {
            srcText = srcText.substr(1, srcText.size() - 2);
            if(!valfromstring(srcText.c_str(), &address))
            {
                goto InvalidSrc;
            }
            if(!MemRead(address, &newValue, sizeof(newValue)))
            {
                dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read (all) memory..."));
                return false;
            }
        }
#endif
        dstText = dstText.substr(1);
        int registerindex;
        bool found = true;
        registerindex = atoi(dstText.c_str());
        if(registerindex >= 8)
        {
            goto InvalidDest;
        }
        TITAN_ENGINE_CONTEXT_AVX512_t context;
        if(!GetAVX512Context(hActiveThread, &context))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        context.Opmask[registerindex] = newValue;
        SetAVX512Context(hActiveThread, &context);
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else if(isKregisters[0])
    {
        // kmovq [address], k1
        srcText = srcText.substr(1);
        int registerindex;
        bool found = true;
        registerindex = atoi(srcText.c_str());
        if(registerindex >= 8)
        {
            goto InvalidSrc;
        }
        TITAN_ENGINE_CONTEXT_AVX512_t context;
        if(!GetAVX512Context(hActiveThread, &context))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
#ifdef _WIN64
        if(!valtostring(dstText.c_str(), context.Opmask[registerindex], false))
        {
            goto InvalidDest;
        }
#else // x86
        if(dstText[0] == '[' && dstText[dstText.length() - 1] == ']')
        {
            dstText = dstText.substr(1, dstText.size() - 2);
            if(!valfromstring(dstText.c_str(), &address) || !MemIsValidReadPtr(address))
            {
                goto InvalidDest;
            }
            if(!MemWrite(address, &context.Opmask[registerindex], sizeof(ULONGLONG)))
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to write to %p\n"), address);
                return false;
            }
        }
#endif
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Usage: kmovq k1, [address] / kmovq [address], k1 / kmovq k1, k2"));
        return false;
    }
InvalidSrc:
    dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid src \"%s\"\n"), argv[2]);
    return false;
InvalidDest:
    dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid dest \"%s\"\n"), argv[1]);
    return false;
}

bool cbInstrKmovd(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    String dstText = argv[1];
    String srcText = argv[2];
    srcText = StringUtils::Trim(srcText);
    dstText = StringUtils::Trim(dstText);
    const bool isKregisters[2] = { isOpmaskRegister(srcText), isOpmaskRegister(dstText) };
    if(isKregisters[0] && isKregisters[1])
    {
        // kmovq k1, k2
        srcText = srcText.substr(1);
        dstText = dstText.substr(1);
        int registerindex[2];
        bool found = true;
        registerindex[0] = atoi(srcText.c_str());
        if(registerindex[0] >= 8)
        {
            goto InvalidSrc;
        }
        registerindex[1] = atoi(dstText.c_str());
        if(registerindex[1] >= 8)
        {
            goto InvalidDest;
        }
        TITAN_ENGINE_CONTEXT_AVX512_t context;
        if(!GetAVX512Context(hActiveThread, &context))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        // Zero extend to 64-bit
        context.Opmask[registerindex[1]] = DWORD(context.Opmask[registerindex[0]]);
        SetAVX512Context(hActiveThread, &context);
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else if(isKregisters[1])
    {
        duint newValue;
        // kmovd k1, [address]
        if(!valfromstring(srcText.c_str(), &newValue))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid src \"%s\"\n"), srcText.c_str());
            return false;
        }
        // Zero extend to 64 bits
        newValue = DWORD(newValue);
        dstText = dstText.substr(1);
        int registerindex;
        bool found = true;
        registerindex = atoi(dstText.c_str());
        if(registerindex >= 8)
        {
            goto InvalidDest;
        }
        TITAN_ENGINE_CONTEXT_AVX512_t context;
        if(!GetAVX512Context(hActiveThread, &context))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        context.Opmask[registerindex] = newValue;
        SetAVX512Context(hActiveThread, &context);
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else if(isKregisters[0])
    {
        // kmovq [address], k1
        srcText = srcText.substr(1);
        int registerindex;
        bool found = true;
        registerindex = atoi(srcText.c_str());
        if(registerindex >= 8)
        {
            goto InvalidSrc;
        }
        TITAN_ENGINE_CONTEXT_AVX512_t context;
        if(!GetAVX512Context(hActiveThread, &context))
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            return false;
        }
        if(!valtostring(dstText.c_str(), DWORD(context.Opmask[registerindex]), false))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to write to %s\n"), dstText.c_str());
            return false;
        }
        GuiUpdateAllViews(); //refresh disassembly/dump/etc
        return true;
    }
    else
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Usage: kmovd k1, [address] / kmovd [address], k1 / kmovd k1, k2"));
        return false;
    }
InvalidSrc:
    dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid src \"%s\"\n"), argv[2]);
    return false;
InvalidDest:
    dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid dest \"%s\"\n"), argv[1]);
    return false;
}