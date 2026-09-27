#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/ProcessArch.h>
#include <ElfBug/process/ProcessList.h>
#include <ElfBug/process/ProcFs.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/wait.h>
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
        if(pid > 0 && mAttachPid == 0)
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
        mImageId = {};
        reapDetachedChildren();
    }

    bool Debugger::Init(const char* path, const char* const* argv, const char* workingDirectory)
    {
        if(mIsRunning.load(std::memory_order_acquire))
        {
            cbInternalError("cannot launch '" + std::string(path ? path : "") +
                            "': the debug loop is still running");
            return false;
        }

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
        if(mIsRunning.load(std::memory_order_acquire))
        {
            cbInternalError("cannot attach to pid " + std::to_string(processId) +
                            ": the debug loop is still running");
            return false;
        }

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

        const Arch arch = DetectArchFromProcExe(processId);
        if(arch != Arch::X86_64)
        {
            cbInternalError("cannot attach to pid " + std::to_string(processId) + ": " +
                            ArchRejectMessage(arch));
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

    void Debugger::StepInto()
    {
        {
            std::lock_guard lock(mPauseMutex);
            if(!mPaused.load(std::memory_order_acquire))
                return;
            {
                std::shared_lock processLock(mProcessMutex);
                if(mThread && mThread->IsSuspended())
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
                if(mThread && mThread->IsSuspended())
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
        Thread* thread = mProcess->FindThread(tid);
        if(!thread || thread->IsRunning())
            return false;
        mThread = thread;
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
        Thread* thread = mProcess->FindThread(tid);
        if(!thread)
            return false;

        if(!suspended)
        {
            thread->Resume();
            if(!thread->IsSuspended())
                thread->SetWaitReason({});
            if(thread->IsSuspended() || mPaused.load(std::memory_order_acquire))
                return true;

            mPendingResume.insert(tid);
            interruptRunningThreadLocked(tgid, tid);
            return true;
        }

        if(!thread->IsRunning())
        {
            thread->Suspend();
            thread->SetWaitReason("Suspended");
            return true;
        }

        thread->Suspend();
        thread->SetWaitReason("Suspended");
        mPendingSuspend.insert(tid);

        if(tgkill(tgid, tid, SIGSTOP) == 0)
        {
            thread->SetPendingSigstop(true);
            return true;
        }

        mPendingSuspend.erase(tid);
        thread->Resume();
        if(!thread->IsSuspended())
            thread->SetWaitReason({});
        return false;
    }

    std::string Debugger::readWaitReason(const pid_t tgid, const pid_t tid)
    {
        std::string reason = procfs::ReadLine(procfs::TaskPath(tgid, tid, "wchan"));
        if(reason == "0" || reason == "ptrace_stop" || reason == "do_signal_stop")
            return {};
        return reason;
    }

    void Debugger::dispatchBreakpoint(const ptr address)
    {
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
            mThread->ClearSingleStep();

        mSourceRearms.clear();
        mUnregisteredRunning.clear();

        mAllStopped = false;

        if(mProcess)
        {
            std::unique_lock lock(mProcessMutex);
            for(const auto & entry : mProcess->threads)
            {
                entry.second->ClearPendingBreakpoint();
                entry.second->ClearPendingSignal();
            }
        }

        if(mStepOver.active)
        {
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
            if(tid == except || !thread->IsRunning())
                continue;

            if(thread->PendingSigstop())
            {
                owed = true;
                continue;
            }
            if(tgkill(tgid, tid, SIGSTOP) == -1)
                continue;
            thread->SetPendingSigstop(true);
            return true;
        }
        return owed;
    }

    void Debugger::interruptRunningThread(const pid_t tgid)
    {
        {
            std::unique_lock lock(mProcessMutex);
            if(interruptRunningThreadLocked(tgid, 0))
                return;
        }
        kill(tgid, SIGSTOP);
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

    bool Debugger::Detach()
    {
        const pid_t pid = mMainPid.load(std::memory_order_acquire);
        if(pid <= 0)
        {
            if(!mIsRunning.load(std::memory_order_acquire))
                return false;
            mDetachRequested.store(true, std::memory_order_release);
            return true;
        }

        bool wasPaused = false;
        {
            std::lock_guard lock(mPauseMutex);
            if(mDetachRequested.load(std::memory_order_acquire))
                return false;

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

        return true;
    }

    void Debugger::cbCreateProcess(const pid_t pid, const ptr entryPoint) { (void)pid; (void)entryPoint; }
    void Debugger::cbExitProcess(const int exitCode) { (void)exitCode; }
    void Debugger::cbCreateThread(const pid_t tid) { (void)tid; }
    void Debugger::cbExitThread(const pid_t tid) { (void)tid; }
    void Debugger::cbException(const int signal, const ptr address) { (void)signal; (void)address; }
    void Debugger::cbBreakpoint(const BreakpointInfo & info) { (void)info; }
    void Debugger::cbStep() {}
    void Debugger::cbSystemBreakpoint() {}
    void Debugger::cbAttachBreakpoint() {}
    void Debugger::cbDetach() {}
    void Debugger::cbExec() {}
    void Debugger::cbInternalError(const std::string & error) { (void)error; }
    void Debugger::cbDebugString(const std::string & text) { (void)text; }
    void Debugger::cbPaused() {}
    void Debugger::cbPauseTick() {}
}
