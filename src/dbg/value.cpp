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

// Macro for converting a static identifier with no more than 4 characters into an uppercased int for efficient comparison
#define MAKE_WORD_INTO_INT(word) (#word[1]==0 ? #word[0] : (#word[2]==0 ? (#word[0]|(#word[1]<<8)) : (#word[3]==0 ? (#word[0]|(#word[1]<<8)|(#word[2]<<16)) : (#word[4]==0 ? (#word[0]|(#word[1]<<8)|(#word[2]<<16)|(#word[3]<<24)) : 0))))

/**
\brief Convert a string with no more than 4 characters into an uppercased int for efficient comparison
*/
int read_string_4char_ucase(const char* string)
{
    int a = 0;
    char x = 'a' - 'A';
    if(string[0] != '\0')
        a = (string[0] >= 'a' && string[0] <= 'z' ? string[0] - x : string[0]);
    else
        return a;
    if(string[1] != '\0')
        a |= (string[1] >= 'a' && string[1] <= 'z' ? string[1] - x : string[1]) << 8;
    else
        return a;
    if(string[2] != '\0')
        a |= (string[2] >= 'a' && string[2] <= 'z' ? string[2] - x : string[2]) << 16;
    else
        return a;
    if(string[3] != '\0')
        a |= (string[3] >= 'a' && string[3] <= 'z' ? string[3] - x : string[3]) << 24;
    else
        return a;
    if(string[4] != '\0')  // overflow
        return 0;
    else
        return a;
}

/**
\brief Check if a string is a flag.
\param string The string to check.
\return true if the string is a flag, false otherwise.
*/
static bool isflag(const char* string)
{
    switch(read_string_4char_ucase(string))
    {
    case MAKE_WORD_INTO_INT(CF):
    case MAKE_WORD_INTO_INT(PF):
    case MAKE_WORD_INTO_INT(AF):
    case MAKE_WORD_INTO_INT(ZF):
    case MAKE_WORD_INTO_INT(SF):
    case MAKE_WORD_INTO_INT(TF):
    case MAKE_WORD_INTO_INT(IF):
    case MAKE_WORD_INTO_INT(DF):
    case MAKE_WORD_INTO_INT(OF):
    case MAKE_WORD_INTO_INT(RF):
    case MAKE_WORD_INTO_INT(VM):
    case MAKE_WORD_INTO_INT(AC):
    case MAKE_WORD_INTO_INT(VIF):
    case MAKE_WORD_INTO_INT(VIP):
    case MAKE_WORD_INTO_INT(ID):
        return true;
    default:
        return false;
    }
}

/**
\brief Check if a string is a register.
\param string The string to check.
\return true if the string is a register, false otherwise.
*/
static bool isregister(const char* string)
{
    switch(read_string_4char_ucase(string))
    {
    case MAKE_WORD_INTO_INT(EAX):
    case MAKE_WORD_INTO_INT(EBX):
    case MAKE_WORD_INTO_INT(ECX):
    case MAKE_WORD_INTO_INT(EDX):
    case MAKE_WORD_INTO_INT(EDI):
    case MAKE_WORD_INTO_INT(ESI):
    case MAKE_WORD_INTO_INT(EBP):
    case MAKE_WORD_INTO_INT(ESP):
    case MAKE_WORD_INTO_INT(EIP):
    case MAKE_WORD_INTO_INT(AX):
    case MAKE_WORD_INTO_INT(BX):
    case MAKE_WORD_INTO_INT(CX):
    case MAKE_WORD_INTO_INT(DX):
    case MAKE_WORD_INTO_INT(SI):
    case MAKE_WORD_INTO_INT(DI):
    case MAKE_WORD_INTO_INT(BP):
    case MAKE_WORD_INTO_INT(SP):
    case MAKE_WORD_INTO_INT(IP):
    case MAKE_WORD_INTO_INT(AH):
    case MAKE_WORD_INTO_INT(AL):
    case MAKE_WORD_INTO_INT(BH):
    case MAKE_WORD_INTO_INT(BL):
    case MAKE_WORD_INTO_INT(CH):
    case MAKE_WORD_INTO_INT(CL):
    case MAKE_WORD_INTO_INT(DH):
    case MAKE_WORD_INTO_INT(DL):
    case MAKE_WORD_INTO_INT(SIH):
    case MAKE_WORD_INTO_INT(SIL):
    case MAKE_WORD_INTO_INT(DIH):
    case MAKE_WORD_INTO_INT(DIL):
    case MAKE_WORD_INTO_INT(BPH):
    case MAKE_WORD_INTO_INT(BPL):
    case MAKE_WORD_INTO_INT(SPH):
    case MAKE_WORD_INTO_INT(SPL):
    case MAKE_WORD_INTO_INT(IPH):
    case MAKE_WORD_INTO_INT(IPL):
    case MAKE_WORD_INTO_INT(DR0):
    case MAKE_WORD_INTO_INT(DR1):
    case MAKE_WORD_INTO_INT(DR2):
    case MAKE_WORD_INTO_INT(DR3):
    case MAKE_WORD_INTO_INT(DR6):
    case MAKE_WORD_INTO_INT(DR4):
    case MAKE_WORD_INTO_INT(DR7):
    case MAKE_WORD_INTO_INT(DR5):
    case MAKE_WORD_INTO_INT(CAX):
    case MAKE_WORD_INTO_INT(CBX):
    case MAKE_WORD_INTO_INT(CCX):
    case MAKE_WORD_INTO_INT(CDX):
    case MAKE_WORD_INTO_INT(CSI):
    case MAKE_WORD_INTO_INT(CDI):
    case MAKE_WORD_INTO_INT(CIP):
    case MAKE_WORD_INTO_INT(CSP):
    case MAKE_WORD_INTO_INT(CBP):
    case MAKE_WORD_INTO_INT(GS):
    case MAKE_WORD_INTO_INT(FS):
    case MAKE_WORD_INTO_INT(ES):
    case MAKE_WORD_INTO_INT(DS):
    case MAKE_WORD_INTO_INT(CS):
    case MAKE_WORD_INTO_INT(SS):
#ifdef _WIN64
    case MAKE_WORD_INTO_INT(RAX):
    case MAKE_WORD_INTO_INT(RBX):
    case MAKE_WORD_INTO_INT(RCX):
    case MAKE_WORD_INTO_INT(RDX):
    case MAKE_WORD_INTO_INT(RDI):
    case MAKE_WORD_INTO_INT(RSI):
    case MAKE_WORD_INTO_INT(RBP):
    case MAKE_WORD_INTO_INT(RSP):
    case MAKE_WORD_INTO_INT(RIP):
    case MAKE_WORD_INTO_INT(R8):
    case MAKE_WORD_INTO_INT(R9):
    case MAKE_WORD_INTO_INT(R10):
    case MAKE_WORD_INTO_INT(R11):
    case MAKE_WORD_INTO_INT(R12):
    case MAKE_WORD_INTO_INT(R13):
    case MAKE_WORD_INTO_INT(R14):
    case MAKE_WORD_INTO_INT(R15):
    case MAKE_WORD_INTO_INT(R8D):
    case MAKE_WORD_INTO_INT(R9D):
    case MAKE_WORD_INTO_INT(R10D):
    case MAKE_WORD_INTO_INT(R11D):
    case MAKE_WORD_INTO_INT(R12D):
    case MAKE_WORD_INTO_INT(R13D):
    case MAKE_WORD_INTO_INT(R14D):
    case MAKE_WORD_INTO_INT(R15D):
    case MAKE_WORD_INTO_INT(R8W):
    case MAKE_WORD_INTO_INT(R9W):
    case MAKE_WORD_INTO_INT(R10W):
    case MAKE_WORD_INTO_INT(R11W):
    case MAKE_WORD_INTO_INT(R12W):
    case MAKE_WORD_INTO_INT(R13W):
    case MAKE_WORD_INTO_INT(R14W):
    case MAKE_WORD_INTO_INT(R15W):
    case MAKE_WORD_INTO_INT(R8B):
    case MAKE_WORD_INTO_INT(R9B):
    case MAKE_WORD_INTO_INT(R10B):
    case MAKE_WORD_INTO_INT(R11B):
    case MAKE_WORD_INTO_INT(R12B):
    case MAKE_WORD_INTO_INT(R13B):
    case MAKE_WORD_INTO_INT(R14B):
    case MAKE_WORD_INTO_INT(R15B):
#endif //_WIN64
        return true;
    default:
        if(scmp(string, "eflags"))
            return true;
        if(scmp(string, "cflags"))
            return true;
        if(scmp(string, "lasterror"))
            return true;
        if(scmp(string, "laststatus"))
            return true;
#ifdef _WIN64
        if(scmp(string, "rflags"))
            return true;
#endif //_WIN64
        return false;
    };
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

#define MXCSR_NAME_FLAG_TABLE_ENTRY(flag_name) case MAKE_WORD_INTO_INT(#flag_name): return MXCSRFLAG_##flag_name;

/**
\brief Gets the MXCSR flag AND value from a string.
\param string The flag name.
\return The value to AND the MXCSR value with to get the flag. 0 when not found.
*/
static unsigned int getmxcsrflagfromstring(const char* string)
{
    switch(read_string_4char_ucase(string))
    {
        MXCSR_NAME_FLAG_TABLE_ENTRY(IE)
        MXCSR_NAME_FLAG_TABLE_ENTRY(DE)
        MXCSR_NAME_FLAG_TABLE_ENTRY(ZE)
        MXCSR_NAME_FLAG_TABLE_ENTRY(OE)
        MXCSR_NAME_FLAG_TABLE_ENTRY(UE)
        MXCSR_NAME_FLAG_TABLE_ENTRY(PE)
        MXCSR_NAME_FLAG_TABLE_ENTRY(DAZ)
        MXCSR_NAME_FLAG_TABLE_ENTRY(IM)
        MXCSR_NAME_FLAG_TABLE_ENTRY(DM)
        MXCSR_NAME_FLAG_TABLE_ENTRY(ZM)
        MXCSR_NAME_FLAG_TABLE_ENTRY(OM)
        MXCSR_NAME_FLAG_TABLE_ENTRY(UM)
        MXCSR_NAME_FLAG_TABLE_ENTRY(PM)
        MXCSR_NAME_FLAG_TABLE_ENTRY(FZ)
    default:
        return 0;
    }
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

#define X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(flag_name) case MAKE_WORD_INTO_INT(#flag_name): return x87STATUSWORD_FLAG_##flag_name;

/**
\brief Gets the x87 status word AND value from a string.
\param string The status word name.
\return The value to AND the status word with to get the flag. 0 when not found.
*/
static unsigned int getx87statuswordflagfromstring(const char* string)
{
    switch(read_string_4char_ucase(string))
    {
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(I)
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(D)
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(Z)
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(O)
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(U)
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(P)
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(SF)
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(ES)
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(C0)
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(C1)
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(C2)
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(C3)
        X87STATUSWORD_NAME_FLAG_TABLE_ENTRY(B)
    default:
        return 0;
    }
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

#define X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(flag_name) case MAKE_WORD_INTO_INT(#flag_name): return x87CONTROLWORD_FLAG_##flag_name;

/**
\brief Gets the x87 control word flag AND value from a string.
\param string The name of the control word.
\return The value to AND the control word with to get the flag. 0 when not found.
*/
static unsigned int getx87controlwordflagfromstring(const char* string)
{
    switch(read_string_4char_ucase(string))
    {
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(IM)
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(DM)
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(ZM)
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(OM)
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(UM)
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(PM)
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(IEM)
        X87CONTROLWORD_NAME_FLAG_TABLE_ENTRY(IC)
    default:
        return 0;
    }
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
    switch(read_string_4char_ucase(string))
    {
    case MAKE_WORD_INTO_INT(PC):
        return ((controlword & 0x300) >> 8);
    case MAKE_WORD_INTO_INT(RC):
        return ((controlword & 0xC00) >> 10);
    default:
        return 0;
    }
}

/**
\brief Gets a flag from a string.
\param eflags The eflags value to get the flag from.
\param string The name of the flag.
\return true if the flag equals to 1, false if the flag is 0 or not found.
*/
bool valflagfromstring(duint eflags, const char* string)
{
    switch(read_string_4char_ucase(string))
    {
    case MAKE_WORD_INTO_INT(CF):
        return (bool)((int)(eflags & 0x1) != 0);
    case MAKE_WORD_INTO_INT(PF):
        return (bool)((int)(eflags & 0x4) != 0);
    case MAKE_WORD_INTO_INT(AF):
        return (bool)((int)(eflags & 0x10) != 0);
    case MAKE_WORD_INTO_INT(ZF):
        return (bool)((int)(eflags & 0x40) != 0);
    case MAKE_WORD_INTO_INT(SF):
        return (bool)((int)(eflags & 0x80) != 0);
    case MAKE_WORD_INTO_INT(TF):
        return (bool)((int)(eflags & 0x100) != 0);
    case MAKE_WORD_INTO_INT(IF):
        return (bool)((int)(eflags & 0x200) != 0);
    case MAKE_WORD_INTO_INT(DF):
        return (bool)((int)(eflags & 0x400) != 0);
    case MAKE_WORD_INTO_INT(OF):
        return (bool)((int)(eflags & 0x800) != 0);
    case MAKE_WORD_INTO_INT(RF):
        return (bool)((int)(eflags & 0x10000) != 0);
    case MAKE_WORD_INTO_INT(VM):
        return (bool)((int)(eflags & 0x20000) != 0);
    case MAKE_WORD_INTO_INT(AC):
        return (bool)((int)(eflags & 0x40000) != 0);
    case MAKE_WORD_INTO_INT(VIF):
        return (bool)((int)(eflags & 0x80000) != 0);
    case MAKE_WORD_INTO_INT(VIP):
        return (bool)((int)(eflags & 0x100000) != 0);
    case MAKE_WORD_INTO_INT(ID):
        return (bool)((int)(eflags & 0x200000) != 0);
    default:
        return false;
    }
}

/**
\brief Sets a flag value.
\param string The name of the flag.
\param set The value of the flag.
\return true if the flag was successfully set, false otherwise.
*/
bool setflag(const char* string, bool set)
{
    duint flag = 0;
    switch(read_string_4char_ucase(string))
    {
    case MAKE_WORD_INTO_INT(CF):
        flag = 0x1;
        break;
    case MAKE_WORD_INTO_INT(PF):
        flag = 0x4;
        break;
    case MAKE_WORD_INTO_INT(AF):
        flag = 0x10;
        break;
    case MAKE_WORD_INTO_INT(ZF):
        flag = 0x40;
        break;
    case MAKE_WORD_INTO_INT(SF):
        flag = 0x80;
        break;
    case MAKE_WORD_INTO_INT(TF):
        flag = 0x100;
        break;
    case MAKE_WORD_INTO_INT(IF):
        flag = 0x200;
        break;
    case MAKE_WORD_INTO_INT(DF):
        flag = 0x400;
        break;
    case MAKE_WORD_INTO_INT(OF):
        flag = 0x800;
        break;
    case MAKE_WORD_INTO_INT(RF):
        flag = 0x10000;
        break;
    case MAKE_WORD_INTO_INT(VM):
        flag = 0x20000;
        break;
    case MAKE_WORD_INTO_INT(AC):
        flag = 0x40000;
        break;
    case MAKE_WORD_INTO_INT(VIF):
        flag = 0x80000;
        break;
    case MAKE_WORD_INTO_INT(VIP):
        flag = 0x100000;
        break;
    case MAKE_WORD_INTO_INT(ID):
        flag = 0x200000;
        break;
    default:
        return false;
    }
    duint eflags = GetContextDataEx(hActiveThread, UE_CFLAGS);
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
    TitanRegister TitanIndex = UE_XMM0; // Tian register index with UE_XMM0 as invalid marker
    int string_int = read_string_4char_ucase(string);
    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(EAX):
        TitanIndex = UE_EAX;
        break;
    case MAKE_WORD_INTO_INT(EBX):
        TitanIndex = UE_EBX;
        break;
    case MAKE_WORD_INTO_INT(ECX):
        TitanIndex = UE_ECX;
        break;
    case MAKE_WORD_INTO_INT(EDX):
        TitanIndex = UE_EDX;
        break;
    case MAKE_WORD_INTO_INT(EDI):
        TitanIndex = UE_EDI;
        break;
    case MAKE_WORD_INTO_INT(ESI):
        TitanIndex = UE_ESI;
        break;
    case MAKE_WORD_INTO_INT(EBP):
        TitanIndex = UE_EBP;
        break;
    case MAKE_WORD_INTO_INT(ESP):
        TitanIndex = UE_ESP;
        break;
    case MAKE_WORD_INTO_INT(EIP):
        TitanIndex = UE_EIP;
        break;
    case MAKE_WORD_INTO_INT(GS):
        TitanIndex = UE_SEG_GS;
        break;
    case MAKE_WORD_INTO_INT(FS):
        TitanIndex = UE_SEG_FS;
        break;
    case MAKE_WORD_INTO_INT(ES):
        TitanIndex = UE_SEG_ES;
        break;
    case MAKE_WORD_INTO_INT(DS):
        TitanIndex = UE_SEG_DS;
        break;
    case MAKE_WORD_INTO_INT(CS):
        TitanIndex = UE_SEG_CS;
        break;
    case MAKE_WORD_INTO_INT(SS):
        TitanIndex = UE_SEG_SS;
        break;
    default:
        if(scmp(string, "eflags"))
            TitanIndex = UE_EFLAGS;
        else
            TitanIndex = UE_XMM0;  // Invalid marker
    }
    if(TitanIndex != UE_XMM0)
        return GetContextDataEx(hActiveThread, TitanIndex);

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
    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(AX):
        TitanIndex = UE_EAX;
        break;
    case MAKE_WORD_INTO_INT(BX):
        TitanIndex = UE_EBX;
        break;
    case MAKE_WORD_INTO_INT(CX):
        TitanIndex = UE_ECX;
        break;
    case MAKE_WORD_INTO_INT(DX):
        TitanIndex = UE_EDX;
        break;
    case MAKE_WORD_INTO_INT(SI):
        TitanIndex = UE_ESI;
        break;
    case MAKE_WORD_INTO_INT(DI):
        TitanIndex = UE_EDI;
        break;
    case MAKE_WORD_INTO_INT(BP):
        TitanIndex = UE_EBP;
        break;
    case MAKE_WORD_INTO_INT(SP):
        TitanIndex = UE_ESP;
        break;
    case MAKE_WORD_INTO_INT(IP):
        TitanIndex = UE_EIP;
        break;
    default:
        TitanIndex = UE_XMM0; // Invalid marker
    }
    if(TitanIndex != UE_XMM0)
    {
        return GetContextDataEx(hActiveThread, TitanIndex) & 0xFFFF;
    }

    if(size)
        *size = 1;
    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(AH):
        return (GetContextDataEx(hActiveThread, UE_EAX) >> 8) & 0xFF;
    case MAKE_WORD_INTO_INT(AL):
        return GetContextDataEx(hActiveThread, UE_EAX) & 0xFF;
    case MAKE_WORD_INTO_INT(BH):
        return (GetContextDataEx(hActiveThread, UE_EBX) >> 8) & 0xFF;
    case MAKE_WORD_INTO_INT(BL):
        return GetContextDataEx(hActiveThread, UE_EBX) & 0xFF;
    case MAKE_WORD_INTO_INT(CH):
        return (GetContextDataEx(hActiveThread, UE_ECX) >> 8) & 0xFF;
    case MAKE_WORD_INTO_INT(CL):
        return GetContextDataEx(hActiveThread, UE_ECX) & 0xFF;
    case MAKE_WORD_INTO_INT(DH):
        return (GetContextDataEx(hActiveThread, UE_EDX) >> 8) & 0xFF;
    case MAKE_WORD_INTO_INT(DL):
        return GetContextDataEx(hActiveThread, UE_EDX) & 0xFF;
    case MAKE_WORD_INTO_INT(SIH):
        return (GetContextDataEx(hActiveThread, UE_ESI) >> 8) & 0xFF;
    case MAKE_WORD_INTO_INT(SIL):
        return GetContextDataEx(hActiveThread, UE_ESI) & 0xFF;
    case MAKE_WORD_INTO_INT(DIH):
        return (GetContextDataEx(hActiveThread, UE_EDI) >> 8) & 0xFF;
    case MAKE_WORD_INTO_INT(DIL):
        return GetContextDataEx(hActiveThread, UE_EDI) & 0xFF;
    case MAKE_WORD_INTO_INT(BPH):
        return (GetContextDataEx(hActiveThread, UE_EBP) >> 8) & 0xFF;
    case MAKE_WORD_INTO_INT(BPL):
        return GetContextDataEx(hActiveThread, UE_EBP) & 0xFF;
    case MAKE_WORD_INTO_INT(SPH):
        return (GetContextDataEx(hActiveThread, UE_ESP) >> 8) & 0xFF;
    case MAKE_WORD_INTO_INT(SPL):
        return GetContextDataEx(hActiveThread, UE_ESP) & 0xFF;
    case MAKE_WORD_INTO_INT(IPH):
        return (GetContextDataEx(hActiveThread, UE_EIP) >> 8) & 0xFF;
    case MAKE_WORD_INTO_INT(IPL):
        return GetContextDataEx(hActiveThread, UE_EIP) & 0xFF;
    }

    if(size)
        *size = sizeof(duint);
    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(DR0):
        TitanIndex = UE_DR0;
        break;
    case MAKE_WORD_INTO_INT(DR1):
        TitanIndex = UE_DR1;
        break;
    case MAKE_WORD_INTO_INT(DR2):
        TitanIndex = UE_DR2;
        break;
    case MAKE_WORD_INTO_INT(DR3):
        TitanIndex = UE_DR3;
        break;
    case MAKE_WORD_INTO_INT(DR4):
    case MAKE_WORD_INTO_INT(DR6):
        TitanIndex = UE_DR6;
        break;
    case MAKE_WORD_INTO_INT(DR5):
    case MAKE_WORD_INTO_INT(DR7):
        TitanIndex = UE_DR7;
        break;
    case MAKE_WORD_INTO_INT(CAX):
        TitanIndex = ArchValue(UE_EAX, UE_RAX);
        break;
    case MAKE_WORD_INTO_INT(CBX):
        TitanIndex = ArchValue(UE_EBX, UE_RBX);
        break;
    case MAKE_WORD_INTO_INT(CCX):
        TitanIndex = ArchValue(UE_ECX, UE_RCX);
        break;
    case MAKE_WORD_INTO_INT(CDX):
        TitanIndex = ArchValue(UE_EDX, UE_RDX);
        break;
    case MAKE_WORD_INTO_INT(CSI):
        TitanIndex = ArchValue(UE_ESI, UE_RSI);
        break;
    case MAKE_WORD_INTO_INT(CDI):
        TitanIndex = ArchValue(UE_EDI, UE_RDI);
        break;
    case MAKE_WORD_INTO_INT(CIP):
        TitanIndex = UE_CIP;
        break;
    case MAKE_WORD_INTO_INT(CSP):
        TitanIndex = UE_CSP;
        break;
    case MAKE_WORD_INTO_INT(CBP):
        TitanIndex = ArchValue(UE_EBP, UE_RBP);
        break;
    default:
        if(scmp(string, "cflags"))
            TitanIndex = UE_CFLAGS;
        else
            TitanIndex = UE_XMM0; // Invalid marker
    }
    if(TitanIndex != UE_XMM0)
    {
        return GetContextDataEx(hActiveThread, TitanIndex);
    }

#ifdef _WIN64
    if(size)
        *size = 8;
    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(RAX):
        TitanIndex = UE_RAX;
        break;
    case MAKE_WORD_INTO_INT(RBX):
        TitanIndex = UE_RBX;
        break;
    case MAKE_WORD_INTO_INT(RCX):
        TitanIndex = UE_RCX;
        break;
    case MAKE_WORD_INTO_INT(RDX):
        TitanIndex = UE_RDX;
        break;
    case MAKE_WORD_INTO_INT(RDI):
        TitanIndex = UE_RDI;
        break;
    case MAKE_WORD_INTO_INT(RSI):
        TitanIndex = UE_RSI;
        break;
    case MAKE_WORD_INTO_INT(RBP):
        TitanIndex = UE_RBP;
        break;
    case MAKE_WORD_INTO_INT(RSP):
        TitanIndex = UE_RSP;
        break;
    case MAKE_WORD_INTO_INT(RIP):
        TitanIndex = UE_RIP;
        break;
    case MAKE_WORD_INTO_INT(R8):
        TitanIndex = UE_R8;
        break;
    case MAKE_WORD_INTO_INT(R9):
        TitanIndex = UE_R9;
        break;
    case MAKE_WORD_INTO_INT(R10):
        TitanIndex = UE_R10;
        break;
    case MAKE_WORD_INTO_INT(R11):
        TitanIndex = UE_R11;
        break;
    case MAKE_WORD_INTO_INT(R12):
        TitanIndex = UE_R12;
        break;
    case MAKE_WORD_INTO_INT(R13):
        TitanIndex = UE_R13;
        break;
    case MAKE_WORD_INTO_INT(R14):
        TitanIndex = UE_R14;
        break;
    case MAKE_WORD_INTO_INT(R15):
        TitanIndex = UE_R15;
        break;
    default:
        if(scmp(string, "rflags"))
            TitanIndex = UE_RFLAGS;
        else
            TitanIndex = UE_XMM0; // Invalid marker
        break;
    }
    if(TitanIndex != UE_XMM0)
        return GetContextDataEx(hActiveThread, TitanIndex);

    if(size)
        *size = 4;
    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(R8D):
        TitanIndex = UE_R8;
        break;
    case MAKE_WORD_INTO_INT(R9D):
        TitanIndex = UE_R9;
        break;
    case MAKE_WORD_INTO_INT(R10D):
        TitanIndex = UE_R10;
        break;
    case MAKE_WORD_INTO_INT(R11D):
        TitanIndex = UE_R11;
        break;
    case MAKE_WORD_INTO_INT(R12D):
        TitanIndex = UE_R12;
        break;
    case MAKE_WORD_INTO_INT(R13D):
        TitanIndex = UE_R13;
        break;
    case MAKE_WORD_INTO_INT(R14D):
        TitanIndex = UE_R14;
        break;
    case MAKE_WORD_INTO_INT(R15D):
        TitanIndex = UE_R15;
        break;
    default:
        TitanIndex = UE_XMM0;
    }
    if(TitanIndex != UE_XMM0)
    {
        return GetContextDataEx(hActiveThread, TitanIndex) & 0xFFFFFFFF;
    }

    if(size)
        *size = 2;
    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(R8W):
        TitanIndex = UE_R8;
        break;
    case MAKE_WORD_INTO_INT(R9W):
        TitanIndex = UE_R9;
        break;
    case MAKE_WORD_INTO_INT(R10W):
        TitanIndex = UE_R10;
        break;
    case MAKE_WORD_INTO_INT(R11W):
        TitanIndex = UE_R11;
        break;
    case MAKE_WORD_INTO_INT(R12W):
        TitanIndex = UE_R12;
        break;
    case MAKE_WORD_INTO_INT(R13W):
        TitanIndex = UE_R13;
        break;
    case MAKE_WORD_INTO_INT(R14W):
        TitanIndex = UE_R14;
        break;
    case MAKE_WORD_INTO_INT(R15W):
        TitanIndex = UE_R15;
        break;
    default:
        TitanIndex = UE_XMM0;
    }
    if(TitanIndex != UE_XMM0)
    {
        return GetContextDataEx(hActiveThread, TitanIndex) & 0xFFFF;
    }

    if(size)
        *size = 1;
    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(R8B):
        TitanIndex = UE_R8;
        break;
    case MAKE_WORD_INTO_INT(R9B):
        TitanIndex = UE_R9;
        break;
    case MAKE_WORD_INTO_INT(R10B):
        TitanIndex = UE_R10;
        break;
    case MAKE_WORD_INTO_INT(R11B):
        TitanIndex = UE_R11;
        break;
    case MAKE_WORD_INTO_INT(R12B):
        TitanIndex = UE_R12;
        break;
    case MAKE_WORD_INTO_INT(R13B):
        TitanIndex = UE_R13;
        break;
    case MAKE_WORD_INTO_INT(R14B):
        TitanIndex = UE_R14;
        break;
    case MAKE_WORD_INTO_INT(R15B):
        TitanIndex = UE_R15;
        break;
    default:
        TitanIndex = UE_XMM0;
    }
    if(TitanIndex != UE_XMM0)
    {
        return GetContextDataEx(hActiveThread, TitanIndex) & 0xFF;
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
    TitanRegister titanIndex = UE_XMM0;
    const int string_int = read_string_4char_ucase(string);
    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(EAX):
        titanIndex = UE_EAX;
        break;
    case MAKE_WORD_INTO_INT(EBX):
        titanIndex = UE_EBX;
        break;
    case MAKE_WORD_INTO_INT(ECX):
        titanIndex = UE_ECX;
        break;
    case MAKE_WORD_INTO_INT(EDX):
        titanIndex = UE_EDX;
        break;
    case MAKE_WORD_INTO_INT(EDI):
        titanIndex = UE_EDI;
        break;
    case MAKE_WORD_INTO_INT(ESI):
        titanIndex = UE_ESI;
        break;
    case MAKE_WORD_INTO_INT(EBP):
        titanIndex = UE_EBP;
        break;
    case MAKE_WORD_INTO_INT(ESP):
        titanIndex = UE_ESP;
        break;
    case MAKE_WORD_INTO_INT(EIP):
        titanIndex = UE_EIP;
        break;
    default:
        if(scmp(string, "eflags"))
            titanIndex = UE_EFLAGS;
        else
            titanIndex = UE_XMM0;
    }
    if(titanIndex != UE_XMM0)
        return SetContextDataEx(hActiveThread, titanIndex, value & 0xFFFFFFFF);

    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(CAX):
        titanIndex = ArchValue(UE_EAX, UE_RAX);
        break;
    case MAKE_WORD_INTO_INT(CBX):
        titanIndex = ArchValue(UE_EBX, UE_RBX);
        break;
    case MAKE_WORD_INTO_INT(CCX):
        titanIndex = ArchValue(UE_ECX, UE_RCX);
        break;
    case MAKE_WORD_INTO_INT(CDX):
        titanIndex = ArchValue(UE_EDX, UE_RDX);
        break;
    case MAKE_WORD_INTO_INT(CDI):
        titanIndex = ArchValue(UE_EDI, UE_RDI);
        break;
    case MAKE_WORD_INTO_INT(CSI):
        titanIndex = ArchValue(UE_ESI, UE_RSI);
        break;
    case MAKE_WORD_INTO_INT(CBP):
        titanIndex = ArchValue(UE_EBP, UE_RBP);
        break;
    case MAKE_WORD_INTO_INT(CSP):
        titanIndex = UE_CSP;
        break;
    case MAKE_WORD_INTO_INT(CIP):
        titanIndex = UE_CIP;
        break;
#ifdef _WIN64
    case MAKE_WORD_INTO_INT(RAX):
        titanIndex = UE_RAX;
        break;
    case MAKE_WORD_INTO_INT(RBX):
        titanIndex = UE_RBX;
        break;
    case MAKE_WORD_INTO_INT(RCX):
        titanIndex = UE_RCX;
        break;
    case MAKE_WORD_INTO_INT(RDX):
        titanIndex = UE_RDX;
        break;
    case MAKE_WORD_INTO_INT(RDI):
        titanIndex = UE_RDI;
        break;
    case MAKE_WORD_INTO_INT(RSI):
        titanIndex = UE_RSI;
        break;
    case MAKE_WORD_INTO_INT(RBP):
        titanIndex = UE_RBP;
        break;
    case MAKE_WORD_INTO_INT(RSP):
        titanIndex = UE_RSP;
        break;
    case MAKE_WORD_INTO_INT(RIP):
        titanIndex = UE_RIP;
        break;
    case MAKE_WORD_INTO_INT(R9):
        titanIndex = UE_R9;
        break;
    case MAKE_WORD_INTO_INT(R10):
        titanIndex = UE_R10;
        break;
    case MAKE_WORD_INTO_INT(R11):
        titanIndex = UE_R11;
        break;
    case MAKE_WORD_INTO_INT(R12):
        titanIndex = UE_R12;
        break;
    case MAKE_WORD_INTO_INT(R13):
        titanIndex = UE_R13;
        break;
    case MAKE_WORD_INTO_INT(R14):
        titanIndex = UE_R14;
        break;
    case MAKE_WORD_INTO_INT(R15):
        titanIndex = UE_R15;
        break;
#endif //_WIN64
    default:
        if(scmp(string, "cflags"))
            titanIndex = UE_CFLAGS;
#ifdef _WIN64
        else if(scmp(string, "rflags"))
            titanIndex = UE_RFLAGS;
#endif //_WIN64
        else
            titanIndex = UE_XMM0;
    }
    if(titanIndex != UE_XMM0)
        return SetContextDataEx(hActiveThread, titanIndex, value);

    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(AX):
        titanIndex = UE_EAX;
        break;
    case MAKE_WORD_INTO_INT(BX):
        titanIndex = UE_EBX;
        break;
    case MAKE_WORD_INTO_INT(CX):
        titanIndex = UE_ECX;
        break;
    case MAKE_WORD_INTO_INT(DX):
        titanIndex = UE_EDX;
        break;
    case MAKE_WORD_INTO_INT(SI):
        titanIndex = UE_ESI;
        break;
    case MAKE_WORD_INTO_INT(DI):
        titanIndex = UE_EDI;
        break;
    case MAKE_WORD_INTO_INT(SP):
        titanIndex = UE_ESP;
        break;
    case MAKE_WORD_INTO_INT(BP):
        titanIndex = UE_EBP;
        break;
    case MAKE_WORD_INTO_INT(IP):
        titanIndex = UE_EIP;
        break;
    default:
        titanIndex = UE_XMM0;
    }
    if(titanIndex != UE_XMM0)
        return SetContextDataEx(hActiveThread, titanIndex, (value & 0xFFFF) | (GetContextDataEx(hActiveThread, titanIndex) & 0xFFFF0000));

    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(AH):
        titanIndex = UE_EAX;
        break;
    case MAKE_WORD_INTO_INT(BH):
        titanIndex = UE_EBX;
        break;
    case MAKE_WORD_INTO_INT(CH):
        titanIndex = UE_ECX;
        break;
    case MAKE_WORD_INTO_INT(DH):
        titanIndex = UE_EDX;
        break;
    case MAKE_WORD_INTO_INT(SIH):
        titanIndex = UE_ESI;
        break;
    case MAKE_WORD_INTO_INT(DIH):
        titanIndex = UE_EDI;
        break;
    case MAKE_WORD_INTO_INT(BPH):
        titanIndex = UE_EBP;
        break;
    case MAKE_WORD_INTO_INT(SPH):
        titanIndex = UE_ESP;
        break;
    case MAKE_WORD_INTO_INT(IPH):
        titanIndex = UE_EIP;
        break;
    default:
        titanIndex = UE_XMM0;
    }
    if(titanIndex != UE_XMM0)
        return SetContextDataEx(hActiveThread, titanIndex, ((value & 0xFF) << 8) | (GetContextDataEx(hActiveThread, titanIndex) & 0xFFFF00FF));

    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(AL):
        titanIndex = UE_EAX;
        break;
    case MAKE_WORD_INTO_INT(BL):
        titanIndex = UE_EBX;
        break;
    case MAKE_WORD_INTO_INT(CL):
        titanIndex = UE_ECX;
        break;
    case MAKE_WORD_INTO_INT(DL):
        titanIndex = UE_EDX;
        break;
    case MAKE_WORD_INTO_INT(SIL):
        titanIndex = UE_ESI;
        break;
    case MAKE_WORD_INTO_INT(DIL):
        titanIndex = UE_EDI;
        break;
    case MAKE_WORD_INTO_INT(BPL):
        titanIndex = UE_EBP;
        break;
    case MAKE_WORD_INTO_INT(SPL):
        titanIndex = UE_ESP;
        break;
    case MAKE_WORD_INTO_INT(IPL):
        titanIndex = UE_EIP;
        break;
    default:
        titanIndex = UE_XMM0;
    }
    if(titanIndex != UE_XMM0)
        return SetContextDataEx(hActiveThread, titanIndex, (value & 0xFF) | (GetContextDataEx(hActiveThread, titanIndex) & 0xFFFFFF00));

#ifdef _WIN64
    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(R9D):
    case MAKE_WORD_INTO_INT(R9W):
    case MAKE_WORD_INTO_INT(R9B):
        titanIndex = UE_R9;
        break;
    case MAKE_WORD_INTO_INT(R10D):
    case MAKE_WORD_INTO_INT(R10W):
    case MAKE_WORD_INTO_INT(R10B):
        titanIndex = UE_R10;
        break;
    case MAKE_WORD_INTO_INT(R11D):
    case MAKE_WORD_INTO_INT(R11W):
    case MAKE_WORD_INTO_INT(R11B):
        titanIndex = UE_R11;
        break;
    case MAKE_WORD_INTO_INT(R12D):
    case MAKE_WORD_INTO_INT(R12W):
    case MAKE_WORD_INTO_INT(R12B):
        titanIndex = UE_R12;
        break;
    case MAKE_WORD_INTO_INT(R13D):
    case MAKE_WORD_INTO_INT(R13W):
    case MAKE_WORD_INTO_INT(R13B):
        titanIndex = UE_R13;
        break;
    case MAKE_WORD_INTO_INT(R14D):
    case MAKE_WORD_INTO_INT(R14W):
    case MAKE_WORD_INTO_INT(R14B):
        titanIndex = UE_R14;
        break;
    case MAKE_WORD_INTO_INT(R15D):
    case MAKE_WORD_INTO_INT(R15W):
    case MAKE_WORD_INTO_INT(R15B):
        titanIndex = UE_R15;
        break;
    default:
        titanIndex = UE_XMM0;
    }
    if(titanIndex != UE_XMM0)
    {
        duint mask;
        if((string_int & 0xFF0000) == 0x440000 || (string_int & 0xFF000000) == 0x44000000)  // contains D
        {
            mask = 0xFFFFFFFF;
        }
        else if((string_int & 0xFF0000) == 0x570000 || (string_int & 0xFF000000) == 0x57000000)  // contains W
        {
            mask = 0xFFFF;
        }
        else if((string_int & 0xFF0000) == 0x420000 || (string_int & 0xFF000000) == 0x42000000)  // contains B
        {
            mask = 0xFF;
        }
        else
        {
            return false; // not possible
        }
        return SetContextDataEx(hActiveThread, titanIndex, (value & mask) | (GetContextDataEx(hActiveThread, titanIndex) & ~mask));
    }
#endif // _WIN64

    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(DR0):
        titanIndex = UE_DR0;
        break;
    case MAKE_WORD_INTO_INT(DR1):
        titanIndex = UE_DR1;
        break;
    case MAKE_WORD_INTO_INT(DR2):
        titanIndex = UE_DR2;
        break;
    case MAKE_WORD_INTO_INT(DR3):
        titanIndex = UE_DR3;
        break;
    case MAKE_WORD_INTO_INT(DR4):
    case MAKE_WORD_INTO_INT(DR6):
        titanIndex = UE_DR6;
        break;
    case MAKE_WORD_INTO_INT(DR5):
    case MAKE_WORD_INTO_INT(DR7):
        titanIndex = UE_DR7;
        break;
    default:
        titanIndex = UE_XMM0;
    }
    if(titanIndex != UE_XMM0)
        return SetContextDataEx(hActiveThread, titanIndex, value);

    if(scmp(string, "lasterror"))
        return MemWrite((duint)GetTEBLocation(hActiveThread) + ArchValue(0x34, 0x68), &value, 4);
    if(scmp(string, "laststatus"))
        return MemWrite((duint)GetTEBLocation(hActiveThread) + ArchValue(0xBF4, 0x1250), &value, 4);

    switch(string_int)
    {
    case MAKE_WORD_INTO_INT(GS):
        titanIndex = UE_SEG_GS;
        break;
    case MAKE_WORD_INTO_INT(FS):
        titanIndex = UE_SEG_FS;
        break;
    case MAKE_WORD_INTO_INT(ES):
        titanIndex = UE_SEG_ES;
        break;
    case MAKE_WORD_INTO_INT(DS):
        titanIndex = UE_SEG_DS;
        break;
    case MAKE_WORD_INTO_INT(CS):
        titanIndex = UE_SEG_CS;
        break;
    case MAKE_WORD_INTO_INT(SS):
        titanIndex = UE_SEG_SS;
        break;
    }
    if(titanIndex != UE_XMM0)
        return SetContextDataEx(hActiveThread, titanIndex, value & 0xFFFF);

    return false;
}

/**
\brief Gets the address of an API from a name.
\param name The name of the API, see the command help for more information about valid constructions.
\param [out] value The address of the retrieved API. Cannot be null.
\param silent true to have no console output.
\return true if the API was found and a value retrieved, false otherwise.
*/
bool valapifromstring(const char* name, duint* value, bool silent)
{
    if(!value || !DbgIsDebugging())
        return false;
    //explicit API handling 'module:export'
    const char* apiname = strchr(name, ':'); //the ':' character cannot be in a path: https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx#naming_conventions
    bool resolveForwards = true;
    if(!apiname) //not found
    {
        apiname = strstr(name, "..") ? strchr(name, '.') : strrchr(name, '.'); //kernel32.GetProcAddress support
        if(!apiname) //not found
        {
            apiname = strchr(name, '?'); //the '?' character cannot be in a path either
            resolveForwards = false;
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

        SHARED_ACQUIRE(LockModules);
        duint modbase = ModBaseFromName(modname);
        auto modInfo = ModInfoFromAddr(modbase);
        if(modInfo == nullptr)
            return false;

        duint addr = resolveForwards ? modInfo->getProcAddress(apiname) : 0;
        if(addr != 0)
        {
            *value = addr;
            return true;
        }

        if(scmp(apiname, "base") || scmp(apiname, "imagebase") || scmp(apiname, "header")) //get loaded base
            addr = modbase;
        else if(scmp(apiname, "entrypoint") || scmp(apiname, "entry") || scmp(apiname, "oep") || scmp(apiname, "ep")) //get entry point
            addr = modInfo->entry;
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
            if(!resolveForwards) //get the exported functions with the '?' delimiter
            {
                addr = modInfo->getProcAddress(apiname, 0);
            }
            else
            {
                // module:ordinal
                duint ordinal;
                auto radix = 16;
                if(*apiname == '.') //decimal
                    radix = 10, apiname++;
                if(convertNumber(apiname, ordinal, radix) && ordinal <= 0xFFFF)
                {
                    auto index = ordinal - modInfo->exportOrdinalBase;
                    if(index < modInfo->exports.size()) //found exported function
                        addr = modbase + modInfo->exports[index].rva;
                    else if(ordinal == 0) //support for getting the image base using <modname>:0
                        addr = modbase;
                }
            }
        }

        if(addr == 0)
            return false;

        *value = addr;
        return true;
    }
    else
    {
        // enumerate all modules and look for the export by name
        size_t kernel32 = -1;
        std::vector<duint> addrfound;
        ModEnum([&](const MODINFO & info)
        {
            duint funcAddress = info.getProcAddress(name);
            if(funcAddress != 0)
            {
                if(std::find(addrfound.begin(), addrfound.end(), funcAddress) == addrfound.end())
                {
                    if(scmp(info.name, "kernel32") && scmp(info.extension, ".dll"))
                        kernel32 = addrfound.size();
                    addrfound.push_back(funcAddress);
                }
            }
        });

        if(addrfound.empty())
            return false;

        // prioritize kernel32 exports
        if(kernel32 != -1)
            std::swap(addrfound[0], addrfound[kernel32]);

        *value = addrfound[0];
        if(!silent)
        {
            // print the other exports we found to the log
            for(size_t i = 1; i < addrfound.size(); i++)
            {
                auto symbolicName = SymGetSymbolicName(addrfound[i], false);
                dprintf_untranslated("%p %s\n", addrfound[i], symbolicName.c_str());
            }
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
    return digit >= '1' && digit <= ArchValue('4', '8');
}

/**
\brief Check whether the given string is a variable or a constant
*/
int valfromstring_noexpr_isvar(const char* string)
{
    if(!string || !*string)
        return 0;

    if(isdecnumber(string))  //decimal numbers come 'first'
    {
        return 1;
    }
    else if(ishexnumber(string))  //then hex numbers
    {
        return 2;
    }
    duint value;
    if(ConstantFromName(string, value))
    {
        return 3;
    }
    return 0;
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

    switch(valfromstring_noexpr_isvar(string))
    {
    case 1:
        if(value_size)
            *value_size = 0;
        if(isvar)
            *isvar = false;
        return convertNumber(string + 1, *value, 10);
    case 2:
        if(value_size)
            *value_size = 0;
        if(isvar)
            *isvar = false;
        //hexadecimal value
        return convertNumber(string + ((*string == 'x') ? 1 : 0), *value, 16);
    case 3:
        if(value_size)
            *value_size = 0;
        if(isvar)
            *isvar = false;
        return ConstantFromName(string, *value);
    default:
        if(isvar)
            *isvar = true;
        break;
    }
    int len = (int)strlen(string);
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
            return true;
        }

        int read_size = sizeof(duint);
        int prefix_size = 1;
        size_t seg_offset = 0;
        if(len > 3 && string[1] == ':') //n:[ (number of bytes to read)
        {
            prefix_size = 3;
            int new_size = string[0] - '0';
            if(new_size < read_size)
                read_size = new_size;
        }
        else if(len > 4 && string[1] == 's' && string[2] == ':')
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
        else if(len > 6 && string[0] == 'b'
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
        else if(len > 6 && string[0] == 'w'
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
        else if(len > 7 && string[0] == 'd'
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
        else if(len > 7 && string[0] == 'q'
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
        return true;
    }
    else if(varget(string, value, value_size, 0)) //then come variables
    {
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
            return true;
        }
        *value = getregister(value_size, string);
        return true;
    }
    else if(len > 1 && *string == '_' && isflag(string + 1)) //flag
    {
        if(!DbgIsDebugging())
        {
            if(!silent)
                dputs(QT_TRANSLATE_NOOP("DBG", "Not debugging"));
            *value = 0;
            if(value_size)
                *value_size = 0;
            return true;
        }
        duint eflags = GetContextDataEx(hActiveThread, UE_CFLAGS);
        if(valflagfromstring(eflags, string + 1))
            *value = 1;
        else
            *value = 0;
        if(value_size)
            *value_size = 0;
        return true;
    }
    if(hexonly)
        *hexonly = true;
    if(value_size)
        *value_size = sizeof(duint);

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

    if(valapifromstring(string, value, silent)) //then come APIs
        return true;
    else if(LabelFromString(string, value)) //then come labels
        return true;
    else if(SymAddrFromName(string, value)) //then come symbols
        return true;
    else if(len > 4 && string[0] == 's' && string[1] == 'u' && string[2] == 'b' && string[3] == '_') //then come sub_ functions
    {
        bool result = convertNumber(string + 4, *value, 16);
        duint start;
        return result && FunctionGet(*value, &start, nullptr) && *value == start;
    }
    else if((*value = ModBaseFromName(string))) // then come modules
        return true;

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
\brief Checks if a string starts with another string (ignores case).
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
#define ZMM_PRE_FIELD_STRING "ZMM"
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
        TitanRegister registerindex;
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
            SetContextDataEx(hActiveThread, (TitanRegister)registerindex, value);
        }
    }
    else if(startsWith(MMX_PRE_FIELD_STRING, string))
    {
        string += STRLEN_USING_SIZEOF(MMX_PRE_FIELD_STRING);
        TitanRegister registerindex;
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
        registerindex = atoi(string);
        if(registerindex < ArchValue(8, 16))
        {
            registerindex += UE_XMM0;
        }
        else
        {
            found = false;
        }
        if(found)
            SetContextDataEx(hActiveThread, (TitanRegister)registerindex, value);
    }
    else if(startsWith(YMM_PRE_FIELD_STRING, string))
    {
        string += STRLEN_USING_SIZEOF(YMM_PRE_FIELD_STRING);
        TitanRegister registerindex;
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
            found = false;
            break;
        }
        if(found)
            SetContextDataEx(hActiveThread, registerindex, value);
    }
    else if(startsWith("K", string))  // Opmask registers
    {
        DWORD registerindex;
        registerindex = atoi(string + 1);
        if(registerindex < 8)
        {
            TITAN_ENGINE_CONTEXT_AVX512_t context = {};
            if(!GetAVX512Context(hActiveThread, &context))
            {
                dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            }
            else
            {
                context.Opmask[registerindex] = (ULONGLONG)value;
                SetAVX512Context(hActiveThread, &context);
            }
        }
    }
    else if(startsWith(ZMM_PRE_FIELD_STRING, string))
    {
        DWORD registerindex;
        registerindex = atoi(string + STRLEN_USING_SIZEOF(ZMM_PRE_FIELD_STRING));
        if(registerindex < ArchValue(8, 32))
        {
            TITAN_ENGINE_CONTEXT_AVX512_t context = {};
            if(!GetAVX512Context(hActiveThread, &context))
            {
                dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read register context..."));
            }
            else
            {
                context.ZmmRegisters[registerindex] = *(ZmmRegister_t*)value;
                SetAVX512Context(hActiveThread, &context);
            }
        }
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
            dbgtraceexecute(cip);
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
        if(modInfo->loadedSize > MAXDWORD)
            return 0;
        ULONGLONG offset = ConvertVAtoFileOffsetEx(modInfo->fileMapVA, (DWORD)modInfo->loadedSize, 0, va - modInfo->base, true, false);
        return (duint)offset;
    }
    return 0;
}
