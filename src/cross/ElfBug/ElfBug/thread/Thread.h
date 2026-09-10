#pragma once

#include <sys/types.h>
#include <string>
#include <utility>
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

        // Constructed stopped: a clone is reported to us already stopped, and the main
        // thread stops on exec.
        void setRunning(const bool running)
        {
            mRunning = running;
            if(running)
            {
                mAtBreakpoint = false;
                mWaitReason.clear();
            }
        }
        [[nodiscard]] bool isRunning() const { return mRunning; }

        // RIP was rewound onto an armed breakpoint this thread hit. Only such a thread is
        // stepped off the byte on resume; one frozen just before the byte has not hit it
        // and must trap when it runs. Cleared by anything that lets the thread run.
        void setAtBreakpoint(const bool at) { mAtBreakpoint = at; }
        [[nodiscard]] bool atBreakpoint() const { return mAtBreakpoint; }

        // Frozen by the user. Every resume leaves it stopped until it is cleared.
        void setSuspended(const bool suspended) { mSuspended = suspended; }
        [[nodiscard]] bool isSuspended() const { return mSuspended; }

        // Kernel function the thread was blocked in when the debugger stopped it from
        // outside. Empty for a thread that was running or stopped on its own.
        void setWaitReason(std::string reason) { mWaitReason = std::move(reason); }
        [[nodiscard]] const std::string & waitReason() const { return mWaitReason; }

        // Set when our SIGSTOP was still queued because the thread stopped for its own
        // reason first. It must be consumed before this thread is single-stepped.
        void setPendingSigstop(const bool pending) { mPendingSigstop = pending; }
        [[nodiscard]] bool pendingSigstop() const { return mPendingSigstop; }

        // A breakpoint this thread hit just before the sweep froze it. Reported on the
        // next resume instead of being absorbed.
        void setPendingBreakpoint(const ptr address)
        {
            mHasPendingBreakpoint = true;
            mPendingBreakpoint = address;
        }
        void clearPendingBreakpoint() { mHasPendingBreakpoint = false; mPendingBreakpoint = 0; }
        [[nodiscard]] bool hasPendingBreakpoint() const { return mHasPendingBreakpoint; }
        [[nodiscard]] ptr pendingBreakpoint() const { return mPendingBreakpoint; }

        // A signal the sweep read off the wait status, forwarded on the resume that
        // unfreezes the thread. 0 means none. Unreported until pauseAndResume reports it.
        void setPendingSignal(const int signal, const ptr address, const bool unreported)
        {
            mPendingSignal = signal;
            mPendingSignalAddress = address;
            mPendingSignalUnreported = unreported;
        }
        void clearPendingSignal() { setPendingSignal(0, 0, false); }
        [[nodiscard]] int pendingSignal() const { return mPendingSignal; }
        [[nodiscard]] ptr pendingSignalAddress() const { return mPendingSignalAddress; }
        [[nodiscard]] bool pendingSignalUnreported() const { return mPendingSignalUnreported; }

        // TODO: implement via PTRACE_POKEUSER on debug register offsets
        [[nodiscard]] bool GetFreeHardwareBreakpointSlot(const HardwareSlot & slot) const;
        bool SetHardwareBreakpoint(ptr address, HardwareSlot slot, HardwareType type = HardwareType::Execute, HardwareSize size = HardwareSize::Byte, bool singleshot = false);
        bool SetHardwareBreakpoint(ptr address, HardwareSlot slot, const BreakpointCallback & cbBreakpoint, HardwareType type = HardwareType::Execute, HardwareSize size = HardwareSize::Byte, bool singleshot = false);
        bool DeleteHardwareBreakpoint(ptr address);

    private:
        bool mIsSingleStepping = false;
        bool mStepsPushf = false;
        bool mRunning = false;
        bool mAtBreakpoint = false;
        bool mSuspended = false;
        std::string mWaitReason;
        bool mPendingSigstop = false;
        bool mHasPendingBreakpoint = false;
        ptr mPendingBreakpoint = 0;
        int mPendingSignal = 0;
        ptr mPendingSignalAddress = 0;
        bool mPendingSignalUnreported = false;
    };
}
