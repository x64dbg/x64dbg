#include "capstone_wrapper.h"
#include <windows.h>

csh Capstone::mHandle = 0;
bool Capstone::mInitialized = false;

void Capstone::GlobalInitialize()
{
    if(!mInitialized)
    {
        mInitialized = true;
#ifdef _WIN64
        cs_open(CS_ARCH_X86, CS_MODE_64, &mHandle);
#else //x86
        cs_open(CS_ARCH_X86, CS_MODE_32, &mHandle);
#endif //_WIN64
        cs_option(mHandle, CS_OPT_DETAIL, CS_OPT_ON);
    }
}

void Capstone::GlobalFinalize()
{
    if(mHandle) //close handle
        cs_close(&mHandle);
    mInitialized = false;
}

Capstone::Capstone()
{
    GlobalInitialize();
    mInstr = cs_malloc(mHandle);
    mSuccess = false;
}

Capstone::Capstone(const Capstone & capstone)
    : mInstr(cs_malloc(mHandle)),
      mSuccess(false)
{
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
    if(!data || !size)
        return false;

    size_t codeSize = size;
    uint64_t addr64 = addr;

    return (mSuccess = cs_disasm_iter(mHandle, &data, &codeSize, &addr64, mInstr));
}

bool Capstone::DisassembleSafe(size_t addr, const unsigned char* data, int size)
{
    unsigned char dataSafe[MAX_DISASM_BUFFER];
    memset(dataSafe, 0, sizeof(dataSafe));
    memcpy(dataSafe, data, min(MAX_DISASM_BUFFER, size_t(size)));
    return Disassemble(addr, dataSafe);
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
    if(group == CS_GRP_PRIVILEGE)
    {
        auto id = GetId();
        // I/O instructions
        if(id == X86_INS_OUT || id == X86_INS_OUTSB || id == X86_INS_OUTSD || id == X86_INS_OUTSW
                || id == X86_INS_IN || id == X86_INS_INSB || id == X86_INS_INSD || id == X86_INS_INSW
                // system instructions
                || id == X86_INS_RDMSR || id == X86_INS_SMSW)
            return true;
    }
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

const cs_x86_op & Capstone::operator[](int index) const
{
    if(!Success() || index < 0 || index >= OpCount())
        DebugBreak();
    return x86().operands[index];
}

static bool isSafe64NopRegOp(const cs_x86_op & op)
{
    if(op.type != X86_OP_REG)
        return true; //a non-register is safe
#ifdef _WIN64
    switch(op.reg)
    {
    case X86_REG_EAX:
    case X86_REG_EBX:
    case X86_REG_ECX:
    case X86_REG_EDX:
    case X86_REG_EBP:
    case X86_REG_ESP:
    case X86_REG_ESI:
    case X86_REG_EDI:
        return false; //32 bit register modifications clear the high part of the 64 bit register
    default:
        return true; //all other registers are safe
    }
#else
    return true;
#endif //_WIN64
}

bool Capstone::IsNop() const
{
    if(!Success())
        return false;
    const auto & ops = x86().operands;
    cs_x86_op op;
    switch(GetId())
    {
    case X86_INS_NOP:
    case X86_INS_PAUSE:
    case X86_INS_FNOP:
        // nop
        return true;
    case X86_INS_MOV:
    case X86_INS_CMOVA:
    case X86_INS_CMOVAE:
    case X86_INS_CMOVB:
    case X86_INS_CMOVBE:
    case X86_INS_CMOVE:
    case X86_INS_CMOVNE:
    case X86_INS_CMOVG:
    case X86_INS_CMOVGE:
    case X86_INS_CMOVL:
    case X86_INS_CMOVLE:
    case X86_INS_CMOVO:
    case X86_INS_CMOVNO:
    case X86_INS_CMOVP:
    case X86_INS_CMOVNP:
    case X86_INS_CMOVS:
    case X86_INS_CMOVNS:
    case X86_INS_MOVAPS:
    case X86_INS_MOVAPD:
    case X86_INS_MOVUPS:
    case X86_INS_MOVUPD:
    case X86_INS_XCHG:
        // mov edi, edi
        return ops[0].type == X86_OP_REG && ops[1].type == X86_OP_REG && ops[0].reg == ops[1].reg && isSafe64NopRegOp(ops[0]);
    case X86_INS_LEA:
    {
        // lea eax, [eax + 0]
        auto reg = ops[0].reg;
        auto mem = ops[1].mem;
        return ops[0].type == X86_OP_REG && ops[1].type == X86_OP_MEM && mem.disp == 0 &&
               ((mem.index == X86_REG_INVALID && mem.base == reg) ||
                (mem.index == reg && mem.base == X86_REG_INVALID && mem.scale == 1)) && isSafe64NopRegOp(ops[0]);
    }
    case X86_INS_JMP:
    case X86_INS_JA:
    case X86_INS_JAE:
    case X86_INS_JB:
    case X86_INS_JBE:
    case X86_INS_JE:
    case X86_INS_JNE:
    case X86_INS_JG:
    case X86_INS_JGE:
    case X86_INS_JL:
    case X86_INS_JLE:
    case X86_INS_JO:
    case X86_INS_JNO:
    case X86_INS_JP:
    case X86_INS_JNP:
    case X86_INS_JS:
    case X86_INS_JNS:
    case X86_INS_JECXZ:
    case X86_INS_JCXZ:
    case X86_INS_LOOP:
    case X86_INS_LOOPE:
    case X86_INS_LOOPNE:
        // jmp 0
        op = ops[0];
        return op.type == X86_OP_IMM && op.imm == 0;
    case X86_INS_SHL:
    case X86_INS_SHR:
    case X86_INS_ROL:
    case X86_INS_ROR:
    case X86_INS_SAR:
    case X86_INS_SAL:
        // shl eax, 0
        op = ops[1];
        return op.type == X86_OP_IMM && op.imm == 0 && isSafe64NopRegOp(ops[0]);
    case X86_INS_SHLD:
    case X86_INS_SHRD:
        // shld eax, ebx, 0
        op = ops[2];
        return op.type == X86_OP_IMM && op.imm == 0 && isSafe64NopRegOp(ops[0]) && isSafe64NopRegOp(ops[1]);
    default:
        return false;
    }
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

bool Capstone::IsUnusual() const
{
    auto id = GetId();
    return (InGroup(CS_GRP_PRIVILEGE) || InGroup(CS_GRP_IRET) || InGroup(CS_GRP_INVALID)
            || id == X86_INS_RDTSC || id == X86_INS_SYSCALL || id == X86_INS_SYSENTER || id == X86_INS_CPUID || id == X86_INS_RDTSCP
            || id == X86_INS_RDRAND || id == X86_INS_RDSEED || id == X86_INS_UD2 || id == X86_INS_UD2B);
}

std::string Capstone::Mnemonic() const
{
    if(!Success())
        return "???";
    return mInstr->mnemonic;
}

std::string Capstone::MnemonicId() const
{
    if(!Success())
        return "???";
    return cs_insn_name(mHandle, GetId());
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
    case 14:
        return "m14";
    case 16:
        return "xmmword";
    case 28:
        return "m28";
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
        if(op.type == X86_OP_IMM)
            return size_t(op.imm);
    }
    return 0;
}

size_t Capstone::ResolveOpValue(int opindex, const std::function<size_t(x86_reg)> & resolveReg) const
{
    size_t dest = 0;
    const auto & op = x86().operands[opindex];
    switch(op.type)
    {
    case X86_OP_IMM:
        dest = size_t(op.imm);
        break;
    case X86_OP_REG:
        dest = resolveReg(op.reg);
        break;
    case X86_OP_MEM:
        dest = size_t(op.mem.disp);
        if(op.mem.base == X86_REG_RIP) //rip-relative
            dest += Address() + Size();
        else
            dest += resolveReg(op.mem.base) + resolveReg(op.mem.index) * op.mem.scale;
        break;
    default:
        break;
    }
    return dest;
}

bool Capstone::IsBranchGoingToExecute(size_t cflags, size_t ccx) const
{
    auto bCF = (cflags & (1 << 0)) != 0;
    auto bPF = (cflags & (1 << 2)) != 0;
    auto bZF = (cflags & (1 << 6)) != 0;
    auto bSF = (cflags & (1 << 7)) != 0;
    auto bOF = (cflags & (1 << 11)) != 0;
    switch(GetId())
    {
    case X86_INS_CALL:
    case X86_INS_LJMP:
    case X86_INS_JMP:
    case X86_INS_RET:
    case X86_INS_RETF:
    case X86_INS_RETFQ:
        return true;
    case X86_INS_JAE:
        return !bCF;
    case X86_INS_JA:
        return !bCF && !bZF;
    case X86_INS_JBE:
        return bCF && bZF;
    case X86_INS_JB:
        return bCF;
    case X86_INS_JCXZ:
    case X86_INS_JECXZ:
    case X86_INS_JRCXZ:
        return ccx == 0;
    case X86_INS_JE:
        return bZF;
    case X86_INS_JGE:
        return bSF == bOF;
    case X86_INS_JG:
        return !bZF && bSF == bOF;
    case X86_INS_JLE:
        return bZF || bSF != bOF;
    case X86_INS_JL:
        return bSF != bOF;
    case X86_INS_JNE:
        return !bZF;
    case X86_INS_JNO:
        return !bOF;
    case X86_INS_JNP:
        return !bPF;
    case X86_INS_JNS:
        return !bSF;
    case X86_INS_JO:
        return bOF;
    case X86_INS_JP:
        return bPF;
    case X86_INS_JS:
        return bSF;
    case X86_INS_LOOP:
        return ccx != 0;
    case X86_INS_LOOPE:
        return ccx != 0 && bZF;
    case X86_INS_LOOPNE:
        return ccx != 0 && !bZF;
    default:
        return false;
    }
}
