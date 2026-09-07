#pragma once

#include <sys/types.h>
#include <ElfBug/types/ElfBug.h>
#include <ElfBug/types/Global.h>
#include <ElfBug/thread/Registers.h>

namespace ElfBug
{
    class Thread
    {
    public:
        pid_t tid;
        Registers registers;

        explicit Thread(pid_t tid);

        bool StepInto(int signal = 0);

        [[nodiscard]] bool isSingleStepping() const { return mIsSingleStepping; }
        void clearSingleStep() { mIsSingleStepping = false; mStepsPushf = false; }

        // The stepped instruction pushes EFLAGS, so TF has to be scrubbed from the pushed word.
        void setStepsPushf(const bool stepsPushf) { mStepsPushf = stepsPushf; }
        [[nodiscard]] bool stepsPushf() const { return mStepsPushf; }

        // TODO: implement via PTRACE_POKEUSER on debug register offsets
        [[nodiscard]] bool GetFreeHardwareBreakpointSlot(const HardwareSlot & slot) const;
        bool SetHardwareBreakpoint(ptr address, HardwareSlot slot, HardwareType type = HardwareType::Execute, HardwareSize size = HardwareSize::Byte, bool singleshot = false);
        bool SetHardwareBreakpoint(ptr address, HardwareSlot slot, const BreakpointCallback & cbBreakpoint, HardwareType type = HardwareType::Execute, HardwareSize size = HardwareSize::Byte, bool singleshot = false);
        bool DeleteHardwareBreakpoint(ptr address);

    private:
        bool mIsSingleStepping = false;
        bool mStepsPushf = false;
    };
}
