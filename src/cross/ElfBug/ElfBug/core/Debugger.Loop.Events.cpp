#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/ProcessList.h>

namespace ElfBug
{
    void Debugger::createProcessEvent(pid_t pid, const Arch arch)
    {
        auto [it, inserted] = mProcesses.try_emplace(pid, pid);

        {
            std::unique_lock lock(mProcessMutex);
            mProcess = &it->second;
            mProcess->arch = arch;
            mProcess->threads.emplace(pid, std::make_unique<Thread>(pid));
            mThread = mProcess->threads.at(pid).get();
        }

        mThread->registers.Read();
        const ptr entryPoint = mThread->registers.Gip();
        cbCreateProcess(pid, entryPoint);
    }

    void Debugger::exitProcessEvent(const pid_t pid, const int exitCode)
    {
        cbExitProcess(exitCode);

        {
            std::lock_guard pauseLock(mPauseMutex);
            mPendingSuspend.erase(pid);
            mPendingResume.erase(pid);
        }

        std::unique_lock lock(mProcessMutex);
        mProcesses.erase(pid);

        if(pid == mMainPid.load(std::memory_order_relaxed))
        {
            mProcess = nullptr;
            mThread = nullptr;
            mMainPid.store(0, std::memory_order_release);
        }
    }
    void Debugger::createThreadEvent(pid_t tid)
    {
        if(!mProcess)
            return;

        // Already resumed if its own SIGSTOP was seen before this clone notification.
        const bool alreadyRunning = mUnregisteredRunning.erase(tid) > 0;

        const pid_t tgid = ThreadGroupId(tid);
        if(tgid != 0 && tgid != mMainPid.load(std::memory_order_relaxed))
        {
            releaseForeignClone(tid, tgid, alreadyRunning);
            return;
        }

        bool inserted = false;
        {
            std::unique_lock lock(mProcessMutex);
            const auto result = mProcess->threads.emplace(tid, std::make_unique<Thread>(tid));
            inserted = result.second;
            if(alreadyRunning)
                result.first->second->setRunning(true);
        }

        if(inserted)
            cbCreateThread(tid);
    }

    void Debugger::exitThreadEvent(const pid_t tid)
    {
        if(!mProcess)
            return;

        // Settle the dead thread's re-arm here or the breakpoint stays disarmed forever.
        if(mStepOver.active && mStepOver.tid == tid)
            cancelStepOver(tid);
        else
            restoreSourceByte(tid);

        cbExitThread(tid);

        {
            std::lock_guard pauseLock(mPauseMutex);
            mPendingSuspend.erase(tid);
            mPendingResume.erase(tid);
        }

        std::unique_lock lock(mProcessMutex);
        if(mThread && mThread->tid == tid)
            mThread = nullptr;

        mProcess->threads.erase(tid);
    }
}
