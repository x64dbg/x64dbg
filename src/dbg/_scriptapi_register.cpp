#include "_scriptapi_register.h"
#include "value.h"
#include "debugger.h"
#include "TraceRecord.h"

static const char* regTable[] =
{
    "DR0",
    "DR1",
    "DR2",
    "DR3",
    "DR6",
    "DR7",

    "EAX",
    "AX",
    "AH",
    "AL",
    "EBX",
    "BX",
    "BH",
    "BL",
    "ECX",
    "CX",
    "CH",
    "CL",
    "EDX",
    "DX",
    "DH",
    "DL",
    "EDI",
    "DI",
    "ESI",
    "SI",
    "EBP",
    "BP",
    "ESP",
    "SP",
    "EIP",

#ifdef _WIN64
    "RAX",
    "RBX",
    "RCX",
    "RDX",
    "RSI",
    "SIL",
    "RDI",
    "DIL",
    "RBP",
    "BPL",
    "RSP",
    "SPL",
    "RIP",
    "R8",
    "R8D",
    "R8W",
    "R8B",
    "R9",
    "R9D",
    "R9W",
    "R9B",
    "R10",
    "R10D",
    "R10W",
    "R10B",
    "R11",
    "R11D",
    "R11W",
    "R11B",
    "R12",
    "R12D",
    "R12W",
    "R12B",
    "R13",
    "R13D",
    "R13W",
    "R13B",
    "R14",
    "R14D",
    "R14W",
    "R14B",
    "R15",
    "R15D",
    "R15W",
    "R15",
#endif //_WIN64

    ArchValue("EIP", "RIP"),
    ArchValue("ESP", "RSP"),
    ArchValue("EAX", "RAX"),
    ArchValue("EBX", "RBX"),
    ArchValue("ECX", "RCX"),
    ArchValue("EDX", "RDX"),
    ArchValue("EDI", "RDI"),
    ArchValue("ESI", "RSI"),
    ArchValue("EBP", "RBP"),
    ArchValue("EFLAGS", "RFLAGS")
};

SCRIPT_EXPORT duint Script::Register::Get(Script::Register::RegisterEnum reg)
{
    return getregister(nullptr, regTable[reg]);
}

SCRIPT_EXPORT bool Script::Register::Set(Script::Register::RegisterEnum reg, duint value)
{
    auto result = setregister(regTable[reg], value);

    if(reg == ArchValue(EIP, RIP) || reg == CIP)
    {
        auto cip = GetContextDataEx(hActiveThread, UE_CIP);
        _dbg_dbgtraceexecute(cip);
        DebugUpdateGuiAsync(cip, false); //update disassembly + register view
    }
    else if(reg == ArchValue(ESP, RSP) || reg == SP || reg == CSP) //update stack
    {
        duint csp = GetContextDataEx(hActiveThread, UE_CSP);
        DebugUpdateStack(csp, csp);
    }

    return result;
}

SCRIPT_EXPORT int Script::Register::Size()
{
    return (int)sizeof(duint);
}

SCRIPT_EXPORT duint Script::Register::GetDR0()
{
    return Get(DR0);
}

SCRIPT_EXPORT bool Script::Register::SetDR0(duint value)
{
    return Set(DR0, value);
}

SCRIPT_EXPORT duint Script::Register::GetDR1()
{
    return Get(DR1);
}

SCRIPT_EXPORT bool Script::Register::SetDR1(duint value)
{
    return Set(DR1, value);
}

SCRIPT_EXPORT duint Script::Register::GetDR2()
{
    return Get(DR2);
}

SCRIPT_EXPORT bool Script::Register::SetDR2(duint value)
{
    return Set(DR2, value);
}

SCRIPT_EXPORT duint Script::Register::GetDR3()
{
    return Get(DR3);
}

SCRIPT_EXPORT bool Script::Register::SetDR3(duint value)
{
    return Set(DR3, value);
}

SCRIPT_EXPORT duint Script::Register::GetDR6()
{
    return Get(DR6);
}

SCRIPT_EXPORT bool Script::Register::SetDR6(duint value)
{
    return Set(DR6, value);
}

SCRIPT_EXPORT duint Script::Register::GetDR7()
{
    return Get(DR7);
}

SCRIPT_EXPORT bool Script::Register::SetDR7(duint value)
{
    return Set(DR7, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetEAX()
{
    return (unsigned int)Get(EAX);
}

SCRIPT_EXPORT bool Script::Register::SetEAX(unsigned int value)
{
    return Set(EAX, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetAX()
{
    return (unsigned short)Get(AX);
}

SCRIPT_EXPORT bool Script::Register::SetAX(unsigned short value)
{
    return Set(AX, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetAH()
{
    return (unsigned char)Get(AH);
}

SCRIPT_EXPORT bool Script::Register::SetAH(unsigned char value)
{
    return Set(AH, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetAL()
{
    return (unsigned char)Get(AL);
}

SCRIPT_EXPORT bool Script::Register::SetAL(unsigned char value)
{
    return Set(AL, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetEBX()
{
    return (unsigned int)Get(EBX);
}

SCRIPT_EXPORT bool Script::Register::SetEBX(unsigned int value)
{
    return Set(EBX, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetBX()
{
    return (unsigned short)Get(BX);
}

SCRIPT_EXPORT bool Script::Register::SetBX(unsigned short value)
{
    return Set(BX, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetBH()
{
    return (unsigned char)Get(BH);
}

SCRIPT_EXPORT bool Script::Register::SetBH(unsigned char value)
{
    return Set(BH, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetBL()
{
    return (unsigned char)Get(BL);
}

SCRIPT_EXPORT bool Script::Register::SetBL(unsigned char value)
{
    return Set(BL, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetECX()
{
    return (unsigned int)Get(ECX);
}

SCRIPT_EXPORT bool Script::Register::SetECX(unsigned int value)
{
    return Set(ECX, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetCX()
{
    return (unsigned short)Get(CX);
}

SCRIPT_EXPORT bool Script::Register::SetCX(unsigned short value)
{
    return Set(CX, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetCH()
{
    return (unsigned char)Get(CH);
}

SCRIPT_EXPORT bool Script::Register::SetCH(unsigned char value)
{
    return Set(CH, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetCL()
{
    return (unsigned char)Get(CL);
}

SCRIPT_EXPORT bool Script::Register::SetCL(unsigned char value)
{
    return Set(CL, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetEDX()
{
    return (unsigned int)Get(EDX);
}

SCRIPT_EXPORT bool Script::Register::SetEDX(unsigned int value)
{
    return Set(EDX, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetDX()
{
    return (unsigned short)Get(DX);
}

SCRIPT_EXPORT bool Script::Register::SetDX(unsigned short value)
{
    return Set(DX, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetDH()
{
    return (unsigned char)Get(DH);
}

SCRIPT_EXPORT bool Script::Register::SetDH(unsigned char value)
{
    return Set(DH, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetDL()
{
    return (unsigned char)Get(DL);
}

SCRIPT_EXPORT bool Script::Register::SetDL(unsigned char value)
{
    return Set(DL, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetEDI()
{
    return (unsigned int)Get(EDI);
}

SCRIPT_EXPORT bool Script::Register::SetEDI(unsigned int value)
{
    return Set(EDI, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetDI()
{
    return (unsigned short)Get(DI);
}

SCRIPT_EXPORT bool Script::Register::SetDI(unsigned short value)
{
    return Set(DI, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetESI()
{
    return (unsigned int)Get(ESI);
}

SCRIPT_EXPORT bool Script::Register::SetESI(unsigned int value)
{
    return Set(ESI, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetSI()
{
    return (unsigned short)Get(SI);
}

SCRIPT_EXPORT bool Script::Register::SetSI(unsigned short value)
{
    return Set(SI, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetEBP()
{
    return (unsigned int)Get(EBP);
}

SCRIPT_EXPORT bool Script::Register::SetEBP(unsigned int value)
{
    return Set(EBP, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetBP()
{
    return (unsigned short)Get(BP);
}

SCRIPT_EXPORT bool Script::Register::SetBP(unsigned short value)
{
    return Set(BP, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetESP()
{
    return (unsigned int)Get(ESP);
}

SCRIPT_EXPORT bool Script::Register::SetESP(unsigned int value)
{
    return Set(ESP, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetSP()
{
    return (unsigned short)Get(SP);
}

SCRIPT_EXPORT bool Script::Register::SetSP(unsigned short value)
{
    return Set(SP, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetEIP()
{
    return (unsigned int)Get(EIP);
}

SCRIPT_EXPORT bool Script::Register::SetEIP(unsigned int value)
{
    return Set(EIP, value);
}

#ifdef _WIN64
SCRIPT_EXPORT unsigned long long Script::Register::GetRAX()
{
    return (unsigned long long)Get(RAX);
}

SCRIPT_EXPORT bool Script::Register::SetRAX(unsigned long long value)
{
    return Set(RAX, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetRBX()
{
    return (unsigned long long)Get(RBX);
}

SCRIPT_EXPORT bool Script::Register::SetRBX(unsigned long long value)
{
    return Set(RBX, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetRCX()
{
    return (unsigned long long)Get(RCX);
}

SCRIPT_EXPORT bool Script::Register::SetRCX(unsigned long long value)
{
    return Set(RCX, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetRDX()
{
    return (unsigned long long)Get(RDX);
}

SCRIPT_EXPORT bool Script::Register::SetRDX(unsigned long long value)
{
    return Set(RDX, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetRSI()
{
    return (unsigned long long)Get(RSI);
}

SCRIPT_EXPORT bool Script::Register::SetRSI(unsigned long long value)
{
    return Set(RSI, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetSIL()
{
    return (unsigned char)Get(SIL);
}

SCRIPT_EXPORT bool Script::Register::SetSIL(unsigned char value)
{
    return Set(SIL, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetRDI()
{
    return (unsigned long long)Get(RDI);
}

SCRIPT_EXPORT bool Script::Register::SetRDI(unsigned long long value)
{
    return Set(RDI, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetDIL()
{
    return (unsigned char)Get(DIL);
}

SCRIPT_EXPORT bool Script::Register::SetDIL(unsigned char value)
{
    return Set(DIL, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetRBP()
{
    return (unsigned long long)Get(RBP);
}

SCRIPT_EXPORT bool Script::Register::SetRBP(unsigned long long value)
{
    return Set(RBP, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetBPL()
{
    return (unsigned char)Get(BPL);
}

SCRIPT_EXPORT bool Script::Register::SetBPL(unsigned char value)
{
    return Set(BPL, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetRSP()
{
    return (unsigned long long)Get(RSP);
}

SCRIPT_EXPORT bool Script::Register::SetRSP(unsigned long long value)
{
    return Set(RSP, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetSPL()
{
    return (unsigned char)Get(SPL);
}

SCRIPT_EXPORT bool Script::Register::SetSPL(unsigned char value)
{
    return Set(SPL, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetRIP()
{
    return (unsigned long long)Get(RIP);
}

SCRIPT_EXPORT bool Script::Register::SetRIP(unsigned long long value)
{
    return Set(RIP, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetR8()
{
    return (unsigned long long)Get(R8);
}

SCRIPT_EXPORT bool Script::Register::SetR8(unsigned long long value)
{
    return Set(R8, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetR8D()
{
    return (unsigned int)Get(R8D);
}

SCRIPT_EXPORT bool Script::Register::SetR8D(unsigned int value)
{
    return Set(R8D, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetR8W()
{
    return (unsigned short)Get(R8W);
}

SCRIPT_EXPORT bool Script::Register::SetR8W(unsigned short value)
{
    return Set(R8W, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetR8B()
{
    return (unsigned char)Get(R8B);
}

SCRIPT_EXPORT bool Script::Register::SetR8B(unsigned char value)
{
    return Set(R8B, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetR9()
{
    return (unsigned long long)Get(R9);
}

SCRIPT_EXPORT bool Script::Register::SetR9(unsigned long long value)
{
    return Set(R9, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetR9D()
{
    return (unsigned int)Get(R9D);
}

SCRIPT_EXPORT bool Script::Register::SetR9D(unsigned int value)
{
    return Set(R9D, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetR9W()
{
    return (unsigned short)Get(R9W);
}

SCRIPT_EXPORT bool Script::Register::SetR9W(unsigned short value)
{
    return Set(R9W, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetR9B()
{
    return (unsigned char)Get(R9B);
}

SCRIPT_EXPORT bool Script::Register::SetR9B(unsigned char value)
{
    return Set(R9B, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetR10()
{
    return (unsigned long long)Get(R10);
}

SCRIPT_EXPORT bool Script::Register::SetR10(unsigned long long value)
{
    return Set(R10, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetR10D()
{
    return (unsigned int)Get(R10D);
}

SCRIPT_EXPORT bool Script::Register::SetR10D(unsigned int value)
{
    return Set(R10D, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetR10W()
{
    return (unsigned short)Get(R10W);
}

SCRIPT_EXPORT bool Script::Register::SetR10W(unsigned short value)
{
    return Set(R10W, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetR10B()
{
    return (unsigned char)Get(R10B);
}

SCRIPT_EXPORT bool Script::Register::SetR10B(unsigned char value)
{
    return Set(R10B, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetR11()
{
    return (unsigned long long)Get(R11);
}

SCRIPT_EXPORT bool Script::Register::SetR11(unsigned long long value)
{
    return Set(R11, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetR11D()
{
    return (unsigned int)Get(R11D);
}

SCRIPT_EXPORT bool Script::Register::SetR11D(unsigned int value)
{
    return Set(R11D, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetR11W()
{
    return (unsigned short)Get(R11W);
}

SCRIPT_EXPORT bool Script::Register::SetR11W(unsigned short value)
{
    return Set(R11W, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetR11B()
{
    return (unsigned char)Get(R11B);
}

SCRIPT_EXPORT bool Script::Register::SetR11B(unsigned char value)
{
    return Set(R11B, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetR12()
{
    return (unsigned long long)Get(R12);
}

SCRIPT_EXPORT bool Script::Register::SetR12(unsigned long long value)
{
    return Set(R12, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetR12D()
{
    return (unsigned int)Get(R12D);
}

SCRIPT_EXPORT bool Script::Register::SetR12D(unsigned int value)
{
    return Set(R12D, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetR12W()
{
    return (unsigned short)Get(R12W);
}

SCRIPT_EXPORT bool Script::Register::SetR12W(unsigned short value)
{
    return Set(R12W, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetR12B()
{
    return (unsigned char)Get(R12B);
}

SCRIPT_EXPORT bool Script::Register::SetR12B(unsigned char value)
{
    return Set(R12B, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetR13()
{
    return (unsigned long long)Get(R13);
}

SCRIPT_EXPORT bool Script::Register::SetR13(unsigned long long value)
{
    return Set(R13, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetR13D()
{
    return (unsigned int)Get(R13D);
}

SCRIPT_EXPORT bool Script::Register::SetR13D(unsigned int value)
{
    return Set(R13D, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetR13W()
{
    return (unsigned short)Get(R13W);
}

SCRIPT_EXPORT bool Script::Register::SetR13W(unsigned short value)
{
    return Set(R13W, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetR13B()
{
    return (unsigned char)Get(R13B);
}

SCRIPT_EXPORT bool Script::Register::SetR13B(unsigned char value)
{
    return Set(R13B, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetR14()
{
    return (unsigned long long)Get(R14);
}

SCRIPT_EXPORT bool Script::Register::SetR14(unsigned long long value)
{
    return Set(R14, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetR14D()
{
    return (unsigned int)Get(R14D);
}

SCRIPT_EXPORT bool Script::Register::SetR14D(unsigned int value)
{
    return Set(R14D, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetR14W()
{
    return (unsigned short)Get(R14W);
}

SCRIPT_EXPORT bool Script::Register::SetR14W(unsigned short value)
{
    return Set(R14W, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetR14B()
{
    return (unsigned char)Get(R14B);
}

SCRIPT_EXPORT bool Script::Register::SetR14B(unsigned char value)
{
    return Set(R14B, value);
}

SCRIPT_EXPORT unsigned long long Script::Register::GetR15()
{
    return (unsigned long long)Get(R15);
}

SCRIPT_EXPORT bool Script::Register::SetR15(unsigned long long value)
{
    return Set(R15, value);
}

SCRIPT_EXPORT unsigned int Script::Register::GetR15D()
{
    return (unsigned int)Get(R15D);
}

SCRIPT_EXPORT bool Script::Register::SetR15D(unsigned int value)
{
    return Set(R15D, value);
}

SCRIPT_EXPORT unsigned short Script::Register::GetR15W()
{
    return (unsigned short)Get(R15W);
}

SCRIPT_EXPORT bool Script::Register::SetR15W(unsigned short value)
{
    return Set(R15W, value);
}

SCRIPT_EXPORT unsigned char Script::Register::GetR15B()
{
    return (unsigned char)Get(R15B);
}

SCRIPT_EXPORT bool Script::Register::SetR15B(unsigned char value)
{
    return Set(R15B, value);
}
#endif //_WIN64

SCRIPT_EXPORT duint Script::Register::GetCAX()
{
    return Get(CAX);
}

SCRIPT_EXPORT bool Script::Register::SetCAX(duint value)
{
    return Set(CAX, value);
}

SCRIPT_EXPORT duint Script::Register::GetCBX()
{
    return Get(CBX);
}

SCRIPT_EXPORT bool Script::Register::SetCBX(duint value)
{
    return Set(CBX, value);
}

SCRIPT_EXPORT duint Script::Register::GetCCX()
{
    return Get(CCX);
}

SCRIPT_EXPORT bool Script::Register::SetCCX(duint value)
{
    return Set(CCX, value);
}

SCRIPT_EXPORT duint Script::Register::GetCDX()
{
    return Get(CDX);
}

SCRIPT_EXPORT bool Script::Register::SetCDX(duint value)
{
    return Set(CDX, value);
}

SCRIPT_EXPORT duint Script::Register::GetCDI()
{
    return Get(CDI);
}

SCRIPT_EXPORT bool Script::Register::SetCDI(duint value)
{
    return Set(CDI, value);
}

SCRIPT_EXPORT duint Script::Register::GetCSI()
{
    return Get(CSI);
}

SCRIPT_EXPORT bool Script::Register::SetCSI(duint value)
{
    return Set(CSI, value);
}

SCRIPT_EXPORT duint Script::Register::GetCBP()
{
    return Get(CBP);
}

SCRIPT_EXPORT bool Script::Register::SetCBP(duint value)
{
    return Set(CBP, value);
}

SCRIPT_EXPORT duint Script::Register::GetCSP()
{
    return Get(CSP);
}

SCRIPT_EXPORT bool Script::Register::SetCSP(duint value)
{
    return Set(CSP, value);
}

SCRIPT_EXPORT duint Script::Register::GetCIP()
{
    return Get(CIP);
}

SCRIPT_EXPORT bool Script::Register::SetCIP(duint value)
{
    return Set(CIP, value);
}

SCRIPT_EXPORT duint Script::Register::GetCFLAGS()
{
    return Get(CFLAGS);
}

SCRIPT_EXPORT bool Script::Register::SetCFLAGS(duint value)
{
    return Set(CFLAGS, value);
}