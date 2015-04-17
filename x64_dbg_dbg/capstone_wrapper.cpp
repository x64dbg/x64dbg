#include "console.h"
#include "capstone_wrapper.h"
#include "TitanEngine\TitanEngine.h"

Capstone::Capstone()
{
    mHandle = 0;
    mInstr = 0;
#ifdef _WIN64
    mError = cs_open(CS_ARCH_X86, CS_MODE_64, &mHandle);
#else //x86
    mError = cs_open(CS_ARCH_X86, CS_MODE_32, &mHandle);
#endif //_WIN64
    if(mError)
        mHandle = 0;
    else
        cs_option(mHandle, CS_OPT_DETAIL, CS_OPT_ON);
}

Capstone::~Capstone()
{
    if(mInstr)  //free last disassembled instruction
        cs_free(mInstr, 1);
    if(mHandle)  //close handle
        cs_close(&mHandle);
}

bool Capstone::Disassemble(uint addr, unsigned char data[MAX_DISASM_BUFFER])
{
    if(mInstr) //free last disassembled instruction
    {
        cs_free(mInstr, 1);
        mInstr = 0;
    }
    return !!cs_disasm(mHandle, (const uint8_t*)data, MAX_DISASM_BUFFER, addr, 1, &mInstr);
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
                if(prependPlus)
                    result += "+";
                sprintf_s(temp, "%"fext"X", mem.disp);
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