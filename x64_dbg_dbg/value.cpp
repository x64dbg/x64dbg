#include "value.h"
#include "variable.h"
#include "debugger.h"
#include "console.h"
#include "math.h"
#include "memory.h"
#include <psapi.h>

static bool dosignedcalc=false;

bool valuesignedcalc()
{
    return dosignedcalc;
}

void valuesetsignedcalc(bool a)
{
    dosignedcalc=a;
}

static bool isflag(const char* string)
{
    if(scmp(string, "cf"))
        return true;
    if(scmp(string, "pf"))
        return true;
    if(scmp(string, "af"))
        return true;
    if(scmp(string, "zf"))
        return true;
    if(scmp(string, "sf"))
        return true;
    if(scmp(string, "tf"))
        return true;
    if(scmp(string, "if"))
        return true;
    if(scmp(string, "df"))
        return true;
    if(scmp(string, "of"))
        return true;
    if(scmp(string, "rf"))
        return true;
    if(scmp(string, "vm"))
        return true;
    if(scmp(string, "ac"))
        return true;
    if(scmp(string, "vif"))
        return true;
    if(scmp(string, "vip"))
        return true;
    if(scmp(string, "id"))
        return true;
    return false;
}

static bool isregister(const char* string)
{
    if(scmp(string, "eax"))
        return true;
    if(scmp(string, "ebx"))
        return true;
    if(scmp(string, "ecx"))
        return true;
    if(scmp(string, "edx"))
        return true;
    if(scmp(string, "edi"))
        return true;
    if(scmp(string, "esi"))
        return true;
    if(scmp(string, "ebp"))
        return true;
    if(scmp(string, "esp"))
        return true;
    if(scmp(string, "eip"))
        return true;
    if(scmp(string, "eflags"))
        return true;

    if(scmp(string, "ax"))
        return true;
    if(scmp(string, "bx"))
        return true;
    if(scmp(string, "cx"))
        return true;
    if(scmp(string, "dx"))
        return true;
    if(scmp(string, "si"))
        return true;
    if(scmp(string, "di"))
        return true;
    if(scmp(string, "bp"))
        return true;
    if(scmp(string, "sp"))
        return true;
    if(scmp(string, "ip"))
        return true;

    if(scmp(string, "ah"))
        return true;
    if(scmp(string, "al"))
        return true;
    if(scmp(string, "bh"))
        return true;
    if(scmp(string, "bl"))
        return true;
    if(scmp(string, "ch"))
        return true;
    if(scmp(string, "cl"))
        return true;
    if(scmp(string, "dh"))
        return true;
    if(scmp(string, "dl"))
        return true;
    if(scmp(string, "sih"))
        return true;
    if(scmp(string, "sil"))
        return true;
    if(scmp(string, "dih"))
        return true;
    if(scmp(string, "dil"))
        return true;
    if(scmp(string, "bph"))
        return true;
    if(scmp(string, "bpl"))
        return true;
    if(scmp(string, "sph"))
        return true;
    if(scmp(string, "spl"))
        return true;
    if(scmp(string, "iph"))
        return true;
    if(scmp(string, "ipl"))
        return true;

    if(scmp(string, "dr0"))
        return true;
    if(scmp(string, "dr1"))
        return true;
    if(scmp(string, "dr2"))
        return true;
    if(scmp(string, "dr3"))
        return true;
    if(scmp(string, "dr6") or scmp(string, "dr4"))
        return true;
    if(scmp(string, "dr7") or scmp(string, "dr5"))
        return true;

    if(scmp(string, "cip"))
        return true;
    if(scmp(string, "csp"))
        return true;
    if(scmp(string, "cflags"))
        return true;

    if(scmp(string, "gs"))
        return true;
    if(scmp(string, "fs"))
        return true;
    if(scmp(string, "es"))
        return true;
    if(scmp(string, "ds"))
        return true;
    if(scmp(string, "cs"))
        return true;
    if(scmp(string, "ss"))
        return true;

#ifndef _WIN64
    return false;
#endif // _WIN64
    if(scmp(string, "rax"))
        return true;
    if(scmp(string, "rbx"))
        return true;
    if(scmp(string, "rcx"))
        return true;
    if(scmp(string, "rdx"))
        return true;
    if(scmp(string, "rdi"))
        return true;
    if(scmp(string, "rsi"))
        return true;
    if(scmp(string, "rbp"))
        return true;
    if(scmp(string, "rsp"))
        return true;
    if(scmp(string, "rip"))
        return true;
    if(scmp(string, "rflags"))
        return true;
    if(scmp(string, "r8"))
        return true;
    if(scmp(string, "r9"))
        return true;
    if(scmp(string, "r10"))
        return true;
    if(scmp(string, "r11"))
        return true;
    if(scmp(string, "r12"))
        return true;
    if(scmp(string, "r13"))
        return true;
    if(scmp(string, "r14"))
        return true;
    if(scmp(string, "r15"))
        return true;
    if(scmp(string, "r8d"))
        return true;
    if(scmp(string, "r9d"))
        return true;
    if(scmp(string, "r10d"))
        return true;
    if(scmp(string, "r11d"))
        return true;
    if(scmp(string, "r12d"))
        return true;
    if(scmp(string, "r13d"))
        return true;
    if(scmp(string, "r14d"))
        return true;
    if(scmp(string, "r15d"))
        return true;
    if(scmp(string, "r8w"))
        return true;
    if(scmp(string, "r9w"))
        return true;
    if(scmp(string, "r10w"))
        return true;
    if(scmp(string, "r11w"))
        return true;
    if(scmp(string, "r12w"))
        return true;
    if(scmp(string, "r13w"))
        return true;
    if(scmp(string, "r14w"))
        return true;
    if(scmp(string, "r15w"))
        return true;
    if(scmp(string, "r8b"))
        return true;
    if(scmp(string, "r9b"))
        return true;
    if(scmp(string, "r10b"))
        return true;
    if(scmp(string, "r11b"))
        return true;
    if(scmp(string, "r12b"))
        return true;
    if(scmp(string, "r13b"))
        return true;
    if(scmp(string, "r14b"))
        return true;
    if(scmp(string, "r15b"))
        return true;
    return false;
}

bool valflagfromstring(unsigned int eflags, const char* string)
{
    if(scmp(string, "cf"))
        return (bool)((int)(eflags&0x1)!=0);
    if(scmp(string, "pf"))
        return (bool)((int)(eflags&0x4)!=0);
    if(scmp(string, "af"))
        return (bool)((int)(eflags&0x10)!=0);
    if(scmp(string, "zf"))
        return (bool)((int)(eflags&0x40)!=0);
    if(scmp(string, "sf"))
        return (bool)((int)(eflags&0x80)!=0);
    if(scmp(string, "tf"))
        return (bool)((int)(eflags&0x100)!=0);
    if(scmp(string, "if"))
        return (bool)((int)(eflags&0x200)!=0);
    if(scmp(string, "df"))
        return (bool)((int)(eflags&0x400)!=0);
    if(scmp(string, "of"))
        return (bool)((int)(eflags&0x800)!=0);
    if(scmp(string, "rf"))
        return (bool)((int)(eflags&0x10000)!=0);
    if(scmp(string, "vm"))
        return (bool)((int)(eflags&0x20000)!=0);
    if(scmp(string, "ac"))
        return (bool)((int)(eflags&0x40000)!=0);
    if(scmp(string, "vif"))
        return (bool)((int)(eflags&0x80000)!=0);
    if(scmp(string, "vip"))
        return (bool)((int)(eflags&0x100000)!=0);
    if(scmp(string, "id"))
        return (bool)((int)(eflags&0x200000)!=0);
    return false;
}

static bool setflag(const char* string, bool set)
{
    uint eflags=GetContextData(UE_CFLAGS);
    uint xorval=0;
    uint flag=0;
    if(scmp(string, "cf"))
        flag=0x1;
    else if(scmp(string, "pf"))
        flag=0x4;
    else if(scmp(string, "af"))
        flag=0x10;
    else if(scmp(string, "zf"))
        flag=0x40;
    else if(scmp(string, "sf"))
        flag=0x80;
    else if(scmp(string, "tf"))
        flag=0x100;
    else if(scmp(string, "if"))
        flag=0x200;
    else if(scmp(string, "df"))
        flag=0x400;
    else if(scmp(string, "of"))
        flag=0x800;
    else if(scmp(string, "rf"))
        flag=0x10000;
    else if(scmp(string, "vm"))
        flag=0x20000;
    else if(scmp(string, "ac"))
        flag=0x40000;
    else if(scmp(string, "vif"))
        flag=0x80000;
    else if(scmp(string, "vip"))
        flag=0x100000;
    else if(scmp(string, "id"))
        flag=0x200000;
    if(eflags&flag and !set)
        xorval=flag;
    else if(set)
        xorval=flag;
    return SetContextData(UE_CFLAGS, eflags^xorval);
}

static uint getregister(int* size, const char* string)
{
    if(size)
        *size=4;
    if(scmp(string, "eax"))
    {
        return GetContextData(UE_EAX);
    }
    if(scmp(string, "ebx"))
    {
        return GetContextData(UE_EBX);
    }
    if(scmp(string, "ecx"))
    {
        return GetContextData(UE_ECX);
    }
    if(scmp(string, "edx"))
    {
        return GetContextData(UE_EDX);
    }
    if(scmp(string, "edi"))
    {
        return GetContextData(UE_EDI);
    }
    if(scmp(string, "esi"))
    {
        return GetContextData(UE_ESI);
    }
    if(scmp(string, "ebp"))
    {
        return GetContextData(UE_EBP);
    }
    if(scmp(string, "esp"))
    {
        return GetContextData(UE_ESP);
    }
    if(scmp(string, "eip"))
    {
        return GetContextData(UE_EIP);
    }
    if(scmp(string, "eflags"))
    {
        return GetContextData(UE_EFLAGS);
    }

    if(scmp(string, "gs"))
    {
        return GetContextData(UE_SEG_GS);
    }
    if(scmp(string, "fs"))
    {
        return GetContextData(UE_SEG_FS);
    }
    if(scmp(string, "es"))
    {
        return GetContextData(UE_SEG_ES);
    }
    if(scmp(string, "ds"))
    {
        return GetContextData(UE_SEG_DS);
    }
    if(scmp(string, "cs"))
    {
        return GetContextData(UE_SEG_CS);
    }
    if(scmp(string, "ss"))
    {
        return GetContextData(UE_SEG_SS);
    }

    if(size)
        *size=2;
    if(scmp(string, "ax"))
    {
        uint val=GetContextData(UE_EAX);
        return val&0xFFFF;
    }
    if(scmp(string, "bx"))
    {
        uint val=GetContextData(UE_EBX);
        return val&0xFFFF;
    }
    if(scmp(string, "cx"))
    {
        uint val=GetContextData(UE_ECX);
        return val&0xFFFF;
    }
    if(scmp(string, "dx"))
    {
        uint val=GetContextData(UE_EDX);
        return val&0xFFFF;
    }
    if(scmp(string, "si"))
    {
        uint val=GetContextData(UE_ESI);
        return val&0xFFFF;
    }
    if(scmp(string, "di"))
    {
        uint val=GetContextData(UE_EDI);
        return val&0xFFFF;
    }
    if(scmp(string, "bp"))
    {
        uint val=GetContextData(UE_EBP);
        return val&0xFFFF;
    }
    if(scmp(string, "sp"))
    {
        uint val=GetContextData(UE_ESP);
        return val&0xFFFF;
    }
    if(scmp(string, "ip"))
    {
        uint val=GetContextData(UE_EIP);
        return val&0xFFFF;
    }

    if(size)
        *size=1;
    if(scmp(string, "ah"))
    {
        uint val=GetContextData(UE_EAX);
        return (val>>8)&0xFF;
    }
    if(scmp(string, "al"))
    {
        uint val=GetContextData(UE_EAX);
        return val&0xFF;
    }
    if(scmp(string, "bh"))
    {
        uint val=GetContextData(UE_EBX);
        return (val>>8)&0xFF;
    }
    if(scmp(string, "bl"))
    {
        uint val=GetContextData(UE_EBX);
        return val&0xFF;
    }
    if(scmp(string, "ch"))
    {
        uint val=GetContextData(UE_ECX);
        return (val>>8)&0xFF;
    }
    if(scmp(string, "cl"))
    {
        uint val=GetContextData(UE_ECX);
        return val&0xFF;
    }
    if(scmp(string, "dh"))
    {
        uint val=GetContextData(UE_EDX);
        return (val>>8)&0xFF;
    }
    if(scmp(string, "dl"))
    {
        uint val=GetContextData(UE_EDX);
        return val&0xFF;
    }
    if(scmp(string, "sih"))
    {
        uint val=GetContextData(UE_ESI);
        return (val>>8)&0xFF;
    }
    if(scmp(string, "sil"))
    {
        uint val=GetContextData(UE_ESI);
        return val&0xFF;
    }
    if(scmp(string, "dih"))
    {
        uint val=GetContextData(UE_EDI);
        return (val>>8)&0xFF;
    }
    if(scmp(string, "dil"))
    {
        uint val=GetContextData(UE_EDI);
        return val&0xFF;
    }
    if(scmp(string, "bph"))
    {
        uint val=GetContextData(UE_EBP);
        return (val>>8)&0xFF;
    }
    if(scmp(string, "bpl"))
    {
        uint val=GetContextData(UE_EBP);
        return val&0xFF;
    }
    if(scmp(string, "sph"))
    {
        uint val=GetContextData(UE_ESP);
        return (val>>8)&0xFF;
    }
    if(scmp(string, "spl"))
    {
        uint val=GetContextData(UE_ESP);
        return val&0xFF;
    }
    if(scmp(string, "iph"))
    {
        uint val=GetContextData(UE_EIP);
        return (val>>8)&0xFF;
    }
    if(scmp(string, "ipl"))
    {
        uint val=GetContextData(UE_EIP);
        return val&0xFF;
    }

    if(size)
        *size=sizeof(uint);
    if(scmp(string, "dr0"))
    {
        return GetContextData(UE_DR0);
    }
    if(scmp(string, "dr1"))
    {
        return GetContextData(UE_DR1);
    }
    if(scmp(string, "dr2"))
    {
        return GetContextData(UE_DR2);
    }
    if(scmp(string, "dr3"))
    {
        return GetContextData(UE_DR3);
    }
    if(scmp(string, "dr6") or scmp(string, "dr4"))
    {
        return GetContextData(UE_DR6);
    }
    if(scmp(string, "dr7") or scmp(string, "dr5"))
    {
        return GetContextData(UE_DR7);
    }

    if(scmp(string, "cip"))
    {
        return GetContextData(UE_CIP);
    }
    if(scmp(string, "csp"))
    {
        return GetContextData(UE_CSP);
    }
    if(scmp(string, "cflags"))
    {
        return GetContextData(UE_CFLAGS);
    }

#ifdef _WIN64
    if(size)
        *size=8;
    if(scmp(string, "rax"))
    {
        return GetContextData(UE_RAX);
    }
    if(scmp(string, "rbx"))
    {
        return GetContextData(UE_RBX);
    }
    if(scmp(string, "rcx"))
    {
        return GetContextData(UE_RCX);
    }
    if(scmp(string, "rdx"))
    {
        return GetContextData(UE_RDX);
    }
    if(scmp(string, "rdi"))
    {
        return GetContextData(UE_RDI);
    }
    if(scmp(string, "rsi"))
    {
        return GetContextData(UE_RSI);
    }
    if(scmp(string, "rbp"))
    {
        return GetContextData(UE_RBP);
    }
    if(scmp(string, "rsp"))
    {
        return GetContextData(UE_RSP);
    }
    if(scmp(string, "rip"))
    {
        return GetContextData(UE_RIP);
    }
    if(scmp(string, "rflags"))
    {
        return GetContextData(UE_RFLAGS);
    }
    if(scmp(string, "r8"))
    {
        return GetContextData(UE_R8);
    }
    if(scmp(string, "r9"))
    {
        return GetContextData(UE_R9);
    }
    if(scmp(string, "r10"))
    {
        return GetContextData(UE_R10);
    }
    if(scmp(string, "r11"))
    {
        return GetContextData(UE_R11);
    }
    if(scmp(string, "r12"))
    {
        return GetContextData(UE_R12);
    }
    if(scmp(string, "r13"))
    {
        return GetContextData(UE_R13);
    }
    if(scmp(string, "r14"))
    {
        return GetContextData(UE_R14);
    }
    if(scmp(string, "r15"))
    {
        return GetContextData(UE_R15);
    }

    if(size)
        *size=4;
    if(scmp(string, "r8d"))
    {
        return GetContextData(UE_R8)&0xFFFFFFFF;
    }
    if(scmp(string, "r9d"))
    {
        return GetContextData(UE_R9)&0xFFFFFFFF;
    }
    if(scmp(string, "r10d"))
    {
        return GetContextData(UE_R10)&0xFFFFFFFF;
    }
    if(scmp(string, "r11d"))
    {
        return GetContextData(UE_R11)&0xFFFFFFFF;
    }
    if(scmp(string, "r12d"))
    {
        return GetContextData(UE_R12)&0xFFFFFFFF;
    }
    if(scmp(string, "r13d"))
    {
        return GetContextData(UE_R13)&0xFFFFFFFF;
    }
    if(scmp(string, "r14d"))
    {
        return GetContextData(UE_R14)&0xFFFFFFFF;
    }
    if(scmp(string, "r15d"))
    {
        return GetContextData(UE_R15)&0xFFFFFFFF;
    }

    if(size)
        *size=2;
    if(scmp(string, "r8w"))
    {
        return GetContextData(UE_R8)&0xFFFF;
    }
    if(scmp(string, "r9w"))
    {
        return GetContextData(UE_R9)&0xFFFF;
    }
    if(scmp(string, "r10w"))
    {
        return GetContextData(UE_R10)&0xFFFF;
    }
    if(scmp(string, "r11w"))
    {
        return GetContextData(UE_R11)&0xFFFF;
    }
    if(scmp(string, "r12w"))
    {
        return GetContextData(UE_R12)&0xFFFF;
    }
    if(scmp(string, "r13w"))
    {
        return GetContextData(UE_R13)&0xFFFF;
    }
    if(scmp(string, "r14w"))
    {
        return GetContextData(UE_R14)&0xFFFF;
    }
    if(scmp(string, "r15w"))
    {
        return GetContextData(UE_R15)&0xFFFF;
    }

    if(size)
        *size=1;
    if(scmp(string, "r8b"))
    {
        return GetContextData(UE_R8)&0xFF;
    }
    if(scmp(string, "r9b"))
    {
        return GetContextData(UE_R9)&0xFF;
    }
    if(scmp(string, "r10b"))
    {
        return GetContextData(UE_R10)&0xFF;
    }
    if(scmp(string, "r11b"))
    {
        return GetContextData(UE_R11)&0xFF;
    }
    if(scmp(string, "r12b"))
    {
        return GetContextData(UE_R12)&0xFF;
    }
    if(scmp(string, "r13b"))
    {
        return GetContextData(UE_R13)&0xFF;
    }
    if(scmp(string, "r14b"))
    {
        return GetContextData(UE_R14)&0xFF;
    }
    if(scmp(string, "r15b"))
    {
        return GetContextData(UE_R15)&0xFF;
    }
#endif //_WIN64

    if(size)
        *size=0;
    return 0;
}

static bool setregister(const char* string, uint value)
{
    if(scmp(string, "eax"))
        return SetContextData(UE_EAX, value&0xFFFFFFFF);
    if(scmp(string, "ebx"))
        return SetContextData(UE_EBX, value&0xFFFFFFFF);
    if(scmp(string, "ecx"))
        return SetContextData(UE_ECX, value&0xFFFFFFFF);
    if(scmp(string, "edx"))
        return SetContextData(UE_EDX, value&0xFFFFFFFF);
    if(scmp(string, "edi"))
        return SetContextData(UE_EDI, value&0xFFFFFFFF);
    if(scmp(string, "esi"))
        return SetContextData(UE_ESI, value&0xFFFFFFFF);
    if(scmp(string, "ebp"))
        return SetContextData(UE_EBP, value&0xFFFFFFFF);
    if(scmp(string, "esp"))
        return SetContextData(UE_ESP, value&0xFFFFFFFF);
    if(scmp(string, "eip"))
        return SetContextData(UE_EIP, value&0xFFFFFFFF);
    if(scmp(string, "eflags"))
        return SetContextData(UE_EFLAGS, value&0xFFFFFFFF);

    if(scmp(string, "gs"))
        return SetContextData(UE_SEG_GS, value&0xFFFF);
    if(scmp(string, "fs"))
        return SetContextData(UE_SEG_FS, value&0xFFFF);
    if(scmp(string, "es"))
        return SetContextData(UE_SEG_ES, value&0xFFFF);
    if(scmp(string, "ds"))
        return SetContextData(UE_SEG_DS, value&0xFFFF);
    if(scmp(string, "cs"))
        return SetContextData(UE_SEG_CS, value&0xFFFF);
    if(scmp(string, "ss"))
        return SetContextData(UE_SEG_SS, value&0xFFFF);

    if(scmp(string, "ax"))
        return SetContextData(UE_EAX, (value&0xFFFF)|(GetContextData(UE_EAX)&0xFFFF0000));
    if(scmp(string, "bx"))
        return SetContextData(UE_EBX, (value&0xFFFF)|(GetContextData(UE_EBX)&0xFFFF0000));
    if(scmp(string, "cx"))
        return SetContextData(UE_ECX, (value&0xFFFF)|(GetContextData(UE_ECX)&0xFFFF0000));
    if(scmp(string, "dx"))
        return SetContextData(UE_EDX, (value&0xFFFF)|(GetContextData(UE_EDX)&0xFFFF0000));
    if(scmp(string, "si"))
        return SetContextData(UE_ESI, (value&0xFFFF)|(GetContextData(UE_ESI)&0xFFFF0000));
    if(scmp(string, "di"))
        return SetContextData(UE_EDI, (value&0xFFFF)|(GetContextData(UE_EDI)&0xFFFF0000));
    if(scmp(string, "bp"))
        return SetContextData(UE_EBP, (value&0xFFFF)|(GetContextData(UE_EBP)&0xFFFF0000));
    if(scmp(string, "sp"))
        return SetContextData(UE_ESP, (value&0xFFFF)|(GetContextData(UE_ESP)&0xFFFF0000));
    if(scmp(string, "ip"))
        return SetContextData(UE_EIP, (value&0xFFFF)|(GetContextData(UE_EIP)&0xFFFF0000));

    if(scmp(string, "ah"))
        return SetContextData(UE_EAX, ((value&0xFF)<<8)|(GetContextData(UE_EAX)&0xFFFF00FF));
    if(scmp(string, "al"))
        return SetContextData(UE_EAX, (value&0xFF)|(GetContextData(UE_EAX)&0xFFFFFF00));
    if(scmp(string, "bh"))
        return SetContextData(UE_EBX, ((value&0xFF)<<8)|(GetContextData(UE_EBX)&0xFFFF00FF));
    if(scmp(string, "bl"))
        return SetContextData(UE_EBX, (value&0xFF)|(GetContextData(UE_EBX)&0xFFFFFF00));
    if(scmp(string, "ch"))
        return SetContextData(UE_ECX, ((value&0xFF)<<8)|(GetContextData(UE_ECX)&0xFFFF00FF));
    if(scmp(string, "cl"))
        return SetContextData(UE_ECX, (value&0xFF)|(GetContextData(UE_ECX)&0xFFFFFF00));
    if(scmp(string, "dh"))
        return SetContextData(UE_EDX, ((value&0xFF)<<8)|(GetContextData(UE_EDX)&0xFFFF00FF));
    if(scmp(string, "dl"))
        return SetContextData(UE_EDX, (value&0xFF)|(GetContextData(UE_EDX)&0xFFFFFF00));
    if(scmp(string, "sih"))
        return SetContextData(UE_ESI, ((value&0xFF)<<8)|(GetContextData(UE_ESI)&0xFFFF00FF));
    if(scmp(string, "sil"))
        return SetContextData(UE_ESI, (value&0xFF)|(GetContextData(UE_ESI)&0xFFFFFF00));
    if(scmp(string, "dih"))
        return SetContextData(UE_EDI, ((value&0xFF)<<8)|(GetContextData(UE_EDI)&0xFFFF00FF));
    if(scmp(string, "dil"))
        return SetContextData(UE_EDI, (value&0xFF)|(GetContextData(UE_EDI)&0xFFFFFF00));
    if(scmp(string, "bph"))
        return SetContextData(UE_EBP, ((value&0xFF)<<8)|(GetContextData(UE_EBP)&0xFFFF00FF));
    if(scmp(string, "bpl"))
        return SetContextData(UE_EBP, (value&0xFF)|(GetContextData(UE_EBP)&0xFFFFFF00));
    if(scmp(string, "sph"))
        return SetContextData(UE_ESP, ((value&0xFF)<<8)|(GetContextData(UE_ESP)&0xFFFF00FF));
    if(scmp(string, "spl"))
        return SetContextData(UE_ESP, (value&0xFF)|(GetContextData(UE_ESP)&0xFFFFFF00));
    if(scmp(string, "iph"))
        return SetContextData(UE_EIP, ((value&0xFF)<<8)|(GetContextData(UE_EIP)&0xFFFF00FF));
    if(scmp(string, "ipl"))
        return SetContextData(UE_EIP, (value&0xFF)|(GetContextData(UE_EIP)&0xFFFFFF00));

    if(scmp(string, "dr0"))
        return SetContextData(UE_DR0, value);
    if(scmp(string, "dr1"))
        return SetContextData(UE_DR1, value);
    if(scmp(string, "dr2"))
        return SetContextData(UE_DR2, value);
    if(scmp(string, "dr3"))
        return SetContextData(UE_DR3, value);
    if(scmp(string, "dr6") or scmp(string, "dr4"))
        return SetContextData(UE_DR6, value);
    if(scmp(string, "dr7") or scmp(string, "dr5"))
        return SetContextData(UE_DR7, value);

    if(scmp(string, "cip"))
        return SetContextData(UE_CIP, value);
    if(scmp(string, "csp"))
        return SetContextData(UE_CSP, value);
    if(scmp(string, "cflags"))
        return SetContextData(UE_CFLAGS, value);

#ifdef _WIN64
    if(scmp(string, "rax"))
        return SetContextData(UE_RAX, value);
    if(scmp(string, "rbx"))
        return SetContextData(UE_RBX, value);
    if(scmp(string, "rcx"))
        return SetContextData(UE_RCX, value);
    if(scmp(string, "rdx"))
        return SetContextData(UE_RDX, value);
    if(scmp(string, "rdi"))
        return SetContextData(UE_RDI, value);
    if(scmp(string, "rsi"))
        return SetContextData(UE_RSI, value);
    if(scmp(string, "rbp"))
        return SetContextData(UE_RBP, value);
    if(scmp(string, "rsp"))
        return SetContextData(UE_RSP, value);
    if(scmp(string, "rip"))
        return SetContextData(UE_RIP, value);
    if(scmp(string, "rflags"))
        return SetContextData(UE_RFLAGS, value);
    if(scmp(string, "r8"))
        return SetContextData(UE_R8, value);
    if(scmp(string, "r9"))
        return SetContextData(UE_R9, value);
    if(scmp(string, "r10"))
        return SetContextData(UE_R10, value);
    if(scmp(string, "r11"))
        return SetContextData(UE_R11, value);
    if(scmp(string, "r12"))
        return SetContextData(UE_R12, value);
    if(scmp(string, "r13"))
        return SetContextData(UE_R13, value);
    if(scmp(string, "r14"))
        return SetContextData(UE_R14, value);
    if(scmp(string, "r15"))
        return SetContextData(UE_R15, value);

    if(scmp(string, "r8d"))
        return SetContextData(UE_R8, (value&0xFFFFFFFF)|(GetContextData(UE_R8)&0xFFFFFFFF00000000));
    if(scmp(string, "r9d"))
        return SetContextData(UE_R9, (value&0xFFFFFFFF)|(GetContextData(UE_R9)&0xFFFFFFFF00000000));
    if(scmp(string, "r10d"))
        return SetContextData(UE_R10, (value&0xFFFFFFFF)|(GetContextData(UE_R10)&0xFFFFFFFF00000000));
    if(scmp(string, "r11d"))
        return SetContextData(UE_R11, (value&0xFFFFFFFF)|(GetContextData(UE_R11)&0xFFFFFFFF00000000));
    if(scmp(string, "r12d"))
        return SetContextData(UE_R12, (value&0xFFFFFFFF)|(GetContextData(UE_R12)&0xFFFFFFFF00000000));
    if(scmp(string, "r13d"))
        return SetContextData(UE_R13, (value&0xFFFFFFFF)|(GetContextData(UE_R13)&0xFFFFFFFF00000000));
    if(scmp(string, "r14d"))
        return SetContextData(UE_R14, (value&0xFFFFFFFF)|(GetContextData(UE_R14)&0xFFFFFFFF00000000));
    if(scmp(string, "r15d"))
        return SetContextData(UE_R15, (value&0xFFFFFFFF)|(GetContextData(UE_R15)&0xFFFFFFFF00000000));

    if(scmp(string, "r8w"))
        return SetContextData(UE_R8, (value&0xFFFF)|(GetContextData(UE_R8)&0xFFFFFFFFFFFF0000));
    if(scmp(string, "r9w"))
        return SetContextData(UE_R9, (value&0xFFFF)|(GetContextData(UE_R9)&0xFFFFFFFFFFFF0000));
    if(scmp(string, "r10w"))
        return SetContextData(UE_R10, (value&0xFFFF)|(GetContextData(UE_R10)&0xFFFFFFFFFFFF0000));
    if(scmp(string, "r11w"))
        return SetContextData(UE_R11, (value&0xFFFF)|(GetContextData(UE_R11)&0xFFFFFFFFFFFF0000));
    if(scmp(string, "r12w"))
        return SetContextData(UE_R12, (value&0xFFFF)|(GetContextData(UE_R12)&0xFFFFFFFFFFFF0000));
    if(scmp(string, "r13w"))
        return SetContextData(UE_R13, (value&0xFFFF)|(GetContextData(UE_R13)&0xFFFFFFFFFFFF0000));
    if(scmp(string, "r14w"))
        return SetContextData(UE_R14, (value&0xFFFF)|(GetContextData(UE_R14)&0xFFFFFFFFFFFF0000));
    if(scmp(string, "r15w"))
        return SetContextData(UE_R15, (value&0xFFFF)|(GetContextData(UE_R15)&0xFFFFFFFFFFFF0000));
    if(scmp(string, "r8b"))
        return SetContextData(UE_R8, (value&0xFF)|(GetContextData(UE_R8)&0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r9b"))
        return SetContextData(UE_R9, (value&0xFF)|(GetContextData(UE_R9)&0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r10b"))
        return SetContextData(UE_R10, (value&0xFF)|(GetContextData(UE_R10)&0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r11b"))
        return SetContextData(UE_R11, (value&0xFF)|(GetContextData(UE_R11)&0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r12b"))
        return SetContextData(UE_R12, (value&0xFF)|(GetContextData(UE_R12)&0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r13b"))
        return SetContextData(UE_R13, (value&0xFF)|(GetContextData(UE_R13)&0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r14b"))
        return SetContextData(UE_R14, (value&0xFF)|(GetContextData(UE_R14)&0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r15b"))
        return SetContextData(UE_R15, (value&0xFF)|(GetContextData(UE_R15)&0xFFFFFFFFFFFFFF00));
#endif // _WIN64

    return false;
}

bool valapifromstring(const char* name, uint* value, int* value_size, bool printall, bool silent, bool* hexonly)
{
    if(!value or !IsFileBeingDebugged())
        return false;
    DWORD cbNeeded=1;
    HMODULE hMods[256];
    uint addrfound[256];
    int found=0;
    int kernelbase=-1;
    if(EnumProcessModules(fdProcessInfo->hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for(unsigned int i=0; i<(cbNeeded/sizeof(HMODULE)); i++)
        {
            char szModName[MAX_PATH];
            if(!GetModuleFileNameEx(fdProcessInfo->hProcess, hMods[i], szModName, MAX_PATH) and !silent)
                dprintf("could not get filename of module "fhex"\n", hMods[i]);
            char szBaseName[256]="";
            int len=strlen(szModName);
            while(szModName[len]!='\\')
                len--;
            strcpy(szBaseName, szModName+len+1);
            HMODULE mod=LoadLibraryExA(szModName, 0, DONT_RESOLVE_DLL_REFERENCES|LOAD_LIBRARY_AS_DATAFILE);
            if(!mod and !silent)
                dprintf("unable to load library %s\n", szBaseName);
            uint addr=(uint)GetProcAddress(mod, name);
            FreeLibrary(mod);
            if(addr)
            {
                if(!_stricmp(szBaseName, "kernelbase") or !_stricmp(szBaseName, "kernelbase.dll"))
                    kernelbase=found;
                addrfound[found]=ImporterGetRemoteAPIAddressEx(szBaseName, (char*)name);
                found++;
            }
        }
    }
    if(!found)
        return false;
    if(value_size)
        *value_size=sizeof(uint);
    if(hexonly)
        *hexonly=true;
    if(kernelbase!=-1)
    {
        *value=addrfound[kernelbase];
        if(!printall)
            return true;
        for(int i=0; i<found; i++)
            if(i!=kernelbase)
                dprintf(fhex"\n", addrfound[i]);
    }
    else
    {
        *value=*addrfound;
        if(!printall or silent)
            return true;
        for(int i=1; i<found; i++)
            dprintf(fhex"\n", addrfound[i]);
    }
    return true;
}

/*
check whether a string is a valid dec number
*/
static bool isdecnumber(const char* string)
{
    if(*string!='.' or !string[1]) //dec indicator/no number
        return false;
    int decAdd=1;
    if(string[1]=='-') //minus
    {
        if(!string[2]) //no number
            return false;
        decAdd++;
    }
    int len=strlen(string+decAdd);
    for(int i=0; i<len; i++)
        if(!isdigit(string[i+decAdd]))
            return false;
    return true;
}

/*
check whether a string is a valid hex number
*/
static bool ishexnumber(const char* string)
{
    int add=0;
    if(*string=='x') //hex indicator
        add=1;
    if(!string[add]) //only an indicator, no number
        return false;
    int len=strlen(string+add);
    for(int i=0; i<len; i++)
        if(!isxdigit(string[i+add])) //all must be hex digits
            return false;
    return true;
}

bool valfromstring(const char* string, uint* value, int* value_size, bool* isvar, bool silent, bool* hexonly)
{
    if(!value)
        return false;
    else if(*string=='.' and string[1]=='-' and string[2]) //negative decimal number
    {
        uint finalMul=1;
        if(value_size)
            *value_size=0;
        if(isvar)
            *isvar=false;
        int decAdd=1;
        if(string[1]=='-') //negative number
        {
            decAdd++;
            finalMul=~0;
        }
        uint newValue=0;
        sscanf(string+decAdd, "%"fext"u", &newValue);
        *value=newValue*finalMul;
        return true;
    }
    else if(!*string)
    {
        *value=0;
        return true;
    }
    else if(mathcontains(string)) //handle math
    {
        int len=strlen(string);
        char* string_=(char*)emalloc(len+256);
        strcpy(string_, string);
        int add=0;
        while(mathisoperator(string_[add])>2)
            add++;
        if(!mathhandlebrackets(string_+add))
        {
            efree(string_);
            return false;
        }
        bool ret=mathfromstring(string_+add, value, value_size, isvar);
        efree(string_);
        return ret;
    }
    else if(*string=='@') //memory location
    {
        if(!IsFileBeingDebugged())
        {
            if(!silent)
                dputs("not debugging");
            *value=0;
            if(value_size)
                *value_size=0;
            if(isvar)
                *isvar=true;
            return true;
        }
        int read_size=sizeof(uint);
        int add=1;
        if(string[2]==':' and isdigit((string[1]))) //@n: (number of bytes to read)
        {
            add+=2;
            int new_size=string[1]-0x30;
            if(new_size<read_size)
                read_size=new_size;
        }
        if(!valfromstring(string+add, value, 0, 0, false, 0))
            return false;
        uint addr=*value;
        *value=0;
        bool isrunning=dbgisrunning();
        if(!isrunning)
            dbgdisablebpx();
        bool rpm=memread(fdProcessInfo->hProcess, (void*)addr, value, read_size, 0);
        if(!isrunning)
            dbgenablebpx();
        if(!rpm)
        {
            if(!silent)
                dputs("failed to read memory");
            return false;
        }
        if(value_size)
            *value_size=read_size;
        if(isvar)
            *isvar=true;
        return true;
    }
    else if(isregister(string)) //register
    {
        if(!IsFileBeingDebugged())
        {
            if(!silent)
                dputs("not debugging!");
            *value=0;
            if(value_size)
                *value_size=0;
            if(isvar)
                *isvar=true;
            return true;
        }
        *value=getregister(value_size, string);
        if(isvar)
            *isvar=true;
        return true;
    }
    else if(*string=='!' and isflag(string+1)) //flag
    {
        if(!IsFileBeingDebugged())
        {
            if(!silent)
                dputs("not debugging");
            *value=0;
            if(value_size)
                *value_size=0;
            if(isvar)
                *isvar=true;
            return true;
        }
        uint eflags=GetContextData(UE_CFLAGS);
        if(valflagfromstring(eflags, string+1))
            *value=1;
        else
            *value=0;
        if(value_size)
            *value_size=0;
        if(isvar)
            *isvar=true;
        return true;
    }
    else if(isdecnumber(string)) //decimal numbers come 'first'
    {
        if(value_size)
            *value_size=0;
        if(isvar)
            *isvar=false;
        sscanf(string+1, "%"fext"u", value);
        return true;
    }
    else if(ishexnumber(string)) //then hex numbers
    {
        if(value_size)
            *value_size=0;
        if(isvar)
            *isvar=false;
        //hexadecimal value
        int inc=0;
        if(*string=='x')
            inc=1;
        sscanf(string+inc, "%"fext"x", value);
        return true;
    }
    else if(valapifromstring(string, value, value_size, true, silent, hexonly)) //then come APIs
        return true;
    else if(varget(string, value, value_size, 0)) //finally variables
    {
        if(isvar)
            *isvar=true;
        return true;
    }
    return false; //nothing was OK
}

bool valtostring(const char* string, uint* value, bool silent)
{
    if(!*string or !value)
        return false;
    else if(*string=='@') //memory location
    {
        if(!IsFileBeingDebugged())
        {
            if(!silent)
                dputs("not debugging");
            return false;
        }
        int read_size=sizeof(uint);
        int add=1;
        if(string[2]==':' and isdigit((string[1])))
        {
            add+=2;
            int new_size=string[1]-0x30;
            if(new_size<read_size)
                read_size=new_size;
        }
        uint temp;
        if(!valfromstring(string+add, &temp, 0, 0, false, 0))
            return false;
        bool isrunning=dbgisrunning();
        if(!isrunning)
            dbgdisablebpx();
        bool wpm=WriteProcessMemory(fdProcessInfo->hProcess, (void*)temp, value, read_size, 0);
        if(!isrunning)
            dbgenablebpx();
        if(!wpm)
        {
            if(!silent)
                dputs("failed to write memory");
            return false;
        }
        return true;
    }
    else if(isregister(string)) //register
    {
        if(!IsFileBeingDebugged())
        {
            if(!silent)
                dputs("not debugging!");
            return false;
        }
        bool ok=setregister(string, *value);
        if(strstr(string, "ip"))
            DebugUpdateGui(GetContextData(UE_CIP)); //update disassembly + register view
        else
            GuiUpdateRegisterView(); //update register view
        return ok;
    }
    else if(*string=='!' and isflag(string+1)) //flag
    {
        if(!IsFileBeingDebugged())
        {
            if(!silent)
                dputs("not debugging");
            return false;
        }
        bool set=false;
        if(*value)
            set=true;
        setflag(string+1, set);
        GuiUpdateRegisterView(); //update register view
        return true;
    }
    return varset(string, *value, false); //variable
}
