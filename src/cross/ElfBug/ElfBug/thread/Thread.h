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

        bool stepInto(int signal = 0);

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
                std::lock_guard lock(mWaitReasonMutex);
                mWaitReason.clear();
            }
        }
        [[nodiscard]] bool isRunning() const { return mRunning; }

        // RIP was rewound onto an armed breakpoint this thread hit. Only such a thread is
        // stepped off the byte on resume; one frozen just before the byte has not hit it
        // and must trap when it runs. Cleared by anything that lets the thread run.
        void setAtBreakpoint(const bool at) { mAtBreakpoint = at; }
        [[nodiscard]] bool atBreakpoint() const { return mAtBreakpoint; }

        // Frozen by the user. Suspends nest: every resume leaves it stopped until the
        // count reaches zero. Only written under mProcessMutex; the atomic exists so the
        // tracer's unlocked pre-checks are defined, the locked recheck at each continue
        // is what decides.
        void suspend()
        {
            const uint32_t count = mSuspendCount.load(std::memory_order_relaxed);
            mSuspendCount.store(count + 1, std::memory_order_relaxed);
        }
        void resume()
        {
            const uint32_t count = mSuspendCount.load(std::memory_order_relaxed);
            if(count > 0)
                mSuspendCount.store(count - 1, std::memory_order_relaxed);
        }
        [[nodiscard]] bool isSuspended() const { return mSuspendCount.load(std::memory_order_relaxed) > 0; }
        [[nodiscard]] uint32_t suspendCount() const { return mSuspendCount.load(std::memory_order_relaxed); }

        // Kernel function the thread was blocked in when the debugger stopped it from
        // outside. Empty for a thread that was running or stopped on its own. The
        // string is written by the caller and the tracer with no lock in common, so
        // it has its own. Nothing else is ever taken while it is held.
        void setWaitReason(std::string reason)
        {
            std::lock_guard lock(mWaitReasonMutex);
            mWaitReason = std::move(reason);
        }
        [[nodiscard]] std::string waitReason() const
        {
            std::lock_guard lock(mWaitReasonMutex);
            return mWaitReason;
        }

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
        [[nodiscard]] bool freeHardwareBreakpointSlot(const HardwareSlot & slot) const;
        bool setHardwareBreakpoint(ptr address, HardwareSlot slot, HardwareType type = HardwareType::Execute, HardwareSize size = HardwareSize::Byte, bool singleshot = false);
        bool setHardwareBreakpoint(ptr address, HardwareSlot slot, const BreakpointCallback & cbBreakpoint, HardwareType type = HardwareType::Execute, HardwareSize size = HardwareSize::Byte, bool singleshot = false);
        bool deleteHardwareBreakpoint(ptr address);

    private:
        bool mIsSingleStepping = false;
        bool mStepsPushf = false;
        bool mRunning = false;
        bool mAtBreakpoint = false;
        std::atomic<uint32_t> mSuspendCount{0};
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
