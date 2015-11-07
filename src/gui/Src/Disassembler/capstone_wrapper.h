#ifndef _CAPSTONE_WRAPPER_H
#define _CAPSTONE_WRAPPER_H

#include "..\..\dbg\capstone\capstone.h"
#include "Imports.h"
#include <string>

#define MAX_DISASM_BUFFER 16

class Capstone
{
public:
    static void GlobalInitialize();
    static void GlobalFinalize();
    Capstone();
    ~Capstone();
    bool Disassemble(size_t addr, const unsigned char data[MAX_DISASM_BUFFER]);
    bool Disassemble(size_t addr, const unsigned char* data, int size);
    const cs_insn* GetInstr() const;
    bool Success() const;
    const char* RegName(x86_reg reg) const;
    bool InGroup(cs_group_type group) const;
    std::string OperandText(int opindex) const;
    int Size() const;
    size_t Address() const;
    const cs_x86 & x86() const;
    bool IsFilling() const;
    bool IsLoop() const;
    x86_insn GetId() const;
    std::string InstructionText() const;
    int OpCount() const;
    cs_x86_op operator[](int index) const;
    bool IsNop() const;
    bool IsInt3() const;
    std::string Mnemonic() const;
    const char* MemSizeName(int size) const;
    size_t BranchDestination() const;

private:
    static csh mHandle;
    cs_insn* mInstr;
    bool mSuccess;
};

#endif //_CAPSTONE_WRAPPER_H
