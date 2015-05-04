#include "console.h"
#include "capstone_wrapper.h"
#include "TitanEngine\TitanEngine.h"

csh Capstone::mHandle = 0;

void Capstone::GlobalInitialize()
{
#ifdef _WIN64
    cs_open(CS_ARCH_X86, CS_MODE_64, &mHandle);
#else //x86
    cs_open(CS_ARCH_X86, CS_MODE_32, &mHandle);
#endif //_WIN64
    cs_option(mHandle, CS_OPT_DETAIL, CS_OPT_ON);
}

void Capstone::GlobalFinalize()
{
    if(mHandle)   //close handle
        cs_close(&mHandle);
}

Capstone::Capstone()
{
    mInstr = 0;
    mError = CS_ERR_OK;
}

Capstone::~Capstone()
{
    if(mInstr)  //free last disassembled instruction
        cs_free(mInstr, 1);
}

bool Capstone::Disassemble(uint addr, unsigned char data[MAX_DISASM_BUFFER])
{
    return Disassemble(addr, data, MAX_DISASM_BUFFER);
}

bool Capstone::Disassemble(uint addr, const unsigned char* data, int size)
{
    if(!data)
        return false;
    if(mInstr)  //free last disassembled instruction
    {
        cs_free(mInstr, 1);
        mInstr = 0;
    }
    return !!cs_disasm(mHandle, (const uint8_t*)data, size, addr, 1, &mInstr);
}

const cs_insn* Capstone::GetInstr()
{
    return mInstr;
}

const cs_err Capstone::GetError()
{
    return mError;
}

const char* Capstone::RegName(x86_reg reg)
{
    return cs_reg_name(mHandle, reg);
}

bool Capstone::InGroup(cs_group_type group)
{
    return cs_insn_group(mHandle, mInstr, group);
}

String Capstone::OperandText(int opindex)
{
    if(opindex >= mInstr->detail->x86.op_count)
        return "";
    const cs_x86_op & op = mInstr->detail->x86.operands[opindex];
    String result;
    char temp[32] = "";
    switch(op.type)
    {
    case X86_OP_REG:
    {
        result = RegName((x86_reg)op.reg);
    }
    break;

    case X86_OP_IMM:
    {
        if(InGroup(CS_GRP_JUMP) || InGroup(CS_GRP_CALL))
            sprintf_s(temp, "%"fext"X", op.imm + mInstr->size);
        else
            sprintf_s(temp, "%"fext"X", op.imm);
        result = temp;
    }
    break;

    case X86_OP_MEM:
    {
        const x86_op_mem & mem = op.mem;
        if(op.mem.base == X86_REG_RIP)  //rip-relative
        {
            sprintf_s(temp, "%"fext"X", mInstr->address + op.mem.disp + mInstr->size);
            result += temp;
        }
        else //normal
        {
            bool prependPlus = false;
            if(mem.base)
            {
                result += RegName((x86_reg)mem.base);
                prependPlus = true;
            }
            if(mem.index)
            {
                if(prependPlus)
                    result += "+";
                result += RegName((x86_reg)mem.index);
                sprintf_s(temp, "*%X", mem.scale);
                result += temp;
                prependPlus = true;
            }
            if(mem.disp)
            {
                char operatorText = '+';
                if(mem.disp < 0)
                {
                    operatorText = '-';
                    sprintf_s(temp, "%"fext"X", mem.disp * -1);
                }
                else
                    sprintf_s(temp, "%"fext"X", mem.disp);
                if(prependPlus)
                    result += operatorText;
                result += temp;
            }
        }
    }
    break;

    case X86_OP_FP:
    {
        dprintf("float: %f\n", op.fp);
    }
    break;
    }
    return result;
}

const int Capstone::Size()
{
    return GetInstr()->size;
}

const uint Capstone::Address()
{
    return (uint)GetInstr()->address;
}

const cs_x86 & Capstone::x86()
{
    return GetInstr()->detail->x86;
}

bool Capstone::IsFilling()
{
    uint8_t opcode = x86().opcode[0];
    return opcode == 0x90 || opcode == 0xCC;
}

x86_insn Capstone::GetId()
{
    return (x86_insn)mInstr->id;
}

String Capstone::InstructionText()
{
    String result = mInstr->mnemonic;
    result += " ";
    result += mInstr->op_str;
    return result;
}