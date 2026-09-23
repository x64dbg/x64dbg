#pragma once

#include <sys/types.h>
#include <atomic>
#include <mutex>
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

        [[nodiscard]] bool IsSingleStepping() const { return mIsSingleStepping; }
        void ClearSingleStep() { mIsSingleStepping = false; mStepsPushf = false; }

        // The stepped instruction pushes EFLAGS, so TF has to be scrubbed from the pushed word.
        void SetStepsPushf(const bool stepsPushf) { mStepsPushf = stepsPushf; }
        [[nodiscard]] bool StepsPushf() const { return mStepsPushf; }

        void SetRunning(const bool running)
        {
            mRunning = running;
            if(running)
            {
                mAtBreakpoint = false;
                std::lock_guard lock(mWaitReasonMutex);
                mWaitReason.clear();
            }
        }
        [[nodiscard]] bool IsRunning() const { return mRunning; }

        // Only a thread rewound onto its own hit is stepped off the byte on resume.
        void SetAtBreakpoint(const bool at) { mAtBreakpoint = at; }
        [[nodiscard]] bool AtBreakpoint() const { return mAtBreakpoint; }

        // Suspends nest. Written under mProcessMutex; atomic for unlocked pre-checks.
        void Suspend()
        {
            const uint32 count = mSuspendCount.load(std::memory_order_relaxed);
            mSuspendCount.store(count + 1, std::memory_order_relaxed);
        }
        void Resume()
        {
            const uint32 count = mSuspendCount.load(std::memory_order_relaxed);
            if(count > 0)
                mSuspendCount.store(count - 1, std::memory_order_relaxed);
        }
        [[nodiscard]] bool IsSuspended() const { return mSuspendCount.load(std::memory_order_relaxed) > 0; }
        [[nodiscard]] uint32 SuspendCount() const { return mSuspendCount.load(std::memory_order_relaxed); }

        // Kernel function it was blocked in. Written by caller and tracer, so its own lock.
        void SetWaitReason(std::string reason)
        {
            std::lock_guard lock(mWaitReasonMutex);
            mWaitReason = std::move(reason);
        }
        [[nodiscard]] std::string WaitReason() const
        {
            std::lock_guard lock(mWaitReasonMutex);
            return mWaitReason;
        }

        // Our SIGSTOP queued behind the thread's own stop; consume before single-stepping.
        void SetPendingSigstop(const bool pending) { mPendingSigstop = pending; }
        [[nodiscard]] bool PendingSigstop() const { return mPendingSigstop; }

        // Hit just before the sweep froze it; reported on the next resume.
        void SetPendingBreakpoint(const ptr address)
        {
            mHasPendingBreakpoint = true;
            mPendingBreakpoint = address;
        }
        void ClearPendingBreakpoint() { mHasPendingBreakpoint = false; mPendingBreakpoint = 0; }
        [[nodiscard]] bool HasPendingBreakpoint() const { return mHasPendingBreakpoint; }
        [[nodiscard]] ptr PendingBreakpoint() const { return mPendingBreakpoint; }

        // Read off the wait status by the sweep, forwarded on the resume that unfreezes it.
        void SetPendingSignal(const int signal, const ptr address, const bool unreported)
        {
            mPendingSignal = signal;
            mPendingSignalAddress = address;
            mPendingSignalUnreported = unreported;
        }
        void ClearPendingSignal() { SetPendingSignal(0, 0, false); }
        [[nodiscard]] int PendingSignal() const { return mPendingSignal; }
        [[nodiscard]] ptr PendingSignalAddress() const { return mPendingSignalAddress; }
        [[nodiscard]] bool PendingSignalUnreported() const { return mPendingSignalUnreported; }

        // TODO: implement via PTRACE_POKEUSER on debug register offsets
        [[nodiscard]] bool FreeHardwareBreakpointSlot(const HardwareSlot & slot) const;
        bool SetHardwareBreakpoint(ptr address, HardwareSlot slot, HardwareType type = HardwareType::Execute, HardwareSize size = HardwareSize::Byte, bool singleshot = false);
        bool SetHardwareBreakpoint(ptr address, HardwareSlot slot, const BreakpointCallback & cbBreakpoint, HardwareType type = HardwareType::Execute, HardwareSize size = HardwareSize::Byte, bool singleshot = false);
        bool DeleteHardwareBreakpoint(ptr address);

    private:
        bool mIsSingleStepping = false;
        bool mStepsPushf = false;
        bool mRunning = false;
        bool mAtBreakpoint = false;
        std::atomic<uint32> mSuspendCount{0};
        mutable std::mutex mWaitReasonMutex;
        std::string mWaitReason;
        bool mPendingSigstop = false;
        bool mHasPendingBreakpoint = false;
        ptr mPendingBreakpoint = 0;
        int mPendingSignal = 0;
        ptr mPendingSignalAddress = 0;
        bool mPendingSignalUnreported = false;
    };
}
