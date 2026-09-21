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
#include <ElfBug/process/Process.h>
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
    constexpr auto kStopWaitTimeout = std::chrono::milliseconds(250);

    std::string AttachErrorMessage(pid_t pid, int err, int ptraceScope, bool isOurChild);

    std::string ArchRejectMessage(Arch arch);

    // si_addr is only a fault address for kernel-raised signals.
    ptr FaultAddress(int signal, const siginfo_t & info);

    bool SweepShouldQueue(int signal, bool hardware);

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
        virtual void cbPaused();
        virtual void cbPauseTick();

        // /proc wchan; empty when unreadable, running, or stopped.
        static std::string readWaitReason(pid_t tgid, pid_t tid);

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
        void onExec();
        bool applyExec(pid_t tid);
        void handleExecEvent(pid_t tid);
        void replaceExecedThread(pid_t formerTid, pid_t tid);

        std::optional<int> stopAllThreads(pid_t except);
        void reportLeaderExit(int exitCode);
        void drainPendingResumes();
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
        ContinueResult continueOrPark(pid_t tid, int signal, bool forcePark);
        [[nodiscard]] bool anyThreadRunning() const;
        [[nodiscard]] bool anyThreadRunningLocked() const;
        void resumeAllThreads(pid_t except);
        void abandonAllStop(pid_t except);
        Thread* findPendingBreakpointThread() const;
        Thread* findPendingSignalThread() const;
        void repairStoppedThread(Thread* thread, int status);
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
        void cancelStepOverIfOwner(pid_t tid);
        void restoreSourceByte(pid_t tid);
        // A single-stepped pushf pushes EFLAGS with TF set; clear it from the pushed word.
        void maskPushedTrapFlag();
        void dispatchBreakpoint(ptr address);
        void beginPause();
        void createProcessEvent(pid_t pid, Arch arch);
        void exitProcessEvent(pid_t pid, int exitCode);
        void createThreadEvent(pid_t tid);
        void exitThreadEvent(pid_t tid);
        void releaseForeignClone(pid_t tid, pid_t tgid, bool running);
        // Brings a thread a failed drain left running back to a stop we waited on.
        bool restopForDetach(pid_t tid, pid_t tgid);

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
        // Tids whose next SIGSTOP came from SetThreadSuspended. Guarded by mPauseMutex.
        std::unordered_set<pid_t> mPendingSuspend;
        // Resume requests for a tid whose count reached zero while running. Guarded by mPauseMutex.
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
