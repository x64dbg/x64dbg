#include <ElfBug/thread/Registers.h>
#include <sys/ptrace.h>

namespace ElfBug
{
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

    Registers::Word & Registers::Gax() { return mRegs.rax; }
    Registers::Word & Registers::Gbx() { return mRegs.rbx; }
    Registers::Word & Registers::Gcx() { return mRegs.rcx; }
    Registers::Word & Registers::Gdx() { return mRegs.rdx; }
    Registers::Word & Registers::Gdi() { return mRegs.rdi; }
    Registers::Word & Registers::Gsi() { return mRegs.rsi; }
    Registers::Word & Registers::Gbp() { return mRegs.rbp; }
    Registers::Word & Registers::Gsp() { return mRegs.rsp; }
    Registers::Word & Registers::Gip() { return mRegs.rip; }

    Registers::Word & Registers::Rax() { return mRegs.rax; }
    Registers::Word & Registers::Rbx() { return mRegs.rbx; }
    Registers::Word & Registers::Rcx() { return mRegs.rcx; }
    Registers::Word & Registers::Rdx() { return mRegs.rdx; }
    Registers::Word & Registers::Rsi() { return mRegs.rsi; }
    Registers::Word & Registers::Rdi() { return mRegs.rdi; }
    Registers::Word & Registers::Rbp() { return mRegs.rbp; }
    Registers::Word & Registers::Rsp() { return mRegs.rsp; }
    Registers::Word & Registers::Rip() { return mRegs.rip; }
    Registers::Word & Registers::R8()  { return mRegs.r8; }
    Registers::Word & Registers::R9()  { return mRegs.r9; }
    Registers::Word & Registers::R10() { return mRegs.r10; }
    Registers::Word & Registers::R11() { return mRegs.r11; }
    Registers::Word & Registers::R12() { return mRegs.r12; }
    Registers::Word & Registers::R13() { return mRegs.r13; }
    Registers::Word & Registers::R14() { return mRegs.r14; }
    Registers::Word & Registers::R15() { return mRegs.r15; }

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
