#include <ElfBug/thread/Registers.h>
#include <sys/ptrace.h>

namespace ElfBug
{
    static_assert(sizeof(ptr) == sizeof(decltype(user_regs_struct{} .rax)),
                  "ptr and user_regs_struct register fields must have matching size "
                  "for the reinterpret_cast accessors below to be safe");

    Registers::Registers(const pid_t tid)
        : mTid(tid)
    {
    }

    bool Registers::Read()
    {
        return ptrace(PTRACE_GETREGS, mTid, nullptr, &mRegs) != -1;
    }

    bool Registers::Write()
    {
        return ptrace(PTRACE_SETREGS, mTid, nullptr, &mRegs) != -1;
    }

    ptr & Registers::Gax() { return reinterpret_cast<ptr &>(mRegs.rax); }
    ptr & Registers::Gbx() { return reinterpret_cast<ptr &>(mRegs.rbx); }
    ptr & Registers::Gcx() { return reinterpret_cast<ptr &>(mRegs.rcx); }
    ptr & Registers::Gdx() { return reinterpret_cast<ptr &>(mRegs.rdx); }
    ptr & Registers::Gdi() { return reinterpret_cast<ptr &>(mRegs.rdi); }
    ptr & Registers::Gsi() { return reinterpret_cast<ptr &>(mRegs.rsi); }
    ptr & Registers::Gbp() { return reinterpret_cast<ptr &>(mRegs.rbp); }
    ptr & Registers::Gsp() { return reinterpret_cast<ptr &>(mRegs.rsp); }
    ptr & Registers::Gip() { return reinterpret_cast<ptr &>(mRegs.rip); }

    ptr & Registers::Rax() { return reinterpret_cast<ptr &>(mRegs.rax); }
    ptr & Registers::Rbx() { return reinterpret_cast<ptr &>(mRegs.rbx); }
    ptr & Registers::Rcx() { return reinterpret_cast<ptr &>(mRegs.rcx); }
    ptr & Registers::Rdx() { return reinterpret_cast<ptr &>(mRegs.rdx); }
    ptr & Registers::Rsi() { return reinterpret_cast<ptr &>(mRegs.rsi); }
    ptr & Registers::Rdi() { return reinterpret_cast<ptr &>(mRegs.rdi); }
    ptr & Registers::Rbp() { return reinterpret_cast<ptr &>(mRegs.rbp); }
    ptr & Registers::Rsp() { return reinterpret_cast<ptr &>(mRegs.rsp); }
    ptr & Registers::Rip() { return reinterpret_cast<ptr &>(mRegs.rip); }
    ptr & Registers::R8()  { return reinterpret_cast<ptr &>(mRegs.r8); }
    ptr & Registers::R9()  { return reinterpret_cast<ptr &>(mRegs.r9); }
    ptr & Registers::R10() { return reinterpret_cast<ptr &>(mRegs.r10); }
    ptr & Registers::R11() { return reinterpret_cast<ptr &>(mRegs.r11); }
    ptr & Registers::R12() { return reinterpret_cast<ptr &>(mRegs.r12); }
    ptr & Registers::R13() { return reinterpret_cast<ptr &>(mRegs.r13); }
    ptr & Registers::R14() { return reinterpret_cast<ptr &>(mRegs.r14); }
    ptr & Registers::R15() { return reinterpret_cast<ptr &>(mRegs.r15); }

    bool Registers::TrapFlag() const
    {
        return (mRegs.eflags & (1 << 8)) != 0;
    }

    void Registers::SetTrapFlag(const bool set)
    {
        if(set)
            mRegs.eflags |= (1 << 8);
        else
            mRegs.eflags &= ~(1 << 8);
    }

    bool Registers::ResumeFlag() const
    {
        return (mRegs.eflags & (1 << 16)) != 0;
    }

    void Registers::SetResumeFlag(const bool set)
    {
        if(set)
            mRegs.eflags |= (1 << 16);
        else
            mRegs.eflags &= ~(1 << 16);
    }

    ptr Registers::Dr0() const { return 0; }
    ptr Registers::Dr1() const { return 0; }
    ptr Registers::Dr2() const { return 0; }
    ptr Registers::Dr3() const { return 0; }
    ptr Registers::Dr6() const { return 0; }
    ptr Registers::Dr7() const { return 0; }
    bool Registers::SetDr0(const ptr value) { (void)value; return false; }
    bool Registers::SetDr1(const ptr value) { (void)value; return false; }
    bool Registers::SetDr2(const ptr value) { (void)value; return false; }
    bool Registers::SetDr3(const ptr value) { (void)value; return false; }
    bool Registers::SetDr6(const ptr value) { (void)value; return false; }
    bool Registers::SetDr7(const ptr value) { (void)value; return false; }
}
