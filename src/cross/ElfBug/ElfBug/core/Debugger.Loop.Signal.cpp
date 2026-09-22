#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/ProcessArch.h>
#include <ElfBug/process/ProcessList.h>
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

    bool ForeignTrapShouldQueue(const int signal, const siginfo_t & info)
    {
        if(signal != SIGTRAP || info.si_code <= 0)
            return false;
        return info.si_code == SI_KERNEL || info.si_code == TRAP_BRKPT;
    }

    WaitResult WaitForStop(const pid_t tid, int & status)
    {
        const auto deadline = std::chrono::steady_clock::now() + kStopWaitTimeout;
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
            std::this_thread::sleep_for(kPollInterval);
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

    bool Debugger::claimBreakpointTrap(Thread* thread, const int status)
    {
        if(!mProcess || !thread || WSTOPSIG(status) != SIGTRAP)
            return false;
        // A ptrace event, not an int3 trap.
        if(((status >> 16) & 0xffff) != 0)
            return false;
        if(thread->IsSingleStepping())
            return false;

        const ptr bpAddr = thread->registers.Gip() - 1;
        if(!mProcess->HasBreakpoint(bpAddr))
            return false;

        // A thread killed by exit_group refuses the write, but the trap is still ours.
        thread->registers.Gip() = bpAddr;
        if(!thread->registers.Write())
        {
            const bool dying = errno == ESRCH;
            thread->registers.Gip() = bpAddr + 1;
            return dying;
        }
        thread->SetAtBreakpoint(true);
        thread->SetPendingBreakpoint(bpAddr);
        return true;
    }

    void Debugger::repairStoppedThread(Thread* thread, const int status)
    {
        if(!thread)
            return;

        thread->registers.Read();

        siginfo_t info{};
        const bool haveInfo = ptrace(PTRACE_GETSIGINFO, thread->tid, nullptr, &info) != -1;

        const bool ours = claimBreakpointTrap(thread, status);

        int signal = 0;
        ptr address = 0;
        if(StopShouldQueue(status, haveInfo, info, signal, address))
            thread->SetPendingSignal(signal, address, true);
        else if(!ours && !thread->IsSingleStepping() && haveInfo &&
                ((status >> 16) & 0xffff) == 0 &&
                ForeignTrapShouldQueue(WSTOPSIG(status), info))
            thread->SetPendingSignal(SIGTRAP, 0, true);
    }

    void Debugger::registerClone(const pid_t parent)
    {
        unsigned long newTid = 0;
        if(ptrace(PTRACE_GETEVENTMSG, parent, nullptr, &newTid) == -1)
        {
            if(errno != ESRCH)
                cbInternalError("PTRACE_GETEVENTMSG failed: " + std::string(strerror(errno)));
            return;
        }
        const pid_t child = static_cast<pid_t>(newTid);
        createThreadEvent(child, ThreadGroupId(child), true);
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
        if(!thread || !thread->PendingSigstop())
            return true;

        // Restarting a thread that is not in ptrace-stop fails. Never clear the flag on a path
        // that did not see the signal: the caller reads it to decide on the SIGCONT.
        bool inPtraceStop = false;
        {
            std::shared_lock lock(mProcessMutex);
            inPtraceStop = !thread->IsRunning();
        }

        constexpr int kDrainAttempts = 32;
        for(int attempt = 0; attempt < kDrainAttempts; attempt++)
        {
            if(inPtraceStop && ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
            {
                if(errno != ESRCH)
                    return true;
                inPtraceStop = false;
            }

            int status = 0;
            const WaitResult waited = WaitForStop(tid, status);
            if(waited == WaitResult::TimedOut)
            {
                std::unique_lock lock(mProcessMutex);
                thread->SetRunning(true);
                return false;
            }
            if(waited != WaitResult::Stopped)
                return false;

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
                std::unique_lock lock(mProcessMutex);
                thread->SetPendingSigstop(false);
                thread->SetRunning(false);
                return true;
            }

            const int event = (status >> 16) & 0xffff;
            if(event == PTRACE_EVENT_CLONE)
            {
                registerClone(tid);
                continue;
            }
            if(event == PTRACE_EVENT_EXEC)
            {
                const bool followed = applyExec(tid);
                Thread* current = nullptr;
                {
                    std::shared_lock lock(mProcessMutex);
                    if(mProcess)
                    {
                        const auto it = mProcess->threads.find(tid);
                        if(it != mProcess->threads.end())
                            current = it->second.get();
                    }
                }
                // A sibling's exec replaces the leader record, and the owed SIGSTOP dies with the old leader.
                const bool replaced = current != thread;
                if(!followed)
                {
                    if(replaced)
                        tgkill(mMainPid.load(std::memory_order_relaxed), tid, SIGSTOP);
                    mPauseRequested.store(true, std::memory_order_release);
                    if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) != -1 && current)
                    {
                        std::unique_lock lock(mProcessMutex);
                        current->SetPendingSigstop(false);
                        current->SetRunning(true);
                    }
                    return false;
                }
                if(replaced)
                    return true;
                continue;
            }
            if(event != 0)
                continue;

            bool ours = false;
            if(sig == SIGTRAP && thread->registers.Read())
                ours = claimBreakpointTrap(thread, status);

            if(thread->PendingSignal() == 0)
            {
                siginfo_t info{};
                const bool haveInfo = ptrace(PTRACE_GETSIGINFO, tid, nullptr, &info) != -1;
                int signal = 0;
                ptr address = 0;
                if(StopShouldQueue(status, haveInfo, info, signal, address))
                    thread->SetPendingSignal(signal, address, false);
                else if(!ours && ((status >> 16) & 0xffff) == 0 && haveInfo &&
                        ForeignTrapShouldQueue(sig, info))
                    thread->SetPendingSignal(SIGTRAP, 0, false);
            }
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
                    if(tid != except && thread->IsRunning())
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
                        stopped->second->SetRunning(false);
                };

                bool alreadyOwed = false;
                {
                    const std::string reason = readWaitReason(tgid, tid);
                    std::unique_lock lock(mProcessMutex);
                    if(!thread->IsSuspended())
                        thread->SetWaitReason(reason);
                    alreadyOwed = thread->PendingSigstop();
                }
                if(!alreadyOwed && tgkill(tgid, tid, SIGSTOP) == -1)
                {
                    if(errno != ESRCH)
                        cbInternalError("tgkill() failed: " + std::string(strerror(errno)));
                    markStopped();
                    continue;
                }

                int status = 0;
                const WaitResult waited = WaitForStop(tid, status);
                if(waited == WaitResult::TimedOut)
                {
                    cbInternalError("thread " + std::to_string(tid) +
                                    " did not stop; it may be stuck in uninterruptible I/O");
                    {
                        std::unique_lock lock(mProcessMutex);
                        thread->SetRunning(false);
                        thread->SetPendingSigstop(true);
                    }
                    continue;
                }
                if(waited == WaitResult::Gone)
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
                        registerClone(tid);
                    else if(event == PTRACE_EVENT_EXEC && tid == tgid)
                    {
                        (void)applyExec(tid);
                        std::unique_lock lock(mProcessMutex);
                        const auto execed = mProcess->threads.find(tid);
                        if(execed != mProcess->threads.end())
                        {
                            execed->second->SetRunning(false);
                            execed->second->SetPendingSigstop(execed->second.get() == thread);
                        }
                        continue;
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
                    else
                    {
                        std::unique_lock lock(mProcessMutex);
                        const auto owed = mProcess->threads.find(tid);
                        if(owed != mProcess->threads.end())
                            owed->second->SetPendingSigstop(true);
                    }
                    continue;
                }

                const bool ownReason = WSTOPSIG(status) != SIGSTOP;
                {
                    std::unique_lock lock(mProcessMutex);
                    thread->SetRunning(false);
                    if(ownReason && !thread->IsSuspended())
                        thread->SetWaitReason({});
                    thread->SetPendingSigstop(ownReason);
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

    bool Debugger::pendingResumeOnBreakpoint()
    {
        std::lock_guard pauseLock(mPauseMutex);
        std::shared_lock lock(mProcessMutex);
        if(!mProcess)
            return false;
        for(const pid_t tid : mPendingResume)
        {
            const auto it = mProcess->threads.find(tid);
            if(it == mProcess->threads.end())
                continue;
            Thread & thread = *it->second;
            if(!thread.IsSuspended() && !thread.IsRunning() && thread.AtBreakpoint() &&
                    mProcess->HasBreakpoint(thread.registers.Gip()))
                return true;
        }
        return false;
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
            if(!thread || thread->IsSuspended() || thread->IsRunning())
                return true;
        }

        if(thread->registers.Read() && thread->AtBreakpoint() &&
                mProcess->HasBreakpoint(thread->registers.Gip()))
        {
            const ptr rip = thread->registers.Gip();
            Thread* previous = nullptr;
            {
                std::unique_lock lock(mProcessMutex);
                previous = mThread;
                mThread = thread;
            }

            mPendingSignal = thread->PendingSignal();
            thread->ClearPendingSignal();

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
            if(!thread || thread->IsSuspended() || thread->IsRunning())
                return true;

            const int sig = thread->PendingSignal();
            thread->ClearPendingSignal();

            if(ptrace(PTRACE_CONT, tid, nullptr,
                      reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
                contError = errno;
            else
                thread->SetRunning(true);
        }
        if(contError != 0 && contError != ESRCH)
            cbInternalError("PTRACE_CONT failed: " + std::string(strerror(contError)));
        return true;
    }

    void Debugger::continueUnlessSuspended(const pid_t tid)
    {
        const ContinueResult continued = continueOrPark(tid, 0, false);
        if(continued == ContinueResult::Parked || continued == ContinueResult::ParkedAlone)
            leaveParked(tid);
    }

    void Debugger::leaveParked(const pid_t tid)
    {
        {
            std::shared_lock lock(mProcessMutex);
            if(!mProcess || !mThread)
                return;
            if(anyThreadRunningLocked())
                return;
        }

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

        if(sig == SIGSTOP && mProcess)
        {
            bool known = false;
            {
                std::shared_lock lock(mProcessMutex);
                known = mProcess->threads.count(tid) > 0;
            }
            // A new thread's first stop can beat the clone event that announces it.
            const pid_t tgid = known ? 0 : ThreadGroupId(tid);
            if(!known && tgid == mMainPid.load(std::memory_order_relaxed))
                createThreadEvent(tid, tgid, false);
        }

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
                wasRunning = mThread->IsRunning();
                suspended = mThread->IsSuspended();
                mThread->SetRunning(false);
                if(sig == SIGSTOP)
                    mThread->SetPendingSigstop(false);
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
                    if(requestedStop && mThread && !mThread->IsSuspended())
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
                        if(!anyThreadRunning())
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
                    if(pendingResumeOnBreakpoint())
                    {
                        {
                            std::lock_guard pauseLock(mPauseMutex);
                            mPendingResume.clear();
                        }
                        if(const auto leaderExit = stopAllThreads(tid))
                        {
                            reportLeaderExit(*leaderExit);
                            return;
                        }
                        pauseAndResume(tid);
                        break;
                    }
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
            else if(mThread && mThread->IsSingleStepping())
            {
                // Not our SIGSTOP: the step is still owed, so re-issue it.
                if(mThread->StepInto())
                {
                    mThread->SetRunning(true);
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
                        mThread->SetRunning(true);
                }
            }

            else if(!suspended && (!mAllStopped || wasRunning || tid == mSteppingOff || !mThread))
            {
                const ContinueResult continued = continueOrPark(tid, 0, false);
                if(continued == ContinueResult::ContinuedUntracked)
                    mUnregisteredRunning.insert(tid);
                else if(continued == ContinueResult::ParkedAlone)
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
        bool groupStop = false;
        siginfo_t sigInfo;
        errno = 0;
        if(ptrace(PTRACE_GETSIGINFO, tid, nullptr, &sigInfo) != -1)
            faultAddr = FaultAddress(sig, sigInfo);
        else
            groupStop = errno == EINVAL;

        const int deliver = groupStop ? 0 : sig;
        if(mThread)
        {
            mThread->registers.Read();
            mPendingSignal = deliver;
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
                      reinterpret_cast<void*>(static_cast<uintptr_t>(deliver))) == -1)
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
        const bool lifted = mProcess->DisarmBreakpointByte(addr);

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
            if(mThread->IsSuspended())
            {
                leftStopped = true;
                if(sig != 0)
                    mThread->SetPendingSignal(sig, 0, false);
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
            if(ptrace(PTRACE_CONT, tid, nullptr,
                      reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
            {
                if(errno != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            }
            else
            {
                mThread->SetRunning(true);
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
                mThread->SetRunning(true);
            }
            return StepOff::Consumed;
        }

        mThread->ClearSingleStep();

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
                if(!applyExec(tid))
                {
                    parkForRejectedExec(tid);
                    return StepOff::Consumed;
                }
                if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
                {
                    if(errno != ESRCH)
                        cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                }
                else
                {
                    mThread->SetRunning(true);
                }
                return StepOff::Consumed;
            }

            if(lifted)
                mProcess->RearmBreakpointByte(addr);

            if(stepEvent == PTRACE_EVENT_CLONE)
                registerClone(tid);

            if(stepSig != SIGTRAP)
            {
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
            mThread->ClearSingleStep();

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

        const ImageId execedImage = ReadImageIdFromProcExe(tid);
        const bool sameImage = mImageId.Known() && execedImage.Known() && mImageId == execedImage;
        mImageId = execedImage;

        if(mProcess)
        {
            // TODO: drop once breakpoints are stored module-relative and re-resolved on module load events.
            if(sameImage && !AddressesRandomized(tid))
                mProcess->ReseatBreakpointsAfterExec();
            else
            {
                mProcess->ForgetBreakpointsAfterExec();
                cbDebugString(sameImage ? "execve reloaded the image at a randomized base, breakpoints did not carry over"
                                        : "execve replaced the image, breakpoints did not carry over");
            }
        }

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
            std::unique_lock lock(mProcessMutex);
            mProcess->arch = arch;
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
            parkForRejectedExec(tid);
            return;
        }

        continueUnlessSuspended(tid);
    }

    void Debugger::parkForRejectedExec(const pid_t tid)
    {
        if(const auto leaderExit = stopAllThreads(tid))
        {
            reportLeaderExit(*leaderExit);
            return;
        }
        beginPause();
        pauseAndResume(tid);
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
            const bool wasStepping = mThread && mThread->IsSingleStepping();
            abandonSingleStep(tid);
            registerClone(tid);

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
                mUnregisteredRunning.erase(tid);
                createThreadEvent(tid, ThreadGroupId(tid), false);
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

            if(mThread->IsSingleStepping())
            {
                const bool stepsPushf = mThread->StepsPushf();
                mThread->ClearSingleStep();
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
                    mThread->ClearSingleStep();
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
                        mThread->SetAtBreakpoint(!planted);

                        restoreSourceByte(tid);

                        if(const auto leaderExit = stopAllThreads(tid))
                        {
                            reportLeaderExit(*leaderExit);
                            return;
                        }
                        beginPause();
                        if(!planted)
                            dispatchBreakpoint(target);
                        cbStep();
                        pauseAndResume(tid);
                        break;
                    }

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
                            abandonAllStop(tid);
                            break;
                        }
                        break;
                    }
                }

                if(mProcess->HasBreakpoint(bpAddr))
                {
                    mThread->registers.Gip() = bpAddr;
                    mThread->registers.Write();
                    mThread->SetAtBreakpoint(true);

                    cancelStepOverIfOwner(tid);
                    if(const auto leaderExit = stopAllThreads(tid))
                    {
                        reportLeaderExit(*leaderExit);
                        return;
                    }
                    beginPause();
                    dispatchBreakpoint(bpAddr);
                    pauseAndResume(tid);
                    break;
                }

                if(stepOverHit)
                    cancelStepOver(tid);
            }

            restoreSourceByte(tid);

            if(mProcess && !stepOverHit)
            {
                siginfo_t trapInfo{};
                if(ptrace(PTRACE_GETSIGINFO, tid, nullptr, &trapInfo) != -1 &&
                        ForeignTrapShouldQueue(SIGTRAP, trapInfo))
                {
                    reportSignal(tid, SIGTRAP);
                    break;
                }
            }

            continueUnlessSuspended(tid);
            break;
        }
        }
    }
}
