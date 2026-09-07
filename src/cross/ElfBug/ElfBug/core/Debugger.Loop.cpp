#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/ProcessArch.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <chrono>

namespace ElfBug
{
    // Publishes mPaused=true under the mutex before the event callback fires,
    // so a concurrent Continue()/Stop() can't race past and strand pauseAndResume().
    void Debugger::beginPause()
    {
        std::lock_guard lock(mPauseMutex);
        mPaused.store(true, std::memory_order_release);
    }

    bool Debugger::pauseAndResume(const pid_t pid)
    {
        std::unique_lock lock(mPauseMutex);

        while(mPaused.load(std::memory_order_acquire) && mIsRunning.load(std::memory_order_acquire))
        {
            if(mPauseCv.wait_for(lock, std::chrono::milliseconds(10)) == std::cv_status::timeout)
                cbPauseTick();
        }

        lock.unlock();

        if(!mIsRunning.load(std::memory_order_acquire))
            return false;

        bool stepOverRequested = mStepOverPending.exchange(false, std::memory_order_acq_rel);

        // A breakpoint hit leaves its 0xCC armed with RIP on it; every resume except a
        // step-over must step past that byte first.
        const bool stepIntoRequested = mStepPending.load(std::memory_order_acquire);

        // pushf would observe the single-step TF; use the step-over path instead.
        if(stepIntoRequested && !stepOverRequested && mThread && mProcess)
        {
            ptr next = 0;
            if(mProcess->ClassifyStepOverAt(mThread->registers.Gip(), next) == StepOverKind::Pushf)
                stepOverRequested = true;
        }

        if(!stepOverRequested && mThread && mProcess &&
                mProcess->HasBreakpoint(mThread->registers.Gip()))
        {
            const ptr rip = mThread->registers.Gip();
            if(stepIntoRequested)
            {
                // stepPastBreakpointByte would consume the user's step; just lift the byte.
                if(mProcess->DisarmBreakpointByte(rip))
                    mSourceRearms[pid] = rip;
            }
            else if(!stepPastBreakpointByte(pid, rip))
            {
                return false;
            }
        }

        if(stepOverRequested && mThread && armStepOver(pid))
        {
            // A StepInto queued just before this StepOver is subsumed by it.
            mStepPending.store(false, std::memory_order_release);

            const int sig = mPendingSignal;
            mPendingSignal = 0;
            if(ptrace(PTRACE_CONT, pid, nullptr,
                      reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
            {
                if(errno != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
                cancelStepOver(pid);
            }
            return true;
        }

        if((stepIntoRequested || stepOverRequested) && mThread)
        {
            mStepPending.store(false, std::memory_order_release);
            const int sig = mPendingSignal;
            mPendingSignal = 0;
            if(!mThread->StepInto(sig))
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
                }
            }
        }
        else
        {
            const int sig = mPendingSignal;
            mPendingSignal = 0;
            if(ptrace(PTRACE_CONT, pid, nullptr,
                      reinterpret_cast<void*>(static_cast<uintptr_t>(sig))) == -1)
            {
                if(errno != ESRCH)
                    cbInternalError("PTRACE_CONT failed: " + std::string(strerror(errno)));
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
