#include <ElfBug/core/Debugger.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>

namespace ElfBug
{
    Debugger::Debugger() = default;

    Debugger::~Debugger()
    {
        const pid_t pid = mMainPid.load(std::memory_order_acquire);
        if(pid > 0)
        {
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, __WALL);
        }
    }

    bool Debugger::Init(const char* szFilePath, const char* const* argv, const char* szCurrentDirectory)
    {
        mHasLaunchArgs = false;
        mFilePath.clear();
        mArgv.clear();
        mCwd.clear();

        mPaused.store(false, std::memory_order_release);
        mStepPending.store(false, std::memory_order_release);
        mStepOverPending.store(false, std::memory_order_release);
        mStepOver = {};
        mSourceRearms.clear();
        mUnregisteredRunning.clear();
        mAllStopped = false;
        mPauseRequested.store(false, std::memory_order_release);
        mStopRequested.store(false, std::memory_order_release);
        mPendingSignal = 0;

        if(!szFilePath)
            return false;

        if(!szCurrentDirectory && access(szFilePath, X_OK) != 0)
        {
            cbInternalError("cannot execute '" + std::string(szFilePath) + "': " + std::string(strerror(errno)));
            return false;
        }

        mFilePath = szFilePath;
        mCwd = szCurrentDirectory ? szCurrentDirectory : "";
        if(argv)
        {
            for(const char* const* p = argv; *p != nullptr; ++p)
                mArgv.emplace_back(*p);
        }
        mHasLaunchArgs = true;
        return true;
    }

    bool Debugger::launchChild()
    {
        if(!mHasLaunchArgs)
        {
            cbInternalError("launchChild called without Init");
            return false;
        }

        int pipeFds[2];
        if(pipe2(pipeFds, O_CLOEXEC) == -1)
        {
            cbInternalError("pipe2() failed: " + std::string(strerror(errno)));
            return false;
        }

        const pid_t pid = fork();
        if(pid == -1)
        {
            close(pipeFds[0]);
            close(pipeFds[1]);
            cbInternalError("fork() failed: " + std::string(strerror(errno)));
            return false;
        }

        if(pid == 0)
        {
            close(pipeFds[0]);

            auto childError = [&](const char* msg)
            {
                write(pipeFds[1], msg, strlen(msg));
                _exit(1);
            };

            if(setpgid(0, 0) < 0)
                childError("setpgid failed");

            if(!mCwd.empty())
            {
                if(chdir(mCwd.c_str()) == -1)
                    childError("chdir failed");
            }

            if(personality(ADDR_NO_RANDOMIZE) == -1)
                childError("personality(ADDR_NO_RANDOMIZE) failed");

            if(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1)
                childError("PTRACE_TRACEME failed");

            std::vector<char*> argvPtrs;
            if(!mArgv.empty())
            {
                argvPtrs.reserve(mArgv.size() + 1);
                for(auto & s : mArgv)
                    argvPtrs.push_back(s.data());
                argvPtrs.push_back(nullptr);
                execv(mFilePath.c_str(), argvPtrs.data());
            }
            else
            {
                char* defaultArgv[] = { const_cast<char*>(mFilePath.c_str()), nullptr };
                execv(mFilePath.c_str(), defaultArgv);
            }

            childError("execv failed");
        }

        close(pipeFds[1]);

        char errBuf[256] = {};
        ssize_t n;
        do
        {
            n = read(pipeFds[0], errBuf, sizeof(errBuf) - 1);
        }
        while(n == -1 && errno == EINTR);
        close(pipeFds[0]);

        if(n == -1)
        {
            const std::string err = strerror(errno);
            waitpid(pid, nullptr, 0);
            cbInternalError("read() from child pipe failed: " + err);
            return false;
        }

        if(n > 0)
        {
            waitpid(pid, nullptr, 0);
            cbInternalError("child process failed: " + std::string(errBuf));
            return false;
        }

        mMainPid.store(pid, std::memory_order_release);
        return true;
    }

    bool Debugger::Attach(pid_t)
    {
        // TODO: implement ptrace attach
        cbInternalError("Attach not implemented");
        return false;
    }

    void Debugger::Start()
    {
        mIsRunning.store(true, std::memory_order_release);
        debugLoop();
    }

    void Debugger::Continue()
    {
        {
            std::lock_guard lock(mPauseMutex);
            mStepPending.store(false, std::memory_order_release);
            mStepOverPending.store(false, std::memory_order_release);
            mPaused.store(false, std::memory_order_release);
        }
        mPauseCv.notify_one();
    }

    // Step requests only mean something while paused; otherwise they would latch
    // and turn the next Continue into a step.
    void Debugger::StepInto()
    {
        {
            std::lock_guard lock(mPauseMutex);
            if(!mPaused.load(std::memory_order_acquire))
                return;
            mStepPending.store(true, std::memory_order_release);
            mPaused.store(false, std::memory_order_release);
        }
        mPauseCv.notify_one();
    }

    void Debugger::StepOver()
    {
        {
            std::lock_guard lock(mPauseMutex);
            if(!mPaused.load(std::memory_order_acquire))
                return;
            mStepOverPending.store(true, std::memory_order_release);
            mPaused.store(false, std::memory_order_release);
        }
        mPauseCv.notify_one();
    }

    void Debugger::maskPushedTrapFlag() const
    {
        if(!mThread || !mProcess)
            return;

        // TF is bit 8 of EFLAGS: bit 0 of the second pushed byte for pushf and pushfq alike.
        const ptr flagsHigh = mThread->registers.Gsp() + 1;
        uint8 byte = 0;
        if(!mProcess->MemRead(flagsHigh, &byte, 1))
            return;
        byte &= static_cast<uint8>(~0x01);
        mProcess->MemWrite(flagsHigh, &byte, 1);
    }

    void Debugger::dispatchBreakpoint(const ptr address)
    {
        // Both are copies: the callback may delete the breakpoint out from under us,
        // and it must not run while the breakpoint lock is held.
        BreakpointInfo info;
        BreakpointCallback callback;
        if(!mProcess->TakeBreakpointDispatch(address, info, callback))
            return;

        if(callback)
            callback(info);

        cbBreakpoint(info);

        if(info.singleshot)
            mProcess->DeleteBreakpoint(address);
    }

    // Only the lifting thread may re-arm; anyone else would re-trap it in place.
    void Debugger::restoreSourceByte(const pid_t pid)
    {
        const auto it = mSourceRearms.find(pid);
        if(it == mSourceRearms.end())
            return;

        if(mProcess)
            mProcess->RearmBreakpointByte(it->second);
        mSourceRearms.erase(it);
    }

    void Debugger::cancelStepOver(const pid_t pid)
    {
        restoreSourceByte(pid);

        if(!mStepOver.active)
            return;
        if(mStepOver.planted && mProcess)
            mProcess->DeleteBreakpoint(mStepOver.target);
        mStepOver = {};
    }

    void Debugger::cancelStepOverIfOwner(const pid_t pid)
    {
        if(mStepOver.active && mStepOver.tid != pid)
        {
            restoreSourceByte(pid);
            return;
        }
        cancelStepOver(pid);
    }

    void Debugger::onExec()
    {
        if(mThread)
            mThread->clearSingleStep();

        // exec killed every other thread and replaced the image, so every entry is stale.
        // A worker's exec is reported under the leader's tid, so the owner is not checked.
        mSourceRearms.clear();
        mUnregisteredRunning.clear();

        mAllStopped = false;

        if(mProcess)
        {
            std::shared_lock lock(mProcessMutex);
            for(const auto & [tid, thread] : mProcess->threads)
            {
                thread->clearPendingBreakpoint();
                thread->clearPendingSignal();
            }
        }

        if(mStepOver.active)
        {
            // User breakpoints are left to the re-exec TODO in handleSigtrap.
            if(mStepOver.planted && mProcess)
                mProcess->ForgetBreakpoint(mStepOver.target);
            mStepOver = {};
        }

        // The old /proc/pid/mem descriptor is bound to the replaced address space.
        if(mProcess)
            mProcess->ResetMemFd();
    }

    Debugger::StepOverArm Debugger::armStepOver(const pid_t pid)
    {
        cancelStepOver(pid);

        if(!mThread || !mProcess)
            return StepOverArm::SingleStep;

        const ptr rip = mThread->registers.Gip();

        ptr target = 0;
        const StepOverKind kind = mProcess->ClassifyStepOverAt(rip, target);
        if(kind == StepOverKind::None || kind == StepOverKind::Pushf)
        {
            // Plain step: lift the breakpoint under RIP, restored when the step traps.
            if(mProcess->DisarmBreakpointByte(rip))
                mSourceRearms[pid] = rip;
            return StepOverArm::SingleStep;
        }

        bool planted = false;
        if(!mProcess->HasBreakpoint(target))
        {
            if(!mProcess->SetBreakpoint(target, false, SoftwareType::ShortInt3))
            {
                char message[64];
                snprintf(message, sizeof(message), "step-over: failed to set breakpoint at 0x%llx",
                         static_cast<unsigned long long>(target));
                cbInternalError(message);
                if(mProcess->DisarmBreakpointByte(rip))
                    mSourceRearms[pid] = rip;
                return StepOverArm::SingleStep;
            }
            planted = true;
        }

        mStepOver.active = true;
        mStepOver.target = target;
        mStepOver.tid = pid;
        mStepOver.rspFloor = mThread->registers.Gsp();
        mStepOver.planted = planted;

        if(mProcess->HasBreakpoint(rip))
        {
            if(kind == StepOverKind::Rep)
            {
                // A single step runs one iteration and leaves RIP on the instruction, so
                // the byte stays lifted until the whole loop is done.
                if(mProcess->DisarmBreakpointByte(rip))
                    mSourceRearms[pid] = rip;
            }
            // Step off the call now so its breakpoint is armed again while the callee
            // runs, for this thread's deeper frames and for every other thread.
            else if(!stepPastBreakpointByte(pid, rip))
            {
                // On an error path nothing else settles it, and a planted 0xCC left
                // behind would divert every later thread reaching the target.
                cancelStepOver(pid);
                return StepOverArm::Consumed;
            }
        }

        return StepOverArm::Armed;
    }

    void Debugger::Pause()
    {
        const pid_t pid = mMainPid.load(std::memory_order_acquire);
        if(pid > 0)
        {
            mPauseRequested.store(true, std::memory_order_release);
            kill(pid, SIGSTOP);
        }
    }

    bool Debugger::Stop()
    {
        const pid_t pid = mMainPid.load(std::memory_order_acquire);
        if(pid <= 0)
            return false;

        {
            std::lock_guard lock(mPauseMutex);
            mStopRequested.store(true, std::memory_order_release);
            mPaused.store(false, std::memory_order_release);
        }
        mPauseCv.notify_one();

        return kill(pid, SIGKILL) == 0;
    }

    void Debugger::Detach()
    {
        // TODO: implement ptrace detach
        cbInternalError("Detach not implemented");
    }

    void Debugger::cbCreateProcessEvent(const pid_t pid, const ptr entryPoint) { (void)pid; (void)entryPoint; }
    void Debugger::cbExitProcessEvent(const int exitCode) { (void)exitCode; }
    void Debugger::cbCreateThreadEvent(const pid_t tid) { (void)tid; }
    void Debugger::cbExitThreadEvent(const pid_t tid) { (void)tid; }
    void Debugger::cbLoadDllEvent(const ptr baseAddress, const std::string & path) { (void)baseAddress; (void)path; }
    void Debugger::cbUnloadDllEvent(const ptr baseAddress) { (void)baseAddress; }
    void Debugger::cbExceptionEvent(const int signal, const ptr address) { (void)signal; (void)address; }
    void Debugger::cbBreakpoint(const BreakpointInfo & info) { (void)info; }
    void Debugger::cbStep() {}
    void Debugger::cbSystemBreakpoint() {}
    void Debugger::cbAttachBreakpoint() {}
    void Debugger::cbUnhandledException(const int signal, const ptr address) { (void)signal; (void)address; }
    void Debugger::cbInternalError(const std::string & error) { (void)error; }
    void Debugger::cbDebugStringEvent(const std::string & text) { (void)text; }
    void Debugger::cbPaused() {}
    void Debugger::cbPauseTick() {}
}
