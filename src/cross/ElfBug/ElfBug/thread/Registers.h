#pragma once

#include <sys/types.h>
#include <sys/user.h>
#include <ElfBug/types/ElfBug.h>

namespace ElfBug
{
    class Registers
    {
    public:
        using Word = decltype(user_regs_struct::rax);

        Registers() = default;
        explicit Registers(pid_t tid);

        bool Read();
        bool Write();

        Word & Gax();
        Word & Gbx();
        Word & Gcx();
        Word & Gdx();
        Word & Gdi();
        Word & Gsi();
        Word & Gbp();
        Word & Gsp();
        Word & Gip();

        Word & Rax();
        Word & Rbx();
        Word & Rcx();
        Word & Rdx();
        Word & Rsi();
        Word & Rdi();
        Word & Rbp();
        Word & Rsp();
        Word & Rip();
        Word & R8();
        Word & R9();
        Word & R10();
        Word & R11();
        Word & R12();
        Word & R13();
        Word & R14();
        Word & R15();

        [[nodiscard]] bool TrapFlag() const;
        void SetTrapFlag(bool set);
        [[nodiscard]] bool ResumeFlag() const;
        void SetResumeFlag(bool set);

        // TODO: implement
        [[nodiscard]] ptr Dr0() const;
        [[nodiscard]] ptr Dr1() const;
        [[nodiscard]] ptr Dr2() const;
        [[nodiscard]] ptr Dr3() const;
        [[nodiscard]] ptr Dr6() const;
        [[nodiscard]] ptr Dr7() const;
        bool SetDr0(ptr value);
        bool SetDr1(ptr value);
        bool SetDr2(ptr value);
        bool SetDr3(ptr value);
        bool SetDr6(ptr value);
        bool SetDr7(ptr value);

        [[nodiscard]] const user_regs_struct & Native() const { return mRegs; }

    private:
        pid_t mTid = 0;
        user_regs_struct mRegs = {};
    };
}
