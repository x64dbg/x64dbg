#ifndef _CAPSTONE_WRAPPER_H
#define _CAPSTONE_WRAPPER_H

#include "capstone\capstone.h"

#define MAX_DISASM_BUFFER 16
#define INVALID_TITAN_REG 0

class Capstone
{
public:
    Capstone();
    ~Capstone();
    bool Disassemble(uint addr, unsigned char data[MAX_DISASM_BUFFER]);
    bool Disassemble(uint addr, const unsigned char* data, int size);
    const cs_insn* GetInstr();
    const cs_err GetError();
    const char* RegName(x86_reg reg);
    bool InGroup(cs_group_type group);
    String OperandText(int opindex);
    const int Size();
    const uint Address();
    const cs_x86 & x86();
    bool IsFilling();
    x86_insn GetId();
    String InstructionText();

private:
    csh mHandle;
    cs_insn* mInstr;
    cs_err mError;
};

#endif //_CAPSTONE_WRAPPER_H