/**
 @file disasm_fast.cpp

 @brief Implements the disasm fast class.
 */

#include "disasm_fast.h"
#include "memory.h"
#include "datainst_helper.h"

void fillbasicinfo(Zydis* cp, BASIC_INSTRUCTION_INFO* basicinfo, bool instrText)
{
    //zero basicinfo
    memset(basicinfo, 0, sizeof(BASIC_INSTRUCTION_INFO));
    //copy instruction text
    if(instrText)
        strcpy_s(basicinfo->instruction, cp->InstructionText().c_str());
    //instruction size
    basicinfo->size = cp->Size();
    //branch/call info
    if(cp->IsCall())
    {
        basicinfo->branch = true;
        basicinfo->call = true;
    }
    else if(cp->IsJump() || cp->IsLoop())
    {
        basicinfo->branch = true;
    }
    //handle operands
    for(int i = 0; i < cp->OpCount(); i++)
    {
        const auto & op = (*cp)[i];
        switch(op.type)
        {
        case ZYDIS_OPERAND_TYPE_IMMEDIATE:
        {
            if(basicinfo->branch)
            {
                basicinfo->type |= TYPE_ADDR;
                basicinfo->addr = duint(op.imm.value.u);
                basicinfo->value.value = duint(op.imm.value.u);
            }
            else
            {
                basicinfo->type |= TYPE_VALUE;
                basicinfo->value.size = VALUE_SIZE(op.size / 8);
                basicinfo->value.value = duint(op.imm.value.u);
            }
        }
        break;

        case ZYDIS_OPERAND_TYPE_MEMORY:
        {
            const auto & mem = op.mem;
            if(instrText)
            {
                auto opText = cp->OperandText(i);
                StringUtils::ReplaceAll(opText, "0x", "");
                strcpy_s(basicinfo->memory.mnemonic, opText.c_str());
            }
            basicinfo->memory.size = MEMORY_SIZE(op.size / 8);
            if(op.mem.base == ZYDIS_REGISTER_RIP) //rip-relative
            {
                basicinfo->memory.value = ULONG_PTR(cp->Address() + op.mem.disp.value + basicinfo->size);
                basicinfo->type |= TYPE_MEMORY;
            }
            else if(mem.disp.value)
            {
                basicinfo->type |= TYPE_MEMORY;
                basicinfo->memory.value = ULONG_PTR(mem.disp.value);
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
    Zydis cp;
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