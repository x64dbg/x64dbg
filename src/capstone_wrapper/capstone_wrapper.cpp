#include "capstone_wrapper.h"
#include <windows.h>

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
    mSuccess = false;
}

Capstone::~Capstone()
{
    if(mInstr) //free last disassembled instruction
        cs_free(mInstr, 1);
}

bool Capstone::Disassemble(size_t addr, const unsigned char data[MAX_DISASM_BUFFER])
{
    return Disassemble(addr, data, MAX_DISASM_BUFFER);
}

bool Capstone::Disassemble(size_t addr, const unsigned char* data, int size)
{
    if(!data)
        return false;
    if(mInstr) //free last disassembled instruction
    {
        cs_free(mInstr, 1);
        mInstr = nullptr;
    }
    return mSuccess = !!cs_disasm(mHandle, data, size, addr, 1, &mInstr);
}

const cs_insn* Capstone::GetInstr() const
{
    if(!Success())
        return nullptr;
    return mInstr;
}

bool Capstone::Success() const
{
    return mSuccess;
}

const char* Capstone::RegName(x86_reg reg) const
{
    return cs_reg_name(mHandle, reg);
}

bool Capstone::InGroup(cs_group_type group) const
{
    if(!Success())
        return false;
    return cs_insn_group(mHandle, mInstr, group);
}

std::string Capstone::OperandText(int opindex) const
{
    if(!Success() || opindex >= mInstr->detail->x86.op_count)
        return "";
    const auto & op = mInstr->detail->x86.operands[opindex];
    std::string result;
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
        sprintf_s(temp, "%llX", op.imm);
        result = temp;
    }
    break;

    case X86_OP_MEM:
    {
        const auto & mem = op.mem;
        if(op.mem.base == X86_REG_RIP)  //rip-relative
        {
            sprintf_s(temp, "%llX", Address() + op.mem.disp + Size());
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
                    sprintf_s(temp, "%llX", mem.disp * -1);
                }
                else
                    sprintf_s(temp, "%llX", mem.disp);
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
    if(!Success())
        return 1;
    return GetInstr()->size;
}

size_t Capstone::Address() const
{
    if(!Success())
        return 0;
    return size_t(GetInstr()->address);
}

const cs_x86 & Capstone::x86() const
{
    if(!Success())
        DebugBreak();
    return GetInstr()->detail->x86;
}

bool Capstone::IsFilling() const
{
    if(!Success())
        return false;
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
    if(!Success())
        return false;
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
    if(!Success())
        DebugBreak();
    return x86_insn(mInstr->id);
}

std::string Capstone::InstructionText() const
{
    if(!Success())
        return "???";
    std::string result = Mnemonic();
    if(OpCount())
    {
        result += " ";
        result += mInstr->op_str;
    }
    return result;
}

int Capstone::OpCount() const
{
    if(!Success())
        return 0;
    return x86().op_count;
}

cs_x86_op Capstone::operator[](int index) const
{
    if(!Success() || index >= OpCount())
        DebugBreak();
    return x86().operands[index];
}

bool Capstone::IsNop() const
{
    return GetId() == X86_INS_NOP;
}

bool Capstone::IsInt3() const
{
    if(!Success())
        return false;
    switch(GetId())
    {
    case X86_INS_INT3:
        return true;
    case X86_INS_INT:
    {
        cs_x86_op op = x86().operands[0];
        return op.type == X86_OP_IMM && op.imm == 3;
    }
    default:
        return false;
    }
}

std::string Capstone::Mnemonic() const
{
    if(!Success())
        return "???";
    return mInstr->mnemonic;
}

const char* Capstone::MemSizeName(int size) const
{
    switch(size)
    {
    case 1:
        return "byte";
    case 2:
        return "word";
    case 4:
        return "dword";
    case 6:
        return "fword";
    case 8:
        return "qword";
    case 10:
        return "tword";
    case 16:
        return "dqword";
    case 32:
        return "yword";
    case 64:
        return "zword";
    default:
        return nullptr;
    }
}

size_t Capstone::BranchDestination() const
{
    if(!Success())
        return 0;
    if(InGroup(CS_GRP_JUMP) || InGroup(CS_GRP_CALL) || IsLoop())
    {
        const auto & op = x86().operands[0];
        if(op.type == CS_OP_IMM)
            return size_t(op.imm);
    }
    return 0;
}