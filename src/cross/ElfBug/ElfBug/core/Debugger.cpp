#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/ProcessArch.h>
#include <ElfBug/process/ProcessList.h>
#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <csignal>
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
        reapDetachedChildren();
    }

    void Debugger::reapDetachedChildren()
    {
        auto it = mDetachedChildren.begin();
        while(it != mDetachedChildren.end())
        {
            const pid_t reaped = waitpid(*it, nullptr, WNOHANG | __WALL);
            if(reaped == *it || (reaped == -1 && errno == ECHILD))
                it = mDetachedChildren.erase(it);
            else
                ++it;
        }
    }

    void Debugger::resetSessionState()
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
        {
            std::lock_guard pauseLock(mPauseMutex);
            mPendingSuspend.clear();
            mPendingResume.clear();
        }
        mAllStopped = false;
        mPauseRequested.store(false, std::memory_order_release);
        mStopRequested.store(false, std::memory_order_release);
        mDetachRequested.store(false, std::memory_order_release);
        mPendingSignal = 0;
        mAttachPid = 0;
        mWasGroupStopped = false;
        reapDetachedChildren();
    }

    bool Debugger::Init(const char* path, const char* const* argv, const char* workingDirectory)
    {
        resetSessionState();

        if(!path)
            return false;

        if(!workingDirectory)
        {
            const std::string filePath(path);
            struct stat info = {};
            if(stat(path, &info) != 0)
            {
                cbInternalError("cannot execute '" + filePath + "': " + std::string(strerror(errno)));
                return false;
            }
            if(!S_ISREG(info.st_mode))
            {
                cbInternalError("cannot execute '" + filePath + "': not a regular file");
                return false;
            }
            if(access(path, X_OK) != 0)
            {
                if(errno == EACCES && (info.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0)
                    cbInternalError("cannot execute '" + filePath + "': file is not executable, run chmod +x '" + filePath + "'");
                else
                    cbInternalError("cannot execute '" + filePath + "': " + std::string(strerror(errno)));
                return false;
            }
        }

        mFilePath = path;
        mCwd = workingDirectory ? workingDirectory : "";
        if(argv)
        {
            for(const char* const* p = argv; *p != nullptr; ++p)
                mArgv.emplace_back(*p);
        }
        mHasLaunchArgs = true;
        return true;
    }

    bool Debugger::Attach(const pid_t processId)
    {
        resetSessionState();

        if(processId <= 0 || processId == getpid())
        {
            cbInternalError("cannot attach to pid " + std::to_string(processId) +
                            ": not a debuggable process");
            return false;
        }

        std::vector<pid_t> tids;
        if(!ReadTaskList(processId, tids) || tids.empty())
        {
            cbInternalError("cannot attach to pid " + std::to_string(processId) + ": no such process");
            return false;
        }

        if(TracerPid(processId) != 0)
        {
            cbInternalError("cannot attach to pid " + std::to_string(processId) +
                            ": already being debugged");
            return false;
        }

        const Arch arch = detectArchFromProcExe(processId);
        if(arch != Arch::X86_64)
        {
            const char* archName = arch == Arch::I386 ? "i386" : "unknown";
            cbInternalError("cannot attach to pid " + std::to_string(processId) +
                            ": unsupported architecture (" + std::string(archName) +
                            "); only x86_64 is supported");
            return false;
        }

        mAttachPid = processId;
        return true;
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
            {
                std::shared_lock processLock(mProcessMutex);
                if(mThread && mThread->isSuspended())
                    return;
            }
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
            {
                std::shared_lock processLock(mProcessMutex);
                if(mThread && mThread->isSuspended())
                    return;
            }
            mStepOverPending.store(true, std::memory_order_release);
            mPaused.store(false, std::memory_order_release);
        }
        mPauseCv.notify_one();
    }

    bool Debugger::SwitchThread(const pid_t tid)
    {
        std::lock_guard pauseLock(mPauseMutex);
        if(!mPaused.load(std::memory_order_acquire))
            return false;

        std::unique_lock lock(mProcessMutex);
        if(!mProcess)
            return false;
        const auto it = mProcess->threads.find(tid);
        if(it == mProcess->threads.end() || it->second->isRunning())
            return false;
        mThread = it->second.get();
        return true;
    }

    bool Debugger::SetThreadSuspended(const pid_t tid, const bool suspended)
    {
        const pid_t tgid = mMainPid.load(std::memory_order_acquire);
        if(tgid <= 0)
            return false;

        std::lock_guard pauseLock(mPauseMutex);

        std::unique_lock lock(mProcessMutex);
        if(!mProcess)
            return false;
        const auto it = mProcess->threads.find(tid);
        if(it == mProcess->threads.end())
            return false;

        if(!suspended)
        {
            it->second->resume();
            if(!it->second->isSuspended())
                it->second->setWaitReason({});
            if(it->second->isSuspended() || mPaused.load(std::memory_order_acquire))
                return true;

            mPendingResume.insert(tid);
            interruptRunningThreadLocked(tgid, tid);
            return true;
        }

        if(!it->second->isRunning())
        {
            it->second->suspend();
            it->second->setWaitReason("Suspended");
            return true;
        }

        it->second->suspend();
        it->second->setWaitReason("Suspended");
        mPendingSuspend.insert(tid);

        if(tgkill(tgid, tid, SIGSTOP) == 0)
        {
            it->second->setPendingSigstop(true);
            return true;
        }

        mPendingSuspend.erase(tid);
        it->second->resume();
        if(!it->second->isSuspended())
            it->second->setWaitReason({});
        return false;
    }

    std::string Debugger::readWaitReason(const pid_t tgid, const pid_t tid)
    {
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/task/%d/wchan", tgid, tid);
        const int fd = open(path, O_RDONLY | O_CLOEXEC);
        if(fd == -1)
            return {};
        char buffer[64];
        const ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        close(fd);
        if(n <= 0)
            return {};
        buffer[n] = '\0';
        if(strcmp(buffer, "0") == 0 || strcmp(buffer, "ptrace_stop") == 0 ||
                strcmp(buffer, "do_signal_stop") == 0)
            return {};
        return buffer;
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
            std::unique_lock lock(mProcessMutex);
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

    bool Debugger::interruptRunningThreadLocked(const pid_t tgid, const pid_t except)
    {
        if(!mProcess)
            return false;

        bool owed = false;
        for(const auto & [tid, thread] : mProcess->threads)
        {
            if(tid == except || !thread->isRunning())
                continue;

            if(thread->pendingSigstop())
            {
                owed = true;
                continue;
            }
            if(tgkill(tgid, tid, SIGSTOP) == -1)
                continue;
            thread->setPendingSigstop(true);
            return true;
        }
        return owed;
    }

    void Debugger::interruptRunningThread(const pid_t pid)
    {
        {
            std::unique_lock lock(mProcessMutex);
            if(interruptRunningThreadLocked(pid, 0))
                return;
        }
        kill(pid, SIGSTOP);
    }

    void Debugger::Pause()
    {
        const pid_t pid = mMainPid.load(std::memory_order_acquire);
        if(pid <= 0)
            return;
        {
            std::lock_guard lock(mPauseMutex);
            if(mPaused.load(std::memory_order_acquire))
                return;
            mPauseRequested.store(true, std::memory_order_release);
        }
        interruptRunningThread(pid);
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
        const pid_t pid = mMainPid.load(std::memory_order_acquire);
        if(pid <= 0)
        {
            if(mIsRunning.load(std::memory_order_acquire))
                mDetachRequested.store(true, std::memory_order_release);
            return;
        }

        bool wasPaused = false;
        {
            std::lock_guard lock(mPauseMutex);
            if(mDetachRequested.load(std::memory_order_acquire))
                return;

            wasPaused = mPaused.load(std::memory_order_acquire);
            if(!wasPaused)
            {
                mPauseRequested.store(true, std::memory_order_release);
                interruptRunningThread(pid);
            }
            mDetachRequested.store(true, std::memory_order_release);
        }

        if(wasPaused)
            mPauseCv.notify_one();
    }

    void Debugger::cbCreateProcess(const pid_t pid, const ptr entryPoint) { (void)pid; (void)entryPoint; }
    void Debugger::cbExitProcess(const int exitCode) { (void)exitCode; }
    void Debugger::cbCreateThread(const pid_t tid) { (void)tid; }
    void Debugger::cbExitThread(const pid_t tid) { (void)tid; }
    void Debugger::cbLoadModule(const ptr baseAddress, const std::string & path) { (void)baseAddress; (void)path; }
    void Debugger::cbUnloadModule(const ptr baseAddress) { (void)baseAddress; }
    void Debugger::cbException(const int signal, const ptr address) { (void)signal; (void)address; }
    void Debugger::cbBreakpoint(const BreakpointInfo & info) { (void)info; }
    void Debugger::cbStep() {}
    void Debugger::cbSystemBreakpoint() {}
    void Debugger::cbAttachBreakpoint() {}
    void Debugger::cbDetach() {}
    void Debugger::cbUnhandledException(const int signal, const ptr address) { (void)signal; (void)address; }
    void Debugger::cbInternalError(const std::string & error) { (void)error; }
    void Debugger::cbDebugString(const std::string & text) { (void)text; }
    void Debugger::cbPaused() {}
    void Debugger::cbPauseTick() {}
}
