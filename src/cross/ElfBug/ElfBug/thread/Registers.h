#pragma once

#include <sys/types.h>
#include <sys/user.h>
#include <ElfBug/types/ElfBug.h>

namespace ElfBug
{
    class Registers
    {
    public:
        Registers() = default;
        explicit Registers(pid_t tid);

        bool Read();
        bool Write();

        ptr & Gax();
        ptr & Gbx();
        ptr & Gcx();
        ptr & Gdx();
        ptr & Gdi();
        ptr & Gsi();
        ptr & Gbp();
        ptr & Gsp();
        ptr & Gip();

        ptr & Rax();
        ptr & Rbx();
        ptr & Rcx();
        ptr & Rdx();
        ptr & Rsi();
        ptr & Rdi();
        ptr & Rbp();
        ptr & Rsp();
        ptr & Rip();
        ptr & R8();
        ptr & R9();
        ptr & R10();
        ptr & R11();
        ptr & R12();
        ptr & R13();
        ptr & R14();
        ptr & R15();

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
