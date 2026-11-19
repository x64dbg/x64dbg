#pragma once

#include <sys/ptrace.h>
#include <sys/types.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <condition_variable>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <ElfBug/types/ElfBug.h>
#include <ElfBug/types/Global.h>
#include <ElfBug/types/Ptrace.h>
#include <ElfBug/process/Process.h>
#include <ElfBug/process/ProcessArch.h>
#include <ElfBug/core/SignalFilters.h>
#include <ElfBug/thread/Thread.h>

namespace ElfBug
{
    constexpr long kPtraceOptions =
        PTRACE_O_TRACESYSGOOD |
        PTRACE_O_TRACECLONE |
        PTRACE_O_TRACEEXEC |
        PTRACE_O_TRACEEXIT;

    constexpr long kLaunchPtraceOptions = kPtraceOptions | PTRACE_O_EXITKILL;

    constexpr auto kPollInterval = std::chrono::milliseconds(1);
    constexpr auto kStopPollMin = std::chrono::microseconds(10);
    constexpr int kStopPollYields = 32;
    constexpr auto kStopWaitTimeout = std::chrono::milliseconds(250);

    int ReadPtraceScope();

    std::string PtraceErrorMessage(int err, int ptraceScope);

    std::string ArchRejectMessage(Arch arch);

    // si_addr is only a fault address for kernel-raised signals.
    ptr FaultAddress(int signal, const siginfo_t & info);

    // Filtered and no fault: a step does not show its handler.
    bool HidesHandler(const OwedSignal & owed);

    bool SweepShouldQueue(int signal, bool hardware);

    bool ForeignTrapShouldQueue(int signal, const siginfo_t & info);

    enum class WaitResult
    {
        Stopped,
        Gone,
        TimedOut,
    };

    WaitResult WaitForStop(pid_t tid, int & status);

    bool StopShouldQueue(int status, bool haveInfo, const siginfo_t & info,
                         int & signal, ptr & address);

    class Debugger
    {
    public:
        Debugger();
        virtual ~Debugger();

        bool Init(const char* path, const char* const* argv = nullptr, const char* workingDirectory = nullptr);

        bool Attach(pid_t processId);
        void Start();
        void Continue();
        void StepInto();
        void StepOver();
        void Pause();
        bool Stop();
        bool Detach();

        [[nodiscard]] bool IsPaused() const { return mPaused.load(std::memory_order_acquire); }

        bool SwitchThread(pid_t tid);
        bool SetThreadSuspended(pid_t tid, bool suspended);

        [[nodiscard]] SignalFilters & Signals() { return mSignals; }
        [[nodiscard]] const SignalFilters & Signals() const { return mSignals; }

    protected:
        virtual void cbCreateProcess(pid_t pid, ptr entryPoint);
        virtual void cbExitProcess(int exitCode);
        virtual void cbCreateThread(pid_t tid);
        virtual void cbExitThread(pid_t tid);
        virtual void cbException(int signal, ptr address);
        virtual void cbBreakpoint(const BreakpointInfo & info);
        virtual void cbStep();
        virtual void cbSystemBreakpoint();
        virtual void cbAttachBreakpoint();
        virtual void cbDetach();
        virtual void cbExec();
        virtual void cbInternalError(const std::string & error);
        virtual void cbDebugString(const std::string & text);
        virtual void cbPaused();
        virtual void cbPauseTick();
        // A signal that did not stop and has log on.
        virtual void cbSignalFiltered(int signal, pid_t tid, bool passed);

        // /proc wchan; empty when unreadable, running, or stopped.
        static std::string readWaitReason(pid_t tgid, pid_t tid);

        // Also forgets any silent step-off owed at the address.
        bool deleteBreakpoint(ptr address);

        Process* mProcess = nullptr;
        Thread* mThread = nullptr;
        std::unordered_map<pid_t, Process> mProcesses;
        mutable std::shared_mutex mProcessMutex;

    private:
        void debugLoop();
        void resetSessionState();
        bool launchChild();
        bool startLaunchedProcess();
        bool attachToProcess();
        void interruptRunningThread(pid_t tgid);
        bool interruptRunningThreadLocked(pid_t tgid, pid_t except);
        void reapDetachedChildren();
        void detachFromProcess(pid_t reportedTid);
        void handleSignal(pid_t tid, int status);
        void handleSigtrap(pid_t tid, int status, bool wasRunning);
        bool pauseAndResume(pid_t reported);
        enum class StepOff
        {
            Stepped,  // RIP is past the byte and it is armed again
            Parked,   // the thread is suspended: byte armed again, RIP still on it, signal parked
            Consumed, // the stop was used up (exit, forwarded signal, error); abandon it
            Deferred  // byte armed, thread still stopped: the caller continues it with mOwedSignal
        };
        StepOff stepPastBreakpointByte(pid_t tid, ptr addr);
        StepOff deferStepOff(pid_t tid, ptr addr, int signal, const siginfo_t* info);
        // A suspended thread whose slot is taken is continued anyway, so neither signal is lost.
        StepOff resumeDeferred(pid_t tid);
        // Parks the signal for the caller's continue; a slot already in use continues it now.
        StepOff holdDeferred(pid_t tid);
        // Steps the thread off its breakpoint with the signal parked on it.
        StepOff stepOffHeld(Thread* thread, ptr addr);
        // A Deferred thread is continued at once.
        StepOff stepOffAlone(pid_t tid, ptr addr);
        // False when the stop was used up.
        bool stepOffAndRun(pid_t tid, ptr addr);
        void abandonSingleStep(pid_t tid);
        void onExec();
        bool applyExec(pid_t tid);
        void handleExecEvent(pid_t tid);
        void parkForRejectedExec(pid_t tid);
        void replaceExecedThread(pid_t formerTid, pid_t tid);

        std::optional<int> stopAllThreads(pid_t except);
        void reportLeaderExit(int exitCode);
        void drainPendingResumes();
        bool pendingResumeNeedsAllStop();
        bool resumeStoppedThread(pid_t tid);
        void continueUnlessSuspended(pid_t tid);
        void leaveParked(pid_t tid);
        bool swallowPendingSigstop(pid_t tid);
        enum class ContinueResult
        {
            Continued,
            ContinuedUntracked,
            Parked,
            ParkedAlone
        };
        ContinueResult continueOrPark(pid_t tid, const OwedSignal & signal, bool forcePark);
        [[nodiscard]] bool anyThreadRunning() const;
        [[nodiscard]] bool anyThreadRunningLocked() const;
        void resumeAllThreads(pid_t except);
        void abandonAllStop(pid_t except);
        Thread* findPendingBreakpointThread() const;
        Thread* findPendingSignalThread() const;
        void repairStoppedThread(Thread* thread, int status);
        bool claimBreakpointTrap(Thread* thread, int status);
        void reportSignal(pid_t tid, int sig, bool wasRunning);
        void resumeFilteredSignal(pid_t tid, const OwedSignal & owed, bool fault, bool wasRunning);
        // Single-steps the current thread again, or continues it when the step fails.
        void reissueStep(pid_t tid, int signal);
        // Reports a pause when the park leaves nothing running.
        void continueOrPause(pid_t tid, const OwedSignal & signal, bool forcePark);
        // Injected at a later stop, the kernel would otherwise build an SI_USER siginfo.
        void restoreSignalInfo(pid_t tid, const OwedSignal & owed) const;
        // PTRACE_CONT with the signal and its siginfo. False with errno set when it fails.
        bool continueWith(pid_t tid, const OwedSignal & owed) const;

        struct SignalDecision
        {
            bool stop = true;
            int deliver = 0; // the signal to inject on resume, or 0
        };
        // Logs a signal that does not stop and has log on.
        [[nodiscard]] SignalDecision decideSignal(int signal, pid_t tid, bool fault, bool groupStop);
        // A sweep never parks a kernel fault, so it is decided as no fault.
        [[nodiscard]] SignalDecision decideParkedSignal(const Thread* thread);
        // Before a step, the current thread's undecided signal is delivered unless it is to stop.
        void takeUndecidedSignal();
        // Moves the current thread's decided signal into mOwedSignal when nothing is held there.
        void takeDecidedSignal();
        // Dropped when the thread is gone. Caller holds mProcessMutex.
        void parkOwedSignal(pid_t tid);

        struct StepOverRequest
        {
            bool active = false;
            ptr target = 0;
            pid_t tid = 0;
            ptr rspFloor = 0;
            bool planted = false;
        };

        enum class StepOverArm
        {
            Armed,      // temp breakpoint planted, caller continues the thread
            SingleStep, // nothing to run to, caller single-steps
            Parked,     // the thread is suspended: nothing armed, nothing stepped, caller drops the step
            Consumed    // the stop was used up stepping off the source breakpoint
        };
        StepOverArm armStepOver(pid_t tid);
        void cancelStepOver(pid_t tid);
        void cancelStepOverIfOwner(pid_t tid);

        // A thread running a hidden handler back to the instruction a step was interrupted at.
        struct StepResume
        {
            bool active = false;
            pid_t tid = 0;
            ptr address = 0;
            ptr rsp = 0;
            bool planted = false;
        };
        bool armStepResume(pid_t tid, const OwedSignal & owed);
        void cancelStepResume();
        // sigreturn restores RSP exactly; a handler on an alternate stack can run above it, so no floor test.
        bool takeStepResumeTrap(pid_t tid, ptr address);
        void restoreSourceByte(pid_t tid);
        // A thread returning from a hidden handler to a breakpoint it was already reported at.
        struct SilentStepOff
        {
            pid_t tid = 0;
            ptr address = 0;
            ptr rsp = 0;
            bool stepOver = false; // the user's step over is made again after it
        };
        std::optional<SilentStepOff> takeSilentStepOff(pid_t tid, ptr address, ptr rsp);
        // A single-stepped pushf pushes EFLAGS with TF set; clear it from the pushed word.
        void maskPushedTrapFlag();
        void dispatchBreakpoint(ptr address);
        void beginPause();
        void createProcessEvent(pid_t pid, Arch arch);
        void exitProcessEvent(pid_t pid, int exitCode);
        void createThreadEvent(pid_t tid, pid_t tgid, bool fromClone);
        void registerClone(pid_t parent);
        void exitThreadEvent(pid_t tid);
        void releaseForeignClone(pid_t tid, pid_t tgid, bool running);
        // Brings a thread a failed drain left running back to a stop we waited on.
        WaitResult restopForDetach(pid_t tid, pid_t tgid, bool owed, std::vector<OwedSignal> & signals);
        // Keeps the thread traced until it reaches a stop it can be detached from.
        void releaseRunningThread(pid_t tid, pid_t tgid, bool owed, std::vector<OwedSignal> signals);
        void releaseThread(pid_t tid, pid_t tgid, const std::vector<OwedSignal> & signals);

        std::atomic<bool> mIsRunning{false};
        std::atomic<bool> mPaused{false};
        std::atomic<bool> mStepPending{false};
        std::atomic<bool> mStepOverPending{false};
        StepOverRequest mStepOver;
        StepResume mStepResume;
        std::vector<SilentStepOff> mSilentStepOffs;
        // Lifted breakpoint bytes, re-armed when the lifting thread next stops.
        std::unordered_map<pid_t, ptr> mSourceRearms;
        std::vector<pid_t> mResumeExcept;
        std::unordered_set<pid_t> mUnregisteredRunning;
        bool mAllStopped = false;
        pid_t mSteppingOff = 0;
        // Tids whose next SIGSTOP came from SetThreadSuspended. Guarded by mPauseMutex.
        std::unordered_set<pid_t> mPendingSuspend;
        // Resume requests for a tid whose count reached zero while running. Guarded by mPauseMutex.
        std::unordered_set<pid_t> mPendingResume;
        std::atomic<bool> mPauseRequested{false};
        std::atomic<bool> mStopRequested{false};
        std::atomic<bool> mDetachRequested{false};
        std::atomic<pid_t> mMainPid{0};
        pid_t mAttachPid = 0;
        ImageId mImageId;
        std::vector<pid_t> mDetachedChildren;
        // What the current thread is continued with; parked on its slot when another thread becomes current.
        OwedSignal mOwedSignal;
        std::mutex mPauseMutex;
        std::condition_variable mPauseCv;
        SignalFilters mSignals;

        std::string mFilePath;
        std::vector<std::string> mArgv;
        std::string mCwd;
        bool mHasLaunchArgs = false;
    };
}
