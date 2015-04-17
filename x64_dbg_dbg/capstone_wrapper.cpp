#include "console.h"
#include "capstone_wrapper.h"

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

const char* Capstone::RegName(unsigned int reg)
{
    return cs_reg_name(mHandle, reg);
}