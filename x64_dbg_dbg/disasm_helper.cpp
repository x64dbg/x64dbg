#include "disasm_helper.h"
#include "BeaEngine\BeaEngine.h"

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

static bool HandleArgument(ARGTYPE* Argument, INSTRTYPE* Instruction, DISASM_ARG* arg)
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
        value=DbgValFromString(argmnemonic);
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
        uint value=DbgValFromString(argmnemonic);
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
    else
        instr->type=instr_normal;
    if(HandleArgument(&disasm.Argument1, &disasm.Instruction, &instr->arg[instr->argcount]))
        instr->argcount++;
    if(HandleArgument(&disasm.Argument2, &disasm.Instruction, &instr->arg[instr->argcount]))
        instr->argcount++;
    if(HandleArgument(&disasm.Argument3, &disasm.Instruction, &instr->arg[instr->argcount]))
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
