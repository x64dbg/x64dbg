#ifndef _CAPSTONE_WRAPPER_H
#define _CAPSTONE_WRAPPER_H

#include "capstone\capstone.h"

#define MAX_DISASM_BUFFER 16

class Capstone
{
public:
    Capstone();
    ~Capstone();
    bool Disassemble(uint addr, unsigned char data[MAX_DISASM_BUFFER]);
    const cs_insn* GetInstr();
    const cs_err GetError();

private:
    csh mHandle;
    cs_insn* mInstr;
    cs_err mError;
};

#endif //_CAPSTONE_WRAPPER_H