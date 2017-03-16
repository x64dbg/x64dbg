/**
 @file disasm_fast.cpp

 @brief Implements the disasm fast class.
 */

#include "disasm_fast.h"
#include "memory.h"
#include "datainst_helper.h"

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

void fillbasicinfo(Capstone* cp, BASIC_INSTRUCTION_INFO* basicinfo, bool instrText)
{
    //zero basicinfo
    memset(basicinfo, 0, sizeof(BASIC_INSTRUCTION_INFO));
    //copy instruction text
    if(instrText)
        strcpy_s(basicinfo->instruction, cp->InstructionText().c_str());
    //instruction size
    basicinfo->size = cp->Size();
    //branch/call info
    if(cp->InGroup(CS_GRP_CALL))
    {
        basicinfo->branch = true;
        basicinfo->call = true;
    }
    else if(cp->InGroup(CS_GRP_JUMP) || cp->IsLoop())
    {
        basicinfo->branch = true;
    }
    //handle operands
    for(int i = 0; i < cp->x86().op_count; i++)
    {
        const cs_x86_op & op = cp->x86().operands[i];
        switch(op.type)
        {
        case X86_OP_IMM:
        {
            if(basicinfo->branch)
            {
                basicinfo->type |= TYPE_ADDR;
                basicinfo->addr = duint(op.imm);
                basicinfo->value.value = duint(op.imm);
            }
            else
            {
                basicinfo->type |= TYPE_VALUE;
                basicinfo->value.size = VALUE_SIZE(op.size);
                basicinfo->value.value = duint(op.imm);
            }
        }
        break;

        case X86_OP_MEM:
        {
            const x86_op_mem & mem = op.mem;
            if(instrText)
                strcpy_s(basicinfo->memory.mnemonic, cp->OperandText(i).c_str());
            basicinfo->memory.size = MEMORY_SIZE(op.size);
            if(op.mem.base == X86_REG_RIP) //rip-relative
            {
                basicinfo->memory.value = ULONG_PTR(cp->Address() + op.mem.disp + basicinfo->size);
                basicinfo->type |= TYPE_MEMORY;
            }
            else if(mem.disp)
            {
                basicinfo->type |= TYPE_MEMORY;
                basicinfo->memory.value = ULONG_PTR(mem.disp);
            }
        }
        break;

        default:
            break;
        }
    }
}

bool disasmfast(const unsigned char* data, duint addr, BASIC_INSTRUCTION_INFO* basicinfo)
{
    if(!data || !basicinfo)
        return false;
    Capstone cp;
    cp.Disassemble(addr, data, MAX_DISASM_BUFFER);
    if(trydisasmfast(data, addr, basicinfo, cp.Success() ? cp.Size() : 1))
        return true;
    if(!cp.Success())
    {
        strcpy_s(basicinfo->instruction, "???");
        basicinfo->size = 1;
        return false;
    }
    fillbasicinfo(&cp, basicinfo);
    return true;
}

bool disasmfast(duint addr, BASIC_INSTRUCTION_INFO* basicinfo, bool cache)
{
    unsigned int data[16];
    if(!MemRead(addr, data, sizeof(data), nullptr, cache))
        return false;
    return disasmfast((unsigned char*)data, addr, basicinfo);
}