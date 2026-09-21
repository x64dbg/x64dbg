#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/ProcessArch.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <thread>
#include <vector>

namespace ElfBug
{
    namespace
    {
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
    }

    ptr FaultAddress(const int signal, const siginfo_t & info)
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

    bool SweepShouldQueue(const int signal, const bool hardware)
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

    WaitResult WaitForStop(const pid_t tid, int & status)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
        for(;;)
        {
            const pid_t waited = waitpid(tid, &status, __WALL | WNOHANG);
            if(waited == tid)
                return WaitResult::Stopped;
            if(waited == -1 && errno == EINTR)
                continue;
            if(waited == -1)
                return WaitResult::Gone;
            if(std::chrono::steady_clock::now() >= deadline)
                return WaitResult::TimedOut;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    bool StopShouldQueue(const int status, const bool haveInfo, const siginfo_t & info,
                         int & signal, ptr & address)
    {
        if(((status >> 16) & 0xffff) != 0 || !haveInfo)
            return false;

        const int sig = WSTOPSIG(status);
        if(!SweepShouldQueue(sig, info.si_code > 0))
            return false;

        signal = sig;
        address = FaultAddress(sig, info);
        return true;
    }

    bool Debugger::rewindOntoBreakpoint(Thread* thread, const int status)
    {
        if(!mProcess || !thread || WSTOPSIG(status) != SIGTRAP)
            return false;
        // A ptrace event, not an int3 trap.
        if(((status >> 16) & 0xffff) != 0)
            return false;
        if(thread->isSingleStepping())
            return false;

        const ptr bpAddr = thread->registers.Gip() - 1;
        if(!mProcess->HasBreakpoint(bpAddr))
            return false;

        thread->registers.Gip() = bpAddr;
        thread->registers.Write();
        thread->setAtBreakpoint(true);
        thread->setPendingBreakpoint(bpAddr);
        return true;
    }

    void Debugger::repairStoppedThread(Thread* thread, const int status)
    {
        if(!thread)
            return;

        thread->registers.Read();

        // si_code > 0 means the kernel raised it, not kill/tkill/sigqueue.
        siginfo_t info{};
        const bool haveInfo = ptrace(PTRACE_GETSIGINFO, thread->tid, nullptr, &info) != -1;

        int signal = 0;
        ptr address = 0;
        if(StopShouldQueue(status, haveInfo, info, signal, address))
            thread->setPendingSignal(signal, address, true);

        rewindOntoBreakpoint(thread, status);
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

        // A thread that has not stopped yet is not in ptrace-stop, so restarting it fails
        // and the SIGSTOP is still on its way. Only restart one we know is stopped, and
        // never clear the flag on a path that did not observe the signal: the caller reads
        // it to decide whether the released process still needs a SIGCONT.
        bool inPtraceStop = false;
        {
            std::shared_lock lock(mProcessMutex);
            inPtraceStop = !thread->isRunning();
        }

        constexpr int kDrainAttempts = 32;
        for(int attempt = 0; attempt < kDrainAttempts; attempt++)
        {
            if(inPtraceStop && ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
                return true;

            int status = 0;
            if(WaitForStop(tid, status) != WaitResult::Stopped)
                return true;

            inPtraceStop = true;

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

            if(!WIFSTOPPED(status))
                continue;

            const int sig = WSTOPSIG(status);
            if(sig == SIGSTOP)
            {
                thread->setPendingSigstop(false);
                return true;
            }

            if(thread->pendingSignal() == 0)
            {
                siginfo_t info{};
                const bool haveInfo = ptrace(PTRACE_GETSIGINFO, tid, nullptr, &info) != -1;
                int signal = 0;
                ptr address = 0;
                if(StopShouldQueue(status, haveInfo, info, signal, address))
                    thread->setPendingSignal(signal, address, false);
            }

            if(sig == SIGTRAP && thread->registers.Read())
                rewindOntoBreakpoint(thread, status);
        }

        return true;
    }

    void Debugger::reportLeaderExit(const int exitCode)
    {
        exitProcessEvent(mMainPid.load(std::memory_order_relaxed), exitCode);
        mIsRunning.store(false, std::memory_order_release);
    }

    std::optional<int> Debugger::stopAllThreads(const pid_t except)
    {
        if(!mProcess)
            return std::nullopt;

        const pid_t tgid = mMainPid.load(std::memory_order_relaxed);
        if(tgid <= 0)
            return std::nullopt;

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
                return std::nullopt;

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
                    const auto stopped = mProcess->threads.find(tid);
                    if(stopped != mProcess->threads.end())
                        stopped->second->setRunning(false);
                };

                bool alreadyOwed = false;
                {
                    const std::string reason = readWaitReason(tgid, tid);
                    std::unique_lock lock(mProcessMutex);
                    if(!thread->isSuspended())
                        thread->setWaitReason(reason);
                    alreadyOwed = thread->pendingSigstop();
                }
                if(!alreadyOwed && tgkill(tgid, tid, SIGSTOP) == -1)
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
                    const int code = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
                    if(tid == tgid)
                        return code;
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
                        {
                            if(errno != ESRCH)
                                cbInternalError("PTRACE_GETEVENTMSG failed: " + std::string(strerror(errno)));
                        }
                        else
                            createThreadEvent(static_cast<pid_t>(newTid));
                    }
                    else if(event == PTRACE_EVENT_EXEC && tid == tgid)
                    {
                        (void)applyExec(tid);
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

    void Debugger::continueUnlessSuspended(const pid_t tid)
    {
        bool leftStopped = false;
        int contError = 0;
        {
            std::unique_lock lock(mProcessMutex);
            if(mThread && mThread->isSuspended())
                leftStopped = true;
            else if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
                contError = errno;
            else if(mThread)
                mThread->setRunning(true);
        }
        if(contError != 0 && contError != ESRCH)
            cbInternalError("PTRACE_CONT failed: " + std::string(strerror(contError)));

        if(leftStopped)
            leaveParked(tid);
    }

    void Debugger::leaveParked(const pid_t tid)
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
        pauseAndResume(tid);
    }

    void Debugger::handleSignal(const pid_t tid, const int status)
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
                const auto it = mProcess->threads.find(tid);
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
                    ptrace(PTRACE_GETSIGINFO, tid, nullptr, &info) != -1 && info.si_code <= 0)
                reportSignal(tid, sig);
            else
                handleSigtrap(tid, status);
            break;
        }

        case SIGSTOP:
        {
            {
                bool requestedStop = false;
                bool resumedMeanwhile = false;
                {
                    std::lock_guard pauseLock(mPauseMutex);
                    requestedStop = mPendingSuspend.erase(tid) != 0;
                    if(requestedStop && mThread && !mThread->isSuspended())
                    {
                        mPendingResume.erase(tid);
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
                            pauseAndResume(tid);
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
                    continueUnlessSuspended(tid);
                    break;
                }
            }
            if(mPauseRequested.load(std::memory_order_acquire))
            {
                mPauseRequested.store(false, std::memory_order_release);
                if(mThread)
                {
                    mThread->registers.Read();
                    abandonSingleStep(tid);
                    cancelStepOverIfOwner(tid);
                    if(const auto leaderExit = stopAllThreads(tid))
                    {
                        reportLeaderExit(*leaderExit);
                        return;
                    }
                    beginPause();
                    cbPaused();

                    if(!pauseAndResume(tid))
                        break;
                }
                else
                {
                    if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
                    {
                        if(errno != ESRCH)
                            cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                    }
                    else
                        mUnregisteredRunning.insert(tid);
                }
            }
            else if(mThread && mThread->isSingleStepping())
            {
                // Not our SIGSTOP: the step is still owed, so re-issue it.
                if(mThread->stepInto())
                {
                    mThread->setRunning(true);
                }
                else
                {
                    abandonSingleStep(tid);
                    if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
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
            else if(!suspended && (!mAllStopped || wasRunning || tid == mSteppingOff || !mThread))
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
                    else if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
                        contError = errno;
                    else if(mThread)
                        mThread->setRunning(true);
                    else
                        mUnregisteredRunning.insert(tid);
                }
                if(contError != 0 && contError != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(contError)));

                if(leftStopped && !anyRunning)
                {
                    mPauseRequested.store(false, std::memory_order_release);
                    beginPause();
                    cbPaused();
                    pauseAndResume(tid);
                }
            }
            break;
        }

        default:
            reportSignal(tid, sig);
            break;
        }
    }

    void Debugger::reportSignal(const pid_t tid, const int sig)
    {
        ptr faultAddr = 0;
        siginfo_t sigInfo;
        if(ptrace(PTRACE_GETSIGINFO, tid, nullptr, &sigInfo) != -1)
            faultAddr = FaultAddress(sig, sigInfo);
        if(mThread)
        {
            mThread->registers.Read();
            mPendingSignal = sig;
            abandonSingleStep(tid);
            cancelStepOverIfOwner(tid);
            if(const auto leaderExit = stopAllThreads(tid))
            {
                reportLeaderExit(*leaderExit);
                return;
            }
            beginPause();
            cbException(sig, faultAddr);
            pauseAndResume(tid);
        }
        else
        {
            cbException(sig, faultAddr);
            if(ptrace(PTRACE_CONT, tid, nullptr,
                      reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
            {
                if(errno != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            }
        }
    }

    Debugger::StepOff Debugger::stepPastBreakpointByte(const pid_t tid, const ptr addr)
    {
        if(!mProcess || !mThread)
            return StepOff::Consumed;

        const ScopedSteppingOff scopedSteppingOff(mSteppingOff, tid);

        ptr next = 0;
        const bool stepsPushf = mProcess->ClassifyStepOverAt(addr, next) == StepOverKind::Pushf;

        // Only re-arm what this call lifted; another thread may hold the byte for its own step.
        const bool lifted = mProcess->DisarmBreakpointByte(addr);

        // The pending signal rides the step; otherwise the faulting instruction is retried.
        const int sig = mPendingSignal;
        mPendingSignal = 0;

        if(!swallowPendingSigstop(tid))
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
            else if(!mThread->stepInto(sig))
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
            if(ptrace(PTRACE_CONT, tid, nullptr,
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
            waited = waitpid(tid, &stepStatus, __WALL);
        }
        while(waited == -1 && errno == EINTR);

        if(waited == -1)
        {
            cbInternalError("waitpid(step) failed: " + std::string(strerror(errno)));
            if(lifted)
                mProcess->RearmBreakpointByte(addr);
            if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
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
            if(tid == mMainPid.load(std::memory_order_relaxed))
            {
                exitProcessEvent(tid, code);
                mIsRunning.store(false, std::memory_order_release);
            }
            else
            {
                exitThreadEvent(tid);
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
                (void)applyExec(tid);
                if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
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
                if(ptrace(PTRACE_GETEVENTMSG, tid, nullptr, &newTid) == -1)
                {
                    if(errno != ESRCH)
                        cbInternalError("PTRACE_GETEVENTMSG failed: " + std::string(strerror(errno)));
                }
                else
                    createThreadEvent(static_cast<pid_t>(newTid));
            }

            if(stepSig != SIGTRAP)
            {
                // Not the step's trap: report it like any other stop. The byte is re-armed,
                // so the breakpoint fires again when a handler returns to the instruction.
                handleSignal(tid, stepStatus);
                return StepOff::Consumed;
            }

            if(stepsPushf)
                maskPushedTrapFlag();
        }

        return StepOff::Stepped;
    }

    void Debugger::abandonSingleStep(const pid_t tid)
    {
        if(mThread)
            mThread->clearSingleStep();

        restoreSourceByte(tid);
    }

    void Debugger::replaceExecedThread(const pid_t formerTid, const pid_t tid)
    {
        bool known = false;
        {
            std::shared_lock lock(mProcessMutex);
            known = mProcess && mProcess->threads.count(formerTid) > 0;
        }
        if(known)
            exitThreadEvent(formerTid);

        // The leader's record, its suspend count above all, describes a dead thread.
        std::unique_lock lock(mProcessMutex);
        if(!mProcess)
            return;
        const auto it = mProcess->threads.find(tid);
        if(it == mProcess->threads.end())
            return;
        it->second = std::make_unique<Thread>(tid);
        mThread = it->second.get();
    }

    bool Debugger::applyExec(const pid_t tid)
    {
        unsigned long formerTid = 0;
        if(ptrace(PTRACE_GETEVENTMSG, tid, nullptr, &formerTid) == -1)
        {
            if(errno != ESRCH)
                cbInternalError("PTRACE_GETEVENTMSG failed: " + std::string(strerror(errno)));
            formerTid = 0;
        }

        onExec();

        if(formerTid != 0 && static_cast<pid_t>(formerTid) != tid)
            replaceExecedThread(static_cast<pid_t>(formerTid), tid);

        const Arch arch = DetectArchFromProcExe(tid);
        if(arch != Arch::X86_64)
        {
            // Neither the decoder nor the stack arithmetic is 32-bit aware.
            cbInternalError("cannot follow execve: " + ArchRejectMessage(arch));
            mDetachRequested.store(true, std::memory_order_release);
            return false;
        }

        if(mProcess)
        {
            {
                std::unique_lock lock(mProcessMutex);
                mProcess->arch = arch;
            }
            mProcess->ReseatBreakpointsAfterExec();
        }

        cbExec();
        return true;
    }

    void Debugger::handleExecEvent(const pid_t tid)
    {
        if(tid != mMainPid.load(std::memory_order_relaxed))
        {
            if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1 && errno != ESRCH)
                cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            return;
        }

        if(!applyExec(tid))
        {
            if(const auto leaderExit = stopAllThreads(tid))
            {
                reportLeaderExit(*leaderExit);
                return;
            }
            beginPause();
            pauseAndResume(tid);
            return;
        }

        continueUnlessSuspended(tid);
    }

    void Debugger::handleSigtrap(const pid_t tid, const int status)
    {
        const int event = (status >> 16) & 0xffff;

        switch(event)
        {
        case PTRACE_EVENT_EXEC:
        {
            handleExecEvent(tid);
            break;
        }

        case PTRACE_EVENT_CLONE:
        {
            const bool wasStepping = mThread && mThread->isSingleStepping();
            abandonSingleStep(tid);

            unsigned long newTid = 0;
            if(ptrace(PTRACE_GETEVENTMSG, tid, nullptr, &newTid) == -1)
            {
                if(errno != ESRCH)
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
                if(const auto leaderExit = stopAllThreads(tid))
                {
                    reportLeaderExit(*leaderExit);
                    return;
                }
                beginPause();
                cbStep();
                pauseAndResume(tid);
                break;
            }

            continueUnlessSuspended(tid);
            break;
        }

        case PTRACE_EVENT_EXIT:
        {
            abandonSingleStep(tid);
            // Notification only; exit is emitted via WIFEXITED/WIFSIGNALED in debugLoop.
            // Left marked stopped: it owes no stop, so a sweep would block in waitpid.
            if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
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
                mUnregisteredRunning.erase(tid);
                createThreadEvent(tid);
                {
                    std::shared_lock lock(mProcessMutex);
                    if(mProcess)
                    {
                        const auto it = mProcess->threads.find(tid);
                        if(it != mProcess->threads.end())
                            mThread = it->second.get();
                    }
                }
            }
            if(!mThread)
            {
                if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
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
                restoreSourceByte(tid);
                if(const auto leaderExit = stopAllThreads(tid))
                {
                    reportLeaderExit(*leaderExit);
                    return;
                }
                beginPause();
                cbStep();
                if(!pauseAndResume(tid))
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

                    const bool rightThread = (tid == mStepOver.tid);
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

                        restoreSourceByte(tid);

                        if(const auto leaderExit = stopAllThreads(tid))
                        {
                            reportLeaderExit(*leaderExit);
                            return;
                        }
                        beginPause();
                        // Not ours: the user's own breakpoint fired at the target.
                        if(!planted)
                            dispatchBreakpoint(target);
                        cbStep();
                        pauseAndResume(tid);
                        break;
                    }

                    // Wrong thread or frame: skip our own trap, report a user breakpoint.
                    if(mStepOver.planted)
                    {
                        switch(stepPastBreakpointByte(tid, bpAddr))
                        {
                        case StepOff::Stepped:
                            continueUnlessSuspended(tid);
                            break;
                        case StepOff::Parked:
                            leaveParked(tid);
                            break;
                        case StepOff::Consumed:
                            cancelStepOver(tid);
                            // An armed run is still inside a frozen window; this is the last
                            // chance to lift it.
                            abandonFreeze(tid);
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

                    cancelStepOverIfOwner(tid);
                    if(const auto leaderExit = stopAllThreads(tid))
                    {
                        reportLeaderExit(*leaderExit);
                        return;
                    }
                    beginPause();
                    dispatchBreakpoint(bpAddr);
                    // The 0xCC stays armed with RIP on it; the next resume steps past it.
                    pauseAndResume(tid);
                    break;
                }

                if(stepOverHit)
                    cancelStepOver(tid);
            }

            restoreSourceByte(tid);

            continueUnlessSuspended(tid);
            break;
        }
        }
    }
}
