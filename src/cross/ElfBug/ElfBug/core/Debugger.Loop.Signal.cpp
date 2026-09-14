#include <ElfBug/core/Debugger.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#include <csignal>
#include <vector>
#include <unistd.h>

namespace ElfBug
{
    namespace
    {
        // stepPastBreakpointByte can re-enter itself through the stop it reports, so the
        // previous tid has to come back rather than be cleared.
        struct ScopedSteppingOff
        {
            pid_t & slot;
            const pid_t previous;

            ScopedSteppingOff(pid_t & steppingOff, const pid_t tid)
                : slot(steppingOff), previous(steppingOff)
            {
                slot = tid;
            }

            ~ScopedSteppingOff()
            {
                slot = previous;
            }

            ScopedSteppingOff(const ScopedSteppingOff &) = delete;
            ScopedSteppingOff & operator=(const ScopedSteppingOff &) = delete;
        };

        // si_addr only means something for kernel-raised faults; for kill and tkill the
        // same union bytes hold the sender's pid and uid.
        ptr faultAddress(const int signal, const siginfo_t & info)
        {
            if(info.si_code <= 0)
                return 0;
            switch(signal)
            {
            case SIGSEGV:
            case SIGBUS:
            case SIGILL:
            case SIGFPE:
            case SIGTRAP:
                return reinterpret_cast<ptr>(info.si_addr);
            default:
                return 0;
            }
        }

        bool sweepShouldQueue(const int signal, const bool hardware)
        {
            switch(signal)
            {
            case SIGSTOP:
                return false;
            case SIGSEGV:
            case SIGBUS:
            case SIGFPE:
            case SIGILL:
            case SIGSYS:
            case SIGTRAP:
                return !hardware;
            default:
                return true;
            }
        }
    }

    void Debugger::repairStoppedThread(Thread* thread, const int status) const
    {
        if(!thread)
            return;

        thread->registers.Read();

        const int sig = WSTOPSIG(status);

        // si_code is positive when the kernel raised the signal and zero or negative for
        // kill, tkill and sigqueue. Unreadable means the thread already left this stop
        // (exit_group kicked it out), so there is nothing left to forward.
        siginfo_t info{};
        const bool haveInfo = ptrace(PTRACE_GETSIGINFO, thread->tid, nullptr, &info) != -1;
        const bool hardware = haveInfo && info.si_code > 0;

        // Nothing else records it, so queue it for pauseAndResume to report and forward.
        if(haveInfo && sweepShouldQueue(sig, hardware))
            thread->setPendingSignal(sig, faultAddress(sig, info), true);

        if(!mProcess || sig != SIGTRAP)
            return;
        // A ptrace event, not an int3 trap.
        if(((status >> 16) & 0xffff) != 0)
            return;
        if(thread->isSingleStepping())
            return;

        const ptr bpAddr = thread->registers.Gip() - 1;
        if(!mProcess->HasBreakpoint(bpAddr))
            return;

        thread->registers.Gip() = bpAddr;
        thread->registers.Write();
        thread->setAtBreakpoint(true);
        thread->setPendingBreakpoint(bpAddr);
    }

    bool Debugger::swallowPendingSigstop(const pid_t tid)
    {
        if(!mProcess)
            return true;

        Thread* thread = nullptr;
        {
            std::shared_lock lock(mProcessMutex);
            const auto it = mProcess->threads.find(tid);
            if(it != mProcess->threads.end())
                thread = it->second.get();
        }
        if(!thread || !thread->pendingSigstop())
            return true;

        if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
        {
            thread->setPendingSigstop(false);
            return true;
        }

        int status = 0;
        pid_t waited = -1;
        do
        {
            waited = waitpid(tid, &status, __WALL);
        }
        while(waited == -1 && errno == EINTR);

        if(waited == -1)
        {
            thread->setPendingSigstop(false);
            return true;
        }

        if(WIFEXITED(status) || WIFSIGNALED(status))
        {
            const int code = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
            if(tid == mMainPid.load(std::memory_order_relaxed))
            {
                exitProcessEvent(tid, code);
                mIsRunning.store(false, std::memory_order_release);
            }
            else
            {
                exitThreadEvent(tid);
            }
            return false;
        }

        if(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP)
            thread->setPendingSigstop(false);
        return true;
    }

    void Debugger::stopAllThreads(const pid_t except)
    {
        if(!mProcess)
            return;

        const pid_t tgid = mMainPid.load(std::memory_order_relaxed);
        if(tgid <= 0)
            return;

        for(;;)
        {
            std::vector<pid_t> running;
            {
                std::shared_lock lock(mProcessMutex);
                for(const auto & [tid, thread] : mProcess->threads)
                {
                    if(tid != except && thread->isRunning())
                        running.push_back(tid);
                }
            }

            if(running.empty())
                return;

            for(const pid_t tid : running)
            {
                Thread* thread = nullptr;
                {
                    std::shared_lock lock(mProcessMutex);
                    const auto it = mProcess->threads.find(tid);
                    if(it != mProcess->threads.end())
                        thread = it->second.get();
                }
                if(!thread)
                    continue;

                const auto markStopped = [&]
                {
                    std::unique_lock lock(mProcessMutex);
                    thread->setRunning(false);
                };

                {
                    const std::string reason = readWaitReason(tgid, tid);
                    std::unique_lock lock(mProcessMutex);
                    if(!thread->isSuspended())
                        thread->setWaitReason(reason);
                }
                if(tgkill(tgid, tid, SIGSTOP) == -1)
                {
                    if(errno != ESRCH)
                        cbInternalError("tgkill failed: " + std::string(strerror(errno)));
                    markStopped();
                    continue;
                }

                int status = 0;
                pid_t waited = -1;
                do
                {
                    waited = waitpid(tid, &status, __WALL);
                }
                while(waited == -1 && errno == EINTR);

                if(waited == -1)
                {
                    markStopped();
                    continue;
                }

                if(WIFEXITED(status) || WIFSIGNALED(status))
                {
                    exitThreadEvent(tid);
                    continue;
                }

                const int event = (status >> 16) & 0xffff;
                if(WSTOPSIG(status) == SIGTRAP && event != 0)
                {
                    if(event == PTRACE_EVENT_CLONE)
                    {
                        unsigned long newTid = 0;
                        if(ptrace(PTRACE_GETEVENTMSG, tid, nullptr, &newTid) == -1)
                            cbInternalError("PTRACE_GETEVENTMSG failed: " + std::string(strerror(errno)));
                        else
                            createThreadEvent(static_cast<pid_t>(newTid));
                    }
                    else if(event == PTRACE_EVENT_EXEC)
                    {
                        onExec();
                    }

                    if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
                    {
                        if(errno != ESRCH)
                            cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                        markStopped();
                    }
                    else if(event == PTRACE_EVENT_EXIT)
                    {
                        markStopped();
                    }
                    continue;
                }

                const bool ownReason = WSTOPSIG(status) != SIGSTOP;
                {
                    std::unique_lock lock(mProcessMutex);
                    thread->setRunning(false);
                    if(ownReason && !thread->isSuspended())
                        thread->setWaitReason({});
                    thread->setPendingSigstop(ownReason);
                }
                if(ownReason)
                    repairStoppedThread(thread, status);
            }
        }
    }

    void Debugger::drainPendingResumes()
    {
        std::vector<pid_t> resuming;
        {
            std::lock_guard pauseLock(mPauseMutex);
            resuming.assign(mPendingResume.begin(), mPendingResume.end());
            mPendingResume.clear();
        }

        for(const pid_t tid : resuming)
        {
            if(!resumeStoppedThread(tid))
                return;
        }
    }

    bool Debugger::resumeStoppedThread(const pid_t tid)
    {
        Thread* thread = nullptr;
        {
            std::shared_lock lock(mProcessMutex);
            if(!mProcess)
                return false;
            const auto it = mProcess->threads.find(tid);
            if(it != mProcess->threads.end())
                thread = it->second.get();
            if(!thread || thread->isSuspended() || thread->isRunning())
                return true;
        }

        if(thread->registers.Read() && thread->atBreakpoint() &&
                mProcess->HasBreakpoint(thread->registers.Gip()))
        {
            const ptr rip = thread->registers.Gip();
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
                return false;

            {
                std::unique_lock lock(mProcessMutex);
                mThread = previous;
            }

            if(stepped == StepOff::Parked)
                return true;
        }

        int contError = 0;
        {
            std::unique_lock lock(mProcessMutex);
            if(!mProcess)
                return false;
            const auto it = mProcess->threads.find(tid);
            thread = it != mProcess->threads.end() ? it->second.get() : nullptr;
            if(!thread || thread->isSuspended() || thread->isRunning())
                return true;

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
        return true;
    }

    void Debugger::continueUnlessSuspended(const pid_t pid)
    {
        bool leftStopped = false;
        int contError = 0;
        {
            std::unique_lock lock(mProcessMutex);
            if(mThread && mThread->isSuspended())
                leftStopped = true;
            else if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                contError = errno;
            else if(mThread)
                mThread->setRunning(true);
        }
        if(contError != 0 && contError != ESRCH)
            cbInternalError("PTRACE_CONT failed: " + std::string(strerror(contError)));

        if(leftStopped)
            leaveParked(pid);
    }

    void Debugger::leaveParked(const pid_t pid)
    {
        bool anyRunning = false;
        {
            std::shared_lock lock(mProcessMutex);
            if(!mProcess || !mThread)
                return;
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

        mPauseRequested.store(false, std::memory_order_release);
        mThread->registers.Read();
        beginPause();
        cbPaused();
        pauseAndResume(pid);
    }

    void Debugger::handleSignal(const pid_t pid, const int status)
    {
        const int sig = WSTOPSIG(status);
        mPendingSignal = 0;

        bool wasRunning = false;
        bool suspended = false;
        {
            std::unique_lock lock(mProcessMutex);
            mThread = nullptr;
            if(mProcess)
            {
                const auto it = mProcess->threads.find(pid);
                if(it != mProcess->threads.end())
                    mThread = it->second.get();
            }
            if(mThread)
            {
                wasRunning = mThread->isRunning();
                suspended = mThread->isSuspended();
                mThread->setRunning(false);
                if(sig == SIGSTOP)
                    mThread->setPendingSigstop(false);
            }
        }

        switch(sig)
        {
        case SIGTRAP:
        {
            siginfo_t info{};
            if(((status >> 16) & 0xffff) == 0 &&
                    ptrace(PTRACE_GETSIGINFO, pid, nullptr, &info) != -1 && info.si_code <= 0)
                reportSignal(pid, sig);
            else
                handleSigtrap(pid, status);
            break;
        }

        case SIGSTOP:
        {
            {
                bool requestedStop = false;
                bool resumedMeanwhile = false;
                {
                    std::lock_guard pauseLock(mPauseMutex);
                    requestedStop = mPendingSuspend.erase(pid) != 0;
                    if(requestedStop && mThread && !mThread->isSuspended())
                    {
                        mPendingResume.erase(pid);
                        requestedStop = false;
                        resumedMeanwhile = true;
                    }
                }
                if(requestedStop)
                {
                    if(!mPauseRequested.load(std::memory_order_acquire))
                    {
                        bool anyRunning = false;
                        {
                            std::shared_lock lock(mProcessMutex);
                            if(mProcess)
                            {
                                for(const auto & [tid, thread] : mProcess->threads)
                                {
                                    if(thread->isRunning())
                                    {
                                        anyRunning = true;
                                        break;
                                    }
                                }
                            }
                        }

                        if(!anyRunning)
                        {
                            mPauseRequested.store(false, std::memory_order_release);
                            if(mThread)
                                mThread->registers.Read();
                            beginPause();
                            cbPaused();
                            pauseAndResume(pid);
                        }
                        break;
                    }
                }

                bool hasResumes = false;
                {
                    std::lock_guard pauseLock(mPauseMutex);
                    hasResumes = !mPendingResume.empty();
                }
                if((hasResumes || resumedMeanwhile) && !mPauseRequested.load(std::memory_order_acquire))
                {
                    drainPendingResumes();
                    continueUnlessSuspended(pid);
                    break;
                }
            }
            if(mPauseRequested.load(std::memory_order_acquire))
            {
                mPauseRequested.store(false, std::memory_order_release);
                if(mThread)
                {
                    mThread->registers.Read();
                    abandonSingleStep(pid);
                    cancelStepOverIfOwner(pid);
                    stopAllThreads(pid);
                    beginPause();
                    cbPaused();

                    if(!pauseAndResume(pid))
                        break;
                }
                else
                {
                    if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                    {
                        if(errno != ESRCH)
                            cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                    }
                    else
                        mUnregisteredRunning.insert(pid);
                }
            }
            else if(mThread && mThread->isSingleStepping())
            {
                // Not our SIGSTOP: the step is still owed, so re-issue it.
                if(mThread->StepInto())
                {
                    mThread->setRunning(true);
                }
                else
                {
                    abandonSingleStep(pid);
                    if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                    {
                        if(errno != ESRCH)
                            cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                    }
                    else
                        mThread->setRunning(true);
                }
            }

            // A stray SIGSTOP for a thread the user froze; leave it stopped rather than
            // letting this catch-all wake it back up.
            else if(!suspended && (!mAllStopped || wasRunning || pid == mSteppingOff || !mThread))
            {
                bool leftStopped = false;
                bool anyRunning = false;
                int contError = 0;
                {
                    std::unique_lock lock(mProcessMutex);
                    if(mThread && mThread->isSuspended())
                    {
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
                    else if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                        contError = errno;
                    else if(mThread)
                        mThread->setRunning(true);
                    else
                        mUnregisteredRunning.insert(pid);
                }
                if(contError != 0 && contError != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(contError)));

                if(leftStopped && !anyRunning)
                {
                    mPauseRequested.store(false, std::memory_order_release);
                    beginPause();
                    cbPaused();
                    pauseAndResume(pid);
                }
            }
            break;
        }

        default:
            reportSignal(pid, sig);
            break;
        }
    }

    void Debugger::reportSignal(const pid_t pid, const int sig)
    {
        ptr faultAddr = 0;
        siginfo_t sigInfo;
        if(ptrace(PTRACE_GETSIGINFO, pid, nullptr, &sigInfo) != -1)
            faultAddr = faultAddress(sig, sigInfo);
        if(mThread)
        {
            mThread->registers.Read();
            mPendingSignal = sig;
            abandonSingleStep(pid);
            cancelStepOverIfOwner(pid);
            stopAllThreads(pid);
            beginPause();
            cbExceptionEvent(sig, faultAddr);
            pauseAndResume(pid);
        }
        else
        {
            cbExceptionEvent(sig, faultAddr);
            if(ptrace(PTRACE_CONT, pid, nullptr,
                      reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
            {
                if(errno != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            }
        }
    }

    Debugger::StepOff Debugger::stepPastBreakpointByte(const pid_t pid, const ptr addr)
    {
        if(!mProcess || !mThread)
            return StepOff::Consumed;

        const ScopedSteppingOff scopedSteppingOff(mSteppingOff, pid);

        ptr next = 0;
        const bool stepsPushf = mProcess->ClassifyStepOverAt(addr, next) == StepOverKind::Pushf;

        // Only re-arm what this call lifted; another thread may hold the byte for its own step.
        const bool lifted = mProcess->DisarmBreakpointByte(addr);

        // The pending signal rides the step; otherwise the faulting instruction is retried.
        const int sig = mPendingSignal;
        mPendingSignal = 0;

        if(!swallowPendingSigstop(pid))
        {
            if(lifted && mProcess)
                mProcess->RearmBreakpointByte(addr);
            return StepOff::Consumed;
        }

        bool leftStopped = false;
        int stepErrno = 0;
        {
            std::unique_lock lock(mProcessMutex);
            if(mThread->isSuspended())
            {
                leftStopped = true;
                if(sig != 0)
                    mThread->setPendingSignal(sig, 0, false);
            }
            else if(!mThread->StepInto(sig))
                stepErrno = errno;
        }
        if(leftStopped)
        {
            if(lifted)
                mProcess->RearmBreakpointByte(addr);
            return StepOff::Parked;
        }

        if(stepErrno != 0)
        {
            if(stepErrno != ESRCH)
                cbInternalError("PTRACE_SINGLESTEP failed: " + std::string(strerror(stepErrno)));
            if(lifted)
                mProcess->RearmBreakpointByte(addr);
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
            return StepOff::Consumed;
        }

        int stepStatus = 0;
        pid_t waited = -1;
        do
        {
            waited = waitpid(pid, &stepStatus, __WALL);
        }
        while(waited == -1 && errno == EINTR);

        if(waited == -1)
        {
            cbInternalError("waitpid(step) failed: " + std::string(strerror(errno)));
            if(lifted)
                mProcess->RearmBreakpointByte(addr);
            if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
            {
                if(errno != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            }
            else
            {
                mThread->setRunning(true);
            }
            return StepOff::Consumed;
        }

        mThread->clearSingleStep();

        if(WIFEXITED(stepStatus) || WIFSIGNALED(stepStatus))
        {
            if(lifted && mProcess)
                mProcess->RearmBreakpointByte(addr);

            const int code = WIFEXITED(stepStatus) ? WEXITSTATUS(stepStatus) : -WTERMSIG(stepStatus);
            if(pid == mMainPid.load(std::memory_order_relaxed))
            {
                exitProcessEvent(pid, code);
                mIsRunning.store(false, std::memory_order_release);
            }
            else
            {
                exitThreadEvent(pid);
            }
            return StepOff::Consumed;
        }

        if(WIFSTOPPED(stepStatus))
        {
            const int stepSig = WSTOPSIG(stepStatus);
            const int stepEvent = (stepStatus >> 16) & 0xffff;
            mThread->registers.Read();

            if(stepEvent == PTRACE_EVENT_EXEC)
            {
                // The image is gone; re-arming would write into the new one.
                onExec();
                if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                {
                    if(errno != ESRCH)
                        cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                }
                else
                {
                    mThread->setRunning(true);
                }
                return StepOff::Consumed;
            }

            if(lifted)
                mProcess->RearmBreakpointByte(addr);

            if(stepEvent == PTRACE_EVENT_CLONE)
            {
                unsigned long newTid = 0;
                if(ptrace(PTRACE_GETEVENTMSG, pid, nullptr, &newTid) == -1)
                    cbInternalError("PTRACE_GETEVENTMSG failed: " + std::string(strerror(errno)));
                else
                    createThreadEvent(static_cast<pid_t>(newTid));
            }

            if(stepSig != SIGTRAP)
            {
                // Not the step's trap: report it like any other stop. The byte is re-armed,
                // so the breakpoint fires again when a handler returns to the instruction.
                handleSignal(pid, stepStatus);
                return StepOff::Consumed;
            }

            if(stepsPushf)
                maskPushedTrapFlag();
        }

        return StepOff::Stepped;
    }

    void Debugger::abandonSingleStep(const pid_t pid)
    {
        if(mThread)
            mThread->clearSingleStep();

        restoreSourceByte(pid);
    }

    void Debugger::handleSigtrap(const pid_t pid, const int status)
    {
        const int event = (status >> 16) & 0xffff;

        switch(event)
        {
        case PTRACE_EVENT_EXEC:
        {
            onExec();
            // TODO: re-exec handling - re-detect arch and reject if no longer x86_64, clear breakpoints, refresh memory map, fire callback
            continueUnlessSuspended(pid);
            break;
        }

        case PTRACE_EVENT_CLONE:
        {
            const bool wasStepping = mThread && mThread->isSingleStepping();
            abandonSingleStep(pid);

            unsigned long newTid = 0;
            if(ptrace(PTRACE_GETEVENTMSG, pid, nullptr, &newTid) == -1)
            {
                cbInternalError("PTRACE_GETEVENTMSG failed: " + std::string(strerror(errno)));
            }
            else
            {
                createThreadEvent(static_cast<pid_t>(newTid));
            }

            // This event replaced the step's trap; resuming here would swallow the step.
            if(wasStepping && mThread)
            {
                mThread->registers.Read();
                stopAllThreads(pid);
                beginPause();
                cbStep();
                pauseAndResume(pid);
                break;
            }

            continueUnlessSuspended(pid);
            break;
        }

        case PTRACE_EVENT_EXIT:
        {
            abandonSingleStep(pid);
            // Notification only; exit is emitted via WIFEXITED/WIFSIGNALED in debugLoop.
            // Left marked stopped: it owes no stop, so a sweep would block in waitpid.
            if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
            {
                if(errno != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            }
            break;
        }

        default:
        {
            if(!mThread)
            {
                // Its stop is being handled here, so drop any stale resumed bit
                // and let createThreadEvent register the thread as stopped.
                mUnregisteredRunning.erase(pid);
                createThreadEvent(pid);
                {
                    std::shared_lock lock(mProcessMutex);
                    if(mProcess)
                    {
                        const auto it = mProcess->threads.find(pid);
                        if(it != mProcess->threads.end())
                            mThread = it->second.get();
                    }
                }
            }
            if(!mThread)
            {
                if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                {
                    if(errno != ESRCH)
                        cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                }
                break;
            }

            mThread->registers.Read();

            const ptr bpAddr = mThread->registers.Gip() - 1;
            const bool stepOverHit = mProcess && mStepOver.active && bpAddr == mStepOver.target;

            // A step-over always completes under PTRACE_CONT, so a single-step trap is
            // never one. Letting the address match win would rewind RIP over a real step.
            if(mThread->isSingleStepping())
            {
                const bool stepsPushf = mThread->stepsPushf();
                mThread->clearSingleStep();
                if(stepsPushf)
                    maskPushedTrapFlag();
                restoreSourceByte(pid);
                stopAllThreads(pid);
                beginPause();
                cbStep();
                if(!pauseAndResume(pid))
                    break;
                break;
            }

            if(mProcess)
            {
                if(stepOverHit)
                {
                    mThread->clearSingleStep();
                    mThread->registers.Gip() = bpAddr;
                    mThread->registers.Write();

                    const bool rightThread = (pid == mStepOver.tid);
                    const ptr rsp = mThread->registers.Gsp();
                    // get-PC idiom: the return address is still pushed when we trap.
                    bool completedWithoutReturn = false;
                    if(rsp == mStepOver.rspFloor - 8)
                    {
                        ptr pushed = 0;
                        completedWithoutReturn = mProcess->MemRead(rsp, &pushed, sizeof(pushed)) &&
                                                 pushed == mStepOver.target;
                    }
                    const bool rightFrame = rsp >= mStepOver.rspFloor || completedWithoutReturn;

                    if(rightThread && rightFrame)
                    {
                        const bool planted = mStepOver.planted;
                        const ptr target = mStepOver.target;
                        mStepOver = {};

                        if(planted)
                            mProcess->DeleteBreakpoint(target);
                        mThread->setAtBreakpoint(!planted);

                        restoreSourceByte(pid);

                        stopAllThreads(pid);
                        beginPause();
                        // Not ours: the user's own breakpoint fired at the target.
                        if(!planted)
                            dispatchBreakpoint(target);
                        cbStep();
                        pauseAndResume(pid);
                        break;
                    }

                    // Wrong thread or frame: skip our own trap, report a user breakpoint.
                    if(mStepOver.planted)
                    {
                        switch(stepPastBreakpointByte(pid, bpAddr))
                        {
                        case StepOff::Stepped:
                            continueUnlessSuspended(pid);
                            break;
                        case StepOff::Parked:
                            leaveParked(pid);
                            break;
                        case StepOff::Consumed:
                            cancelStepOver(pid);
                            // An armed run is still inside a frozen window; this is the last
                            // chance to lift it.
                            abandonFreeze(pid);
                            break;
                        }
                        break;
                    }
                }

                if(mProcess->HasBreakpoint(bpAddr))
                {
                    mThread->registers.Gip() = bpAddr;
                    mThread->registers.Write();
                    mThread->setAtBreakpoint(true);

                    cancelStepOverIfOwner(pid);
                    stopAllThreads(pid);
                    beginPause();
                    dispatchBreakpoint(bpAddr);
                    // The 0xCC stays armed with RIP on it; the next resume steps past it.
                    pauseAndResume(pid);
                    break;
                }

                if(stepOverHit)
                    cancelStepOver(pid);
            }

            restoreSourceByte(pid);

            continueUnlessSuspended(pid);
            break;
        }
        }
    }
}
