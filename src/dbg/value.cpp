/**
 @file value.cpp

 @brief Implements the value class.
 */

#include "value.h"
#include "variable.h"
#include "debugger.h"
#include "console.h"
#include "memory.h"
#include "symbolinfo.h"
#include "module.h"
#include "label.h"
#include "expressionparser.h"
#include "function.h"
#include "threading.h"
#include "TraceRecord.h"
#include "plugin_loader.h"
#include "exception.h"

static bool dosignedcalc = false;

/**
\brief Returns whether we do signed or unsigned calculations.
\return true if we do signed calculations, false for unsigned calculationss.
*/
bool valuesignedcalc()
{
    return dosignedcalc;
}

/**
\brief Set whether we do signed or unsigned calculations.
\param a true for signed calculations, false for unsigned calculations.
*/
void valuesetsignedcalc(bool a)
{
    dosignedcalc = a;
}

/**
\brief Check if a string is a flag.
\param string The string to check.
\return true if the string is a flag, false otherwise.
*/
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

/**
\brief Check if a string is a register.
\param string The string to check.
\return true if the string is a register, false otherwise.
*/
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
    if(scmp(string, "dr6") || scmp(string, "dr4"))
        return true;
    if(scmp(string, "dr7") || scmp(string, "dr5"))
        return true;

    if(scmp(string, "cax"))
        return true;
    if(scmp(string, "cbx"))
        return true;
    if(scmp(string, "ccx"))
        return true;
    if(scmp(string, "cdx"))
        return true;
    if(scmp(string, "csi"))
        return true;
    if(scmp(string, "cdi"))
        return true;
    if(scmp(string, "cip"))
        return true;
    if(scmp(string, "csp"))
        return true;
    if(scmp(string, "cbp"))
        return true;
    if(scmp(string, "cflags"))
        return true;

    if(scmp(string, "lasterror"))
        return true;
    if(scmp(string, "laststatus"))
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

/**
\brief Gets the MXCSR flag AND value from a string.
\param string The flag name.
\return The value to AND the MXCSR value with to get the flag. 0 when not found.
*/
static unsigned int getmxcsrflagfromstring(const char* string)
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

    for(int i = 0; i < (sizeof(mxcsrnameflagtable) / sizeof(*mxcsrnameflagtable)); i++)
    {
        if(scmp(string, mxcsrnameflagtable[i].name))
            return mxcsrnameflagtable[i].flag;
    }

    return 0;
}

/**
\brief Gets the MXCSR flag from a string and a flags value.
\param mxcsrflags The flags value to get the flag from.
\param string The string with the flag name.
\return true if the flag is 1, false if the flag is 0.
*/
bool valmxcsrflagfromstring(duint mxcsrflags, const char* string)
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
#define x87STATUSWORD_FLAG_ES 0x80
#define x87STATUSWORD_FLAG_C0 0x100
#define x87STATUSWORD_FLAG_C1 0x200
#define x87STATUSWORD_FLAG_C2 0x400
#define x87STATUSWORD_FLAG_C3 0x4000
#define x87STATUSWORD_FLAG_B 0x8000

#define X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(flag_name) { #flag_name, x87STATUSWORD_FLAG_##flag_name }

/**
\brief Gets the x87 status word AND value from a string.
\param string The status word name.
\return The value to AND the status word with to get the flag. 0 when not found.
*/
static unsigned int getx87statuswordflagfromstring(const char* string)
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
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(ES),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(C0),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(C1),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(C2),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(C3),
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(B)
    };

    for(int i = 0; i < (sizeof(statuswordflagtable) / sizeof(*statuswordflagtable)); i++)
    {
        if(scmp(string, statuswordflagtable[i].name))
            return statuswordflagtable[i].flag;
    }

    return 0;
}

/**
\brief Gets an x87 status flag from a string.
\param statusword The status word value.
\param string The flag name.
\return true if the flag is 1, false if the flag is 0.
*/
bool valx87statuswordflagfromstring(duint statusword, const char* string)
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

/**
\brief Gets the x87 control word flag AND value from a string.
\param string The name of the control word.
\return The value to AND the control word with to get the flag. 0 when not found.
*/
static unsigned int getx87controlwordflagfromstring(const char* string)
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

    for(int i = 0; i < (sizeof(controlwordflagtable) / sizeof(*controlwordflagtable)); i++)
    {
        if(scmp(string, controlwordflagtable[i].name))
            return controlwordflagtable[i].flag;
    }

    return 0;
}

/**
\brief Get an x87 control word flag from a string.
\param controlword The control word to get the flag from.
\param string The flag name.
\return true if the flag is 1, false when the flag is 0.
*/
bool valx87controlwordflagfromstring(duint controlword, const char* string)
{
    unsigned int flag = getx87controlwordflagfromstring(string);

    if(flag == 0)
        return false;

    return (bool)((int)(controlword & flag) != 0);
}

/**
\brief Gets the MXCSR field from a string.
\param mxcsrflags The mxcsrflags to get the field from.
\param string The name of the field (should be "RC").
\return The MXCSR field word.
*/
unsigned short valmxcsrfieldfromstring(duint mxcsrflags, const char* string)
{
    if(scmp(string, "RC"))
        return ((mxcsrflags & 0x6000) >> 13);

    return 0;
}

/**
\brief Gets the x87 status word field from a string.
\param statusword The status word to get the field from.
\param string The name of the field (should be "TOP").
\return The x87 status word field.
*/
unsigned short valx87statuswordfieldfromstring(duint statusword, const char* string)
{
    if(scmp(string, "TOP"))
        return ((statusword & 0x3800) >> 11);

    return 0;
}

/**
\brief Gets the x87 control word field from a string.
\param controlword The control word to get the field from.
\param string The name of the field.
\return The x87 control word field.
*/
unsigned short valx87controlwordfieldfromstring(duint controlword, const char* string)
{
    if(scmp(string, "PC"))
        return ((controlword & 0x300) >> 8);
    if(scmp(string, "RC"))
        return ((controlword & 0xC00) >> 10);

    return 0;
}

/**
\brief Gets a flag from a string.
\param eflags The eflags value to get the flag from.
\param string The name of the flag.
\return true if the flag equals to 1, false if the flag is 0 or not found.
*/
bool valflagfromstring(duint eflags, const char* string)
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

/**
\brief Sets a flag value.
\param string The name of the flag.
\param set The value of the flag.
\return true if the flag was successfully set, false otherwise.
*/
bool setflag(const char* string, bool set)
{
    duint eflags = GetContextDataEx(hActiveThread, UE_CFLAGS);
    duint xorval = 0;
    duint flag = 0;
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
    if(set)
        eflags |= flag;
    else
        eflags &= ~flag;
    return SetContextDataEx(hActiveThread, UE_CFLAGS, eflags);
}

/**
\brief Gets a register from a string.
\param [out] size This function can store the register size in bytes in this parameter. Can be null, in that case it will be ignored.
\param string The name of the register to get. Cannot be null.
\return The register value.
*/
duint getregister(int* size, const char* string)
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

    if(scmp(string, "lasterror"))
    {
        duint error = 0;
        MemReadUnsafe((duint)GetTEBLocation(hActiveThread) + ArchValue(0x34, 0x68), &error, 4);
        return error;
    }

    if(scmp(string, "laststatus"))
    {
        duint status = 0;
        MemReadUnsafe((duint)GetTEBLocation(hActiveThread) + ArchValue(0xBF4, 0x1250), &status, 4);
        return status;
    }

    if(size)
        *size = 2;
    if(scmp(string, "ax"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EAX);
        return val & 0xFFFF;
    }
    if(scmp(string, "bx"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EBX);
        return val & 0xFFFF;
    }
    if(scmp(string, "cx"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_ECX);
        return val & 0xFFFF;
    }
    if(scmp(string, "dx"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EDX);
        return val & 0xFFFF;
    }
    if(scmp(string, "si"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_ESI);
        return val & 0xFFFF;
    }
    if(scmp(string, "di"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EDI);
        return val & 0xFFFF;
    }
    if(scmp(string, "bp"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EBP);
        return val & 0xFFFF;
    }
    if(scmp(string, "sp"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_ESP);
        return val & 0xFFFF;
    }
    if(scmp(string, "ip"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EIP);
        return val & 0xFFFF;
    }

    if(size)
        *size = 1;
    if(scmp(string, "ah"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EAX);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "al"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EAX);
        return val & 0xFF;
    }
    if(scmp(string, "bh"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EBX);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "bl"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EBX);
        return val & 0xFF;
    }
    if(scmp(string, "ch"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_ECX);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "cl"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_ECX);
        return val & 0xFF;
    }
    if(scmp(string, "dh"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EDX);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "dl"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EDX);
        return val & 0xFF;
    }
    if(scmp(string, "sih"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_ESI);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "sil"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_ESI);
        return val & 0xFF;
    }
    if(scmp(string, "dih"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EDI);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "dil"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EDI);
        return val & 0xFF;
    }
    if(scmp(string, "bph"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EBP);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "bpl"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EBP);
        return val & 0xFF;
    }
    if(scmp(string, "sph"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_ESP);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "spl"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_ESP);
        return val & 0xFF;
    }
    if(scmp(string, "iph"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EIP);
        return (val >> 8) & 0xFF;
    }
    if(scmp(string, "ipl"))
    {
        duint val = GetContextDataEx(hActiveThread, UE_EIP);
        return val & 0xFF;
    }

    if(size)
        *size = sizeof(duint);
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
    if(scmp(string, "dr6") || scmp(string, "dr4"))
    {
        return GetContextDataEx(hActiveThread, UE_DR6);
    }
    if(scmp(string, "dr7") || scmp(string, "dr5"))
    {
        return GetContextDataEx(hActiveThread, UE_DR7);
    }

    if(scmp(string, "cax"))
    {
#ifdef _WIN64
        return GetContextDataEx(hActiveThread, UE_RAX);
#else
        return GetContextDataEx(hActiveThread, UE_EAX);
#endif //_WIN64
    }
    if(scmp(string, "cbx"))
    {
#ifdef _WIN64
        return GetContextDataEx(hActiveThread, UE_RBX);
#else
        return GetContextDataEx(hActiveThread, UE_EBX);
#endif //_WIN64
    }
    if(scmp(string, "ccx"))
    {
#ifdef _WIN64
        return GetContextDataEx(hActiveThread, UE_RCX);
#else
        return GetContextDataEx(hActiveThread, UE_ECX);
#endif //_WIN64
    }
    if(scmp(string, "cdx"))
    {
#ifdef _WIN64
        return GetContextDataEx(hActiveThread, UE_RDX);
#else
        return GetContextDataEx(hActiveThread, UE_EDX);
#endif //_WIN64
    }
    if(scmp(string, "csi"))
    {
#ifdef _WIN64
        return GetContextDataEx(hActiveThread, UE_RSI);
#else
        return GetContextDataEx(hActiveThread, UE_ESI);
#endif //_WIN64
    }
    if(scmp(string, "cdi"))
    {
#ifdef _WIN64
        return GetContextDataEx(hActiveThread, UE_RDI);
#else
        return GetContextDataEx(hActiveThread, UE_EDI);
#endif //_WIN64
    }
    if(scmp(string, "cip"))
    {
        return GetContextDataEx(hActiveThread, UE_CIP);
    }
    if(scmp(string, "csp"))
    {
        return GetContextDataEx(hActiveThread, UE_CSP);
    }
    if(scmp(string, "cbp"))
    {
#ifdef _WIN64
        return GetContextDataEx(hActiveThread, UE_RBP);
#else
        return GetContextDataEx(hActiveThread, UE_EBP);
#endif //_WIN64
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

/**
\brief Sets a register value based on the register name.
\param string The name of the register to set.
\param value The new register value.
\return true if the register was set, false otherwise.
*/
bool setregister(const char* string, duint value)
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

    if(scmp(string, "lasterror"))
        return MemWrite((duint)GetTEBLocation(hActiveThread) + ArchValue(0x34, 0x68), &value, 4);
    if(scmp(string, "laststatus"))
        return MemWrite((duint)GetTEBLocation(hActiveThread) + ArchValue(0xBF4, 0x1250), &value, 4);

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
    if(scmp(string, "dr6") || scmp(string, "dr4"))
        return SetContextDataEx(hActiveThread, UE_DR6, value);
    if(scmp(string, "dr7") || scmp(string, "dr5"))
        return SetContextDataEx(hActiveThread, UE_DR7, value);

    if(scmp(string, "cax"))
#ifdef _WIN64
        return SetContextDataEx(hActiveThread, UE_RAX, value);
#else
        return SetContextDataEx(hActiveThread, UE_EAX, value);
#endif //_WIN64
    if(scmp(string, "cbx"))
#ifdef _WIN64
        return SetContextDataEx(hActiveThread, UE_RBX, value);
#else
        return SetContextDataEx(hActiveThread, UE_EBX, value);
#endif //_WIN64
    if(scmp(string, "ccx"))
#ifdef _WIN64
        return SetContextDataEx(hActiveThread, UE_RCX, value);
#else
        return SetContextDataEx(hActiveThread, UE_ECX, value);
#endif //_WIN64
    if(scmp(string, "cdx"))
#ifdef _WIN64
        return SetContextDataEx(hActiveThread, UE_RDX, value);
#else
        return SetContextDataEx(hActiveThread, UE_EDX, value);
#endif //_WIN64
    if(scmp(string, "csi"))
#ifdef _WIN64
        return SetContextDataEx(hActiveThread, UE_RSI, value);
#else
        return SetContextDataEx(hActiveThread, UE_ESI, value);
#endif //_WIN64
    if(scmp(string, "cdi"))
#ifdef _WIN64
        return SetContextDataEx(hActiveThread, UE_RDI, value);
#else
        return SetContextDataEx(hActiveThread, UE_EDI, value);
#endif //_WIN64
    if(scmp(string, "cip"))
        return SetContextDataEx(hActiveThread, UE_CIP, value);
    if(scmp(string, "csp"))
        return SetContextDataEx(hActiveThread, UE_CSP, value);
    if(scmp(string, "cbp"))
#ifdef _WIN64
        return SetContextDataEx(hActiveThread, UE_RBP, value);
#else
        return SetContextDataEx(hActiveThread, UE_EBP, value);
#endif //_WIN64
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

duint SafeGetProcAddress(HMODULE hModule, const char* lpProcName)
{
    __try
    {
        return duint(GetProcAddress(hModule, lpProcName));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

/**
\brief Gets the address of an API from a name.
\param name The name of the API, see the command help for more information about valid constructions.
\param [out] value The address of the retrieved API. Cannot be null.
\param [out] value_size This function sets this value to the size of the address, sizeof(duint).
\param printall true to print all possible API values to the console.
\param silent true to have no console output. If true, the \p printall parameter is ignored.
\param [out] hexonly If set to true, the values should be printed in hex only. Usually this function sets it to true.
\return true if the API was found and a value retrieved, false otherwise.
*/
bool valapifromstring(const char* name, duint* value, int* value_size, bool printall, bool silent, bool* hexonly)
{
    if(!value || !DbgIsDebugging())
        return false;
    //explicit API handling
    const char* apiname = strchr(name, ':'); //the ':' character cannot be in a path: https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx#naming_conventions
    bool noexports = false;
    if(!apiname) //not found
    {
        apiname = strstr(name, "..") ? strchr(name, '.') : strrchr(name, '.'); //kernel32.GetProcAddress support
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
            if(!ModNameFromAddr(seldata.start, modname, true))
                return false;
        }
        else
        {
            strncpy_s(modname, name, _TRUNCATE);
            auto idx = apiname - name;
            if(idx < _countof(modname))
                modname[idx] = '\0';
        }
        apiname++;
        if(!strlen(apiname))
            return false;
        duint modbase = ModBaseFromName(modname);
        char szModPath[MAX_PATH];
        if(!ModPathFromAddr(modbase, szModPath, _countof(szModPath)))
        {
            if(!silent)
                dprintf(QT_TRANSLATE_NOOP("DBG", "Could not get filename of module %p\n"), modbase);
        }
        else
        {
            HMODULE mod = LoadLibraryExW(StringUtils::Utf8ToUtf16(szModPath).c_str(), 0, DONT_RESOLVE_DLL_REFERENCES);
            if(!mod)
            {
                if(!silent)
                    dprintf(QT_TRANSLATE_NOOP("DBG", "Unable to load library %s\n"), szModPath);
            }
            else
            {
                duint addr = noexports ? 0 : SafeGetProcAddress(mod, apiname);
                if(addr) //found exported function
                    addr = modbase + (addr - (duint)mod); //correct for loaded base
                else //not found
                {
                    if(scmp(apiname, "base") || scmp(apiname, "imagebase") || scmp(apiname, "header")) //get loaded base
                        addr = modbase;
                    else if(scmp(apiname, "entrypoint") || scmp(apiname, "entry") || scmp(apiname, "oep") || scmp(apiname, "ep")) //get entry point
                        addr = ModEntryFromAddr(modbase);
                    else if(*apiname == '$') //RVA
                    {
                        duint rva;
                        if(valfromstring(apiname + 1, &rva))
                            addr = modbase + rva;
                    }
                    else if(*apiname == '#') //File Offset
                    {
                        duint offset;
                        if(valfromstring(apiname + 1, &offset))
                            addr = valfileoffsettova(modname, offset);
                    }
                    else
                    {
                        if(noexports) //get the exported functions with the '?' delimiter
                        {
                            addr = SafeGetProcAddress(mod, apiname);
                            if(addr) //found exported function
                                addr = modbase + (addr - (duint)mod); //correct for loaded base
                        }
                        else
                        {
                            duint ordinal;
                            auto radix = 16;
                            if(*apiname == '.') //decimal
                                radix = 10, apiname++;
                            if(convertNumber(apiname, ordinal, radix) && ordinal <= 0xFFFF)
                            {
                                addr = SafeGetProcAddress(mod, LPCSTR(ordinal));
                                if(addr) //found exported function
                                    addr = modbase + (addr - (duint)mod); //correct for loaded base
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
                        *value_size = sizeof(duint);
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
    Memory<duint*> addrfound;
    if(EnumProcessModules(fdProcessInfo->hProcess, 0, 0, &cbNeeded))
    {
        addrfound.realloc(cbNeeded * sizeof(duint), "valapifromstring:addrfound");
        Memory<HMODULE*> hMods(cbNeeded * sizeof(HMODULE), "valapifromstring:hMods");
        if(EnumProcessModules(fdProcessInfo->hProcess, hMods(), cbNeeded, &cbNeeded))
        {
            for(unsigned int i = 0; i < cbNeeded / sizeof(HMODULE); i++)
            {
                wchar_t szModuleName[MAX_PATH] = L"";
                if(GetModuleFileNameExW(fdProcessInfo->hProcess, hMods()[i], szModuleName, MAX_PATH))
                {
                    wchar_t* szBaseName = wcsrchr(szModuleName, L'\\');
                    if(szBaseName)
                    {
                        szBaseName++;
                        HMODULE hModule = LoadLibraryExW(szModuleName, 0, DONT_RESOLVE_DLL_REFERENCES);
                        if(hModule)
                        {
                            duint funcAddress = SafeGetProcAddress(hModule, name);
                            if(funcAddress)
                            {
                                if(!_wcsicmp(szBaseName, L"kernel32.dll"))
                                    kernel32 = found;
                                duint rva = funcAddress - (duint)hModule;
                                addrfound()[found] = (duint)hMods()[i] + rva;
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
        *value_size = sizeof(duint);
    if(hexonly)
        *hexonly = true;
    if(kernel32 != -1) //prioritize kernel32 exports
    {
        *value = addrfound()[kernel32];
        if(!printall || silent)
            return true;
        for(int i = 0; i < found; i++)
            if(i != kernel32)
            {
                auto symbolic = SymGetSymbolicName(addrfound()[i]);
                if(symbolic.length())
                    dprintf_untranslated("%p %s\n", addrfound()[i], symbolic.c_str());
                else
                    dprintf_untranslated("%p\n", addrfound()[i]);
            }
    }
    else
    {
        *value = *addrfound();
        if(!printall || silent)
            return true;
        for(int i = 1; i < found; i++)
        {
            auto symbolic = SymGetSymbolicName(addrfound()[i]);
            if(symbolic.length())
                dprintf_untranslated("%p %s\n", addrfound()[i], symbolic.c_str());
            else
                dprintf_untranslated("%p\n", addrfound()[i]);
        }
    }
    return true;
}

/**
\brief Check if a string is a valid decimal number. This function also accepts "-" or "." as prefix.
\param string The string to check.
\return true if the string is a valid decimal number.
*/
static bool isdecnumber(const char* string)
{
    if(*string != '.' || !string[1]) //dec indicator/no number
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

/**
\brief Check if a string is a valid hexadecimal number. This function also accepts "0x" or "x" as prefix.
\param string The string to check.
\return true if the string is a valid hexadecimal number.
*/
static bool ishexnumber(const char* string)
{
    int add = 0;
    if(*string == '0' && string[1] == 'x') //0x prefix
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

bool convertNumber(const char* str, duint & result, int radix)
{
    unsigned long long llr;
    if(!convertLongLongNumber(str, llr, radix))
        return false;
    result = duint(llr);
    return true;
}

bool convertLongLongNumber(const char* str, unsigned long long & result, int radix)
{
    errno = 0;
    char* end;
    result = strtoull(str, &end, radix);
    if(!result && end == str)
        return false;
    if(result == ULLONG_MAX && errno)
        return false;
    if(*end)
        return false;
    return true;
}

/**
\brief Check if a character is a valid hexadecimal digit that is smaller than the size of a pointer.
\param digit The character to check.
\return true if the character is a valid hexadecimal digit.
*/
static bool isdigitduint(char digit)
{
#ifdef _WIN64
    return digit >= '1' && digit <= '8';
#else //x86
    return digit >= '1' && digit <= '4';
#endif //_WIN64
}

/**
\brief Gets a value from a string. This function can parse expressions, memory locations, registers, flags, API names, labels, symbols and variables.
\param string The string to parse.
\param [out] value The value of the expression. This value cannot be null.
\param silent true to not output anything to the console.
\param baseonly true to skip parsing API names, labels and symbols (basic expressions only).
\param [out] value_size This function can output the value size parsed (for example memory location size or register size). Can be null.
\param [out] isvar This function can output if the expression is variable (for example memory locations, registers or variables are variable). Can be null.
\param [out] hexonly This function can output if the output value should only be printed as hexadecimal (for example addresses). Can be null.
\return true if the expression was parsed successful, false otherwise.
*/
bool valfromstring_noexpr(const char* string, duint* value, bool silent, bool baseonly, int* value_size, bool* isvar, bool* hexonly)
{
    if(!value || !string || !*string)
        return false;

    if(string[0] == '['
            || (isdigitduint(string[0]) && string[1] == ':' && string[2] == '[')
            || (string[1] == 's' && (string[0] == 'c' || string[0] == 'd' || string[0] == 'e' || string[0] == 'f' || string[0] == 'g' || string[0] == 's') && string[2] == ':' && string[3] == '[') //memory location
            || strstr(string, "byte:[")
            || strstr(string, "word:[")
      )
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs(QT_TRANSLATE_NOOP("DBG", "Not debugging"));
            *value = 0;
            if(value_size)
                *value_size = 0;
            if(isvar)
                *isvar = true;
            return true;
        }
        int len = (int)strlen(string);

        int read_size = sizeof(duint);
        int prefix_size = 1;
        size_t seg_offset = 0;
        if(string[1] == ':') //n:[ (number of bytes to read)
        {
            prefix_size = 3;
            int new_size = string[0] - '0';
            if(new_size < read_size)
                read_size = new_size;
        }
        else if(string[1] == 's' && string[2] == ':')
        {
            prefix_size = 4;
            if(string[0] == 'f') // fs:[...]
            {
                // TODO: get real segment offset instead of assuming them
#ifdef _WIN64
                seg_offset = 0;
#else //x86
                seg_offset = (size_t)GetTEBLocation(hActiveThread);
#endif //_WIN64
            }
            else if(string[0] == 'g') // gs:[...]
            {
#ifdef _WIN64
                seg_offset = (size_t)GetTEBLocation(hActiveThread);
#else //x86
                seg_offset = 0;
#endif //_WIN64
            }
        }
        else if(string[0] == 'b'
                && string[1] == 'y'
                && string[2] == 't'
                && string[3] == 'e'
                && string[4] == ':'
               ) // byte:[...]
        {
            prefix_size = 6;
            int new_size = 1;
            if(new_size < read_size)
                read_size = new_size;
        }
        else if(string[0] == 'w'
                && string[1] == 'o'
                && string[2] == 'r'
                && string[3] == 'd'
                && string[4] == ':'
               ) // word:[...]
        {
            prefix_size = 6;
            int new_size = 2;
            if(new_size < read_size)
                read_size = new_size;
        }
        else if(string[0] == 'd'
                && string[1] == 'w'
                && string[2] == 'o'
                && string[3] == 'r'
                && string[4] == 'd'
                && string[5] == ':'
               ) // dword:[...]
        {
            prefix_size = 7;
            int new_size = 4;
            if(new_size < read_size)
                read_size = new_size;
        }
#ifdef _WIN64
        else if(string[0] == 'q'
                && string[1] == 'w'
                && string[2] == 'o'
                && string[3] == 'r'
                && string[4] == 'd'
                && string[5] == ':'
               ) // qword:[...]
        {
            prefix_size = 7;
            int new_size = 8;
            if(new_size < read_size)
                read_size = new_size;
        }
#endif //_WIN64

        String ptrstring;
        for(auto i = prefix_size, depth = 1; i < len; i++)
        {
            if(string[i] == '[')
                depth++;
            else if(string[i] == ']')
            {
                depth--;
                if(!depth)
                    break;
            }
            ptrstring += string[i];
        }

        if(!valfromstring(ptrstring.c_str(), value, silent))
        {
            if(!silent)
                dprintf(QT_TRANSLATE_NOOP("DBG", "valfromstring_noexpr failed on %s\n"), ptrstring.c_str());
            return false;
        }
        duint addr = *value;
        *value = 0;
        if(!MemRead(addr + seg_offset, value, read_size))
        {
            if(!silent)
                dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read memory"));
            return false;
        }
        if(value_size)
            *value_size = read_size;
        if(isvar)
            *isvar = true;
        return true;
    }
    else if(varget(string, value, value_size, 0)) //then come variables
    {
        if(isvar)
            *isvar = true;
        return true;
    }
    else if(isregister(string)) //register
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs(QT_TRANSLATE_NOOP("DBG", "Not debugging!"));
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
    else if(*string == '_' && isflag(string + 1)) //flag
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs(QT_TRANSLATE_NOOP("DBG", "Not debugging"));
            *value = 0;
            if(value_size)
                *value_size = 0;
            if(isvar)
                *isvar = true;
            return true;
        }
        duint eflags = GetContextDataEx(hActiveThread, UE_CFLAGS);
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
        return convertNumber(string + 1, *value, 10);
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
        return convertNumber(string + inc, *value, 16);
    }

    if(isvar)
        *isvar = false;
    if(hexonly)
        *hexonly = true;
    if(value_size)
        *value_size = sizeof(duint);

    if(ConstantFromName(string, *value))
        return true;

    PLUG_CB_VALFROMSTRING info;
    info.string = string;
    info.value = 0;
    info.value_size = value_size;
    info.isvar = isvar;
    info.hexonly = hexonly;
    info.retval = false;
    plugincbcall(CB_VALFROMSTRING, &info);
    if(info.retval)
    {
        *value = info.value;
        return true;
    }

    if(baseonly)
        return false;

    if(valapifromstring(string, value, value_size, true, silent, hexonly)) //then come APIs
        return true;
    else if(LabelFromString(string, value)) //then come labels
        return true;
    else if(SymAddrFromName(string, value)) //then come symbols
        return true;
    else if(strstr(string, "sub_") == string) //then come sub_ functions
    {
#ifdef _WIN64
        bool result = sscanf_s(string, "sub_%llX", value) == 1;
#else //x86
        bool result = sscanf_s(string, "sub_%X", value) == 1;
#endif //_WIN64
        duint start;
        return result && FunctionGet(*value, &start, nullptr) && *value == start;
    }

    if(!silent)
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid value: \"%s\"!\n"), string);
    return false; //nothing was OK
}

/**
\brief Gets a value from a string. This function can parse expressions, memory locations, registers, flags, API names, labels, symbols and variables.
\param string The string to parse.
\param [out] value The value of the expression. This value cannot be null. When the expression is invalid, value is not changed.
\param silent true to not output anything to the console.
\param baseonly true to skip parsing API names, labels, symbols and variables (basic expressions only).
\param [out] value_size This function can output the value size parsed (for example memory location size or register size). Can be null.
\param [out] isvar This function can output if the expression is variable (for example memory locations, registers or variables are variable). Can be null.
\param [out] hexonly This function can output if the output value should only be printed as hexadecimal (for example addresses). Can be null.
\return true if the expression was parsed successful, false otherwise.
*/
bool valfromstring(const char* string, duint* value, bool silent, bool baseonly, int* value_size, bool* isvar, bool* hexonly, bool allowassign)
{
    if(!value || !string)
        return false;
    if(!*string)
    {
        *value = 0;
        return true;
    }
    ExpressionParser parser(string);
    duint result;
    if(!parser.Calculate(result, valuesignedcalc(), allowassign, silent, baseonly, value_size, isvar, hexonly))
        return false;
    *value = result;
    return true;
}

/**
\brief Checks if a string is long enough.
\param str The string to check.
\param min_length The minimum length of \p str.
\return true if the string is long enough, false otherwise.
*/
static bool longEnough(const char* str, size_t min_length)
{
    size_t length = 0;
    while(length < min_length && str[length])
        length++;
    if(length == min_length)
        return true;
    return false;
}

/**
\brief Checks if a string starts with another string.
\param pre The desired prefix of the string.
\param str The complete string.
\return true if \p str starts with \p pre.
*/
static bool startsWith(const char* pre, const char* str)
{
    size_t lenpre = strlen(pre);
    return longEnough(str, lenpre) ? _strnicmp(str, pre, (int) lenpre) == 0 : false;
}

#define MxCsr_PRE_FIELD_STRING "MxCsr_"
#define x87SW_PRE_FIELD_STRING "x87SW_"
#define x87CW_PRE_FIELD_STRING "x87CW_"
#define x87TW_PRE_FIELD_STRING "x87TW_"
#define MMX_PRE_FIELD_STRING "MM"
#define XMM_PRE_FIELD_STRING "XMM"
#define YMM_PRE_FIELD_STRING "YMM"
#define x8780BITFPU_PRE_FIELD_STRING "x87r"
#define x8780BITFPU_PRE_FIELD_STRING_ST "st"
#define STRLEN_USING_SIZEOF(string) (sizeof(string) - 1)

/**
\brief Sets an FPU value (MXCSR fields, MMX fields, etc.) by name.
\param string The name of the FPU value to set.
\param value The value to set.
*/
static void setfpuvalue(const char* string, duint value)
{
    duint xorval = 0;
    duint flags = 0;
    duint flag = 0;
    bool set = false;

    if(value)
        set = true;

    if(startsWith(MxCsr_PRE_FIELD_STRING, string))
    {
        if(_strnicmp(string + STRLEN_USING_SIZEOF(MxCsr_PRE_FIELD_STRING), "RC", (int)strlen("RC")) == 0)
        {
            flags = GetContextDataEx(hActiveThread, UE_MXCSR);
            int i = 3;
            i <<= 13;
            flags &= ~i;
            value <<= 13;
            flags |= value;
            SetContextDataEx(hActiveThread, UE_MXCSR, flags);
        }
        else
        {
            flags = GetContextDataEx(hActiveThread, UE_MXCSR);
            flag = getmxcsrflagfromstring(string + STRLEN_USING_SIZEOF(MxCsr_PRE_FIELD_STRING));
            if(flags & flag && !set)
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
        if(_strnicmp(string + STRLEN_USING_SIZEOF(x87SW_PRE_FIELD_STRING), "TOP", (int)strlen("TOP")) == 0)
        {
            flags = GetContextDataEx(hActiveThread, UE_X87_STATUSWORD);
            int i = 7;
            i <<= 11;
            flags &= ~i;
            value <<= 11;
            flags |= value;
            SetContextDataEx(hActiveThread, UE_X87_STATUSWORD, flags);
        }
        else
        {
            flags = GetContextDataEx(hActiveThread, UE_X87_STATUSWORD);
            flag = getx87statuswordflagfromstring(string + STRLEN_USING_SIZEOF(x87SW_PRE_FIELD_STRING));
            if(flags & flag && !set)
                xorval = flag;
            else if(set)
                xorval = flag;
            SetContextDataEx(hActiveThread, UE_X87_STATUSWORD, flags ^ xorval);
        }
    }
    else if(startsWith(x87CW_PRE_FIELD_STRING, string))
    {
        if(_strnicmp(string + STRLEN_USING_SIZEOF(x87CW_PRE_FIELD_STRING), "RC", (int)strlen("RC")) == 0)
        {
            flags = GetContextDataEx(hActiveThread, UE_X87_CONTROLWORD);
            int i = 3;
            i <<= 10;
            flags &= ~i;
            value <<= 10;
            flags |= value;
            SetContextDataEx(hActiveThread, UE_X87_CONTROLWORD, flags);
        }
        else if(_strnicmp(string + STRLEN_USING_SIZEOF(x87CW_PRE_FIELD_STRING), "PC", (int)strlen("PC")) == 0)
        {
            flags = GetContextDataEx(hActiveThread, UE_X87_CONTROLWORD);
            int i = 3;
            i <<= 8;
            flags &= ~i;
            value <<= 8;
            flags |= value;
            SetContextDataEx(hActiveThread, UE_X87_CONTROLWORD, flags);
        }
        else
        {
            flags = GetContextDataEx(hActiveThread, UE_X87_CONTROLWORD);
            flag = getx87controlwordflagfromstring(string + STRLEN_USING_SIZEOF(x87CW_PRE_FIELD_STRING));
            if(flags & flag && !set)
                xorval = flag;
            else if(set)
                xorval = flag;
            SetContextDataEx(hActiveThread, UE_X87_CONTROLWORD, flags ^ xorval);
        }
    }
    else if(_strnicmp(string, "x87TagWord", (int)strlen(string)) == 0)
    {
        SetContextDataEx(hActiveThread, UE_X87_TAGWORD, (unsigned short) value);
    }
    else if(_strnicmp(string, "x87StatusWord", (int)strlen(string)) == 0)
    {
        SetContextDataEx(hActiveThread, UE_X87_STATUSWORD, (unsigned short) value);
    }
    else if(_strnicmp(string, "x87ControlWord", (int)strlen(string)) == 0)
    {
        SetContextDataEx(hActiveThread, UE_X87_CONTROLWORD, (unsigned short) value);
    }
    else if(_strnicmp(string, "MxCsr", (int)strlen(string)) == 0)
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
    else if(startsWith(x8780BITFPU_PRE_FIELD_STRING_ST, string))
    {
        flags = GetContextDataEx(hActiveThread, UE_X87_STATUSWORD);
        flags >>= 11;
        flags &= 7;
        string += STRLEN_USING_SIZEOF(x8780BITFPU_PRE_FIELD_STRING_ST);
        bool found = true;
        DWORD registerindex;
        switch(*string)
        {
        case '0':
            registerindex = (DWORD)flags;
            break;

        case '1':
            registerindex = ((1 + flags) & 7);
            break;

        case '2':
            registerindex = ((2 + flags) & 7);
            break;

        case '3':
            registerindex = ((3 + flags) & 7);
            break;

        case '4':
            registerindex = ((4 + flags) & 7);
            break;

        case '5':
            registerindex = ((5 + flags) & 7);
            break;

        case '6':
            registerindex = ((6 + flags) & 7);
            break;

        case '7':
            registerindex = ((7 + flags) & 7);
            break;

        default:
            found = false;
            break;
        }
        if(found)
        {
            registerindex += UE_x87_r0;
            SetContextDataEx(hActiveThread, registerindex, value);
        }
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
#ifdef _WIN64
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
#endif
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
#ifdef _WIN64
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
#endif
        default:
            registerindex = 0;
            found = false;
            break;
        }
        if(found)
            SetContextDataEx(hActiveThread, registerindex, value);
    }
}

/**
\brief Sets a register, variable, flag, memory location or FPU value by name.
\param string The name of the thing to set.
\param value The value to set.
\param silent true to not have output to the console.
\return true if the value was set successfully, false otherwise.
*/
bool valtostring(const char* string, duint value, bool silent)
{
    if(!*string)
        return false;
    if(string[0] == '['
            || (isdigitduint(string[0]) && string[1] == ':' && string[2] == '[')
            || (string[1] == 's' && (string[0] == 'c' || string[0] == 'd' || string[0] == 'e' || string[0] == 'f' || string[0] == 'g' || string[0] == 's') && string[2] == ':' && string[3] == '[') //memory location
            || strstr(string, "byte:[")
            || strstr(string, "word:[")
      )
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs(QT_TRANSLATE_NOOP("DBG", "Not debugging"));
            return false;
        }
        int len = (int)strlen(string);

        int read_size = sizeof(duint);
        int prefix_size = 1;
        size_t seg_offset = 0;
        if(string[1] == ':') //n:[ (number of bytes to read)
        {
            prefix_size = 3;
            int new_size = string[0] - '0';
            if(new_size < read_size)
                read_size = new_size;
        }
        else if(string[1] == 's' && string[2] == ':')
        {
            prefix_size = 4;
            if(string[0] == 'f') // fs:[...]
            {
                // TODO: get real segment offset instead of assuming them
#ifdef _WIN64
                seg_offset = 0;
#else //x86
                seg_offset = (size_t)GetTEBLocation(hActiveThread);
#endif //_WIN64
            }
            else if(string[0] == 'g') // gs:[...]
            {
#ifdef _WIN64
                seg_offset = (size_t)GetTEBLocation(hActiveThread);
#else //x86
                seg_offset = 0;
#endif //_WIN64
            }
        }
        else if(string[0] == 'b'
                && string[1] == 'y'
                && string[2] == 't'
                && string[3] == 'e'
                && string[4] == ':'
               ) // byte:[...]
        {
            prefix_size = 6;
            int new_size = 1;
            if(new_size < read_size)
                read_size = new_size;
        }
        else if(string[0] == 'w'
                && string[1] == 'o'
                && string[2] == 'r'
                && string[3] == 'd'
                && string[4] == ':'
               ) // word:[...]
        {
            prefix_size = 6;
            int new_size = 2;
            if(new_size < read_size)
                read_size = new_size;
        }
        else if(string[0] == 'd'
                && string[1] == 'w'
                && string[2] == 'o'
                && string[3] == 'r'
                && string[4] == 'd'
                && string[5] == ':'
               ) // dword:[...]
        {
            prefix_size = 7;
            int new_size = 4;
            if(new_size < read_size)
                read_size = new_size;
        }
#ifdef _WIN64
        else if(string[0] == 'q'
                && string[1] == 'w'
                && string[2] == 'o'
                && string[3] == 'r'
                && string[4] == 'd'
                && string[5] == ':'
               ) // qword:[...]
        {
            prefix_size = 7;
            int new_size = 8;
            if(new_size < read_size)
                read_size = new_size;
        }
#endif //_WIN64

        String ptrstring;
        for(auto i = prefix_size, depth = 1; i < len; i++)
        {
            if(string[i] == '[')
                depth++;
            else if(string[i] == ']')
            {
                depth--;
                if(!depth)
                    break;
            }
            ptrstring += string[i];
        }

        duint temp;
        if(!valfromstring(ptrstring.c_str(), &temp, silent))
            return false;
        duint value_ = value;
        if(!MemPatch(temp + seg_offset, &value_, read_size))
        {
            if(!silent)
                dputs(QT_TRANSLATE_NOOP("DBG", "Failed to write memory"));
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
                dputs(QT_TRANSLATE_NOOP("DBG", "Not debugging!"));
            return false;
        }
        bool ok = setregister(string, value);
        int len = (int)strlen(string);
        Memory<char*> regName(len + 1, "valtostring:regname");
        strcpy_s(regName(), len + 1, string);
        _strlwr_s(regName(), regName.size());
        if(strstr(regName(), "ip"))
        {
            auto cip = GetContextDataEx(hActiveThread, UE_CIP);
            _dbg_dbgtraceexecute(cip);
            DebugUpdateGuiAsync(cip, false); //update disassembly + register view
        }
        else if(strstr(regName(), "sp")) //update stack
        {
            duint csp = GetContextDataEx(hActiveThread, UE_CSP);
            DebugUpdateStack(csp, csp);
            GuiUpdateRegisterView();
        }
        else
            GuiUpdateAllViews(); //repaint gui
        return ok;
    }
    else if(*string == '_' && isflag(string + 1)) //flag
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs(QT_TRANSLATE_NOOP("DBG", "Not debugging"));
            return false;
        }
        bool set = false;
        if(value)
            set = true;
        setflag(string + 1, set);
        GuiUpdateAllViews(); //repaint gui
        return true;
    }
    else if((*string == '_')) //FPU values
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs(QT_TRANSLATE_NOOP("DBG", "Not debugging!"));
            return false;
        }
        setfpuvalue(string + 1, value);
        GuiUpdateAllViews(); //repaint gui
        return true;
    }

    PLUG_CB_VALTOSTRING info;
    info.string = string;
    info.value = value;
    info.retval = false;
    plugincbcall(CB_VALTOSTRING, &info);
    if(info.retval)
        return true;

    return varset(string, value, false); //variable
}

/**
\brief Converts a file offset to a virtual address.
\param modname The name (not the path) of the module the file offset is in.
\param offset The file offset.
\return The VA of the file offset, 0 when there was a problem with the conversion.
*/
duint valfileoffsettova(const char* modname, duint offset)
{
    SHARED_ACQUIRE(LockModules);
    const auto modInfo = ModInfoFromAddr(ModBaseFromName(modname));
    if(modInfo && modInfo->fileMapVA)
    {
        ULONGLONG rva = ConvertFileOffsetToVA(modInfo->fileMapVA, //FileMapVA
                                              modInfo->fileMapVA + (ULONG_PTR)offset, //Offset inside FileMapVA
                                              false); //Return without ImageBase
        return offset < modInfo->loadedSize ? (duint)rva + ModBaseFromName(modname) : 0;
    }
    return 0;
}

/**
\brief Converts a virtual address to a file offset.
\param va The virtual address (must be inside a module).
\return The file offset. 0 when there was a problem with the conversion.
*/
duint valvatofileoffset(duint va)
{
    SHARED_ACQUIRE(LockModules);
    const auto modInfo = ModInfoFromAddr(va);
    if(modInfo && modInfo->fileMapVA)
    {
        ULONGLONG offset = ConvertVAtoFileOffsetEx(modInfo->fileMapVA, modInfo->loadedSize, 0, va - modInfo->base, true, false);
        return (duint)offset;
    }
    return 0;
}
