#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include "zydis_wrapper.h"
#include <algorithm>
#include <stdexcept>

static const char* ZydisMnemonicGetStringHook(ZydisMnemonic mnemonic)
{
    switch(mnemonic)
    {
    case ZYDIS_MNEMONIC_JZ:
        return "je";
    case ZYDIS_MNEMONIC_JNZ:
        return "jne";
    case ZYDIS_MNEMONIC_JNBE:
        return "ja";
    case ZYDIS_MNEMONIC_JNB:
        return "jae";
    case ZYDIS_MNEMONIC_JNLE:
        return "jg";
    case ZYDIS_MNEMONIC_JNL:
        return "jge";
    case ZYDIS_MNEMONIC_CMOVNBE:
        return "cmova";
    case ZYDIS_MNEMONIC_CMOVNB:
        return "cmovae";
    case ZYDIS_MNEMONIC_CMOVZ:
        return "cmove";
    case ZYDIS_MNEMONIC_CMOVNLE:
        return "cmovg";
    case ZYDIS_MNEMONIC_CMOVNL:
        return "cmovge";
    case ZYDIS_MNEMONIC_CMOVNZ:
        return "cmovne";
    case ZYDIS_MNEMONIC_SETNBE:
        return "seta";
    case ZYDIS_MNEMONIC_SETNB:
        return "setae";
    case ZYDIS_MNEMONIC_SETZ:
        return "sete";
    case ZYDIS_MNEMONIC_SETNLE:
        return "setg";
    case ZYDIS_MNEMONIC_SETNL:
        return "setge";
    case ZYDIS_MNEMONIC_SETNZ:
        return "setne";
    default:
        return ZydisMnemonicGetString(mnemonic);
    }
}


Zydis::Zydis(bool disasm64)
{
    Reset(disasm64);
    ZydisFormatterInit(&mFormatter, ZYDIS_FORMATTER_STYLE_INTEL);
    ZydisFormatterSetProperty(&mFormatter, ZYDIS_FORMATTER_PROP_FORCE_SIZE, ZYAN_TRUE);
    ZydisFormatterSetProperty(&mFormatter, ZYDIS_FORMATTER_PROP_FORCE_SEGMENT, ZYAN_TRUE);
}

void Zydis::Reset(bool disasm64)
{
    mSuccess = false;
    mDisasm64 = disasm64;

    memset(&mInstr, 0, sizeof(mInstr));
    if(disasm64)
        ZydisDecoderInit(&mDecoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    else
        ZydisDecoderInit(&mDecoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
}

bool Zydis::Disassemble(uint64_t addr, const unsigned char data[MAX_DISASM_BUFFER])
{
    return Disassemble(addr, data, MAX_DISASM_BUFFER);
}

bool Zydis::Disassemble(uint64_t addr, const unsigned char* data, size_t size)
{
    if(data == nullptr || size == 0)
        return false;

    mAddr = addr;
    mSuccess = false;

    // Decode instruction.
    if(!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&mDecoder, data, size, &mInstr.info, mInstr.operands)))
        return false;

    // Format it to human readable representation.
    if(!ZYAN_SUCCESS(ZydisFormatterFormatInstruction(
                         &mFormatter,
                         &mInstr.info,
                         mInstr.operands,
                         mInstr.info.operand_count,
                         mInstrText,
                         sizeof(mInstrText),
                         mAddr,
                         nullptr)))
        return false;

    for(ZyanU8 i = 0; i < mInstr.info.operand_count; ++i)
    {
        auto & op = mInstr.operands[i];

        // Rebase IMM if relative and DISP if absolute (codebase expects it this way).
        // Once, at some point in time, the disassembler is abstracted away more and more,
        // we should probably refrain from hacking the Zydis data structure and perform
        // such transformations in the getters instead.
        if(op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE && op.imm.is_relative)
        {
            ZydisCalcAbsoluteAddress(&mInstr.info, &op, mAddr, &op.imm.value.u);
            op.imm.is_relative = false; //hack to prevent OperandText from returning bogus values
        }
        else if(op.type == ZYDIS_OPERAND_TYPE_MEMORY &&
                op.mem.base == ZYDIS_REGISTER_NONE &&
                op.mem.index == ZYDIS_REGISTER_NONE &&
                op.mem.disp.value != 0)
        {
            //TODO: what is this used for?
            ZydisCalcAbsoluteAddress(&mInstr.info, &op, mAddr, (uint64_t*)&op.mem.disp.value);
        }

    }

    mSuccess = true;
    return true;
}

bool Zydis::DisassembleSafe(uint64_t addr, const unsigned char* data, size_t size)
{
    unsigned char dataSafe[MAX_DISASM_BUFFER];
    memset(dataSafe, 0, sizeof(dataSafe));
    memcpy(dataSafe, data, std::min(sizeof(dataSafe), size));
    return Disassemble(addr, dataSafe);
}

const ZydisDisassembledInstruction* Zydis::GetInstr() const
{
    if(!Success())
        return nullptr;
    return &mInstr;
}

bool Zydis::Success() const
{
    return mSuccess;
}

const char* Zydis::RegName(ZydisRegister reg) const
{
    switch(reg)
    {
    case ZYDIS_REGISTER_ST0:
        return "st(0)";
    case ZYDIS_REGISTER_ST1:
        return "st(1)";
    case ZYDIS_REGISTER_ST2:
        return "st(2)";
    case ZYDIS_REGISTER_ST3:
        return "st(3)";
    case ZYDIS_REGISTER_ST4:
        return "st(4)";
    case ZYDIS_REGISTER_ST5:
        return "st(5)";
    case ZYDIS_REGISTER_ST6:
        return "st(6)";
    case ZYDIS_REGISTER_ST7:
        return "st(7)";
    default:
        return ZydisRegisterGetString(reg);
    }
}

std::string Zydis::OperandText(uint8_t opindex) const
{
    if(!Success() || opindex >= mInstr.info.operand_count)
        return {};

    auto & op = mInstr.operands[opindex];
    char buf[200] = {};
    if(!ZYAN_SUCCESS(ZydisFormatterFormatOperand(&this->mFormatter, &mInstr.info, &op, buf, sizeof(buf), mAddr, nullptr)))
        return {};

    //Extract only the part inside the []
    if(op.type == ZYDIS_OPERAND_TYPE_MEMORY)
    {
        auto openBracket = strchr(buf, '[');
        if(openBracket)
        {
            std::string result;
            result = openBracket + 1;
            if(result.back() == ']')
                result.pop_back();
            return result;
        }
    }
    return buf;
}

uint8_t Zydis::Size() const
{
    if(!Success())
        return 1;
    return GetInstr()->info.length;
}

uint64_t Zydis::Address() const
{
    if(!Success())
        return 0;
    return mAddr;
}

bool Zydis::IsFilling() const
{
    if(!Success())
        return false;

    switch(mInstr.info.mnemonic)
    {
    case ZYDIS_MNEMONIC_NOP:
    case ZYDIS_MNEMONIC_INT3:
        return true;
    default:
        return false;
    }
}

bool Zydis::IsBranchType(std::underlying_type<BranchType>::type bt) const
{
    if(!Success())
        return false;

    std::underlying_type<BranchType>::type ref = 0;
    const auto & op0 = mInstr.operands[0];

    switch(mInstr.info.mnemonic)
    {
    case ZYDIS_MNEMONIC_RET:
        ref = (mInstr.info.meta.branch_type == ZYDIS_BRANCH_TYPE_FAR) ? BTFarRet : BTRet;
        break;
    case ZYDIS_MNEMONIC_CALL:
        ref = (mInstr.info.meta.branch_type == ZYDIS_BRANCH_TYPE_FAR) ? BTFarCall : BTCall;
        break;
    case ZYDIS_MNEMONIC_JMP:
        ref = (mInstr.info.meta.branch_type == ZYDIS_BRANCH_TYPE_FAR) ? BTFarJmp : BTUncondJmp;
        break;
    case ZYDIS_MNEMONIC_JB:
    case ZYDIS_MNEMONIC_JBE:
    case ZYDIS_MNEMONIC_JCXZ:
    case ZYDIS_MNEMONIC_JECXZ:
    case ZYDIS_MNEMONIC_JKNZD:
    case ZYDIS_MNEMONIC_JKZD:
    case ZYDIS_MNEMONIC_JL:
    case ZYDIS_MNEMONIC_JLE:
    case ZYDIS_MNEMONIC_JNB:
    case ZYDIS_MNEMONIC_JNBE:
    case ZYDIS_MNEMONIC_JNL:
    case ZYDIS_MNEMONIC_JNLE:
    case ZYDIS_MNEMONIC_JNO:
    case ZYDIS_MNEMONIC_JNP:
    case ZYDIS_MNEMONIC_JNS:
    case ZYDIS_MNEMONIC_JNZ:
    case ZYDIS_MNEMONIC_JO:
    case ZYDIS_MNEMONIC_JP:
    case ZYDIS_MNEMONIC_JRCXZ:
    case ZYDIS_MNEMONIC_JS:
    case ZYDIS_MNEMONIC_JZ:
        ref = BTCondJmp;
        break;
    case ZYDIS_MNEMONIC_SYSCALL:
    case ZYDIS_MNEMONIC_SYSENTER:
        ref = BTSyscall;
        break;
    case ZYDIS_MNEMONIC_SYSRET:
    case ZYDIS_MNEMONIC_SYSEXIT:
        ref = BTSysret;
        break;
    case ZYDIS_MNEMONIC_INT:
        ref = BTInt;
        break;
    case ZYDIS_MNEMONIC_INT3:
        ref = BTInt3;
        break;
    case ZYDIS_MNEMONIC_INT1:
        ref = BTInt1;
        break;
    case ZYDIS_MNEMONIC_IRET:
    case ZYDIS_MNEMONIC_IRETD:
    case ZYDIS_MNEMONIC_IRETQ:
        ref = BTIret;
        break;
    case ZYDIS_MNEMONIC_XBEGIN:
        ref = BTXbegin;
        break;
    case ZYDIS_MNEMONIC_XABORT:
        ref = BTXabort;
        break;
    case ZYDIS_MNEMONIC_RSM:
        ref = BTRsm;
        break;
    case ZYDIS_MNEMONIC_LOOP:
    case ZYDIS_MNEMONIC_LOOPE:
    case ZYDIS_MNEMONIC_LOOPNE:
        ref = BTLoop;
    default:
        ;
    }

    return (bt & ref) != 0;
}

ZydisMnemonic Zydis::GetId() const
{
    if(!Success())
        return ZYDIS_MNEMONIC_INVALID;
    return mInstr.info.mnemonic;
}

std::string Zydis::InstructionText(bool replaceRipRelative) const
{
    if(!Success())
        return "???";

    std::string result = mInstrText;

    // TODO (ath): We can do that a whole lot sexier using formatter hooks
    if(mDisasm64 && replaceRipRelative)
    {
        //replace [rip +/- 0x?] with the actual address
        bool ripPlus = true;
        auto found = result.find("[rip + ");
        if(found == std::string::npos)
        {
            ripPlus = false;
            found = result.find("[rip - ");
        }
        if(found != std::string::npos)
        {
            auto wVA = Address();
            auto end = result.find("]", found);
            auto ripStr = result.substr(found + 1, end - found - 1);
            uint64_t offset;
#ifdef _MSC_VER
            sscanf_s(ripStr.substr(ripStr.rfind(' ') + 1).c_str(), "%llX", &offset);
#else
            sscanf(ripStr.substr(ripStr.rfind(' ') + 1).c_str(), "%llX", &offset);
#endif // _MSC_VER
            auto dest = ripPlus ? (wVA + offset + Size()) : (wVA - offset + Size());
            char buf[20];
#ifdef _MSC_VER
            sprintf_s(buf, "0x%llx", dest);
#else
            snprintf(buf, sizeof(buf), "0x%llx", dest);
#endif // _MSC_VER
            result.replace(found + 1, ripStr.length(), buf);
        }
    }

    return result;
}


uint8_t Zydis::VisibleOpCount() const
{
    if(!Success())
        return 0;
    return mInstr.info.operand_count_visible;
}

uint8_t Zydis::OpCount() const
{
    if(!Success())
        return 0;
    return mInstr.info.operand_count;
}

const ZydisDecodedOperand & Zydis::operator[](uint8_t index) const
{
    if(!Success() || index >= OpCount())
        throw std::out_of_range("Operand out of range");
    return mInstr.operands[index];
}

bool Zydis::IsSafeNopRegOp(const ZydisDecodedOperand & op) const
{
    if(!mDisasm64)
        return true;

    if(op.type != ZYDIS_OPERAND_TYPE_REGISTER)
        return true; //a non-register is safe
    switch(op.reg.value)
    {
    case ZYDIS_REGISTER_EAX:
    case ZYDIS_REGISTER_EBX:
    case ZYDIS_REGISTER_ECX:
    case ZYDIS_REGISTER_EDX:
    case ZYDIS_REGISTER_EBP:
    case ZYDIS_REGISTER_ESP:
    case ZYDIS_REGISTER_ESI:
    case ZYDIS_REGISTER_EDI:
    case ZYDIS_REGISTER_R8D:
    case ZYDIS_REGISTER_R9D:
    case ZYDIS_REGISTER_R10D:
    case ZYDIS_REGISTER_R11D:
    case ZYDIS_REGISTER_R12D:
    case ZYDIS_REGISTER_R13D:
    case ZYDIS_REGISTER_R14D:
    case ZYDIS_REGISTER_R15D:
        return false; //32 bit register modifications clear the high part of the 64 bit register
    default:
        return true; //all other registers are safe
    }
}

bool Zydis::IsNop() const
{
    if(!Success())
        return false;

    const auto & ops = mInstr.operands;

    switch(mInstr.info.mnemonic)
    {
    case ZYDIS_MNEMONIC_NOP:
    case ZYDIS_MNEMONIC_PAUSE:
    case ZYDIS_MNEMONIC_FNOP:
        // nop
        return true;
    case ZYDIS_MNEMONIC_MOV:
    case ZYDIS_MNEMONIC_CMOVB:
    case ZYDIS_MNEMONIC_CMOVBE:
    case ZYDIS_MNEMONIC_CMOVL:
    case ZYDIS_MNEMONIC_CMOVLE:
    case ZYDIS_MNEMONIC_CMOVNB:
    case ZYDIS_MNEMONIC_CMOVNBE:
    case ZYDIS_MNEMONIC_CMOVNL:
    case ZYDIS_MNEMONIC_CMOVNLE:
    case ZYDIS_MNEMONIC_CMOVNO:
    case ZYDIS_MNEMONIC_CMOVNP:
    case ZYDIS_MNEMONIC_CMOVNS:
    case ZYDIS_MNEMONIC_CMOVNZ:
    case ZYDIS_MNEMONIC_CMOVO:
    case ZYDIS_MNEMONIC_CMOVP:
    case ZYDIS_MNEMONIC_CMOVS:
    case ZYDIS_MNEMONIC_CMOVZ:
    case ZYDIS_MNEMONIC_MOVAPS:
    case ZYDIS_MNEMONIC_MOVAPD:
    case ZYDIS_MNEMONIC_MOVUPS:
    case ZYDIS_MNEMONIC_MOVUPD:
    case ZYDIS_MNEMONIC_XCHG:
        // mov edi, edi
        return ops[0].type == ZYDIS_OPERAND_TYPE_REGISTER
               && ops[1].type == ZYDIS_OPERAND_TYPE_REGISTER
               && ops[0].reg.value == ops[1].reg.value
               && IsSafeNopRegOp(ops[0]);
    case ZYDIS_MNEMONIC_LEA:
    {
        // lea eax, [eax + 0]
        auto reg = ops[0].reg.value;
        auto mem = ops[1].mem;
        return ops[0].type == ZYDIS_OPERAND_TYPE_REGISTER
               && ops[1].type == ZYDIS_OPERAND_TYPE_REGISTER
               && mem.disp.value == 0
               && ((mem.index == ZYDIS_REGISTER_NONE && mem.base == reg) ||
                   (mem.index == reg && mem.base == ZYDIS_REGISTER_NONE && mem.scale == 1))
               && IsSafeNopRegOp(ops[0]);
    }
    case ZYDIS_MNEMONIC_JB:
    case ZYDIS_MNEMONIC_JBE:
    case ZYDIS_MNEMONIC_JCXZ:
    case ZYDIS_MNEMONIC_JECXZ:
    case ZYDIS_MNEMONIC_JKNZD:
    case ZYDIS_MNEMONIC_JKZD:
    case ZYDIS_MNEMONIC_JL:
    case ZYDIS_MNEMONIC_JLE:
    case ZYDIS_MNEMONIC_JMP:
    case ZYDIS_MNEMONIC_JNB:
    case ZYDIS_MNEMONIC_JNBE:
    case ZYDIS_MNEMONIC_JNL:
    case ZYDIS_MNEMONIC_JNLE:
    case ZYDIS_MNEMONIC_JNO:
    case ZYDIS_MNEMONIC_JNP:
    case ZYDIS_MNEMONIC_JNS:
    case ZYDIS_MNEMONIC_JNZ:
    case ZYDIS_MNEMONIC_JO:
    case ZYDIS_MNEMONIC_JP:
    case ZYDIS_MNEMONIC_JRCXZ:
    case ZYDIS_MNEMONIC_JS:
    case ZYDIS_MNEMONIC_JZ:
        // jmp $0
        return ops[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE
               && ops[0].imm.value.u == this->Address() + this->Size();
    case ZYDIS_MNEMONIC_SHL:
    case ZYDIS_MNEMONIC_SHR:
    case ZYDIS_MNEMONIC_ROL:
    case ZYDIS_MNEMONIC_ROR:
    case ZYDIS_MNEMONIC_SAR:
        // shl eax, 0
        return ops[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE
               && ops[1].imm.value.u == 0
               && IsSafeNopRegOp(ops[0]);
    case ZYDIS_MNEMONIC_SHLD:
    case ZYDIS_MNEMONIC_SHRD:
        // shld eax, ebx, 0
        return ops[2].type == ZYDIS_OPERAND_TYPE_IMMEDIATE
               && ops[2].imm.value.u == 0
               && IsSafeNopRegOp(ops[0])
               && IsSafeNopRegOp(ops[1]);
    default:
        return false;
    }
}

bool Zydis::IsPushPop() const
{
    if(!Success())
        return false;

    switch(mInstr.info.meta.category)
    {
    case ZYDIS_CATEGORY_PUSH:
    case ZYDIS_CATEGORY_POP:
        return true;
    default:
        return false;
    }
}


bool Zydis::IsUnusual() const
{
    if(!Success())
        return false;

    auto id = mInstr.info.mnemonic;
    return mInstr.info.attributes & ZYDIS_ATTRIB_IS_PRIVILEGED
           || mInstr.info.meta.category == ZYDIS_CATEGORY_IO
           || mInstr.info.meta.category == ZYDIS_CATEGORY_IOSTRINGOP
           || mInstr.info.meta.category == ZYDIS_CATEGORY_RDWRFSGS
           || mInstr.info.meta.category == ZYDIS_CATEGORY_SGX
           || mInstr.info.meta.category == ZYDIS_CATEGORY_INTERRUPT
           || id == ZYDIS_MNEMONIC_SYSCALL
           || id == ZYDIS_MNEMONIC_SYSENTER
           || id == ZYDIS_MNEMONIC_CPUID
           || id == ZYDIS_MNEMONIC_RDTSC
           || id == ZYDIS_MNEMONIC_RDTSCP
           || id == ZYDIS_MNEMONIC_RDRAND
           || id == ZYDIS_MNEMONIC_RDSEED
           || id == ZYDIS_MNEMONIC_RDPID
           || id == ZYDIS_MNEMONIC_RDPKRU
           // || id == ZYDIS_MNEMONIC_RDPRU
           || id == ZYDIS_MNEMONIC_UD1
           || id == ZYDIS_MNEMONIC_UD2
           || id == ZYDIS_MNEMONIC_VMCALL
           || id == ZYDIS_MNEMONIC_VMFUNC
           || id == ZYDIS_MNEMONIC_OUTSB
           || id == ZYDIS_MNEMONIC_OUTSW
           || id == ZYDIS_MNEMONIC_OUTSD
           || id == ZYDIS_MNEMONIC_WRPKRU;
}

std::string Zydis::Mnemonic() const
{
    if(!Success())
        return "???";
    return ZydisMnemonicGetStringHook(mInstr.info.mnemonic);
}

const char* Zydis::MemSizeName(size_t size) const
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

uint64_t Zydis::BranchDestination() const
{
    if(!Success()
            || mInstr.operands[0].type != ZYDIS_OPERAND_TYPE_IMMEDIATE
            /*|| !mInstr.operands[0].imm.isRelative HACKED*/)
        return 0;

    return uint64_t(mInstr.operands[0].imm.value.u);
}

uint64_t Zydis::ResolveOpValue(uint8_t opindex, const std::function<uint64_t(ZydisRegister)> & resolveReg) const
{
    uint64_t dest = 0;
    const auto & op = mInstr.operands[opindex];
    switch(op.type)
    {
    case ZYDIS_OPERAND_TYPE_IMMEDIATE:
        dest = uint64_t(op.imm.value.u);
        if(!IsBranchType(Zydis::BTJmp | Zydis::BTCall | Zydis::BTLoop | Zydis::BTXbegin))
            dest &= (uint64_t(-1) >> (sizeof(uint64_t) * 8 - mInstr.info.operand_width));
        break;
    case ZYDIS_OPERAND_TYPE_REGISTER:
        dest = resolveReg(op.reg.value);
        break;
    case ZYDIS_OPERAND_TYPE_MEMORY:
        dest = uint64_t(op.mem.disp.value);
        if(op.mem.base == ZYDIS_REGISTER_RIP) //rip-relative
            dest += Address() + Size();
        else
            dest += resolveReg(op.mem.base) + resolveReg(op.mem.index) * op.mem.scale;
        break;
    default:
        break;
    }
    return dest;
}

Zydis::VectorElementType Zydis::getVectorElementType(uint8_t opindex) const
{
    if(!Success())
        return Zydis::VETDefault;
    if(opindex >= mInstr.info.operand_count)
        return Zydis::VETDefault;
    const auto & op = mInstr.operands[opindex];
    switch(op.element_type)
    {
    case ZYDIS_ELEMENT_TYPE_FLOAT32:
        return Zydis::VETFloat32;
    case ZYDIS_ELEMENT_TYPE_FLOAT64:
        return Zydis::VETFloat64;
    default:
        return Zydis::VETDefault;
    }
}

bool Zydis::IsBranchGoingToExecute(uint32_t eflags, uint64_t ccx) const
{
    if(!Success())
        return false;
    return IsBranchGoingToExecute(mInstr.info.mnemonic, eflags, ccx);
}

bool Zydis::IsBranchGoingToExecute(ZydisMnemonic id, uint32_t eflags, uint64_t ccx)
{
    auto bCF = (eflags & (1 << 0)) != 0;
    auto bPF = (eflags & (1 << 2)) != 0;
    auto bZF = (eflags & (1 << 6)) != 0;
    auto bSF = (eflags & (1 << 7)) != 0;
    auto bOF = (eflags & (1 << 11)) != 0;
    switch(id)
    {
    case ZYDIS_MNEMONIC_CALL:
    case ZYDIS_MNEMONIC_JMP:
    case ZYDIS_MNEMONIC_RET:
        return true;
    case ZYDIS_MNEMONIC_JNB: //jump short if above or equal
        return !bCF;
    case ZYDIS_MNEMONIC_JNBE: //jump short if above
        return !bCF && !bZF;
    case ZYDIS_MNEMONIC_JBE: //jump short if below or equal/not above
        return bCF || bZF;
    case ZYDIS_MNEMONIC_JB: //jump short if below/not above nor equal/carry
        return bCF;
    case ZYDIS_MNEMONIC_JCXZ: //jump short if ecx register is zero
    case ZYDIS_MNEMONIC_JECXZ: //jump short if ecx register is zero
    case ZYDIS_MNEMONIC_JRCXZ: //jump short if rcx register is zero
        return ccx == 0;
    case ZYDIS_MNEMONIC_JZ: //jump short if equal
        return bZF;
    case ZYDIS_MNEMONIC_JNL: //jump short if greater or equal
        return bSF == bOF;
    case ZYDIS_MNEMONIC_JNLE: //jump short if greater
        return !bZF && bSF == bOF;
    case ZYDIS_MNEMONIC_JLE: //jump short if less or equal/not greater
        return bZF || bSF != bOF;
    case ZYDIS_MNEMONIC_JL: //jump short if less/not greater
        return bSF != bOF;
    case ZYDIS_MNEMONIC_JNZ: //jump short if not equal/not zero
        return !bZF;
    case ZYDIS_MNEMONIC_JNO: //jump short if not overflow
        return !bOF;
    case ZYDIS_MNEMONIC_JNP: //jump short if not parity/parity odd
        return !bPF;
    case ZYDIS_MNEMONIC_JNS: //jump short if not sign
        return !bSF;
    case ZYDIS_MNEMONIC_JO: //jump short if overflow
        return bOF;
    case ZYDIS_MNEMONIC_JP: //jump short if parity/parity even
        return bPF;
    case ZYDIS_MNEMONIC_JS: //jump short if sign
        return bSF;
    case ZYDIS_MNEMONIC_LOOP: //decrement count; jump short if ecx!=0
        return ccx != 1;
    case ZYDIS_MNEMONIC_LOOPE: //decrement count; jump short if ecx!=0 and zf=1
        return ccx != 1 && bZF;
    case ZYDIS_MNEMONIC_LOOPNE: //decrement count; jump short if ecx!=0 and zf=0
        return ccx != 1 && !bZF;
    default:
        return false;
    }
}

bool Zydis::IsConditionalGoingToExecute(uint32_t eflags, uint64_t ccx) const
{
    if(!Success())
        return false;
    return IsConditionalGoingToExecute(mInstr.info.mnemonic, eflags, ccx);
}

bool Zydis::IsConditionalGoingToExecute(ZydisMnemonic id, uint32_t eflags, uint64_t ccx)
{
    auto bCF = (eflags & (1 << 0)) != 0;
    auto bPF = (eflags & (1 << 2)) != 0;
    auto bZF = (eflags & (1 << 6)) != 0;
    auto bSF = (eflags & (1 << 7)) != 0;
    auto bOF = (eflags & (1 << 11)) != 0;
    switch(id)
    {
    case ZYDIS_MNEMONIC_CMOVNBE: //conditional move - above/not below nor equal
        return !bCF && !bZF;
    case ZYDIS_MNEMONIC_CMOVNB: //conditional move - above or equal/not below/not carry
        return !bCF;
    case ZYDIS_MNEMONIC_CMOVB: //conditional move - below/not above nor equal/carry
        return bCF;
    case ZYDIS_MNEMONIC_CMOVBE: //conditional move - below or equal/not above
        return bCF || bZF;
    case ZYDIS_MNEMONIC_CMOVZ: //conditional move - equal/zero
        return bZF;
    case ZYDIS_MNEMONIC_CMOVNLE: //conditional move - greater/not less nor equal
        return !bZF && bSF == bOF;
    case ZYDIS_MNEMONIC_CMOVNL: //conditional move - greater or equal/not less
        return bSF == bOF;
    case ZYDIS_MNEMONIC_CMOVL: //conditional move - less/not greater nor equal
        return bSF != bOF;
    case ZYDIS_MNEMONIC_CMOVLE: //conditional move - less or equal/not greater
        return bZF || bSF != bOF;
    case ZYDIS_MNEMONIC_CMOVNZ: //conditional move - not equal/not zero
        return !bZF;
    case ZYDIS_MNEMONIC_CMOVNO: //conditional move - not overflow
        return !bOF;
    case ZYDIS_MNEMONIC_CMOVNP: //conditional move - not parity/parity odd
        return !bPF;
    case ZYDIS_MNEMONIC_CMOVNS: //conditional move - not sign
        return !bSF;
    case ZYDIS_MNEMONIC_CMOVO: //conditional move - overflow
        return bOF;
    case ZYDIS_MNEMONIC_CMOVP: //conditional move - parity/parity even
        return bPF;
    case ZYDIS_MNEMONIC_CMOVS: //conditional move - sign
        return bSF;
    case ZYDIS_MNEMONIC_FCMOVBE: //fp conditional move - below or equal
        return bCF || bZF;
    case ZYDIS_MNEMONIC_FCMOVB: //fp conditional move - below
        return bCF;
    case ZYDIS_MNEMONIC_FCMOVE: //fp conditional move - equal
        return bZF;
    case ZYDIS_MNEMONIC_FCMOVNBE: //fp conditional move - not below or equal
        return !bCF && !bZF;
    case ZYDIS_MNEMONIC_FCMOVNB: //fp conditional move - not below
        return !bCF;
    case ZYDIS_MNEMONIC_FCMOVNE: //fp conditional move - not equal
        return !bZF;
    case ZYDIS_MNEMONIC_FCMOVNU: //fp conditional move - not unordered
        return !bPF;
    case ZYDIS_MNEMONIC_FCMOVU: //fp conditional move - unordered
        return bPF;
    case ZYDIS_MNEMONIC_SETNBE: //set byte on condition - above/not below nor equal
        return !bCF && !bZF;
    case ZYDIS_MNEMONIC_SETNB: //set byte on condition - above or equal/not below/not carry
        return !bCF;
    case ZYDIS_MNEMONIC_SETB: //set byte on condition - below/not above nor equal/carry
        return bCF;
    case ZYDIS_MNEMONIC_SETBE: //set byte on condition - below or equal/not above
        return bCF || bZF;
    case ZYDIS_MNEMONIC_SETZ: //set byte on condition - equal/zero
        return bZF;
    case ZYDIS_MNEMONIC_SETNLE: //set byte on condition - greater/not less nor equal
        return !bZF && bSF == bOF;
    case ZYDIS_MNEMONIC_SETNL: //set byte on condition - greater or equal/not less
        return bSF == bOF;
    case ZYDIS_MNEMONIC_SETL: //set byte on condition - less/not greater nor equal
        return bSF != bOF;
    case ZYDIS_MNEMONIC_SETLE: //set byte on condition - less or equal/not greater
        return bZF || bSF != bOF;
    case ZYDIS_MNEMONIC_SETNZ: //set byte on condition - not equal/not zero
        return !bZF;
    case ZYDIS_MNEMONIC_SETNO: //set byte on condition - not overflow
        return !bOF;
    case ZYDIS_MNEMONIC_SETNP: //set byte on condition - not parity/parity odd
        return !bPF;
    case ZYDIS_MNEMONIC_SETNS: //set byte on condition - not sign
        return !bSF;
    case ZYDIS_MNEMONIC_SETO: //set byte on condition - overflow
        return bOF;
    case ZYDIS_MNEMONIC_SETP: //set byte on condition - parity/parity even
        return bPF;
    case ZYDIS_MNEMONIC_SETS: //set byte on condition - sign
        return bSF;
    default:
        return true;
    }
}

void Zydis::RegInfo(uint8_t regs[ZYDIS_REGISTER_MAX_VALUE + 1]) const
{
    memset(regs, 0, sizeof(uint8_t) * (ZYDIS_REGISTER_MAX_VALUE + 1));
    if(!Success() || IsNop())
        return;

    for(int i = 0; i < mInstr.info.operand_count; ++i)
    {
        const auto & op = mInstr.operands[i];

        switch(op.type)
        {
        case ZYDIS_OPERAND_TYPE_REGISTER:
        {
            switch(op.actions)
            {
            case ZYDIS_OPERAND_ACTION_READ:
            case ZYDIS_OPERAND_ACTION_CONDREAD:
                regs[op.reg.value] |= RAIRead;
                break;
            case ZYDIS_OPERAND_ACTION_WRITE:
            case ZYDIS_OPERAND_ACTION_CONDWRITE:
                regs[op.reg.value] |= RAIWrite;
                break;
            case ZYDIS_OPERAND_ACTION_READWRITE:
            case ZYDIS_OPERAND_ACTION_READ_CONDWRITE:
            case ZYDIS_OPERAND_ACTION_CONDREAD_WRITE:
                regs[op.reg.value] |= RAIRead | RAIWrite;
                break;
            }
            regs[op.reg.value] |= op.visibility == ZYDIS_OPERAND_VISIBILITY_HIDDEN ?
                                  RAIImplicit : RAIExplicit;
        }
        break;

        case ZYDIS_OPERAND_TYPE_MEMORY:
        {
            regs[op.mem.segment] |= RAIRead | RAIExplicit;
            if(op.mem.base != ZYDIS_REGISTER_NONE)
                regs[op.mem.base] |= RAIRead | RAIExplicit;
            if(op.mem.index != ZYDIS_REGISTER_NONE)
                regs[op.mem.index] |= RAIRead | RAIExplicit;
        }
        break;

        default:
            break;
        }
    }
}

void Zydis::FlagInfo(uint8_t info[32]) const
{
    auto instr = GetInstr();
    if(instr == nullptr)
    {
        memset(info, 0, 32);
        return;
    }

    for(uint8_t i = 0; i < 32; i++)
    {
        auto flag = 1u << i;
        uint8_t rai = Zydis::RAINone;
        if(FlagName(flag) != nullptr)
        {
            const auto & flags = *instr->info.cpu_flags;
            if((flags.tested & flag) == flag)
            {
                rai |= Zydis::RAIRead;
            }
            auto writeMask = flags.modified | flags.set_0 | flags.set_1 | flags.undefined;
            if((writeMask & flag) == flag)
            {
                rai |= Zydis::RAIWrite;
            }
        }
        info[i] = rai;
    }
}

const char* Zydis::FlagName(uint32_t flag) const
{
    switch(flag)
    {
    case ZYDIS_CPUFLAG_CF:
        return "CF";
    case ZYDIS_CPUFLAG_PF:
        return "PF";
    case ZYDIS_CPUFLAG_AF:
        return "AF";
    case ZYDIS_CPUFLAG_ZF:
        return "ZF";
    case ZYDIS_CPUFLAG_SF:
        return "SF";
    case ZYDIS_CPUFLAG_TF:
        return "TF";
    case ZYDIS_CPUFLAG_IF:
        return "IF";
    case ZYDIS_CPUFLAG_DF:
        return "DF";
    case ZYDIS_CPUFLAG_OF:
        return "OF";
    case ZYDIS_CPUFLAG_IOPL:
        return "IOPL";
    case ZYDIS_CPUFLAG_NT:
        return "NT";
    case ZYDIS_CPUFLAG_RF:
        return "RF";
    case ZYDIS_CPUFLAG_VM:
        return "VM";
    case ZYDIS_CPUFLAG_AC:
        return "AC";
    case ZYDIS_CPUFLAG_VIF:
        return "VIF";
    case ZYDIS_CPUFLAG_VIP:
        return "VIP";
    case ZYDIS_CPUFLAG_ID:
        return "ID";
    default:
        return nullptr;
    }
}

void Zydis::BytesGroup(uint8_t* prefixSize, uint8_t* opcodeSize, uint8_t* group1Size, uint8_t* group2Size, uint8_t* group3Size) const
{
    if(Success())
    {
        *prefixSize = mInstr.info.raw.prefix_count;
        *group1Size = mInstr.info.raw.disp.size / 8;
        *group2Size = mInstr.info.raw.imm[0].size / 8;
        *group3Size = mInstr.info.raw.imm[1].size / 8;
        *opcodeSize = mInstr.info.length - *prefixSize - *group1Size - *group2Size - *group3Size;
    }
    else
    {
        *prefixSize = 0;
        *opcodeSize = mInstr.info.length;
        *group1Size = 0;
        *group2Size = 0;
        *group3Size = 0;
    }
}
