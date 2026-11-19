#pragma once

#include <sys/types.h>
#include <atomic>
#include <csignal>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <ElfBug/types/ElfBug.h>
#include <ElfBug/types/Global.h>
#include <ElfBug/thread/Registers.h>

namespace ElfBug
{
    enum class ParkedSignal
    {
        Unreported, // seen by the engine, the decision is still to be made
        Reported,   // the user was shown it; a step after it steps into its handler
        Filtered    // decided not to stop; its handler is hidden from a step
    };

    // A signal held back for a later resume. No signal means no state, no siginfo and no address.
    class OwedSignal
    {
    public:
        OwedSignal() = default;
        OwedSignal(const int signal, const ParkedSignal state, const siginfo_t* info, const ptr address = 0)
        {
            if(signal == 0)
                return;
            mSignal = signal;
            mState = state;
            if(info)
                mInfo = *info;
            mAddress = address;
        }

        [[nodiscard]] int Signal() const { return mSignal; }
        [[nodiscard]] ParkedSignal State() const { return mState; }
        [[nodiscard]] const siginfo_t* Info() const { return mInfo ? &*mInfo : nullptr; }
        [[nodiscard]] ptr Address() const { return mAddress; }
        [[nodiscard]] bool Undecided() const { return mSignal != 0 && mState == ParkedSignal::Unreported; }
        [[nodiscard]] bool Decided() const { return mSignal != 0 && mState != ParkedSignal::Unreported; }

    private:
        int mSignal = 0;
        ParkedSignal mState = ParkedSignal::Reported;
        std::optional<siginfo_t> mInfo;
        ptr mAddress = 0;
    };

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
        void SetPendingSignal(OwedSignal signal) { mPendingSignal = std::move(signal); }
        void ClearPendingSignal() { mPendingSignal = {}; }
        [[nodiscard]] OwedSignal TakePendingSignal() { return std::exchange(mPendingSignal, {}); }
        [[nodiscard]] const OwedSignal & Owed() const { return mPendingSignal; }
        [[nodiscard]] int PendingSignal() const { return mPendingSignal.Signal(); }

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
        OwedSignal mPendingSignal;
    };
}
