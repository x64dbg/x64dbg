#pragma once

#include <sys/ptrace.h>
#include <sys/types.h>
#include <atomic>
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
#include <ElfBug/process/Process.h>
#include <ElfBug/thread/Thread.h>

namespace ElfBug
{
    constexpr long kPtraceOptions =
        PTRACE_O_TRACESYSGOOD |
        PTRACE_O_TRACECLONE |
        PTRACE_O_TRACEEXEC |
        PTRACE_O_TRACEEXIT |
        PTRACE_O_EXITKILL;

    // Split out so the yama branch is testable without an unattachable process to hand.
    std::string AttachErrorMessage(pid_t pid, int err, int ptraceScope, bool isOurChild);

    // Shared by the launch and both attach paths so the wording cannot drift apart.
    std::string ArchRejectMessage(Arch arch);

    // si_addr only means something for kernel-raised faults; for kill and tkill the same
    // union bytes hold the sender's pid and uid. Shared by the freeze sweep and attach.
    ptr FaultAddress(int signal, const siginfo_t & info);

    // Whether a signal caught outside the normal loop should be queued for later replay,
    // rather than re-injected (a hardware fault re-raises itself) or dropped (our SIGSTOP).
    bool SweepShouldQueue(int signal, bool hardware);

    enum class WaitResult
    {
        Stopped,
        Gone,
        TimedOut,
    };

    // Bounded: teardown has no debug loop left to time out a thread that never reports.
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
        void Detach();

        [[nodiscard]] bool IsPaused() const { return mPaused.load(std::memory_order_acquire); }

        // Makes `tid` the current thread while paused: registers, steps and the next
        // resume act on it. The thread that reported keeps any signal it still owes.
        bool SwitchThread(pid_t tid);

        // Suspends or resumes a thread. Suspended threads stay stopped across Continue and
        // steps. While running the request is serviced by the loop, so a true return means
        // the request was accepted, not that the thread has stopped yet.
        bool SetThreadSuspended(pid_t tid, bool suspended);

    protected:
        virtual void cbCreateProcess(pid_t pid, ptr entryPoint);
        virtual void cbExitProcess(int exitCode);
        virtual void cbCreateThread(pid_t tid);
        virtual void cbExitThread(pid_t tid);
        virtual void cbLoadModule(ptr baseAddress, const std::string & path);
        virtual void cbUnloadModule(ptr baseAddress);
        virtual void cbException(int signal, ptr address);
        virtual void cbBreakpoint(const BreakpointInfo & info);
        virtual void cbStep();
        virtual void cbSystemBreakpoint();
        virtual void cbAttachBreakpoint();
        virtual void cbDetach();
        virtual void cbExec();
        virtual void cbUnhandledException(int signal, ptr address);
        virtual void cbInternalError(const std::string & error);
        virtual void cbDebugString(const std::string & text);
        virtual void cbPaused(); // called when the debuggee is paused by user
        virtual void cbPauseTick(); // called each iteration of the pause spin loop

        // /proc/<tgid>/task/<tid>/wchan; empty when unreadable, running, or stopped: the
        // kernel names our own SIGSTOP as a wait, and stopped-ness is tracked elsewhere.
        static std::string readWaitReason(pid_t tgid, pid_t tid);

        Process* mProcess = nullptr;
        Thread* mThread = nullptr;
        std::unordered_map<pid_t, Process> mProcesses;
        mutable std::shared_mutex mProcessMutex;

    private:
        void debugLoop();
        // Shared by Init and Attach: clears everything a previous session left behind.
        void resetSessionState();
        bool launchChild();
        // Launch prologue: first stop, options, arch check, process event.
        bool startLaunchedProcess();
        // Attach prologue: acquire every thread under /proc/<pid>/task, then the same events.
        bool attachToProcess();
        void interruptRunningThread(pid_t tgid);
        bool interruptRunningThreadLocked(pid_t tgid, pid_t except);
        void reapDetachedChildren();
        // Tracer thread only, from the fully stopped state: unpatches breakpoints and
        // PTRACE_DETACHes every thread, delivering whatever signal each still owes.
        // `reportedTid` is the thread that reported the stop, which owns mPendingSignal.
        void detachFromProcess(pid_t reportedTid);
        void reportAttachError(pid_t pid, pid_t tid, int err);
        void handleSignal(pid_t tid, int status);
        void handleSigtrap(pid_t tid, int status);
        bool pauseAndResume(pid_t reported);
        enum class StepOff
        {
            Stepped,  // RIP is past the byte and it is armed again
            Parked,   // the thread is suspended: byte armed again, RIP still on it, signal parked
            Consumed  // the stop was used up (exit, forwarded signal, error); abandon it
        };
        StepOff stepPastBreakpointByte(pid_t tid, ptr addr);
        void abandonSingleStep(pid_t tid);
        // The image was replaced: drop step state without writing anything back.
        void onExec();
        // Everything bound to the replaced image, shared by the three sites that observe
        // the event. False means the engine cannot drive the new image and has requested
        // a detach. May replace the leader's Thread.
        bool applyExec(pid_t tid);
        // A clone in its own thread group reports its own execve here.
        void handleExecEvent(pid_t tid);
        // A non-leader execve takes over the leader's id, and neither death is reported.
        void replaceExecedThread(pid_t formerTid, pid_t tid);

        // The leader's exit code when it died during the sweep. The caller reports that
        // instead of its own stop: nothing may touch mProcess afterwards.
        std::optional<int> stopAllThreads(pid_t except);
        void reportLeaderExit(int exitCode);
        // Tracer thread only. PTRACE_CONTs every thread whose suspend count reached zero
        // while the process was running.
        void drainPendingResumes();
        // Tracer thread only. PTRACE_CONTs a stopped tid unless it is suspended or running,
        // stepping it off its own armed breakpoint byte first. False means the process is gone.
        bool resumeStoppedThread(pid_t tid);
        // PTRACE_CONTs tid unless a caller suspended it since its stop was snapshotted; then
        // it stays parked and, with nothing else running, the pause is reported.
        void continueUnlessSuspended(pid_t tid);
        // tid stays in ptrace-stop; with nothing else running the pause is reported.
        void leaveParked(pid_t tid);
        bool swallowPendingSigstop(pid_t tid);
        void resumeAllThreads(pid_t except);
        void abandonFreeze(pid_t except);
        Thread* findPendingBreakpointThread() const;
        Thread* findPendingSignalThread() const;
        void repairStoppedThread(Thread* thread, int status);
        // RIP was one byte past one of ours: put it back and queue the hit. Registers
        // must already be read.
        bool rewindOntoBreakpoint(Thread* thread, int status);
        void reportSignal(pid_t tid, int sig);

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
        // Another thread's stop must not end the stepping thread's step-over.
        void cancelStepOverIfOwner(pid_t tid);
        void restoreSourceByte(pid_t tid);
        // A single-stepped pushf pushes EFLAGS with TF set; clear it from the pushed word.
        void maskPushedTrapFlag();
        // Runs the breakpoint's callback and cbBreakpoint, deleting it when singleshot.
        void dispatchBreakpoint(ptr address);
        void beginPause();
        void createProcessEvent(pid_t pid, Arch arch);
        void exitProcessEvent(pid_t pid, int exitCode);
        void createThreadEvent(pid_t tid);
        void exitThreadEvent(pid_t tid);
        void releaseForeignClone(pid_t tid, pid_t tgid, bool running);
        // Brings a thread a failed drain left running back to a stop this thread waited
        // on, which PTRACE_DETACH needs. False means it stays traced.
        bool restopForDetach(pid_t tid, pid_t tgid);

        // Tracer-thread only; caller threads must not write.
        std::atomic<bool> mIsRunning{false};
        std::atomic<bool> mPaused{false};
        std::atomic<bool> mStepPending{false};
        std::atomic<bool> mStepOverPending{false};
        StepOverRequest mStepOver;
        // Lifted breakpoint bytes, re-armed when the lifting thread next stops.
        std::unordered_map<pid_t, ptr> mSourceRearms;
        std::vector<pid_t> mResumeExcept;
        std::unordered_set<pid_t> mUnregisteredRunning;
        bool mAllStopped = false;
        pid_t mSteppingOff = 0;
        // Tids whose next SIGSTOP was sent by SetThreadSuspended. The suspend count is
        // applied at request time; the loop only leaves that stop in place. Guarded by
        // mPauseMutex.
        std::unordered_set<pid_t> mPendingSuspend;
        // Resume requests from caller threads for a tid whose count reached zero while
        // running. Idempotent, so membership is all that matters; a set fits. Guarded
        // by mPauseMutex.
        std::unordered_set<pid_t> mPendingResume;
        std::atomic<bool> mPauseRequested{false};
        std::atomic<bool> mStopRequested{false};
        std::atomic<bool> mDetachRequested{false};
        std::atomic<pid_t> mMainPid{0};
        pid_t mAttachPid = 0;
        bool mWasGroupStopped = false;
        std::vector<pid_t> mDetachedChildren;
        int mPendingSignal = 0;
        std::mutex mPauseMutex;
        std::condition_variable mPauseCv;

        std::string mFilePath;
        std::vector<std::string> mArgv;
        std::string mCwd;
        bool mHasLaunchArgs = false;
    };
}
