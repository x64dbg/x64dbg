#ifndef ZYDIS_WRAPPER_H
#define ZYDIS_WRAPPER_H

#include "Zydis/Zydis.h"
#include <functional>

#define MAX_DISASM_BUFFER 16

class Zydis
{
public:
    static void GlobalInitialize();
    static void GlobalFinalize();
	Zydis();
	Zydis(const Zydis & capstone) = delete;
	~Zydis();
    bool Disassemble(size_t addr, const unsigned char data[MAX_DISASM_BUFFER]);
    bool Disassemble(size_t addr, const unsigned char* data, int size);
    bool DisassembleSafe(size_t addr, const unsigned char* data, int size);
	const ZydisDecodedInstruction* GetInstr() const;
    bool Success() const;
	const char* RegName(ZydisRegister reg) const;
    std::string OperandText(int opindex) const;
    int Size() const;
    size_t Address() const;
    bool IsFilling() const;
    bool IsUnusual() const;
    bool IsNop() const;
    bool IsPushPop() const;
    ZydisMnemonic GetId() const;
    std::string InstructionText(bool replaceRipRelative = true) const;
    int OpCount() const;
	const ZydisDecodedOperand & operator[](int index) const;
    std::string Mnemonic() const;
    std::string MnemonicId() const;
    const char* MemSizeName(int size) const;
    size_t BranchDestination() const;
	size_t ResolveOpValue(int opindex, const std::function<size_t(ZydisRegister)> & resolveReg) const;
    bool IsBranchGoingToExecute(size_t cflags, size_t ccx) const;
    static bool IsBranchGoingToExecute(ZydisMnemonic id, size_t cflags, size_t ccx);
    bool IsConditionalGoingToExecute(size_t cflags, size_t ccx) const;
    static bool IsConditionalGoingToExecute(ZydisMnemonic id, size_t cflags, size_t ccx);

    enum RegInfoAccess : uint8_t
    {
        None        = 1 << 0,
        Read        = 1 << 0,
        Write       = 1 << 1,
        Implicit    = 1 << 2,
        Explicit    = 1 << 3
    };

    void RegInfo(uint8_t info[ZYDIS_REGISTER_MAX_VALUE + 1]) const;
    const char* FlagName(ZydisCPUFlag flag) const;

    enum BranchType : uint16_t
    {
        // Basic types.
        BT_Ret          = 1 << 0,
        BT_Call         = 1 << 1,
        BT_FarCall      = 1 << 2,
        BT_Syscall      = 1 << 3, // Also sysenter
        BT_Sysret       = 1 << 4, // Also sysexit
        BT_Int          = 1 << 5,
        BT_Int3         = 1 << 6,
        BT_Int1         = 1 << 7,
        BT_Iret         = 1 << 8,
        BT_CondJmp      = 1 << 9,
        BT_UncondJmp    = 1 << 10,
        BT_FarJmp       = 1 << 11,
        BT_Xbegin       = 1 << 12,
        BT_Xabort       = 1 << 13,
        BT_Rsm          = 1 << 14,
        BT_Loop         = 1 << 15,

        BT_Jmp          = BT_CondJmp | BT_UncondJmp,

        // Semantic groups (behaves like XX).
        BT_CallSem      = BT_Call | BT_Syscall | BT_Int,
        BT_RetSem       = BT_Ret | BT_Sysret | BT_Iret | BT_Rsm | BT_Xabort,
        BT_IntSem       = BT_Int | BT_Int1 | BT_Int3 | BT_Syscall,
        BT_IretSem      = BT_Iret | BT_Sysret,
        BT_JmpSem       = BT_Jmp | BT_Loop,

        BT_Rtm          = BT_Xabort | BT_Xbegin,
        BT_CtxSwitch    = BT_IntSem | BT_IretSem | BT_Rsm | BT_FarCall | BT_FarJmp,
        
        BT_Any          = std::underlying_type_t<BranchType>(-1)
    };

    bool IsBranchType(std::underlying_type_t<BranchType> bt) const;

    // Shortcuts.
    bool IsRet   () const { return IsBranchType(BT_Ret);    }
    bool IsCall  () const { return IsBranchType(BT_Call);   }
    bool IsJump  () const { return IsBranchType(BT_Jmp);    }
    bool IsLoop  () const { return IsBranchType(BT_Loop);   }
    bool IsInt3  () const { return IsBranchType(BT_Int3);   }
private:
	static ZydisDecoder mZyDecoder;
	static ZydisFormatter mZyFormatter;
    static bool mInitialized;
	ZydisDecodedInstruction mZyInstr;
    char mInstrText[200];
    bool mSuccess;
    uint8_t mExplicitOpCount;
};

#endif //ZYDIS_WRAPPER_H
