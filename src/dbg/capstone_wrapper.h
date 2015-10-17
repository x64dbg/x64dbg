#ifndef _CAPSTONE_WRAPPER_H
#define _CAPSTONE_WRAPPER_H

#include "capstone\capstone.h"

#define MAX_DISASM_BUFFER 16
#define INVALID_TITAN_REG 0

class Capstone
{
public:
    static void GlobalInitialize();
    static void GlobalFinalize();
    Capstone();
    ~Capstone();
    bool Disassemble(duint addr, const unsigned char data[MAX_DISASM_BUFFER]);
    bool Disassemble(duint addr, const unsigned char* data, int size);
    const cs_insn* GetInstr() const;
    cs_err GetError() const;
    const char* RegName(x86_reg reg) const;
    bool InGroup(cs_group_type group) const;
    String OperandText(int opindex) const;
    int Size() const;
    duint Address() const;
    const cs_x86 & x86() const;
    bool IsFilling() const;
    bool IsLoop() const;
    x86_insn GetId() const;
    String InstructionText() const;

private:
    static csh mHandle;
    cs_insn* mInstr;
    cs_err mError;
};

#endif //_CAPSTONE_WRAPPER_H