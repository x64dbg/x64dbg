#ifndef _CAPSTONE_WRAPPER_H
#define _CAPSTONE_WRAPPER_H

#include "capstone/capstone.h"
#include <string>
#include <functional>

#define MAX_DISASM_BUFFER 16

class Capstone
{
public:
    static void GlobalInitialize();
    static void GlobalFinalize();
    Capstone();
    Capstone(const Capstone & capstone); //copy constructor
    ~Capstone();
    bool Disassemble(size_t addr, const unsigned char data[MAX_DISASM_BUFFER]);
    bool Disassemble(size_t addr, const unsigned char* data, int size);
    bool DisassembleSafe(size_t addr, const unsigned char* data, int size);
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
    bool IsUnusual() const;
    x86_insn GetId() const;
    std::string InstructionText() const;
    int OpCount() const;
    const cs_x86_op & operator[](int index) const;
    bool IsNop() const;
    bool IsInt3() const;
    std::string Mnemonic() const;
    std::string MnemonicId() const;
    const char* MemSizeName(int size) const;
    size_t BranchDestination() const;
    size_t ResolveOpValue(int opindex, const std::function<size_t(x86_reg)> & resolveReg) const;
    bool IsBranchGoingToExecute(size_t cflags, size_t ccx) const;

private:
    static csh mHandle;
    static bool mInitialized;
    cs_insn* mInstr;
    bool mSuccess;
};

#endif //_CAPSTONE_WRAPPER_H
