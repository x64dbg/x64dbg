#include <ElfBug/core/Debugger.h>

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
        cbCreateProcessEvent(pid, entryPoint);
    }

    void Debugger::exitProcessEvent(const pid_t pid, const int exitCode)
    {
        cbExitProcessEvent(exitCode);

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
}
