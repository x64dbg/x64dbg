#include <ElfBug/core/Debugger.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <utility>
#include <vector>

namespace ElfBug
{
    namespace
    {
        constexpr auto kPauseTickInterval = std::chrono::milliseconds(10);

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

    void Debugger::beginPause()
    {
        cancelStepResume();
        for(auto & [tid, silent] : mSilentStepOffs)
            silent.stepOver = false;
        mAllStopped = true;

        std::lock_guard lock(mPauseMutex);
        mPaused.store(true, std::memory_order_release);
    }

    void Debugger::takeDecidedSignal()
    {
        std::unique_lock lock(mProcessMutex);
        if(!mThread || !mProcess || mOwedSignal.Signal() != 0 || !mThread->Owed().Decided())
            return;
        mOwedSignal = mThread->TakePendingSignal();
    }

    void Debugger::parkOwedSignal(const pid_t tid)
    {
        OwedSignal owed = std::exchange(mOwedSignal, {});
        if(owed.Signal() == 0 || !mProcess)
            return;
        if(Thread* thread = mProcess->FindThread(tid))
            thread->SetPendingSignal(std::move(owed));
    }

    Thread* Debugger::findPendingBreakpointThread() const
    {
        if(!mProcess)
            return nullptr;

        std::shared_lock lock(mProcessMutex);
        for(const auto & entry : mProcess->threads)
        {
            if(!entry.second->IsSuspended() && entry.second->HasPendingBreakpoint())
                return entry.second.get();
        }
        return nullptr;
    }

    Thread* Debugger::findPendingSignalThread() const
    {
        if(!mProcess)
            return nullptr;

        std::shared_lock lock(mProcessMutex);
        for(const auto & entry : mProcess->threads)
        {
            if(!entry.second->IsSuspended() && entry.second->Owed().Undecided())
                return entry.second.get();
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
                if(tid == except || thread->IsRunning() || thread->IsSuspended())
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
                thread = mProcess->FindThread(tid);
                if(!thread || thread->IsRunning() || thread->IsSuspended())
                    continue;
            }

            // A thread frozen just before the byte has not hit it; stepping it off would
            // skip the hit. Only a rewound thread owes a step.
            if(!thread->AtBreakpoint() || !thread->registers.Read())
                continue;

            const ptr rip = thread->registers.Gip();
            if(!mProcess->HasBreakpoint(rip))
                continue;

            Thread* previous = nullptr;
            {
                std::unique_lock lock(mProcessMutex);
                previous = mThread;
                mThread = thread;
            }

            const StepOff stepped = stepOffHeld(thread, rip);
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

            if(std::find(parked.begin(), parked.end(), tid) != parked.end())
                continue;

            int contError = 0;
            {
                std::unique_lock lock(mProcessMutex);
                Thread* thread = mProcess->FindThread(tid);
                // Pass one can leave a thread running, and it is no longer in ptrace-stop.
                if(!thread || thread->IsRunning() || thread->IsSuspended())
                    continue;

                if(continueWith(tid, thread->TakePendingSignal()))
                    thread->SetRunning(true);
                else
                    contError = errno;
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

    bool Debugger::anyThreadRunningLocked() const
    {
        if(!mProcess)
            return false;
        for(const auto & entry : mProcess->threads)
        {
            if(entry.second->IsRunning())
                return true;
        }
        return false;
    }

    bool Debugger::anyThreadRunning() const
    {
        std::shared_lock lock(mProcessMutex);
        return anyThreadRunningLocked();
    }

    Debugger::ContinueResult Debugger::continueOrPark(const pid_t tid, const OwedSignal & signal, const bool forcePark)
    {
        bool parked = false;
        bool alone = false;
        bool untracked = false;
        int contError = 0;
        {
            std::unique_lock lock(mProcessMutex);
            if(mThread && (forcePark || mThread->IsSuspended()))
            {
                if(signal.Signal() != 0)
                    mThread->SetPendingSignal(signal);
                parked = true;
                alone = !anyThreadRunningLocked();
            }
            else
            {
                if(!continueWith(tid, signal))
                    contError = errno;
                else if(mThread)
                    mThread->SetRunning(true);
                else
                    untracked = true;
            }
        }
        if(contError != 0 && contError != ESRCH)
            cbInternalError("PTRACE_CONT failed: " + std::string(strerror(contError)));

        if(parked)
            return alone ? ContinueResult::ParkedAlone : ContinueResult::Parked;
        return untracked ? ContinueResult::ContinuedUntracked : ContinueResult::Continued;
    }

    void Debugger::abandonAllStop(const pid_t except)
    {
        if(!mAllStopped)
            return;

        if(anyThreadRunning())
            return;

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
                if(mPauseCv.wait_for(lock, kPauseTickInterval) == std::cv_status::timeout)
                    cbPauseTick();
            }

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

            pid_t pid = reportedTid;
            {
                std::unique_lock processLock(mProcessMutex);
                if(mThread && mProcess && mThread->tid != reportedTid)
                {
                    parkOwedSignal(reportedTid);
                    pid = mThread->tid;
                }
            }

            if(!mStepPending.load(std::memory_order_acquire) &&
                    !mStepOverPending.load(std::memory_order_acquire) &&
                    !mStopRequested.load(std::memory_order_acquire))
            {
                while(Thread* queued = findPendingBreakpointThread())
                {
                    const ptr address = queued->PendingBreakpoint();
                    const pid_t queuedTid = queued->tid;
                    queued->ClearPendingBreakpoint();

                    if(!mProcess || !mProcess->HasBreakpoint(address))
                        continue;

                    {
                        std::unique_lock processLock(mProcessMutex);
                        parkOwedSignal(pid);
                        mThread = queued;
                    }
                    beginPause();
                    dispatchBreakpoint(address);
                    return pauseAndResume(queuedTid);
                }

                while(Thread* queued = findPendingSignalThread())
                {
                    const OwedSignal owed = queued->Owed();
                    const pid_t queuedTid = queued->tid;
                    const SignalDecision decision = decideParkedSignal(queued);
                    if(!decision.stop)
                    {
                        std::unique_lock processLock(mProcessMutex);
                        queued->SetPendingSignal(OwedSignal(decision.deliver, ParkedSignal::Filtered, owed.Info(), owed.Address()));
                        continue;
                    }
                    queued->ClearPendingSignal();

                    {
                        std::unique_lock processLock(mProcessMutex);
                        parkOwedSignal(pid);
                        mThread = queued;
                    }
                    mOwedSignal = OwedSignal(decision.deliver, ParkedSignal::Reported, owed.Info());
                    beginPause();
                    cbException(owed.Signal(), owed.Address());
                    return pauseAndResume(queuedTid);
                }
            }

            takeDecidedSignal();

            bool stepOverRequested = mStepOverPending.exchange(false, std::memory_order_acq_rel);

            const bool stepIntoRequested = mStepPending.load(std::memory_order_acquire);

            if((stepIntoRequested || stepOverRequested) && mThread)
            {
                bool suspended = false;
                {
                    std::shared_lock processLock(mProcessMutex);
                    suspended = mThread->IsSuspended();
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
                onArmedByte = !mThread->IsSuspended() &&
                              (mThread->AtBreakpoint() || stepIntoRequested) &&
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
                        abandonAllStop(pid);
                        return false;
                    case StepOff::Deferred:
                        break;
                    }
                    takeDecidedSignal();
                }
            }

            if(stepOverRequested && mThread)
            {
                mStepPending.store(false, std::memory_order_release);
                takeUndecidedSignal();

                switch(armStepOver(pid))
                {
                case StepOverArm::Consumed:
                    abandonAllStop(pid);
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
                    takeDecidedSignal();
                    bool leftStopped = false;
                    int contError = 0;
                    {
                        std::unique_lock processLock(mProcessMutex);
                        if(mThread->IsSuspended())
                            leftStopped = true;
                        else
                        {
                            mThread->ClearPendingBreakpoint();
                            if(continueWith(pid, std::exchange(mOwedSignal, {})))
                                mThread->SetRunning(true);
                            else
                                contError = errno;
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

            const bool stepping = (stepIntoRequested || stepOverRequested) && mThread;
            if(stepping)
                takeUndecidedSignal();

            if(stepping && HidesHandler(mOwedSignal))
            {
                mStepPending.store(false, std::memory_order_release);
                restoreSourceByte(pid);
                if(!swallowPendingSigstop(pid))
                {
                    abandonAllStop(pid);
                    return false;
                }
                if(armStepResume(pid, mOwedSignal))
                {
                    mOwedSignal = {};
                    return true;
                }
            }

            if(stepping)
            {
                mStepPending.store(false, std::memory_order_release);
                ptr next = 0;
                const bool stepsPushf = mProcess &&
                                        mProcess->ClassifyStepOverAt(mThread->registers.Gip(), next) == StepOverKind::Pushf;
                // A queued SIGSTOP would be delivered by the step instead of an instruction.
                if(!swallowPendingSigstop(pid))
                {
                    restoreSourceByte(pid);
                    abandonAllStop(pid);
                    return false;
                }

                bool leftStopped = false;
                int stepErrno = 0;
                int contError = 0;
                {
                    std::unique_lock processLock(mProcessMutex);
                    if(mThread->IsSuspended())
                        leftStopped = true;
                    else
                    {
                        mThread->ClearPendingBreakpoint();
                        const OwedSignal owed = std::exchange(mOwedSignal, {});
                        const int sig = owed.Signal();
                        takeSilentStepOff(pid, mThread->registers.Gip(), mThread->registers.Gsp());
                        restoreSignalInfo(pid, owed);
                        if(mThread->StepInto(sig))
                        {
                            mThread->SetStepsPushf(stepsPushf);
                            mThread->SetRunning(true);
                        }
                        else
                        {
                            stepErrno = errno;
                            if(stepErrno != ESRCH)
                            {
                                restoreSourceByte(pid);
                                if(ptrace(PTRACE_CONT, pid, nullptr, PtraceData(sig)) == -1)
                                    contError = errno;
                                else
                                    mThread->SetRunning(true);
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
                    abandonAllStop(pid);
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
                bool runnable = false;
                {
                    std::shared_lock processLock(mProcessMutex);
                    if(mProcess)
                    {
                        for(const auto & entry : mProcess->threads)
                        {
                            if(!entry.second->IsSuspended())
                            {
                                runnable = true;
                                break;
                            }
                        }
                    }
                }
                if(!runnable)
                {
                    if(mStopRequested.load(std::memory_order_acquire))
                        return false;

                    beginPause();
                    cbPaused();
                    reportedTid = pid;
                    continue;
                }

                const OwedSignal owed = std::exchange(mOwedSignal, {});

                resumeAllThreads(pid);
                if(!mProcess || !mIsRunning.load(std::memory_order_acquire))
                    return false;

                if(continueOrPark(pid, owed, parkedOnByte) == ContinueResult::ParkedAlone)
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
            const pid_t pid = waitpid(-1, &status, __WALL | __WNOTHREAD);
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
