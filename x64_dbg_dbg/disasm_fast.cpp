#include "disasm_fast.h"

static MEMORY_SIZE argsize2memsize(int argsize)
{
    switch(argsize)
    {
    case 8:
        return size_byte;
    case 16:
        return size_word;
    case 32:
        return size_dword;
    case 64:
        return size_qword;
    }
    return size_byte;
}

void fillbasicinfo(DISASM* disasm, BASIC_INSTRUCTION_INFO* basicinfo)
{
    //set type to zero
    basicinfo->type=0;
    //find immidiat
    if(disasm->Instruction.BranchType==0) //no branch
    {
        if((disasm->Argument1.ArgType&CONSTANT_TYPE)==CONSTANT_TYPE)
        {
            basicinfo->type|=TYPE_VALUE;
            basicinfo->value.value=(ULONG_PTR)disasm->Instruction.Immediat;
            basicinfo->value.size=argsize2memsize(disasm->Argument1.ArgSize);
        }
        else if((disasm->Argument2.ArgType&CONSTANT_TYPE)==CONSTANT_TYPE)
        {
            basicinfo->type|=TYPE_VALUE;
            basicinfo->value.value=(ULONG_PTR)disasm->Instruction.Immediat;
            basicinfo->value.size=argsize2memsize(disasm->Argument2.ArgSize);
        }
    }
    else //branch
        basicinfo->branch=true;
    //find memory displacement
    if((disasm->Argument1.ArgType&MEMORY_TYPE)==MEMORY_TYPE || (disasm->Argument2.ArgType&MEMORY_TYPE)==MEMORY_TYPE)
    {
        if(disasm->Argument1.Memory.Displacement)
        {
            basicinfo->type|=TYPE_MEMORY;
            basicinfo->memory.value=(ULONG_PTR)disasm->Argument1.Memory.Displacement;
            strcpy(basicinfo->memory.mnemonic, disasm->Argument1.ArgMnemonic);
            basicinfo->memory.size=argsize2memsize(disasm->Argument1.ArgSize);
        }
        else if(disasm->Argument2.Memory.Displacement)
        {
            basicinfo->type|=TYPE_MEMORY;
            basicinfo->memory.value=(ULONG_PTR)disasm->Argument2.Memory.Displacement;
            strcpy(basicinfo->memory.mnemonic, disasm->Argument2.ArgMnemonic);
            basicinfo->memory.size=argsize2memsize(disasm->Argument2.ArgSize);
        }
    }
    //find address value
    if(disasm->Instruction.BranchType && disasm->Instruction.AddrValue)
    {
        basicinfo->type|=TYPE_ADDR;
        basicinfo->addr=(ULONG_PTR)disasm->Instruction.AddrValue;
    }
    //rip-relative (non-branch)
    if(disasm->Instruction.BranchType==0)
    {
        if((disasm->Argument1.ArgType&RELATIVE_)==RELATIVE_)
        {
            basicinfo->type|=TYPE_MEMORY;
            basicinfo->memory.value=(ULONG_PTR)disasm->Instruction.AddrValue;
            strcpy(basicinfo->memory.mnemonic, disasm->Argument1.ArgMnemonic);
            basicinfo->memory.size=argsize2memsize(disasm->Argument1.ArgSize);
        }
        else if((disasm->Argument2.ArgType&RELATIVE_)==RELATIVE_)
        {
            basicinfo->type|=TYPE_MEMORY;
            basicinfo->memory.value=(ULONG_PTR)disasm->Instruction.AddrValue;
            strcpy(basicinfo->memory.mnemonic, disasm->Argument2.ArgMnemonic);
            basicinfo->memory.size=argsize2memsize(disasm->Argument2.ArgSize);
        }
    }
}