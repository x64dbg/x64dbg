#pragma once

#define ZYCORE_STATIC_BUILD
#define ZYDIS_STATIC_BUILD
#include "Zydis/Zydis.h"
#include <type_traits>
#include <functional>
#include <string>

#define MAX_DISASM_BUFFER 16

class Zydis
{
public:
#ifdef _WIN64
    Zydis() : Zydis(true) {}
#elif _WIN32
    Zydis() : Zydis(false) {}
#endif // _WIN64

    explicit Zydis(bool disasm64);
    Zydis(const Zydis & zydis) = delete;
    ~Zydis() = default;
    void Reset(bool disasm64);
    bool Disassemble(uint64_t addr, const unsigned char data[MAX_DISASM_BUFFER]);
    bool Disassemble(uint64_t addr, const unsigned char* data, size_t size);
    bool DisassembleSafe(uint64_t addr, const unsigned char* data, size_t size);
    const ZydisDisassembledInstruction* GetInstr() const;
    bool Success() const;
    const char* RegName(ZydisRegister reg) const;
    std::string OperandText(uint8_t opindex) const;
    uint8_t Size() const;
    uint64_t Address() const;
    bool IsFilling() const;
    bool IsUnusual() const;
    bool IsNop() const;
    bool IsPushPop() const;
    ZydisMnemonic GetId() const;
    std::string InstructionText(bool replaceRipRelative = true) const;
    uint8_t OpCount() const;
    const ZydisDecodedOperand & operator[](uint8_t index) const;
    std::string Mnemonic() const;
    const char* MemSizeName(size_t size) const;
    uint64_t BranchDestination() const;
    uint64_t ResolveOpValue(uint8_t opindex, const std::function<uint64_t(ZydisRegister)> & resolveReg) const;
    bool IsBranchGoingToExecute(uint32_t eflags, uint64_t ccx) const;
    static bool IsBranchGoingToExecute(ZydisMnemonic id, uint32_t eflags, uint64_t ccx);
    bool IsConditionalGoingToExecute(uint32_t eflags, uint64_t ccx) const;
    static bool IsConditionalGoingToExecute(ZydisMnemonic id, uint32_t eflags, uint64_t ccx);
    void BytesGroup(uint8_t* prefixSize, uint8_t* opcodeSize, uint8_t* group1Size, uint8_t* group2Size, uint8_t* group3Size) const;

    enum RegAccessInfo : uint8_t
    {
        RAINone        = 0,
        RAIRead        = 1 << 0,
        RAIWrite       = 1 << 1,
        RAIImplicit    = 1 << 2,
        RAIExplicit    = 1 << 3
    };

    void RegInfo(uint8_t info[ZYDIS_REGISTER_MAX_VALUE + 1]) const;
    const char* FlagName(uint32_t flag) const;

    enum BranchType : uint32_t
    {
        // Basic types.
        BTRet          = 1 << 0,
        BTCall         = 1 << 1,
        BTFarCall      = 1 << 2,
        BTFarRet       = 1 << 3,
        BTSyscall      = 1 << 4, // Also sysenter
        BTSysret       = 1 << 5, // Also sysexit
        BTInt          = 1 << 6,
        BTInt3         = 1 << 7,
        BTInt1         = 1 << 8,
        BTIret         = 1 << 9,
        BTCondJmp      = 1 << 10,
        BTUncondJmp    = 1 << 11,
        BTFarJmp       = 1 << 12,
        BTXbegin       = 1 << 13,
        BTXabort       = 1 << 14,
        BTRsm          = 1 << 15,
        BTLoop         = 1 << 16,

        BTJmp          = BTCondJmp | BTUncondJmp,

        // Semantic groups (behaves like XX).
        BTCallSem      = BTCall | BTFarCall | BTSyscall | BTInt,
        BTRetSem       = BTRet | BTSysret | BTIret | BTFarRet | BTRsm,
        BTCondJmpSem   = BTCondJmp | BTLoop | BTXbegin,
        BTUncondJmpSem = BTUncondJmp | BTFarJmp | BTXabort,

        BTRtm          = BTXabort | BTXbegin,
        BTFar          = BTFarCall | BTFarJmp | BTFarRet,

        BTAny          = std::underlying_type<BranchType>::type(-1)
    };

    bool IsBranchType(std::underlying_type<BranchType>::type bt) const;

    enum VectorElementType : uint8_t
    {
        VETDefault,
        VETFloat32,
        VETFloat64,
        VETInt32,
        VETInt64
    };
    VectorElementType getVectorElementType(uint8_t opindex) const;

    // Shortcuts.
    bool IsRet() const { return IsBranchType(BTRet); }
    bool IsCall() const { return IsBranchType(BTCall); }
    bool IsJump() const { return IsBranchType(BTJmp); }
    bool IsLoop() const { return IsBranchType(BTLoop); }
    bool IsInt3() const { return IsBranchType(BTInt3); }
    bool IsSafeNopRegOp(const ZydisDecodedOperand & op) const;

private:
    ZydisDecoder mDecoder;
    ZydisFormatter mFormatter;
    bool mDisasm64;
    uint64_t mAddr = 0;
    ZydisDisassembledInstruction mInstr;
    char mInstrText[200];
    bool mSuccess = false;
    uint8_t mVisibleOpCount = 0;
};
