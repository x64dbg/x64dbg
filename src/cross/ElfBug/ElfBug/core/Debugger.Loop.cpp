#include <ElfBug/core/Debugger.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <cerrno>
#include <csignal>
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
            if(!thread->isSuspended() && thread->hasPendingBreakpoint())
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
            if(!thread->isSuspended() && thread->pendingSignal() != 0 && thread->pendingSignalUnreported())
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
                if(tid == except || thread->isRunning() || thread->isSuspended())
                    continue;
                if(std::find(mResumeExcept.begin(), mResumeExcept.end(), tid) != mResumeExcept.end())
                    continue;
                stopped.push_back(tid);
            }
        }

        // Pass one: every thread steps off its own breakpoint while the rest stay frozen.
        std::vector<pid_t> parked;
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
                if(!thread || thread->isRunning() || thread->isSuspended())
                    continue;
            }

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

            const StepOff stepped = stepPastBreakpointByte(tid, rip);
            mPendingSignal = 0;

            if(stepped == StepOff::Consumed && (!mProcess || !mIsRunning.load(std::memory_order_acquire)))
                return;
            if(stepped == StepOff::Parked)
                parked.push_back(tid);

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

            // Still on its armed byte; continuing it would replay the hit.
            if(std::find(parked.begin(), parked.end(), tid) != parked.end())
                continue;

            int contError = 0;
            {
                std::unique_lock lock(mProcessMutex);
                const auto it = mProcess->threads.find(tid);
                Thread* thread = it != mProcess->threads.end() ? it->second.get() : nullptr;
                // Pass one can leave a thread running, and it is no longer in ptrace-stop.
                if(!thread || thread->isRunning() || thread->isSuspended())
                    continue;

                // The only delivery this queued signal will ever get.
                const int sig = thread->pendingSignal();
                thread->clearPendingSignal();

                if(ptrace(PTRACE_CONT, tid, nullptr,
                          reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
                    contError = errno;
                else
                    thread->setRunning(true);
            }
            if(contError != 0 && contError != ESRCH)
                cbInternalError("PTRACE_CONT failed: " + std::string(strerror(contError)));
        }

        // A resume that landed after the park found nothing running to poke, so no drain
        // would pick it up; give each parked thread its step-off and continue now.
        for(const pid_t tid : parked)
        {
            if(!mIsRunning.load(std::memory_order_acquire) || !resumeStoppedThread(tid))
                return;
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

    bool Debugger::pauseAndResume(const pid_t reported)
    {
        pid_t reportedTid = reported;
        for(;;)
        {
            std::unique_lock lock(mPauseMutex);

            while(mPaused.load(std::memory_order_acquire) && mIsRunning.load(std::memory_order_acquire) &&
                    !mStopRequested.load(std::memory_order_acquire) &&
                    !mDetachRequested.load(std::memory_order_acquire))
            {
                if(mPauseCv.wait_for(lock, std::chrono::milliseconds(10)) == std::cv_status::timeout)
                    cbPauseTick();
            }

            // A resume can wake the wait before the tick that would apply a request queued
            // just before it. Tick once more while still stopped.
            cbPauseTick();

            lock.unlock();

            if(mStopRequested.load(std::memory_order_acquire))
                return false;

            if(mDetachRequested.load(std::memory_order_acquire))
            {
                detachFromProcess(reportedTid);
                return false;
            }

            if(!mIsRunning.load(std::memory_order_acquire))
                return false;

            // A switch while paused makes the current thread the one to resume. The thread
            // that reported keeps the signal it still owes, and a signal parked on the new
            // current thread comes back, since the resume skips it.
            pid_t pid = reportedTid;
            {
                std::unique_lock processLock(mProcessMutex);
                if(mThread && mProcess && mThread->tid != reportedTid)
                {
                    if(mPendingSignal != 0)
                    {
                        const auto it = mProcess->threads.find(reportedTid);
                        if(it != mProcess->threads.end())
                            it->second->setPendingSignal(mPendingSignal, 0, false);
                        mPendingSignal = 0;
                    }
                    pid = mThread->tid;
                    // mPendingSignal is 0 here: it was either already clear or handed off above.
                    if(mThread->pendingSignal() != 0 && !mThread->pendingSignalUnreported())
                    {
                        mPendingSignal = mThread->pendingSignal();
                        mThread->clearPendingSignal();
                    }
                }

                if(mThread && mProcess && mPendingSignal == 0 &&
                        mThread->pendingSignal() != 0 && !mThread->pendingSignalUnreported())
                {
                    mPendingSignal = mThread->pendingSignal();
                    mThread->clearPendingSignal();
                }
            }

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
                    cbException(signal, address);
                    return pauseAndResume(queuedTid);
                }
            }

            bool stepOverRequested = mStepOverPending.exchange(false, std::memory_order_acq_rel);

            const bool stepIntoRequested = mStepPending.load(std::memory_order_acquire);

            if((stepIntoRequested || stepOverRequested) && mThread)
            {
                bool suspended = false;
                {
                    std::shared_lock processLock(mProcessMutex);
                    suspended = mThread->isSuspended();
                }
                if(suspended)
                {
                    mStepPending.store(false, std::memory_order_release);
                    if(mStopRequested.load(std::memory_order_acquire))
                        return false;

                    beginPause();
                    cbPaused();
                    reportedTid = pid;
                    continue;
                }
            }

            bool onArmedByte = false;
            if(!stepOverRequested && mThread && mProcess)
            {
                std::shared_lock processLock(mProcessMutex);
                onArmedByte = !mThread->isSuspended() &&
                              (mThread->atBreakpoint() || stepIntoRequested) &&
                              mProcess->HasBreakpoint(mThread->registers.Gip());
            }
            bool parkedOnByte = false;
            if(onArmedByte)
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
                else
                {
                    switch(stepPastBreakpointByte(pid, rip))
                    {
                    case StepOff::Stepped:
                        break;
                    case StepOff::Parked:
                        parkedOnByte = true;
                        break;
                    case StepOff::Consumed:
                        abandonFreeze(pid);
                        return false;
                    }
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

                case StepOverArm::Parked:
                    if(mStopRequested.load(std::memory_order_acquire))
                        return false;

                    beginPause();
                    cbPaused();
                    reportedTid = pid;
                    continue;

                case StepOverArm::Armed:
                {
                    bool leftStopped = false;
                    int contError = 0;
                    {
                        std::unique_lock processLock(mProcessMutex);
                        if(mThread->isSuspended())
                            leftStopped = true;
                        else
                        {
                            mThread->clearPendingBreakpoint();
                            const int sig = mPendingSignal;
                            mPendingSignal = 0;
                            if(ptrace(PTRACE_CONT, pid, nullptr,
                                      reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
                                contError = errno;
                            else
                                mThread->setRunning(true);
                        }
                    }
                    if(leftStopped)
                    {
                        cancelStepOver(pid);
                        if(mStopRequested.load(std::memory_order_acquire))
                            return false;

                        beginPause();
                        cbPaused();
                        reportedTid = pid;
                        continue;
                    }
                    if(contError != 0)
                    {
                        if(contError != ESRCH)
                            cbInternalError("PTRACE_CONT failed: " + std::string(strerror(contError)));
                        cancelStepOver(pid);
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

                bool leftStopped = false;
                int stepErrno = 0;
                int contError = 0;
                {
                    std::unique_lock processLock(mProcessMutex);
                    if(mThread->isSuspended())
                        leftStopped = true;
                    else
                    {
                        mThread->clearPendingBreakpoint();
                        const int sig = mPendingSignal;
                        mPendingSignal = 0;
                        if(mThread->stepInto(sig))
                        {
                            mThread->setStepsPushf(stepsPushf);
                            mThread->setRunning(true);
                        }
                        else
                        {
                            stepErrno = errno;
                            if(stepErrno != ESRCH)
                            {
                                restoreSourceByte(pid);
                                if(ptrace(PTRACE_CONT, pid, nullptr,
                                          reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
                                    contError = errno;
                                else
                                    mThread->setRunning(true);
                            }
                        }
                    }
                }
                if(leftStopped)
                {
                    restoreSourceByte(pid);
                    if(mStopRequested.load(std::memory_order_acquire))
                        return false;

                    beginPause();
                    cbPaused();
                    reportedTid = pid;
                    continue;
                }
                if(stepErrno == ESRCH)
                {
                    restoreSourceByte(pid);
                    abandonFreeze(pid);
                }
                else if(stepErrno != 0)
                {
                    cbInternalError("PTRACE_SINGLESTEP failed: " + std::string(strerror(stepErrno)));
                    if(contError != 0 && contError != ESRCH)
                        cbInternalError("PTRACE_CONT failed: " + std::string(strerror(contError)));
                }
            }
            else
            {
                // With every thread suspended nothing would run, so report the pause again.
                bool runnable = false;
                {
                    std::shared_lock processLock(mProcessMutex);
                    if(mProcess)
                    {
                        for(const auto & [tid, thread] : mProcess->threads)
                        {
                            if(!thread->isSuspended())
                            {
                                runnable = true;
                                break;
                            }
                        }
                    }
                }
                if(!runnable)
                {
                    // Stop() won't resume anyone either; looping here would spin forever.
                    if(mStopRequested.load(std::memory_order_acquire))
                        return false;

                    beginPause();
                    cbPaused();
                    reportedTid = pid;
                    continue;
                }

                const int sig = mPendingSignal;
                mPendingSignal = 0;

                resumeAllThreads(pid);
                if(!mProcess || !mIsRunning.load(std::memory_order_acquire))
                    return false;

                bool leftStopped = false;
                bool anyRunning = false;
                int contError = 0;
                {
                    std::unique_lock processLock(mProcessMutex);
                    if(mThread && (parkedOnByte || mThread->isSuspended()))
                    {
                        if(sig != 0)
                            mThread->setPendingSignal(sig, 0, false);
                        leftStopped = true;
                        for(const auto & [tid, thread] : mProcess->threads)
                        {
                            if(thread->isRunning())
                            {
                                anyRunning = true;
                                break;
                            }
                        }
                    }
                    else if(ptrace(PTRACE_CONT, pid, nullptr,
                                   reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
                        contError = errno;
                    else if(mThread)
                        mThread->setRunning(true);
                }
                if(contError != 0 && contError != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(contError)));

                if(leftStopped && !anyRunning)
                {
                    mPauseRequested.store(false, std::memory_order_release);
                    beginPause();
                    cbPaused();
                    reportedTid = pid;
                    continue;
                }
            }
            return true;
        }
    }

    void Debugger::debugLoop()
    {
        if(!(mAttachPid != 0 ? attachToProcess() : startLaunchedProcess()))
        {
            mIsRunning.store(false, std::memory_order_release);
            return;
        }

        const pid_t mainPid = mMainPid.load(std::memory_order_relaxed);

        if(mThread)
        {
            mThread->registers.Read();
            beginPause();
            if(mAttachPid != 0)
                cbAttachBreakpoint();
            else
                cbSystemBreakpoint();
        }

        pauseAndResume(mainPid);

        while(mIsRunning)
        {
            int status = 0;
            const pid_t pid = waitpid(-1, &status, __WALL);
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
