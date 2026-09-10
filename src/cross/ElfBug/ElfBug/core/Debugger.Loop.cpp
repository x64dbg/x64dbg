#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/ProcessArch.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <vector>
#include <algorithm>

namespace ElfBug
{
    namespace
    {
        struct ScopedExcept
        {
            std::vector<pid_t> & stack;

            ScopedExcept(std::vector<pid_t> & excepts, const pid_t tid)
                : stack(excepts)
            {
                stack.push_back(tid);
            }

            ~ScopedExcept()
            {
                stack.pop_back();
            }

            ScopedExcept(const ScopedExcept &) = delete;
            ScopedExcept & operator=(const ScopedExcept &) = delete;
        };
    }

    // Publishes mPaused=true under the mutex before the event callback fires,
    // so a concurrent Continue()/Stop() can't race past and strand pauseAndResume().
    void Debugger::beginPause()
    {
        // Every caller reports from a frozen process; only resumeAllThreads lifts it.
        mAllStopped = true;

        std::lock_guard lock(mPauseMutex);
        mPaused.store(true, std::memory_order_release);
    }

    Thread* Debugger::findPendingBreakpointThread() const
    {
        if(!mProcess)
            return nullptr;

        std::shared_lock lock(mProcessMutex);
        for(const auto & [tid, thread] : mProcess->threads)
        {
            if(thread->hasPendingBreakpoint())
                return thread.get();
        }
        return nullptr;
    }

    Thread* Debugger::findPendingSignalThread() const
    {
        if(!mProcess)
            return nullptr;

        std::shared_lock lock(mProcessMutex);
        for(const auto & [tid, thread] : mProcess->threads)
        {
            if(thread->pendingSignal() != 0 && thread->pendingSignalUnreported())
                return thread.get();
        }
        return nullptr;
    }

    void Debugger::resumeAllThreads(const pid_t except)
    {
        mAllStopped = false;

        if(!mProcess)
            return;

        const ScopedExcept scopedExcept(mResumeExcept, except);

        std::vector<pid_t> stopped;
        {
            std::shared_lock lock(mProcessMutex);
            for(const auto & [tid, thread] : mProcess->threads)
            {
                if(tid == except || thread->isRunning())
                    continue;
                if(std::find(mResumeExcept.begin(), mResumeExcept.end(), tid) != mResumeExcept.end())
                    continue;
                stopped.push_back(tid);
            }
        }

        // Pass one: every thread steps off its own breakpoint while the rest stay frozen.
        for(const pid_t tid : stopped)
        {
            if(!mProcess || !mIsRunning.load(std::memory_order_acquire))
                return;

            Thread* thread = nullptr;
            {
                std::shared_lock lock(mProcessMutex);
                const auto it = mProcess->threads.find(tid);
                if(it != mProcess->threads.end())
                    thread = it->second.get();
            }
            if(!thread || thread->isRunning())
                continue;

            if(!thread->registers.Read())
                continue;

            // A thread frozen just before the byte has not hit it; stepping it off would
            // skip the hit. Only a rewound thread owes a step.
            const ptr rip = thread->registers.Gip();
            if(!thread->atBreakpoint() || !mProcess->HasBreakpoint(rip))
                continue;

            Thread* previous = nullptr;
            {
                std::unique_lock lock(mProcessMutex);
                previous = mThread;
                mThread = thread;
            }

            mPendingSignal = thread->pendingSignal();
            thread->clearPendingSignal();

            const bool stepped = stepPastBreakpointByte(tid, rip);
            mPendingSignal = 0;

            if(!stepped && (!mProcess || !mIsRunning.load(std::memory_order_acquire)))
                return;

            {
                std::unique_lock lock(mProcessMutex);
                mThread = previous;
            }
        }

        // Pass two: only now is anything actually running.
        for(const pid_t tid : stopped)
        {
            if(!mProcess || !mIsRunning.load(std::memory_order_acquire))
                return;

            Thread* thread = nullptr;
            {
                std::shared_lock lock(mProcessMutex);
                const auto it = mProcess->threads.find(tid);
                if(it != mProcess->threads.end())
                    thread = it->second.get();
            }
            // Pass one can leave a thread running, and it is no longer in ptrace-stop.
            if(!thread || thread->isRunning())
                continue;

            // The only delivery this queued signal will ever get.
            const int sig = thread->pendingSignal();
            thread->clearPendingSignal();

            if(ptrace(PTRACE_CONT, tid, nullptr,
                      reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
            {
                if(errno != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                continue;
            }
            thread->setRunning(true);
        }
    }

    void Debugger::abandonFreeze(const pid_t except)
    {
        if(!mAllStopped)
            return;

        if(mProcess)
        {
            bool anyRunning = false;
            {
                std::shared_lock lock(mProcessMutex);
                for(const auto & [tid, thread] : mProcess->threads)
                {
                    if(thread->isRunning())
                    {
                        anyRunning = true;
                        break;
                    }
                }
            }
            if(anyRunning)
                return;
        }

        resumeAllThreads(except);
    }

    bool Debugger::pauseAndResume(const pid_t pid)
    {
        std::unique_lock lock(mPauseMutex);

        while(mPaused.load(std::memory_order_acquire) && mIsRunning.load(std::memory_order_acquire) &&
                !mStopRequested.load(std::memory_order_acquire))
        {
            if(mPauseCv.wait_for(lock, std::chrono::milliseconds(10)) == std::cv_status::timeout)
                cbPauseTick();
        }

        // A resume can wake the wait before the tick that would apply a request queued
        // just before it. Tick once more while still stopped.
        cbPauseTick();

        lock.unlock();

        if(!mIsRunning.load(std::memory_order_acquire))
            return false;

        if(!mStepPending.load(std::memory_order_acquire) &&
                !mStepOverPending.load(std::memory_order_acquire) &&
                !mStopRequested.load(std::memory_order_acquire))
        {
            while(Thread* queued = findPendingBreakpointThread())
            {
                const ptr address = queued->pendingBreakpoint();
                const pid_t queuedTid = queued->tid;
                queued->clearPendingBreakpoint();

                if(!mProcess || !mProcess->HasBreakpoint(address))
                    continue;

                if(mPendingSignal != 0)
                {
                    std::shared_lock processLock(mProcessMutex);
                    const auto it = mProcess->threads.find(pid);
                    if(it != mProcess->threads.end())
                        it->second->setPendingSignal(mPendingSignal, 0, false);
                }
                mPendingSignal = 0;

                {
                    std::unique_lock processLock(mProcessMutex);
                    mThread = queued;
                }
                beginPause();
                dispatchBreakpoint(address);
                return pauseAndResume(queuedTid);
            }

            while(Thread* queued = findPendingSignalThread())
            {
                const int signal = queued->pendingSignal();
                const ptr address = queued->pendingSignalAddress();
                const pid_t queuedTid = queued->tid;
                queued->clearPendingSignal();

                if(mPendingSignal != 0)
                {
                    std::shared_lock processLock(mProcessMutex);
                    const auto it = mProcess->threads.find(pid);
                    if(it != mProcess->threads.end())
                        it->second->setPendingSignal(mPendingSignal, 0, false);
                }
                mPendingSignal = signal;

                {
                    std::unique_lock processLock(mProcessMutex);
                    mThread = queued;
                }
                beginPause();
                cbExceptionEvent(signal, address);
                return pauseAndResume(queuedTid);
            }
        }

        bool stepOverRequested = mStepOverPending.exchange(false, std::memory_order_acq_rel);

        const bool stepIntoRequested = mStepPending.load(std::memory_order_acquire);

        // A breakpoint hit leaves its 0xCC armed with RIP on it; every resume except a
        // step-over must step past that byte first.
        if(!stepOverRequested && mThread && mProcess && mThread->atBreakpoint() &&
                mProcess->HasBreakpoint(mThread->registers.Gip()))
        {
            const ptr rip = mThread->registers.Gip();
            ptr next = 0;
            const bool repeats = mProcess->ClassifyStepOverAt(rip, next) == StepOverKind::Rep;

            // Stepping off would consume the user's step, and a rep only advances one
            // iteration, leaving RIP on the byte. Lift it and re-arm at the next stop.
            if(stepIntoRequested || repeats)
            {
                if(mProcess->DisarmBreakpointByte(rip))
                    mSourceRearms[pid] = rip;
            }
            else if(!stepPastBreakpointByte(pid, rip))
            {
                abandonFreeze(pid);
                return false;
            }
        }

        if(stepOverRequested && mThread)
        {
            // A StepInto queued just before this StepOver is subsumed by it.
            mStepPending.store(false, std::memory_order_release);

            switch(armStepOver(pid))
            {
            case StepOverArm::Consumed:
                abandonFreeze(pid);
                return false;

            case StepOverArm::Armed:
            {
                const int sig = mPendingSignal;
                mPendingSignal = 0;
                if(ptrace(PTRACE_CONT, pid, nullptr,
                          reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
                {
                    if(errno != ESRCH)
                        cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                    cancelStepOver(pid);
                }
                else
                {
                    mThread->setRunning(true);
                }
                return true;
            }

            case StepOverArm::SingleStep:
                break;
            }
        }

        if((stepIntoRequested || stepOverRequested) && mThread)
        {
            mStepPending.store(false, std::memory_order_release);
            const int sig = mPendingSignal;
            mPendingSignal = 0;
            ptr next = 0;
            const bool stepsPushf = mProcess &&
                                    mProcess->ClassifyStepOverAt(mThread->registers.Gip(), next) == StepOverKind::Pushf;
            // Otherwise the step is spent delivering the SIGSTOP.
            if(!swallowPendingSigstop(pid))
            {
                restoreSourceByte(pid);
                abandonFreeze(pid);
                return false;
            }
            if(mThread->StepInto(sig))
            {
                mThread->setStepsPushf(stepsPushf);
                mThread->setRunning(true);
            }
            else
            {
                const int stepErrno = errno;
                if(stepErrno != ESRCH)
                {
                    cbInternalError("PTRACE_SINGLESTEP failed: " + std::string(strerror(stepErrno)));
                    restoreSourceByte(pid);
                    if(ptrace(PTRACE_CONT, pid, nullptr,
                              reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
                    {
                        if(errno != ESRCH)
                            cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                    }
                    else
                    {
                        mThread->setRunning(true);
                    }
                }
                else
                {
                    restoreSourceByte(pid);
                    abandonFreeze(pid);
                }
            }
        }
        else
        {
            const int sig = mPendingSignal;
            mPendingSignal = 0;

            resumeAllThreads(pid);
            if(!mProcess || !mIsRunning.load(std::memory_order_acquire))
                return false;

            if(ptrace(PTRACE_CONT, pid, nullptr,
                      reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
            {
                if(errno != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            }
            else if(mThread)
            {
                mThread->setRunning(true);
            }
        }
        return true;
    }

    void Debugger::debugLoop()
    {
        if(!launchChild())
        {
            mIsRunning.store(false, std::memory_order_release);
            return;
        }

        const pid_t mainPid = mMainPid.load(std::memory_order_relaxed);

        int status = 0;
        pid_t pid = waitpid(mainPid, &status, __WALL);
        if(pid == -1)
        {
            cbInternalError("initial waitpid() failed: " + std::string(strerror(errno)));
            mIsRunning.store(false, std::memory_order_release);
            return;
        }

        if(!WIFSTOPPED(status))
        {
            int code = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
            cbInternalError("child exited before reaching first stop (code " + std::to_string(code) + ")");
            cbExitProcessEvent(code);
            mIsRunning.store(false, std::memory_order_release);
            return;
        }

        if(ptrace(PTRACE_SETOPTIONS, mainPid, nullptr,
                  PTRACE_O_TRACESYSGOOD |
                  PTRACE_O_TRACECLONE |
                  PTRACE_O_TRACEEXEC |
                  PTRACE_O_TRACEEXIT |
                  PTRACE_O_EXITKILL) == -1)
        {
            cbInternalError("PTRACE_SETOPTIONS failed: " + std::string(strerror(errno)));
            mIsRunning.store(false, std::memory_order_release);
            return;
        }

        const Arch detectedArch = detectArchFromProcExe(mainPid);
        if(detectedArch != Arch::X86_64)
        {
            const char* archName = detectedArch == Arch::I386 ? "i386" : "unknown";
            cbInternalError("unsupported tracee architecture (" + std::string(archName) +
                            "); only x86_64 is supported");
            kill(mainPid, SIGKILL);
            int killStatus = 0;
            waitpid(mainPid, &killStatus, __WALL);
            mMainPid.store(0, std::memory_order_release);
            mIsRunning.store(false, std::memory_order_release);
            const int exitCode = WIFEXITED(killStatus) ? WEXITSTATUS(killStatus)
                                 : -WTERMSIG(killStatus);
            cbExitProcessEvent(exitCode);
            return;
        }

        createProcessEvent(mainPid, detectedArch);

        if(mThread)
        {
            mThread->registers.Read();
            beginPause();
            cbSystemBreakpoint();
        }

        pauseAndResume(mainPid);

        while(mIsRunning)
        {
            pid = waitpid(-1, &status, __WALL);
            if(pid == -1)
            {
                if(errno == ECHILD)
                {
                    mIsRunning.store(false, std::memory_order_release);
                    break;
                }
                continue;
            }

            if(WIFEXITED(status) || WIFSIGNALED(status))
            {
                const int code = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
                if(pid == mMainPid.load(std::memory_order_relaxed))
                {
                    exitProcessEvent(pid, code);
                    mIsRunning.store(false, std::memory_order_release);
                    break;
                }
                exitThreadEvent(pid);
                continue;
            }

            if(WIFSTOPPED(status))
            {
                handleSignal(pid, status);
            }
        }
    }
}
