#include "disasm_helper.h"
#include "BeaEngine\BeaEngine.h"
#include "value.h"
#include "console.h"
#include "debugger.h"
#include "memory.h"
#include <cwctype>
#include <cwchar>

const char* disasmtext(uint addr)
{
    unsigned char buffer[16]="";
    DbgMemRead(addr, buffer, 16);
    DISASM disasm;
    disasm.Options=NoformatNumeral;
#ifdef _WIN64
    disasm.Archi=64;
#endif // _WIN64
    disasm.VirtualAddr=addr;
    disasm.EIP=(UIntPtr)buffer;
    int len=Disasm(&disasm);
    static char instruction[INSTRUCT_LENGTH]="";
    if(len==UNKNOWN_OPCODE)
        strcpy(instruction, "???");
    else
        strcpy(instruction, disasm.CompleteInstr);
    return instruction;
}

static SEGMENTREG ConvertBeaSeg(int beaSeg)
{
    switch(beaSeg)
    {
    case ESReg:
        return SEG_ES;
        break;
    case DSReg:
        return SEG_DS;
        break;
    case FSReg:
        return SEG_FS;
        break;
    case GSReg:
        return SEG_GS;
        break;
    case CSReg:
        return SEG_CS;
        break;
    case SSReg:
        return SEG_SS;
        break;
    }
    return SEG_DEFAULT;
}

static bool HandleArgument(ARGTYPE* Argument, INSTRTYPE* Instruction, DISASM_ARG* arg, uint addr)
{
    int argtype=Argument->ArgType;
    const char* argmnemonic=Argument->ArgMnemonic;
    if(!*argmnemonic)
        return false;
    arg->memvalue=0;
    strcpy(arg->mnemonic, argmnemonic);
    if((argtype&MEMORY_TYPE)==MEMORY_TYPE)
    {
        arg->type=arg_memory;
        arg->segment=ConvertBeaSeg(Argument->SegmentReg);
        uint value=Argument->Memory.Displacement;
        if((Argument->ArgType&RELATIVE_)==RELATIVE_)
            value=Instruction->AddrValue;
        arg->constant=value;
        arg->value=0;
        if(!valfromstring(argmnemonic, &value, true, true))
			return false;
        if(DbgMemIsValidReadPtr(value))
        {
            arg->value=value;
            switch(Argument->ArgSize) //TODO: segments
            {
            case 8:
                DbgMemRead(value, (unsigned char*)&arg->memvalue, 1);
                break;
            case 16:
                DbgMemRead(value, (unsigned char*)&arg->memvalue, 2);
                break;
            case 32:
                DbgMemRead(value, (unsigned char*)&arg->memvalue, 4);
                break;
            case 64:
                DbgMemRead(value, (unsigned char*)&arg->memvalue, 8);
                break;
            }
        }
    }
    else
    {
        arg->segment=SEG_DEFAULT;
        arg->type=arg_normal;
        uint value=0;
        if(!valfromstring(argmnemonic, &value, true, true))
			return false;
        arg->value=value;
        char sValue[64]="";
        sprintf(sValue, "%"fext"X", value);
        if(_stricmp(argmnemonic, sValue))
            value=0;
        arg->constant=value;
    }
    return true;
}

void disasmget(uint addr, DISASM_INSTR* instr)
{
    if(!DbgIsDebugging() or !instr)
        return;
    memset(instr, 0, sizeof(DISASM_INSTR));
    DISASM disasm;
    memset(&disasm, 0, sizeof(DISASM));
    unsigned char buffer[16]="";
    DbgMemRead(addr, buffer, 16);
    disasm.Options=NoformatNumeral;
#ifdef _WIN64
    disasm.Archi=64;
#endif // _WIN64
    disasm.VirtualAddr=addr;
    disasm.EIP=(UIntPtr)buffer;
    int len=Disasm(&disasm);
    strcpy(instr->instruction, disasm.CompleteInstr);
    if(len==UNKNOWN_OPCODE)
    {
        instr->type=instr_normal;
        instr->argcount=0;
        return;
    }
    if(disasm.Instruction.BranchType)
        instr->type=instr_branch;
    else if(strstr(disasm.CompleteInstr, "sp") or strstr(disasm.CompleteInstr, "bp"))
        instr->type=instr_stack;
    else
        instr->type=instr_normal;
    if(HandleArgument(&disasm.Argument1, &disasm.Instruction, &instr->arg[instr->argcount], addr))
        instr->argcount++;
    if(HandleArgument(&disasm.Argument2, &disasm.Instruction, &instr->arg[instr->argcount], addr))
        instr->argcount++;
    if(HandleArgument(&disasm.Argument3, &disasm.Instruction, &instr->arg[instr->argcount], addr))
        instr->argcount++;
}

void disasmprint(uint addr)
{
    DISASM_INSTR instr;
    disasmget(addr, &instr);
    printf(">%d:\"%s\":\n", instr.type, instr.instruction);
    for(int i=0; i<instr.argcount; i++)
        printf(" %d:%d:%"fext"X:%"fext"X:%"fext"X\n", i, instr.arg[i].type, instr.arg[i].constant, instr.arg[i].value, instr.arg[i].memvalue);
}

static bool isasciistring(const unsigned char* data, int maxlen)
{
    int len=strlen((const char*)data);
    if(len<2 or len+1>=maxlen)
        return false;
    for(int i=0; i<len; i++)
        if(!isprint(data[i]) and !isspace(data[i]))
            return false;
    return true;
}

static bool isunicodestring(const unsigned char* data, int maxlen)
{
    int len=wcslen((const wchar_t*)data);
    if(len<2 or len+1>=maxlen)
        return false;
    for(int i=0; i<len*2; i+=2)
    {
        if(data[i+1])
            return false;
        if(!isprint(data[i]) and !isspace(data[i]))
            return false;
    }
    return true;
}

bool disasmgetstringat(uint addr, STRING_TYPE* type, char* ascii, char* unicode, int maxlen)
{
    if(type)
        *type=str_none;
    unsigned char* data=(unsigned char*)emalloc((maxlen+1)*2, "disasmgetstringat:data");
    memset(data, 0, (maxlen+1)*2);
    if(!memread(fdProcessInfo->hProcess, (const void*)addr, data, (maxlen+1)*2, 0))
        return false;
    if(isasciistring(data, maxlen))
    {
        if(type)
            *type=str_ascii;
        int len=strlen((const char*)data);
        for(int i=0,j=0; i<len; i++)
        {
            switch(data[i])
            {
            case '\t':
                j+=sprintf(ascii+j, "\\t");
                break;
            case '\f':
                j+=sprintf(ascii+j, "\\f");
                break;
            case '\v':
                j+=sprintf(ascii+j, "\\v");
                break;
            case '\n':
                j+=sprintf(ascii+j, "\\n");
                break;
            case '\r':
                j+=sprintf(ascii+j, "\\r");
                break;
            case '\\':
                j+=sprintf(ascii+j, "\\\\");
                break;
            case '\"':
                j+=sprintf(ascii+j, "\\\"");
                break;
            default:
                j+=sprintf(ascii+j, "%c", data[i]);
                break;
            }
        }
        efree(data, "disasmgetstringat:data");
        return true;
    }
    else if(isunicodestring(data, maxlen))
    {
        if(type)
            *type=str_unicode;
        int len=wcslen((const wchar_t*)data);
        for(int i=0,j=0; i<len*2; i+=2)
        {
            switch(data[i])
            {
            case '\t':
                j+=sprintf(unicode+j, "\\t");
                break;
            case '\f':
                j+=sprintf(unicode+j, "\\f");
                break;
            case '\v':
                j+=sprintf(unicode+j, "\\v");
                break;
            case '\n':
                j+=sprintf(unicode+j, "\\n");
                break;
            case '\r':
                j+=sprintf(unicode+j, "\\r");
                break;
            case '\\':
                j+=sprintf(unicode+j, "\\\\");
                break;
            case '\"':
                j+=sprintf(unicode+j, "\\\"");
                break;
            default:
                j+=sprintf(unicode+j, "%c", data[i]);
                break;
            }
        }
        efree(data, "disasmgetstringat:data");
        return true;
    }
    efree(data, "disasmgetstringat:data");
    return false;
}
