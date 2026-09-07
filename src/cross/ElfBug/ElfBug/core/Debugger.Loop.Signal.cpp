#include <ElfBug/core/Debugger.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#include <csignal>

namespace ElfBug
{
    void Debugger::handleSignal(const pid_t pid, const int status)
    {
        const int sig = WSTOPSIG(status);
        mPendingSignal = 0;

        {
            std::unique_lock lock(mProcessMutex);
            mThread = nullptr;
            if(mProcess)
            {
                const auto it = mProcess->threads.find(pid);
                if(it != mProcess->threads.end())
                    mThread = it->second.get();
            }
        }

        switch(sig)
        {
        case SIGTRAP:
            handleSigtrap(pid, status);
            break;

        case SIGSTOP:
        {
            if(mPauseRequested.load(std::memory_order_acquire))
            {
                mPauseRequested.store(false, std::memory_order_release);
                // TODO: all-stop mode - send tgkill(mMainPid, tid, SIGSTOP)
                // to every other thread and waitpid each
                if(mThread)
                {
                    mThread->registers.Read();
                    cancelStepOver(pid);
                    beginPause();
                    cbPaused();

                    if(!pauseAndResume(pid))
                        break;
                }
                else
                {
                    if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                        cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                }
            }
            else
            {
                if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            }
            break;
        }

        default:
        {
            ptr faultAddr = 0;
            siginfo_t sigInfo;
            if(ptrace(PTRACE_GETSIGINFO, pid, nullptr, &sigInfo) != -1)
                faultAddr = reinterpret_cast<ptr>(sigInfo.si_addr);
            if(mThread)
            {
                mThread->registers.Read();
                mPendingSignal = sig;
                cancelStepOver(pid);
                beginPause();
                cbExceptionEvent(sig, faultAddr);
                if(!pauseAndResume(pid))
                    break;
            }
            else
            {
                cbExceptionEvent(sig, faultAddr);
                if(ptrace(PTRACE_CONT, pid, nullptr,
                          reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            }
            break;
        }
        }
    }

    bool Debugger::stepPastBreakpointByte(const pid_t pid, const ptr addr)
    {
        if(!mProcess || !mThread)
            return false;

        mProcess->DisarmBreakpointByte(addr);

        if(!mThread->StepInto())
        {
            const int stepErrno = errno;
            if(stepErrno != ESRCH)
                cbInternalError("PTRACE_SINGLESTEP failed: " + std::string(strerror(stepErrno)));
            mProcess->RearmBreakpointByte(addr);
            if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
            {
                if(errno != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            }
            return false;
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
            mProcess->RearmBreakpointByte(addr);
            if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
            {
                if(errno != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            }
            return false;
        }

        mThread->clearSingleStep();

        if(WIFEXITED(stepStatus) || WIFSIGNALED(stepStatus))
        {
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
            return false;
        }

        if(WIFSTOPPED(stepStatus))
        {
            const int stepSig = WSTOPSIG(stepStatus);
            const int stepEvent = (stepStatus >> 16) & 0xffff;
            mThread->registers.Read();

            if(stepEvent == PTRACE_EVENT_EXEC)
            {
                // The image is gone; re-arming would write into the new one.
                discardStepStateAfterExec(pid);
                if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                {
                    if(errno != ESRCH)
                        cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                }
                return false;
            }

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
                if(ptrace(PTRACE_CONT, pid, nullptr,
                          reinterpret_cast<void*>(static_cast<uintptr_t>(stepSig))) == -1)
                {
                    if(errno != ESRCH)
                        cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                }
                return false;
            }
        }

        return true;
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
            discardStepStateAfterExec(pid);
            // TODO: re-exec handling - re-detect arch and reject if no longer x86_64, clear breakpoints, refresh memory map, fire callback
            if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
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
                beginPause();
                cbStep();
                pauseAndResume(pid);
                break;
            }

            if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            break;
        }

        case PTRACE_EVENT_EXIT:
        {
            abandonSingleStep(pid);
            // Notification only; exit is emitted via WIFEXITED/WIFSIGNALED in debugLoop.
            if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            break;
        }

        default:
        {
            if(!mThread)
            {
                if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                break;
            }

            mThread->registers.Read();

            const ptr bpAddr = mThread->registers.Gip() - 1;
            const bool stepOverHit = mProcess && mStepOver.active && bpAddr == mStepOver.target;

            if(mThread->isSingleStepping() && !stepOverHit)
            {
                mThread->clearSingleStep();
                restoreSourceByte(pid);
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
                    if(mStepOver.frameGuard && rsp == mStepOver.rspFloor - 8)
                    {
                        ptr pushed = 0;
                        completedWithoutReturn = mProcess->MemRead(rsp, &pushed, sizeof(pushed)) &&
                                                 pushed == mStepOver.target;
                    }
                    const bool rightFrame  = !mStepOver.frameGuard ||
                                             rsp >= mStepOver.rspFloor ||
                                             completedWithoutReturn;

                    if(rightThread && rightFrame)
                    {
                        const bool planted = mStepOver.planted;
                        const ptr target = mStepOver.target;
                        mStepOver = {};

                        if(planted)
                            mProcess->DeleteBreakpoint(target);

                        restoreSourceByte(pid);

                        beginPause();
                        cbStep();
                        pauseAndResume(pid);
                        break;
                    }

                    // Wrong thread or frame: skip our own trap, report a user breakpoint.
                    if(mStepOver.planted)
                    {
                        if(!stepPastBreakpointByte(pid, bpAddr))
                        {
                            cancelStepOver(pid);
                            break;
                        }
                        if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                        {
                            if(errno != ESRCH)
                                cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                        }
                        break;
                    }
                }

                const BreakpointKey key{BreakpointType::Software, bpAddr};
                const auto it = mProcess->breakpoints.find(key);

                if(it != mProcess->breakpoints.end())
                {
                    mThread->registers.Gip() = bpAddr;
                    mThread->registers.Write();

                    const auto & info = it->second;
                    const bool singleshot = info.singleshot;

                    cancelStepOver(pid);
                    beginPause();

                    const auto cbIt = mProcess->breakpointCallbacks.find(key);
                    if(cbIt != mProcess->breakpointCallbacks.end())
                        cbIt->second(info);

                    cbBreakpoint(info);

                    if(singleshot)
                    {
                        mProcess->DeleteBreakpoint(bpAddr);
                    }
                    // The 0xCC stays armed with RIP on it; the next resume steps past it.
                    pauseAndResume(pid);
                    break;
                }

                if(stepOverHit)
                    cancelStepOver(pid);
            }

            restoreSourceByte(pid);

            if(ptrace(PTRACE_CONT, pid, nullptr, nullptr) == -1)
                cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
            break;
        }
        }
    }
}
