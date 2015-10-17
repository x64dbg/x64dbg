#include "console.h"
#include "capstone_wrapper.h"

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
    if(mHandle) //close handle
        cs_close(&mHandle);
}

Capstone::Capstone()
{
    mInstr = nullptr;
    mError = CS_ERR_OK;
}

Capstone::~Capstone()
{
    if(mInstr) //free last disassembled instruction
        cs_free(mInstr, 1);
}

bool Capstone::Disassemble(uint addr, const unsigned char data[MAX_DISASM_BUFFER])
{
    return Disassemble(addr, data, MAX_DISASM_BUFFER);
}

bool Capstone::Disassemble(uint addr, const unsigned char* data, int size)
{
    if(!data)
        return false;
    if(mInstr) //free last disassembled instruction
    {
        cs_free(mInstr, 1);
        mInstr = nullptr;
    }
    return !!cs_disasm(mHandle, data, size, addr, 1, &mInstr);
}

const cs_insn* Capstone::GetInstr() const
{
    return mInstr;
}

cs_err Capstone::GetError() const
{
    return mError;
}

const char* Capstone::RegName(x86_reg reg) const
{
    return cs_reg_name(mHandle, reg);
}

bool Capstone::InGroup(cs_group_type group) const
{
    return cs_insn_group(mHandle, mInstr, group);
}

String Capstone::OperandText(int opindex) const
{
    if(opindex >= mInstr->detail->x86.op_count)
        return "";
    const auto & op = mInstr->detail->x86.operands[opindex];
    String result;
    char temp[32] = "";
    switch(op.type)
    {
    case X86_OP_REG:
    {
        result = RegName(x86_reg(op.reg));
    }
    break;

    case X86_OP_IMM:
    {
        if(InGroup(CS_GRP_JUMP) || InGroup(CS_GRP_CALL) || IsLoop())
            sprintf_s(temp, "%" fext "X", op.imm + mInstr->size);
        else
            sprintf_s(temp, "%" fext "X", op.imm);
        result = temp;
    }
    break;

    case X86_OP_MEM:
    {
        const auto & mem = op.mem;
        if(op.mem.base == X86_REG_RIP)  //rip-relative
        {
            sprintf_s(temp, "%" fext "X", mInstr->address + op.mem.disp + mInstr->size);
            result += temp;
        }
        else //normal
        {
            bool prependPlus = false;
            if(mem.base)
            {
                result += RegName(x86_reg(mem.base));
                prependPlus = true;
            }
            if(mem.index)
            {
                if(prependPlus)
                    result += "+";
                result += RegName(x86_reg(mem.index));
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
                    sprintf_s(temp, "%" fext "X", mem.disp * -1);
                }
                else
                    sprintf_s(temp, "%" fext "X", mem.disp);
                if(prependPlus)
                    result += operatorText;
                result += temp;
            }
        }
    }
    break;

    case X86_OP_FP:
    case X86_OP_INVALID:
    {
    }
    break;
    }
    return result;
}

int Capstone::Size() const
{
    return GetInstr()->size;
}

uint Capstone::Address() const
{
    return uint(GetInstr()->address);
}

const cs_x86 & Capstone::x86() const
{
    return GetInstr()->detail->x86;
}

bool Capstone::IsFilling() const
{
    switch(GetId())
    {
    case X86_INS_NOP:
    case X86_INS_INT3:
        return true;
    default:
        return false;
    }
}

bool Capstone::IsLoop() const
{
    switch(GetId())
    {
    case X86_INS_LOOP:
    case X86_INS_LOOPE:
    case X86_INS_LOOPNE:
        return true;
    default:
        return false;
    }
}

x86_insn Capstone::GetId() const
{
    return x86_insn(mInstr->id);
}

String Capstone::InstructionText() const
{
    String result = mInstr->mnemonic;
    result += " ";
    result += mInstr->op_str;
    return result;
}