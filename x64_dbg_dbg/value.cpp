#include "value.h"
#include "variable.h"
#include "debugger.h"
#include "console.h"
#include "math.h"
#include "memory.h"
#include "addrinfo.h"
#include "symbolinfo.h"
#include <psapi.h>

static bool dosignedcalc = false;

bool valuesignedcalc()
{
    return dosignedcalc;
}

void valuesetsignedcalc(bool a)
{
    dosignedcalc = a;
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

#define MXCSRFLAG_IE 0x1
#define MXCSRFLAG_DE 0x2
#define MXCSRFLAG_ZE 0x4
#define MXCSRFLAG_OE 0x8
#define MXCSRFLAG_UE 0x10
#define MXCSRFLAG_PE 0x20
#define MXCSRFLAG_DAZ 0x40
#define MXCSRFLAG_IM 0x80
#define MXCSRFLAG_DM 0x100
#define MXCSRFLAG_ZM 0x200
#define MXCSRFLAG_OM 0x400
#define MXCSRFLAG_UM 0x800
#define MXCSRFLAG_PM 0x1000
#define MXCSRFLAG_FZ 0x8000

typedef struct
{
    char* name;
    unsigned int flag;

} FLAG_NAME_VALUE_TABLE_t;

#define MXCSR_NAME_FLAG_TABLE_ENTRY(flag_name) { #flag_name, MXCSRFLAG_##flag_name }

unsigned int getmxcsrflagfromstring(const char* string)
{
    static FLAG_NAME_VALUE_TABLE_t mxcsrnameflagtable[] =
    {
        MXCSR_NAME_FLAG_TABLE_ENTRY(IE),
        MXCSR_NAME_FLAG_TABLE_ENTRY(DE),
        MXCSR_NAME_FLAG_TABLE_ENTRY(ZE),
        MXCSR_NAME_FLAG_TABLE_ENTRY(OE),
        MXCSR_NAME_FLAG_TABLE_ENTRY(UE),
        MXCSR_NAME_FLAG_TABLE_ENTRY(PE),
        MXCSR_NAME_FLAG_TABLE_ENTRY(DAZ),
        MXCSR_NAME_FLAG_TABLE_ENTRY(IM),
        MXCSR_NAME_FLAG_TABLE_ENTRY(DM),
        MXCSR_NAME_FLAG_TABLE_ENTRY(ZM),
        MXCSR_NAME_FLAG_TABLE_ENTRY(OM),
        MXCSR_NAME_FLAG_TABLE_ENTRY(UM),
        MXCSR_NAME_FLAG_TABLE_ENTRY(PM),
        MXCSR_NAME_FLAG_TABLE_ENTRY(FZ)
    };
    int i;

    for(i = 0; i < (sizeof(mxcsrnameflagtable) / sizeof(*mxcsrnameflagtable)); i++)
    {
        if(scmp(string, mxcsrnameflagtable[i].name))
            return mxcsrnameflagtable[i].flag;
    }

    return 0;
}

bool valmxcsrflagfromstring(uint mxcsrflags, const char* string)
{
    unsigned int flag = getmxcsrflagfromstring(string);
    if(flag == 0)
        return false;

    return (bool)((int)(mxcsrflags & flag) != 0);
}

#define x87STATUSWORD_FLAG_I 0x1
#define x87STATUSWORD_FLAG_D 0x2
#define x87STATUSWORD_FLAG_Z 0x4
#define x87STATUSWORD_FLAG_O 0x8
#define x87STATUSWORD_FLAG_U 0x10
#define x87STATUSWORD_FLAG_P 0x20
#define x87STATUSWORD_FLAG_SF 0x40
#define x87STATUSWORD_FLAG_IR 0x80
#define x87STATUSWORD_FLAG_C0 0x100
#define x87STATUSWORD_FLAG_C1 0x200
#define x87STATUSWORD_FLAG_C2 0x400
#define x87STATUSWORD_FLAG_C3 0x4000
#define x87STATUSWORD_FLAG_B 0x8000

#define X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(flag_name) { #flag_name, x87STATUSWORD_FLAG_##flag_name }

unsigned int getx87statuswordflagfromstring(const char* string)
{
    static FLAG_NAME_VALUE_TABLE_t statuswordflagtable[] =
    {
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(I),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(D),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(Z),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(O),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(U),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(P),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(SF),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(IR),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(C0),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(C1),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(C2),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(C3),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(B)
    };
    int i;

    for(i = 0; i < (sizeof(statuswordflagtable) / sizeof(*statuswordflagtable)); i++)
    {
        if(scmp(string, statuswordflagtable[i].name))
            return statuswordflagtable[i].flag;
    }

    return 0;
}

bool valx87statuswordflagfromstring(uint statusword, const char* string)
{
    unsigned int flag = getx87statuswordflagfromstring(string);
    if(flag == 0)
        return false;

    return (bool)((int)(statusword & flag) != 0);
}

#define x87CONTROLWORD_FLAG_IM 0x1
#define x87CONTROLWORD_FLAG_DM 0x2
#define x87CONTROLWORD_FLAG_ZM 0x4
#define x87CONTROLWORD_FLAG_OM 0x8
#define x87CONTROLWORD_FLAG_UM 0x10
#define x87CONTROLWORD_FLAG_PM 0x20
#define x87CONTROLWORD_FLAG_IEM 0x80
#define x87CONTROLWORD_FLAG_IC 0x1000

#define X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(flag_name) { #flag_name, x87CONTROLWORD_FLAG_##flag_name }

unsigned int getx87controlwordflagfromstring(const char* string)
{
    static FLAG_NAME_VALUE_TABLE_t controlwordflagtable[] =
    {
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(IM),
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(DM),
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(ZM),
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(OM),
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(UM),
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(PM),
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(IEM),
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(IC)
    };
    int i;

    for(i = 0; i < (sizeof(controlwordflagtable) / sizeof(*controlwordflagtable)); i++)
    {
        if(scmp(string, controlwordflagtable[i].name))
            return controlwordflagtable[i].flag;
    }

    return 0;
}

bool valx87controlwordflagfromstring(uint controlword, const char* string)
{
    unsigned int flag = getx87controlwordflagfromstring(string);

    if(flag == 0)
        return false;

    return (bool)((int)(controlword & flag) != 0);
}

unsigned short valmxcsrfieldfromstring(uint mxcsrflags, const char* string)
{
    if(scmp(string, "RC"))
        return ((mxcsrflags & 0x6000) >> 13);

    return 0;
}

unsigned short valx87statuswordfieldfromstring(uint statusword, const char* string)
{
    if(scmp(string, "TOP"))
        return ((statusword & 0x3800) >> 11);

    return 0;
}

unsigned short valx87controlwordfieldfromstring(uint controlword, const char* string)
{
    if(scmp(string, "PC"))
        return ((controlword & 0x300) >> 8);
    if(scmp(string, "RC"))
        return ((controlword & 0xC00) >> 10);

    return 0;
}

bool valflagfromstring(uint eflags, const char* string)
{
    if(scmp(string, "cf"))
        return (bool)((int)(eflags & 0x1) != 0);
    if(scmp(string, "pf"))
        return (bool)((int)(eflags & 0x4) != 0);
    if(scmp(string, "af"))
        return (bool)((int)(eflags & 0x10) != 0);
    if(scmp(string, "zf"))
        return (bool)((int)(eflags & 0x40) != 0);
    if(scmp(string, "sf"))
        return (bool)((int)(eflags & 0x80) != 0);
    if(scmp(string, "tf"))
        return (bool)((int)(eflags & 0x100) != 0);
    if(scmp(string, "if"))
        return (bool)((int)(eflags & 0x200) != 0);
    if(scmp(string, "df"))
        return (bool)((int)(eflags & 0x400) != 0);
    if(scmp(string, "of"))
        return (bool)((int)(eflags & 0x800) != 0);
    if(scmp(string, "rf"))
        return (bool)((int)(eflags & 0x10000) != 0);
    if(scmp(string, "vm"))
        return (bool)((int)(eflags & 0x20000) != 0);
    if(scmp(string, "ac"))
        return (bool)((int)(eflags & 0x40000) != 0);
    if(scmp(string, "vif"))
        return (bool)((int)(eflags & 0x80000) != 0);
    if(scmp(string, "vip"))
        return (bool)((int)(eflags & 0x100000) != 0);
    if(scmp(string, "id"))
        return (bool)((int)(eflags & 0x200000) != 0);
    return false;
}

static bool setflag(const char* string, bool set)
{
    uint eflags = GetContextDataEx(hActiveThread, UE_CFLAGS);
    uint xorval = 0;
    uint flag = 0;
    if(scmp(string, "cf"))
        flag = 0x1;
    else if(scmp(string, "pf"))
        flag = 0x4;
    else if(scmp(string, "af"))
        flag = 0x10;
    else if(scmp(string, "zf"))
        flag = 0x40;
    else if(scmp(string, "sf"))
        flag = 0x80;
    else if(scmp(string, "tf"))
        flag = 0x100;
    else if(scmp(string, "if"))
        flag = 0x200;
    else if(scmp(string, "df"))
        flag = 0x400;
    else if(scmp(string, "of"))
        flag = 0x800;
    else if(scmp(string, "rf"))
        flag = 0x10000;
    else if(scmp(string, "vm"))
        flag = 0x20000;
    else if(scmp(string, "ac"))
        flag = 0x40000;
    else if(scmp(string, "vif"))
        flag = 0x80000;
    else if(scmp(string, "vip"))
        flag = 0x100000;
    else if(scmp(string, "id"))
        flag = 0x200000;
    if(eflags & flag and !set)
        xorval = flag;
    else if(set)
        xorval = flag;
    return SetContextDataEx(hActiveThread, UE_CFLAGS, eflags ^ xorval);
}

static uint getregister(int* size, const char* string)
{
    if(size)
        *size = 4;
    if(scmp(string, "eax"))
    {
        return GetContextDataEx(hActiveThread, UE_EAX);
    }
    if(scmp(string, "ebx"))
    {
        return GetContextDataEx(hActiveThread, UE_EBX);
    }
    if(scmp(string, "ecx"))
    {
        return GetContextDataEx(hActiveThread, UE_ECX);
    }
    if(scmp(string, "edx"))
    {
        return GetContextDataEx(hActiveThread, UE_EDX);
    }
    if(scmp(string, "edi"))
    {
        return GetContextDataEx(hActiveThread, UE_EDI);
    }
    if(scmp(string, "esi"))
    {
        return GetContextDataEx(hActiveThread, UE_ESI);
    }
    if(scmp(string, "ebp"))
    {
        return GetContextDataEx(hActiveThread, UE_EBP);
    }
    if(scmp(string, "esp"))
    {
        return GetContextDataEx(hActiveThread, UE_ESP);
    }
    if(scmp(string, "eip"))
    {
        return GetContextDataEx(hActiveThread, UE_EIP);
    }
    if(scmp(string, "eflags"))
    {
        return GetContextDataEx(hActiveThread, UE_EFLAGS);
    }

    if(scmp(string, "gs"))
    {
        return GetContextDataEx(hActiveThread, UE_SEG_GS);
    }
    if(scmp(string, "fs"))
    {
        return GetContextDataEx(hActiveThread, UE_SEG_FS);
    }
    if(scmp(string, "es"))
    {
        return GetContextDataEx(hActiveThread, UE_SEG_ES);
    }
    if(scmp(string, "ds"))
    {
        return GetContextDataEx(hActiveThread, UE_SEG_DS);
    }
    if(scmp(string, "cs"))
    {
        return GetContextDataEx(hActiveThread, UE_SEG_CS);
    }
    if(scmp(string, "ss"))
    {
        return GetContextDataEx(hActiveThread, UE_SEG_SS);
    }

    if(size)
        *size = 2;
    if(scmp(string, "ax"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EAX);
        return val & 0xFFFF;
    }
    if(scmp(string, "bx"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EBX);
        return val & 0xFFFF;
    }
    if(scmp(string, "cx"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_ECX);
        return val & 0xFFFF;
    }
    if(scmp(string, "dx"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EDX);
        return val & 0xFFFF;
    }
    if(scmp(string, "si"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_ESI);
        return val & 0xFFFF;
    }
    if(scmp(string, "di"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EDI);
        return val & 0xFFFF;
    }
    if(scmp(string, "bp"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EBP);
        return val & 0xFFFF;
    }
    if(scmp(string, "sp"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_ESP);
        return val & 0xFFFF;
    }
    if(scmp(string, "ip"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EIP);
        return val & 0xFFFF;
    }

    if(size)
        *size = 1;
    if(scmp(string, "ah"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EAX);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "al"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EAX);
        return val & 0xFF;
    }
    if(scmp(string, "bh"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EBX);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "bl"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EBX);
        return val & 0xFF;
    }
    if(scmp(string, "ch"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_ECX);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "cl"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_ECX);
        return val & 0xFF;
    }
    if(scmp(string, "dh"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EDX);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "dl"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EDX);
        return val & 0xFF;
    }
    if(scmp(string, "sih"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_ESI);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "sil"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_ESI);
        return val & 0xFF;
    }
    if(scmp(string, "dih"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EDI);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "dil"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EDI);
        return val & 0xFF;
    }
    if(scmp(string, "bph"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EBP);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "bpl"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EBP);
        return val & 0xFF;
    }
    if(scmp(string, "sph"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_ESP);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "spl"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_ESP);
        return val & 0xFF;
    }
    if(scmp(string, "iph"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EIP);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "ipl"))
    {
        uint val = GetContextDataEx(hActiveThread, UE_EIP);
        return val & 0xFF;
    }

    if(size)
        *size = sizeof(uint);
    if(scmp(string, "dr0"))
    {
        return GetContextDataEx(hActiveThread, UE_DR0);
    }
    if(scmp(string, "dr1"))
    {
        return GetContextDataEx(hActiveThread, UE_DR1);
    }
    if(scmp(string, "dr2"))
    {
        return GetContextDataEx(hActiveThread, UE_DR2);
    }
    if(scmp(string, "dr3"))
    {
        return GetContextDataEx(hActiveThread, UE_DR3);
    }
    if(scmp(string, "dr6") or scmp(string, "dr4"))
    {
        return GetContextDataEx(hActiveThread, UE_DR6);
    }
    if(scmp(string, "dr7") or scmp(string, "dr5"))
    {
        return GetContextDataEx(hActiveThread, UE_DR7);
    }

    if(scmp(string, "cip"))
    {
        return GetContextDataEx(hActiveThread, UE_CIP);
    }
    if(scmp(string, "csp"))
    {
        return GetContextDataEx(hActiveThread, UE_CSP);
    }
    if(scmp(string, "cflags"))
    {
        return GetContextDataEx(hActiveThread, UE_CFLAGS);
    }

#ifdef _WIN64
    if(size)
        *size = 8;
    if(scmp(string, "rax"))
    {
        return GetContextDataEx(hActiveThread, UE_RAX);
    }
    if(scmp(string, "rbx"))
    {
        return GetContextDataEx(hActiveThread, UE_RBX);
    }
    if(scmp(string, "rcx"))
    {
        return GetContextDataEx(hActiveThread, UE_RCX);
    }
    if(scmp(string, "rdx"))
    {
        return GetContextDataEx(hActiveThread, UE_RDX);
    }
    if(scmp(string, "rdi"))
    {
        return GetContextDataEx(hActiveThread, UE_RDI);
    }
    if(scmp(string, "rsi"))
    {
        return GetContextDataEx(hActiveThread, UE_RSI);
    }
    if(scmp(string, "rbp"))
    {
        return GetContextDataEx(hActiveThread, UE_RBP);
    }
    if(scmp(string, "rsp"))
    {
        return GetContextDataEx(hActiveThread, UE_RSP);
    }
    if(scmp(string, "rip"))
    {
        return GetContextDataEx(hActiveThread, UE_RIP);
    }
    if(scmp(string, "rflags"))
    {
        return GetContextDataEx(hActiveThread, UE_RFLAGS);
    }
    if(scmp(string, "r8"))
    {
        return GetContextDataEx(hActiveThread, UE_R8);
    }
    if(scmp(string, "r9"))
    {
        return GetContextDataEx(hActiveThread, UE_R9);
    }
    if(scmp(string, "r10"))
    {
        return GetContextDataEx(hActiveThread, UE_R10);
    }
    if(scmp(string, "r11"))
    {
        return GetContextDataEx(hActiveThread, UE_R11);
    }
    if(scmp(string, "r12"))
    {
        return GetContextDataEx(hActiveThread, UE_R12);
    }
    if(scmp(string, "r13"))
    {
        return GetContextDataEx(hActiveThread, UE_R13);
    }
    if(scmp(string, "r14"))
    {
        return GetContextDataEx(hActiveThread, UE_R14);
    }
    if(scmp(string, "r15"))
    {
        return GetContextDataEx(hActiveThread, UE_R15);
    }

    if(size)
        *size = 4;
    if(scmp(string, "r8d"))
    {
        return GetContextDataEx(hActiveThread, UE_R8) & 0xFFFFFFFF;
    }
    if(scmp(string, "r9d"))
    {
        return GetContextDataEx(hActiveThread, UE_R9) & 0xFFFFFFFF;
    }
    if(scmp(string, "r10d"))
    {
        return GetContextDataEx(hActiveThread, UE_R10) & 0xFFFFFFFF;
    }
    if(scmp(string, "r11d"))
    {
        return GetContextDataEx(hActiveThread, UE_R11) & 0xFFFFFFFF;
    }
    if(scmp(string, "r12d"))
    {
        return GetContextDataEx(hActiveThread, UE_R12) & 0xFFFFFFFF;
    }
    if(scmp(string, "r13d"))
    {
        return GetContextDataEx(hActiveThread, UE_R13) & 0xFFFFFFFF;
    }
    if(scmp(string, "r14d"))
    {
        return GetContextDataEx(hActiveThread, UE_R14) & 0xFFFFFFFF;
    }
    if(scmp(string, "r15d"))
    {
        return GetContextDataEx(hActiveThread, UE_R15) & 0xFFFFFFFF;
    }

    if(size)
        *size = 2;
    if(scmp(string, "r8w"))
    {
        return GetContextDataEx(hActiveThread, UE_R8) & 0xFFFF;
    }
    if(scmp(string, "r9w"))
    {
        return GetContextDataEx(hActiveThread, UE_R9) & 0xFFFF;
    }
    if(scmp(string, "r10w"))
    {
        return GetContextDataEx(hActiveThread, UE_R10) & 0xFFFF;
    }
    if(scmp(string, "r11w"))
    {
        return GetContextDataEx(hActiveThread, UE_R11) & 0xFFFF;
    }
    if(scmp(string, "r12w"))
    {
        return GetContextDataEx(hActiveThread, UE_R12) & 0xFFFF;
    }
    if(scmp(string, "r13w"))
    {
        return GetContextDataEx(hActiveThread, UE_R13) & 0xFFFF;
    }
    if(scmp(string, "r14w"))
    {
        return GetContextDataEx(hActiveThread, UE_R14) & 0xFFFF;
    }
    if(scmp(string, "r15w"))
    {
        return GetContextDataEx(hActiveThread, UE_R15) & 0xFFFF;
    }

    if(size)
        *size = 1;
    if(scmp(string, "r8b"))
    {
        return GetContextDataEx(hActiveThread, UE_R8) & 0xFF;
    }
    if(scmp(string, "r9b"))
    {
        return GetContextDataEx(hActiveThread, UE_R9) & 0xFF;
    }
    if(scmp(string, "r10b"))
    {
        return GetContextDataEx(hActiveThread, UE_R10) & 0xFF;
    }
    if(scmp(string, "r11b"))
    {
        return GetContextDataEx(hActiveThread, UE_R11) & 0xFF;
    }
    if(scmp(string, "r12b"))
    {
        return GetContextDataEx(hActiveThread, UE_R12) & 0xFF;
    }
    if(scmp(string, "r13b"))
    {
        return GetContextDataEx(hActiveThread, UE_R13) & 0xFF;
    }
    if(scmp(string, "r14b"))
    {
        return GetContextDataEx(hActiveThread, UE_R14) & 0xFF;
    }
    if(scmp(string, "r15b"))
    {
        return GetContextDataEx(hActiveThread, UE_R15) & 0xFF;
    }
#endif //_WIN64

    if(size)
        *size = 0;
    return 0;
}

static bool setregister(const char* string, uint value)
{
    if(scmp(string, "eax"))
        return SetContextDataEx(hActiveThread, UE_EAX, value & 0xFFFFFFFF);
    if(scmp(string, "ebx"))
        return SetContextDataEx(hActiveThread, UE_EBX, value & 0xFFFFFFFF);
    if(scmp(string, "ecx"))
        return SetContextDataEx(hActiveThread, UE_ECX, value & 0xFFFFFFFF);
    if(scmp(string, "edx"))
        return SetContextDataEx(hActiveThread, UE_EDX, value & 0xFFFFFFFF);
    if(scmp(string, "edi"))
        return SetContextDataEx(hActiveThread, UE_EDI, value & 0xFFFFFFFF);
    if(scmp(string, "esi"))
        return SetContextDataEx(hActiveThread, UE_ESI, value & 0xFFFFFFFF);
    if(scmp(string, "ebp"))
        return SetContextDataEx(hActiveThread, UE_EBP, value & 0xFFFFFFFF);
    if(scmp(string, "esp"))
        return SetContextDataEx(hActiveThread, UE_ESP, value & 0xFFFFFFFF);
    if(scmp(string, "eip"))
        return SetContextDataEx(hActiveThread, UE_EIP, value & 0xFFFFFFFF);
    if(scmp(string, "eflags"))
        return SetContextDataEx(hActiveThread, UE_EFLAGS, value & 0xFFFFFFFF);

    if(scmp(string, "gs"))
        return SetContextDataEx(hActiveThread, UE_SEG_GS, value & 0xFFFF);
    if(scmp(string, "fs"))
        return SetContextDataEx(hActiveThread, UE_SEG_FS, value & 0xFFFF);
    if(scmp(string, "es"))
        return SetContextDataEx(hActiveThread, UE_SEG_ES, value & 0xFFFF);
    if(scmp(string, "ds"))
        return SetContextDataEx(hActiveThread, UE_SEG_DS, value & 0xFFFF);
    if(scmp(string, "cs"))
        return SetContextDataEx(hActiveThread, UE_SEG_CS, value & 0xFFFF);
    if(scmp(string, "ss"))
        return SetContextDataEx(hActiveThread, UE_SEG_SS, value & 0xFFFF);

    if(scmp(string, "ax"))
        return SetContextDataEx(hActiveThread, UE_EAX, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_EAX) & 0xFFFF0000));
    if(scmp(string, "bx"))
        return SetContextDataEx(hActiveThread, UE_EBX, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_EBX) & 0xFFFF0000));
    if(scmp(string, "cx"))
        return SetContextDataEx(hActiveThread, UE_ECX, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_ECX) & 0xFFFF0000));
    if(scmp(string, "dx"))
        return SetContextDataEx(hActiveThread, UE_EDX, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_EDX) & 0xFFFF0000));
    if(scmp(string, "si"))
        return SetContextDataEx(hActiveThread, UE_ESI, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_ESI) & 0xFFFF0000));
    if(scmp(string, "di"))
        return SetContextDataEx(hActiveThread, UE_EDI, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_EDI) & 0xFFFF0000));
    if(scmp(string, "bp"))
        return SetContextDataEx(hActiveThread, UE_EBP, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_EBP) & 0xFFFF0000));
    if(scmp(string, "sp"))
        return SetContextDataEx(hActiveThread, UE_ESP, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_ESP) & 0xFFFF0000));
    if(scmp(string, "ip"))
        return SetContextDataEx(hActiveThread, UE_EIP, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_EIP) & 0xFFFF0000));

    if(scmp(string, "ah"))
        return SetContextDataEx(hActiveThread, UE_EAX, ((value & 0xFF) << 8) | (GetContextDataEx(hActiveThread, UE_EAX) & 0xFFFF00FF));
    if(scmp(string, "al"))
        return SetContextDataEx(hActiveThread, UE_EAX, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_EAX) & 0xFFFFFF00));
    if(scmp(string, "bh"))
        return SetContextDataEx(hActiveThread, UE_EBX, ((value & 0xFF) << 8) | (GetContextDataEx(hActiveThread, UE_EBX) & 0xFFFF00FF));
    if(scmp(string, "bl"))
        return SetContextDataEx(hActiveThread, UE_EBX, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_EBX) & 0xFFFFFF00));
    if(scmp(string, "ch"))
        return SetContextDataEx(hActiveThread, UE_ECX, ((value & 0xFF) << 8) | (GetContextDataEx(hActiveThread, UE_ECX) & 0xFFFF00FF));
    if(scmp(string, "cl"))
        return SetContextDataEx(hActiveThread, UE_ECX, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_ECX) & 0xFFFFFF00));
    if(scmp(string, "dh"))
        return SetContextDataEx(hActiveThread, UE_EDX, ((value & 0xFF) << 8) | (GetContextDataEx(hActiveThread, UE_EDX) & 0xFFFF00FF));
    if(scmp(string, "dl"))
        return SetContextDataEx(hActiveThread, UE_EDX, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_EDX) & 0xFFFFFF00));
    if(scmp(string, "sih"))
        return SetContextDataEx(hActiveThread, UE_ESI, ((value & 0xFF) << 8) | (GetContextDataEx(hActiveThread, UE_ESI) & 0xFFFF00FF));
    if(scmp(string, "sil"))
        return SetContextDataEx(hActiveThread, UE_ESI, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_ESI) & 0xFFFFFF00));
    if(scmp(string, "dih"))
        return SetContextDataEx(hActiveThread, UE_EDI, ((value & 0xFF) << 8) | (GetContextDataEx(hActiveThread, UE_EDI) & 0xFFFF00FF));
    if(scmp(string, "dil"))
        return SetContextDataEx(hActiveThread, UE_EDI, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_EDI) & 0xFFFFFF00));
    if(scmp(string, "bph"))
        return SetContextDataEx(hActiveThread, UE_EBP, ((value & 0xFF) << 8) | (GetContextDataEx(hActiveThread, UE_EBP) & 0xFFFF00FF));
    if(scmp(string, "bpl"))
        return SetContextDataEx(hActiveThread, UE_EBP, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_EBP) & 0xFFFFFF00));
    if(scmp(string, "sph"))
        return SetContextDataEx(hActiveThread, UE_ESP, ((value & 0xFF) << 8) | (GetContextDataEx(hActiveThread, UE_ESP) & 0xFFFF00FF));
    if(scmp(string, "spl"))
        return SetContextDataEx(hActiveThread, UE_ESP, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_ESP) & 0xFFFFFF00));
    if(scmp(string, "iph"))
        return SetContextDataEx(hActiveThread, UE_EIP, ((value & 0xFF) << 8) | (GetContextDataEx(hActiveThread, UE_EIP) & 0xFFFF00FF));
    if(scmp(string, "ipl"))
        return SetContextDataEx(hActiveThread, UE_EIP, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_EIP) & 0xFFFFFF00));

    if(scmp(string, "dr0"))
        return SetContextDataEx(hActiveThread, UE_DR0, value);
    if(scmp(string, "dr1"))
        return SetContextDataEx(hActiveThread, UE_DR1, value);
    if(scmp(string, "dr2"))
        return SetContextDataEx(hActiveThread, UE_DR2, value);
    if(scmp(string, "dr3"))
        return SetContextDataEx(hActiveThread, UE_DR3, value);
    if(scmp(string, "dr6") or scmp(string, "dr4"))
        return SetContextDataEx(hActiveThread, UE_DR6, value);
    if(scmp(string, "dr7") or scmp(string, "dr5"))
        return SetContextDataEx(hActiveThread, UE_DR7, value);

    if(scmp(string, "cip"))
        return SetContextDataEx(hActiveThread, UE_CIP, value);
    if(scmp(string, "csp"))
        return SetContextDataEx(hActiveThread, UE_CSP, value);
    if(scmp(string, "cflags"))
        return SetContextDataEx(hActiveThread, UE_CFLAGS, value);

#ifdef _WIN64
    if(scmp(string, "rax"))
        return SetContextDataEx(hActiveThread, UE_RAX, value);
    if(scmp(string, "rbx"))
        return SetContextDataEx(hActiveThread, UE_RBX, value);
    if(scmp(string, "rcx"))
        return SetContextDataEx(hActiveThread, UE_RCX, value);
    if(scmp(string, "rdx"))
        return SetContextDataEx(hActiveThread, UE_RDX, value);
    if(scmp(string, "rdi"))
        return SetContextDataEx(hActiveThread, UE_RDI, value);
    if(scmp(string, "rsi"))
        return SetContextDataEx(hActiveThread, UE_RSI, value);
    if(scmp(string, "rbp"))
        return SetContextDataEx(hActiveThread, UE_RBP, value);
    if(scmp(string, "rsp"))
        return SetContextDataEx(hActiveThread, UE_RSP, value);
    if(scmp(string, "rip"))
        return SetContextDataEx(hActiveThread, UE_RIP, value);
    if(scmp(string, "rflags"))
        return SetContextDataEx(hActiveThread, UE_RFLAGS, value);
    if(scmp(string, "r8"))
        return SetContextDataEx(hActiveThread, UE_R8, value);
    if(scmp(string, "r9"))
        return SetContextDataEx(hActiveThread, UE_R9, value);
    if(scmp(string, "r10"))
        return SetContextDataEx(hActiveThread, UE_R10, value);
    if(scmp(string, "r11"))
        return SetContextDataEx(hActiveThread, UE_R11, value);
    if(scmp(string, "r12"))
        return SetContextDataEx(hActiveThread, UE_R12, value);
    if(scmp(string, "r13"))
        return SetContextDataEx(hActiveThread, UE_R13, value);
    if(scmp(string, "r14"))
        return SetContextDataEx(hActiveThread, UE_R14, value);
    if(scmp(string, "r15"))
        return SetContextDataEx(hActiveThread, UE_R15, value);

    if(scmp(string, "r8d"))
        return SetContextDataEx(hActiveThread, UE_R8, (value & 0xFFFFFFFF) | (GetContextDataEx(hActiveThread, UE_R8) & 0xFFFFFFFF00000000));
    if(scmp(string, "r9d"))
        return SetContextDataEx(hActiveThread, UE_R9, (value & 0xFFFFFFFF) | (GetContextDataEx(hActiveThread, UE_R9) & 0xFFFFFFFF00000000));
    if(scmp(string, "r10d"))
        return SetContextDataEx(hActiveThread, UE_R10, (value & 0xFFFFFFFF) | (GetContextDataEx(hActiveThread, UE_R10) & 0xFFFFFFFF00000000));
    if(scmp(string, "r11d"))
        return SetContextDataEx(hActiveThread, UE_R11, (value & 0xFFFFFFFF) | (GetContextDataEx(hActiveThread, UE_R11) & 0xFFFFFFFF00000000));
    if(scmp(string, "r12d"))
        return SetContextDataEx(hActiveThread, UE_R12, (value & 0xFFFFFFFF) | (GetContextDataEx(hActiveThread, UE_R12) & 0xFFFFFFFF00000000));
    if(scmp(string, "r13d"))
        return SetContextDataEx(hActiveThread, UE_R13, (value & 0xFFFFFFFF) | (GetContextDataEx(hActiveThread, UE_R13) & 0xFFFFFFFF00000000));
    if(scmp(string, "r14d"))
        return SetContextDataEx(hActiveThread, UE_R14, (value & 0xFFFFFFFF) | (GetContextDataEx(hActiveThread, UE_R14) & 0xFFFFFFFF00000000));
    if(scmp(string, "r15d"))
        return SetContextDataEx(hActiveThread, UE_R15, (value & 0xFFFFFFFF) | (GetContextDataEx(hActiveThread, UE_R15) & 0xFFFFFFFF00000000));

    if(scmp(string, "r8w"))
        return SetContextDataEx(hActiveThread, UE_R8, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_R8) & 0xFFFFFFFFFFFF0000));
    if(scmp(string, "r9w"))
        return SetContextDataEx(hActiveThread, UE_R9, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_R9) & 0xFFFFFFFFFFFF0000));
    if(scmp(string, "r10w"))
        return SetContextDataEx(hActiveThread, UE_R10, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_R10) & 0xFFFFFFFFFFFF0000));
    if(scmp(string, "r11w"))
        return SetContextDataEx(hActiveThread, UE_R11, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_R11) & 0xFFFFFFFFFFFF0000));
    if(scmp(string, "r12w"))
        return SetContextDataEx(hActiveThread, UE_R12, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_R12) & 0xFFFFFFFFFFFF0000));
    if(scmp(string, "r13w"))
        return SetContextDataEx(hActiveThread, UE_R13, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_R13) & 0xFFFFFFFFFFFF0000));
    if(scmp(string, "r14w"))
        return SetContextDataEx(hActiveThread, UE_R14, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_R14) & 0xFFFFFFFFFFFF0000));
    if(scmp(string, "r15w"))
        return SetContextDataEx(hActiveThread, UE_R15, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, UE_R15) & 0xFFFFFFFFFFFF0000));
    if(scmp(string, "r8b"))
        return SetContextDataEx(hActiveThread, UE_R8, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_R8) & 0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r9b"))
        return SetContextDataEx(hActiveThread, UE_R9, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_R9) & 0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r10b"))
        return SetContextDataEx(hActiveThread, UE_R10, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_R10) & 0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r11b"))
        return SetContextDataEx(hActiveThread, UE_R11, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_R11) & 0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r12b"))
        return SetContextDataEx(hActiveThread, UE_R12, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_R12) & 0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r13b"))
        return SetContextDataEx(hActiveThread, UE_R13, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_R13) & 0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r14b"))
        return SetContextDataEx(hActiveThread, UE_R14, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_R14) & 0xFFFFFFFFFFFFFF00));
    if(scmp(string, "r15b"))
        return SetContextDataEx(hActiveThread, UE_R15, (value & 0xFF) | (GetContextDataEx(hActiveThread, UE_R15) & 0xFFFFFFFFFFFFFF00));
#endif // _WIN64

    return false;
}

bool valapifromstring(const char* name, uint* value, int* value_size, bool printall, bool silent, bool* hexonly)
{
    if(!value or !DbgIsDebugging())
        return false;
    //explicit API handling
    const char* apiname = strchr(name, ':'); //the ':' character cannot be in a path: http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx#naming_conventions
    bool noexports = false;
    if(!apiname) //not found
    {
        apiname = strrchr(name, '.'); //kernel32.GetProcAddress support
        if(!apiname) //not found
        {
            apiname = strchr(name, '?'); //the '?' character cannot be in a path either
            noexports = true;
        }
    }
    if(apiname)
    {
        char modname[MAX_MODULE_SIZE] = "";
        if(name == apiname) //:[expression] <= currently selected module
        {
            SELECTIONDATA seldata;
            memset(&seldata, 0, sizeof(seldata));
            GuiSelectionGet(GUI_DISASSEMBLY, &seldata);
            if(!modnamefromaddr(seldata.start, modname, true))
                return false;
        }
        else
        {
            strcpy_s(modname, name);
            modname[apiname - name] = 0;
        }
        apiname++;
        if(!strlen(apiname))
            return false;
        uint modbase = modbasefromname(modname);
        wchar_t szModName[MAX_PATH] = L"";
        if(!GetModuleFileNameExW(fdProcessInfo->hProcess, (HMODULE)modbase, szModName, MAX_PATH))
        {
            if(!silent)
                dprintf("could not get filename of module "fhex"\n", modbase);
        }
        else
        {
            HMODULE mod = LoadLibraryExW(szModName, 0, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
            if(!mod)
            {
                if(!silent)
                    dprintf("unable to load library %s\n", szModName);
            }
            else
            {
                uint addr = noexports ? 0 : (uint)GetProcAddress(mod, apiname);
                if(addr) //found exported function
                    addr = modbase + (addr - (uint)mod); //correct for loaded base
                else //not found
                {
                    if(scmp(apiname, "base") or scmp(apiname, "imagebase") or scmp(apiname, "header")) //get loaded base
                        addr = modbase;
                    else if(scmp(apiname, "entry") or scmp(apiname, "oep") or scmp(apiname, "ep")) //get entry point
                        addr = modbase + GetPE32DataW(szModName, 0, UE_OEP);
                    else if(*apiname == '$') //RVA
                    {
                        uint rva;
                        if(valfromstring(apiname + 1, &rva))
                            addr = modbase + rva;
                    }
                    else if(*apiname == '#') //File Offset
                    {
                        uint offset;
                        if(valfromstring(apiname + 1, &offset))
                            addr = valfileoffsettova(modname, offset);
                    }
                    else
                    {
                        if(noexports) //get the exported functions with the '?' delimiter
                        {
                            addr = (uint)GetProcAddress(mod, apiname);
                            if(addr) //found exported function
                                addr = modbase + (addr - (uint)mod); //correct for loaded base
                        }
                        else
                        {
                            uint ordinal;
                            if(valfromstring(apiname, &ordinal))
                            {
                                addr = (uint)GetProcAddress(mod, (LPCSTR)(ordinal & 0xFFFF));
                                if(addr) //found exported function
                                    addr = modbase + (addr - (uint)mod); //correct for loaded base
                                else if(!ordinal) //support for getting the image base using <modname>:0
                                    addr = modbase;
                            }
                        }
                    }
                }
                FreeLibrary(mod);
                if(addr) //found!
                {
                    if(value_size)
                        *value_size = sizeof(uint);
                    if(hexonly)
                        *hexonly = true;
                    *value = addr;
                    return true;
                }
            }
        }
        return false;
    }
    int found = 0;
    int kernel32 = -1;
    DWORD cbNeeded = 0;
    Memory<uint*> addrfound;
    if(EnumProcessModules(fdProcessInfo->hProcess, 0, 0, &cbNeeded))
    {
        addrfound.realloc(cbNeeded * sizeof(uint), "valapifromstring:addrfound");
        Memory<HMODULE*> hMods(cbNeeded * sizeof(HMODULE), "valapifromstring:hMods");
        if(EnumProcessModules(fdProcessInfo->hProcess, hMods, cbNeeded, &cbNeeded))
        {
            for(unsigned int i = 0; i < cbNeeded / sizeof(HMODULE); i++)
            {
                wchar_t szModuleName[MAX_PATH] = L"";
                if(GetModuleFileNameExW(fdProcessInfo->hProcess, hMods[i], szModuleName, MAX_PATH))
                {
                    wchar_t* szBaseName = wcsrchr(szModuleName, L'\\');
                    if(szBaseName)
                    {
                        szBaseName++;
                        HMODULE hModule = LoadLibraryExW(szModuleName, 0, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
                        if(hModule)
                        {
                            ULONG_PTR funcAddress = (ULONG_PTR)GetProcAddress(hModule, name);
                            if(funcAddress)
                            {
                                if(!_wcsicmp(szBaseName, L"kernel32.dll"))
                                    kernel32 = found;
                                uint rva = funcAddress - (uint)hModule;
                                addrfound[found] = (uint)hMods[i] + rva;
                                found++;
                            }
                            FreeLibrary(hModule);
                        }
                    }
                }
            }
        }
    }
    if(!found)
        return false;
    if(value_size)
        *value_size = sizeof(uint);
    if(hexonly)
        *hexonly = true;
    if(kernel32 != -1) //prioritize kernel32 exports
    {
        *value = addrfound[kernel32];
        if(!printall or silent)
            return true;
        for(int i = 0; i < found; i++)
            if(i != kernel32)
                dprintf(fhex"\n", addrfound[i]);
    }
    else
    {
        *value = *addrfound;
        if(!printall or silent)
            return true;
        for(int i = 1; i < found; i++)
            dprintf(fhex"\n", addrfound[i]);
    }
    return true;
}

/*
check whether a string is a valid dec number
*/
static bool isdecnumber(const char* string)
{
    if(*string != '.' or !string[1]) //dec indicator/no number
        return false;
    int decAdd = 1;
    if(string[1] == '-') //minus
    {
        if(!string[2]) //no number
            return false;
        decAdd++;
    }
    int len = (int)strlen(string + decAdd);
    for(int i = 0; i < len; i++)
        if(!isdigit(string[i + decAdd]))
            return false;
    return true;
}

/*
check whether a string is a valid hex number
*/
static bool ishexnumber(const char* string)
{
    int add = 0;
    if(*string == '0' and string[1] == 'x') //0x prefix
        add = 2;
    else if(*string == 'x') //hex indicator
        add = 1;
    if(!string[add]) //only an indicator, no number
        return false;
    int len = (int)strlen(string + add);
    for(int i = 0; i < len; i++)
        if(!isxdigit(string[i + add])) //all must be hex digits
            return false;
    return true;
}

bool valfromstring(const char* string, uint* value, bool silent, bool baseonly, int* value_size, bool* isvar, bool* hexonly)
{
    if(!value or !string)
        return false;
    if(!*string)
    {
        *value = 0;
        return true;
    }
    else if(mathcontains(string)) //handle math
    {
        int len = (int)strlen(string);
        Memory<char*> newstring(len * 2, "valfromstring:newstring");
        if(strstr(string, "[")) //memory brackets: []
        {
            for(int i = 0, j = 0; i < len; i++)
            {
                if(string[i] == ']')
                    j += sprintf(newstring + j, ")");
                else if(isdigit(string[i]) and string[i + 1] == ':' and string[i + 2] == '[') //n:[
                {
                    j += sprintf(newstring + j, "@%c:(", string[i]);
                    i += 2;
                }
                else if(string[i] == '[')
                    j += sprintf(newstring + j, "@(");
                else
                    j += sprintf(newstring + j, "%c", string[i]);
            }
        }
        else
            strcpy(newstring, string);
        Memory<char*> string_(len + 256, "valfromstring:string_");
        strcpy(string_, newstring);
        int add = 0;
        bool negative = (*string_ == '-');
        while(mathisoperator(string_[add + negative]) > 2)
            add++;
        if(!mathhandlebrackets(string_ + add, silent, baseonly))
            return false;
        return mathfromstring(string_ + add, value, silent, baseonly, value_size, isvar);
    }
    else if(*string == '-') //negative value
    {
        uint val;
        if(!valfromstring(string + 1, &val, silent, baseonly, value_size, isvar, hexonly))
            return false;
        val *= ~0;
        if(value)
            *value = val;
        return true;
    }
    else if(*string == '@' or strstr(string, "[")) //memory location
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs("not debugging");
            *value = 0;
            if(value_size)
                *value_size = 0;
            if(isvar)
                *isvar = true;
            return true;
        }
        int len = (int)strlen(string);
        Memory<char*> newstring(len * 2, "valfromstring:newstring");
        if(strstr(string, "["))
        {
            for(int i = 0, j = 0; i < len; i++)
            {
                if(string[i] == ']')
                    j += sprintf(newstring + j, ")");
                else if(isdigit(string[i]) and string[i + 1] == ':' and string[i + 2] == '[') //n:[
                {
                    j += sprintf(newstring + j, "@%c:(", string[i]);
                    i += 2;
                }
                else if(string[i] == '[')
                    j += sprintf(newstring + j, "@(");
                else
                    j += sprintf(newstring + j, "%c", string[i]);
            }
        }
        else
            strcpy(newstring, string);
        int read_size = sizeof(uint);
        int add = 1;
        if(newstring[2] == ':' and isdigit((newstring[1]))) //@n: (number of bytes to read)
        {
            add += 2;
            int new_size = newstring[1] - 0x30;
            if(new_size < read_size)
                read_size = new_size;
        }
        if(!valfromstring(newstring + add, value, silent, baseonly))
            return false;
        uint addr = *value;
        *value = 0;
        if(!memread(fdProcessInfo->hProcess, (void*)addr, value, read_size, 0))
        {
            if(!silent)
                dputs("failed to read memory");
            return false;
        }
        if(value_size)
            *value_size = read_size;
        if(isvar)
            *isvar = true;
        return true;
    }
    else if(isregister(string)) //register
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs("not debugging!");
            *value = 0;
            if(value_size)
                *value_size = 0;
            if(isvar)
                *isvar = true;
            return true;
        }
        *value = getregister(value_size, string);
        if(isvar)
            *isvar = true;
        return true;
    }
    else if(*string == '!' and isflag(string + 1)) //flag
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs("not debugging");
            *value = 0;
            if(value_size)
                *value_size = 0;
            if(isvar)
                *isvar = true;
            return true;
        }
        uint eflags = GetContextDataEx(hActiveThread, UE_CFLAGS);
        if(valflagfromstring(eflags, string + 1))
            *value = 1;
        else
            *value = 0;
        if(value_size)
            *value_size = 0;
        if(isvar)
            *isvar = true;
        return true;
    }
    else if(isdecnumber(string)) //decimal numbers come 'first'
    {
        if(value_size)
            *value_size = 0;
        if(isvar)
            *isvar = false;
        sscanf(string + 1, "%"fext"u", value);
        return true;
    }
    else if(ishexnumber(string)) //then hex numbers
    {
        if(value_size)
            *value_size = 0;
        if(isvar)
            *isvar = false;
        //hexadecimal value
        int inc = 0;
        if(*string == 'x')
            inc = 1;
        sscanf(string + inc, "%"fext"x", value);
        return true;
    }
    if(baseonly)
        return false;
    else if(valapifromstring(string, value, value_size, true, silent, hexonly)) //then come APIs
        return true;
    else if(labelfromstring(string, value)) //then come labels
        return true;
    else if(symfromname(string, value)) //then come symbols
        return true;
    else if(varget(string, value, value_size, 0)) //finally variables
    {
        if(isvar)
            *isvar = true;
        return true;
    }
    if(!silent)
        dprintf("invalid value: \"%s\"!\n", string);
    return false; //nothing was OK
}

static bool longEnough(const char* str, size_t min_length)
{
    size_t length = 0;
    while(str[length] && length < min_length)
        length++;
    if(length == min_length)
        return true;
    return false;
}

static bool startsWith(const char* pre, const char* str)
{
    size_t lenpre = strlen(pre);
    return longEnough(str, lenpre) ? StrNCmpI(str, pre, (int) lenpre) == 0 : false;
}

#define MxCsr_PRE_FIELD_STRING "MxCsr_"
#define x87SW_PRE_FIELD_STRING "x87SW_"
#define x87CW_PRE_FIELD_STRING "x87CW_"
#define x87TW_PRE_FIELD_STRING "x87TW_"
#define MMX_PRE_FIELD_STRING "MM"
#define XMM_PRE_FIELD_STRING "XMM"
#define YMM_PRE_FIELD_STRING "YMM"
#define x8780BITFPU_PRE_FIELD_STRING "x87r"
#define STRLEN_USING_SIZEOF(string) (sizeof(string) - 1)

static void fpustuff(const char* string, uint value)
{
    uint xorval = 0;
    uint flags = 0;
    uint flag = 0;
    bool set = false;

    if(value)
        set = true;

    if(startsWith(MxCsr_PRE_FIELD_STRING, string))
    {
        if(StrNCmpI(string + STRLEN_USING_SIZEOF(MxCsr_PRE_FIELD_STRING), "RC", (int) strlen("RC")) == 0)
        {
            uint flags = GetContextDataEx(hActiveThread, UE_MXCSR);
            int i = 3;
            i <<= 13;
            flags &= ~i;
            value <<= 13;
            flags |=  value;
            SetContextDataEx(hActiveThread, UE_MXCSR, flags);
        }
        else
        {
            uint flags = GetContextDataEx(hActiveThread, UE_MXCSR);
            flag = getmxcsrflagfromstring(string + STRLEN_USING_SIZEOF(MxCsr_PRE_FIELD_STRING));
            if(flags & flag and !set)
                xorval = flag;
            else if(set)
                xorval = flag;
            SetContextDataEx(hActiveThread, UE_MXCSR, flags ^ xorval);
        }
    }
    else if(startsWith(x87TW_PRE_FIELD_STRING, string))
    {
        unsigned int i;

        string += STRLEN_USING_SIZEOF(x87TW_PRE_FIELD_STRING);
        i = atoi(string);

        if(i > 7)
            return;

        flags = GetContextDataEx(hActiveThread, UE_X87_TAGWORD);

        flag = 3;
        flag <<= i * 2;

        flags &= ~flag;

        flag = value;
        flag <<= i * 2;

        flags |= flag;

        SetContextDataEx(hActiveThread, UE_X87_TAGWORD, (unsigned short) flags);

    }
    else if(startsWith(x87SW_PRE_FIELD_STRING, string))
    {
        if(StrNCmpI(string + STRLEN_USING_SIZEOF(x87SW_PRE_FIELD_STRING), "TOP", (int) strlen("TOP")) == 0)
        {
            uint flags = GetContextDataEx(hActiveThread, UE_X87_STATUSWORD);
            int i = 7;
            i <<= 11;
            flags &= ~i;
            value <<= 11;
            flags |=  value;
            SetContextDataEx(hActiveThread, UE_X87_STATUSWORD, flags);
        }
        else
        {
            uint flags = GetContextDataEx(hActiveThread, UE_X87_STATUSWORD);
            flag = getx87statuswordflagfromstring(string + STRLEN_USING_SIZEOF(x87SW_PRE_FIELD_STRING));
            if(flags & flag and !set)
                xorval = flag;
            else if(set)
                xorval = flag;
            SetContextDataEx(hActiveThread, UE_X87_STATUSWORD, flags ^ xorval);
        }
    }
    else if(startsWith(x87CW_PRE_FIELD_STRING, string))
    {
        if(StrNCmpI(string + STRLEN_USING_SIZEOF(x87CW_PRE_FIELD_STRING), "RC", (int) strlen("RC")) == 0)
        {
            uint flags = GetContextDataEx(hActiveThread, UE_X87_CONTROLWORD);
            int i = 3;
            i <<= 10;
            flags &= ~i;
            value <<= 10;
            flags |=  value;
            SetContextDataEx(hActiveThread, UE_X87_CONTROLWORD, flags);
        }
        else if(StrNCmpI(string + STRLEN_USING_SIZEOF(x87CW_PRE_FIELD_STRING), "PC", (int) strlen("PC")) == 0)
        {
            uint flags = GetContextDataEx(hActiveThread, UE_X87_CONTROLWORD);
            int i = 3;
            i <<= 8;
            flags &= ~i;
            value <<= 8;
            flags |=  value;
            SetContextDataEx(hActiveThread, UE_X87_CONTROLWORD, flags);
        }
        else
        {
            uint flags = GetContextDataEx(hActiveThread, UE_X87_CONTROLWORD);
            flag = getx87controlwordflagfromstring(string + STRLEN_USING_SIZEOF(x87CW_PRE_FIELD_STRING));
            if(flags & flag and !set)
                xorval = flag;
            else if(set)
                xorval = flag;
            SetContextDataEx(hActiveThread, UE_X87_CONTROLWORD, flags ^ xorval);
        }
    }
    else if(StrNCmpI(string, "x87TagWord", (int) strlen(string)) == 0)
    {
        SetContextDataEx(hActiveThread, UE_X87_TAGWORD, (unsigned short) value);
    }
    else if(StrNCmpI(string, "x87StatusWord", (int) strlen(string)) == 0)
    {
        SetContextDataEx(hActiveThread, UE_X87_STATUSWORD, (unsigned short) value);
    }
    else if(StrNCmpI(string, "x87ControlWord", (int) strlen(string)) == 0)
    {
        SetContextDataEx(hActiveThread, UE_X87_CONTROLWORD, (unsigned short) value);
    }
    else if(StrNCmpI(string, "MxCsr", (int) strlen(string)) == 0)
    {
        SetContextDataEx(hActiveThread, UE_MXCSR, value);
    }
    else if(startsWith(x8780BITFPU_PRE_FIELD_STRING, string))
    {
        string += STRLEN_USING_SIZEOF(x8780BITFPU_PRE_FIELD_STRING);
        DWORD registerindex;
        bool found = true;
        switch(*string)
        {
        case '0':
            registerindex = UE_x87_r0;
            break;

        case '1':
            registerindex = UE_x87_r1;
            break;

        case '2':
            registerindex = UE_x87_r2;
            break;

        case '3':
            registerindex = UE_x87_r3;
            break;

        case '4':
            registerindex = UE_x87_r4;
            break;

        case '5':
            registerindex = UE_x87_r5;
            break;

        case '6':
            registerindex = UE_x87_r6;
            break;

        case '7':
            registerindex = UE_x87_r7;
            break;

        default:
            found = false;
            break;
        }
        if(found)
            SetContextDataEx(hActiveThread, registerindex, value);
    }
    else if(startsWith(MMX_PRE_FIELD_STRING, string))
    {
        string += STRLEN_USING_SIZEOF(MMX_PRE_FIELD_STRING);
        DWORD registerindex;
        bool found = true;
        switch(*string)
        {
        case '0':
            registerindex = UE_MMX0;
            break;

        case '1':
            registerindex = UE_MMX1;
            break;

        case '2':
            registerindex = UE_MMX2;
            break;

        case '3':
            registerindex = UE_MMX3;
            break;

        case '4':
            registerindex = UE_MMX4;
            break;

        case '5':
            registerindex = UE_MMX5;
            break;

        case '6':
            registerindex = UE_MMX6;
            break;

        case '7':
            registerindex = UE_MMX7;
            break;

        default:
            found = false;
            break;
        }
        if(found)
            SetContextDataEx(hActiveThread, registerindex, value);
    }
    else if(startsWith(XMM_PRE_FIELD_STRING, string))
    {
        string += STRLEN_USING_SIZEOF(XMM_PRE_FIELD_STRING);
        DWORD registerindex;
        bool found = true;
        switch(atoi(string))
        {
        case 0:
            registerindex = UE_XMM0;
            break;

        case 1:
            registerindex = UE_XMM1;
            break;

        case 2:
            registerindex = UE_XMM2;
            break;

        case 3:
            registerindex = UE_XMM3;
            break;

        case 4:
            registerindex = UE_XMM4;
            break;

        case 5:
            registerindex = UE_XMM5;
            break;

        case 6:
            registerindex = UE_XMM6;
            break;

        case 7:
            registerindex = UE_XMM7;
            break;

        case 8:
            registerindex = UE_XMM8;
            break;

        case 9:
            registerindex = UE_XMM9;
            break;

        case 10:
            registerindex = UE_XMM10;
            break;

        case 11:
            registerindex = UE_XMM11;
            break;

        case 12:
            registerindex = UE_XMM12;
            break;

        case 13:
            registerindex = UE_XMM13;
            break;

        case 14:
            registerindex = UE_XMM14;
            break;

        case 15:
            registerindex = UE_XMM15;
            break;

        default:
            found = false;
            break;
        }
        if(found)
            SetContextDataEx(hActiveThread, registerindex, value);
    }
    else if(startsWith(YMM_PRE_FIELD_STRING, string))
    {
        string += STRLEN_USING_SIZEOF(YMM_PRE_FIELD_STRING);
        DWORD registerindex;
        bool found = true;
        switch(atoi(string))
        {
        case 0:
            registerindex = UE_YMM0;
            break;

        case 1:
            registerindex = UE_YMM1;
            break;

        case 2:
            registerindex = UE_YMM2;
            break;

        case 3:
            registerindex = UE_YMM3;
            break;

        case 4:
            registerindex = UE_YMM4;
            break;

        case 5:
            registerindex = UE_YMM5;
            break;

        case 6:
            registerindex = UE_YMM6;
            break;

        case 7:
            registerindex = UE_YMM7;
            break;

        case 8:
            registerindex = UE_YMM8;
            break;

        case 9:
            registerindex = UE_YMM9;
            break;

        case 10:
            registerindex = UE_YMM10;
            break;

        case 11:
            registerindex = UE_YMM11;
            break;

        case 12:
            registerindex = UE_YMM12;
            break;

        case 13:
            registerindex = UE_YMM13;
            break;

        case 14:
            registerindex = UE_YMM14;
            break;

        case 15:
            registerindex = UE_YMM15;
            break;

        default:
            found = false;
            break;
        }
        if(found)
            SetContextDataEx(hActiveThread, registerindex, value);
    }
}

bool valtostring(const char* string, uint* value, bool silent)
{
    if(!*string or !value)
        return false;
    else if(*string == '@' or strstr(string, "[")) //memory location
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs("not debugging");
            return false;
        }
        int len = (int)strlen(string);
        Memory<char*> newstring(len * 2, "valfromstring:newstring");
        if(strstr(string, "[")) //memory brackets: []
        {
            for(int i = 0, j = 0; i < len; i++)
            {
                if(string[i] == ']')
                    j += sprintf(newstring + j, ")");
                else if(isdigit(string[i]) and string[i + 1] == ':' and string[i + 2] == '[') //n:[
                {
                    j += sprintf(newstring + j, "@%c:(", string[i]);
                    i += 2;
                }
                else if(string[i] == '[')
                    j += sprintf(newstring + j, "@(");
                else
                    j += sprintf(newstring + j, "%c", string[i]);
            }
        }
        else
            strcpy(newstring, string);
        int read_size = sizeof(uint);
        int add = 1;
        if(newstring[2] == ':' and isdigit((newstring[1])))
        {
            add += 2;
            int new_size = newstring[1] - 0x30;
            if(new_size < read_size)
                read_size = new_size;
        }
        uint temp;
        if(!valfromstring(newstring + add, &temp, silent, false))
        {
            return false;
        }
        if(!mempatch(fdProcessInfo->hProcess, (void*)temp, value, read_size, 0))
        {
            if(!silent)
                dputs("failed to write memory");
            return false;
        }
        GuiUpdateAllViews(); //repaint gui
        GuiUpdatePatches(); //update patch dialog
        return true;
    }
    else if(isregister(string)) //register
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs("not debugging!");
            return false;
        }
        bool ok = setregister(string, *value);
        Memory<char*> regName(strlen(string) + 1, "valtostring:regname");
        strcpy(regName, string);
        _strlwr(regName);
        if(strstr(regName, "ip"))
            DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), false); //update disassembly + register view
        else if(strstr(regName, "sp")) //update stack
        {
            uint csp = GetContextDataEx(hActiveThread, UE_CSP);
            GuiStackDumpAt(csp, csp);
            GuiUpdateRegisterView();
        }
        else
            GuiUpdateAllViews(); //repaint gui
        return ok;
    }
    else if((*string == '_'))
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs("not debugging!");
            return false;
        }
        fpustuff(string + 1, * value);
        GuiUpdateAllViews(); //repaint gui

        return true;
    }
    else if(*string == '!' and isflag(string + 1)) //flag
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs("not debugging");
            return false;
        }
        bool set = false;
        if(*value)
            set = true;
        setflag(string + 1, set);
        GuiUpdateAllViews(); //repaint gui
        return true;
    }
    return varset(string, *value, false); //variable
}

uint valfileoffsettova(const char* modname, uint offset)
{
    char modpath[MAX_PATH] = "";
    if(modpathfromname(modname, modpath, MAX_PATH))
    {
        HANDLE FileHandle;
        DWORD LoadedSize;
        HANDLE FileMap;
        ULONG_PTR FileMapVA;
        if(StaticFileLoadW(StringUtils::Utf8ToUtf16(modpath).c_str(), UE_ACCESS_READ, false, &FileHandle, &LoadedSize, &FileMap, &FileMapVA))
        {
            ULONGLONG rva = ConvertFileOffsetToVA(FileMapVA, //FileMapVA
                                                  FileMapVA + (ULONG_PTR)offset, //Offset inside FileMapVA
                                                  false); //Return without ImageBase
            StaticFileUnloadW(StringUtils::Utf8ToUtf16(modpath).c_str(), true, FileHandle, LoadedSize, FileMap, FileMapVA);
            return offset < LoadedSize ? (duint)rva + modbasefromname(modname) : 0;
        }
    }
    return 0;
}

uint valvatofileoffset(uint va)
{
    char modpath[MAX_PATH] = "";
    if(modpathfromaddr(va, modpath, MAX_PATH))
    {
        HANDLE FileHandle;
        DWORD LoadedSize;
        HANDLE FileMap;
        ULONG_PTR FileMapVA;
        if(StaticFileLoadW(StringUtils::Utf8ToUtf16(modpath).c_str(), UE_ACCESS_READ, false, &FileHandle, &LoadedSize, &FileMap, &FileMapVA))
        {
            ULONGLONG offset = ConvertVAtoFileOffsetEx(FileMapVA, LoadedSize, 0, va - modbasefromaddr(va), true, false);
            StaticFileUnloadW(StringUtils::Utf8ToUtf16(modpath).c_str(), true, FileHandle, LoadedSize, FileMap, FileMapVA);
            return (duint)offset;
        }
    }
    return 0;
}