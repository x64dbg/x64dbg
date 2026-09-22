#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/ProcessArch.h>
#include <ElfBug/process/ProcessList.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ElfBug
{
    namespace
    {
        constexpr auto kReleaseGroupStopTimeout = std::chrono::milliseconds(50);

        struct PendingAttachSignal
        {
            int signal = 0;
            ptr address = 0;
        };

        // 'T' is group-stop, 't' is any ptrace-stop. Only the first outlives a detach.
        char processState(const pid_t pid)
        {
            std::ifstream file("/proc/" + std::to_string(pid) + "/stat");
            std::string line;
            std::getline(file, line);
            const auto lastParen = line.rfind(')');
            if(lastParen == std::string::npos || lastParen + 2 >= line.size())
                return '?';
            return line[lastParen + 2];
        }

        void collectClonedChild(const pid_t parent, const int status, std::vector<pid_t> & clones)
        {
            if(((status >> 16) & 0xffff) != PTRACE_EVENT_CLONE)
                return;
            unsigned long child = 0;
            if(ptrace(PTRACE_GETEVENTMSG, parent, nullptr, &child) != -1 && child != 0)
                clones.push_back(static_cast<pid_t>(child));
        }

        bool drainQueuedSigstop(const pid_t tid, int & deliver, std::vector<pid_t> & clones)
        {
            constexpr int kDrainAttempts = 32;
            for(int attempt = 0; attempt < kDrainAttempts; attempt++)
            {
                if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
                    return true;

                int status = 0;
                switch(WaitForStop(tid, status))
                {
                case WaitResult::Gone:
                    return true;
                case WaitResult::TimedOut:
                    return false;
                case WaitResult::Stopped:
                    break;
                }

                if(WIFEXITED(status) || WIFSIGNALED(status))
                    return true;
                if(!WIFSTOPPED(status))
                    continue;

                if(WSTOPSIG(status) == SIGSTOP)
                    return true;

                collectClonedChild(tid, status, clones);
                if(deliver == 0)
                {
                    siginfo_t info{};
                    const bool haveInfo = ptrace(PTRACE_GETSIGINFO, tid, nullptr, &info) != -1;
                    int signal = 0;
                    ptr address = 0;
                    if(StopShouldQueue(status, haveInfo, info, signal, address))
                        deliver = signal;
                }
            }
            return false;
        }
    }

    std::string AttachErrorMessage(const pid_t pid, const int err, const int ptraceScope,
                                   const bool isOurChild)
    {
        const std::string prefix = "cannot attach to pid " + std::to_string(pid) + ": ";

        if(err != EPERM)
            return prefix + strerror(err);

        if(ptraceScope >= 3)
        {
            return prefix + "permission denied because /proc/sys/kernel/yama/ptrace_scope is " +
                   std::to_string(ptraceScope) + ", which disables ptrace system-wide.";
        }

        if(ptraceScope == 2)
        {
            return prefix + "permission denied because /proc/sys/kernel/yama/ptrace_scope is 2, "
                            "which only allows ptrace for processes with the CAP_SYS_PTRACE capability. "
                            "Run setcap cap_sys_ptrace=+eip on the x64dbg binary to attach.";
        }

        if(ptraceScope == 1 && !isOurChild)
        {
            return prefix + "permission denied because /proc/sys/kernel/yama/ptrace_scope is 1, "
                            "which only allows tracing your own children. Run setcap cap_sys_ptrace=+eip "
                            "on the x64dbg binary to attach to other processes.";
        }

        return prefix + strerror(err);
    }

    std::string ArchRejectMessage(const Arch arch)
    {
        const char* name = arch == Arch::I386 ? "i386" : "unknown";
        return "unsupported architecture (" + std::string(name) + "); only x86_64 is supported";
    }

    void Debugger::reportAttachError(const pid_t pid, const pid_t tid, const int err)
    {
        int scope = 0;
        std::ifstream file("/proc/sys/kernel/yama/ptrace_scope");
        if(file)
            file >> scope;

        std::string message = AttachErrorMessage(pid, err, scope, ParentPid(pid) == getpid());
        if(tid != pid)
            message += " (thread " + std::to_string(tid) + ")";
        cbInternalError(message);
    }

    bool Debugger::attachToProcess()
    {
        const pid_t pid = mAttachPid;
        mWasGroupStopped = processState(pid) == 'T';
        std::vector<pid_t> attached;
        std::vector<pid_t> acquired;
        std::vector<pid_t> owesSigstop;
        std::unordered_map<pid_t, PendingAttachSignal> attachPendingSignals;

        auto rollback = [&]
        {
            // The sweep's own SIGSTOPs are still queued on these threads; detaching
            // without taking them back freezes the process we failed to attach to.
            bool undrained = false;
            std::unordered_set<pid_t> leftRunning;
            std::unordered_map<pid_t, int> deliverOnRelease;
            std::vector<pid_t> clones;
            for(const pid_t tid : owesSigstop)
            {
                const auto pending = attachPendingSignals.find(tid);
                int deliver = pending != attachPendingSignals.end() ? pending->second.signal : 0;
                if(!drainQueuedSigstop(tid, deliver, clones))
                {
                    undrained = true;
                    leftRunning.insert(tid);
                }
                if(deliver != 0)
                    deliverOnRelease[tid] = deliver;
            }
            for(const pid_t tid : attached)
            {
                const auto deliver = deliverOnRelease.find(tid);
                const int signal = deliver != deliverOnRelease.end() ? deliver->second : 0;
                int raced = 0;
                if(leftRunning.count(tid) > 0 && !restopForDetach(tid, pid, true, raced))
                    cbInternalError("thread " + std::to_string(tid) +
                                    " stays traced: it could not be stopped to release it");
                releaseThread(tid, pid, signal, raced);
            }
            for(const pid_t child : clones)
                releaseForeignClone(child, pid, false);
            if(undrained && !mWasGroupStopped)
                kill(pid, SIGCONT);
        };

        // Rescan until a pass finds nothing new. Only a running thread can clone and every pass
        // stops the ones it finds, so this terminates.
        std::unordered_set<pid_t> seen;
        bool added = true;
        while(added)
        {
            added = false;

            std::vector<pid_t> tids;
            if(!ReadTaskList(pid, tids))
            {
                rollback();
                cbInternalError("cannot attach to pid " + std::to_string(pid) + ": the process exited");
                return false;
            }

            for(const pid_t tid : tids)
            {
                if(!seen.insert(tid).second)
                    continue;
                added = true;

                if(ptrace(PTRACE_ATTACH, tid, nullptr, nullptr) == -1)
                {
                    if(errno == ESRCH)
                        continue;
                    const int err = errno;
                    rollback();
                    reportAttachError(pid, tid, err);
                    return false;
                }
                attached.push_back(tid);

                int status = 0;
                const WaitResult waited = WaitForStop(tid, status);
                if(waited == WaitResult::TimedOut)
                {
                    rollback();
                    cbInternalError("cannot attach to pid " + std::to_string(pid) +
                                    ": thread " + std::to_string(tid) +
                                    " did not stop; it may be stuck in uninterruptible I/O");
                    return false;
                }
                if(waited == WaitResult::Gone || !WIFSTOPPED(status))
                    continue;

                acquired.push_back(tid);

                if(ptrace(PTRACE_SETOPTIONS, tid, nullptr, kPtraceOptions) == -1)
                {
                    const std::string err = strerror(errno);
                    rollback();
                    cbInternalError("PTRACE_SETOPTIONS failed for thread " +
                                    std::to_string(tid) + ": " + err);
                    return false;
                }

                if(WSTOPSIG(status) != SIGSTOP)
                {
                    const int sig = WSTOPSIG(status);
                    siginfo_t info{};
                    if(ptrace(PTRACE_GETSIGINFO, tid, nullptr, &info) != -1)
                    {
                        if(SweepShouldQueue(sig, info.si_code > 0) ||
                                ForeignTrapShouldQueue(sig, info))
                            attachPendingSignals[tid] = {sig, FaultAddress(sig, info)};
                    }
                    else if(errno != EINVAL)
                    {
                        // EINVAL is a group-stop and carries nothing; anything else is a signal we lost.
                        if(SweepShouldQueue(sig, false))
                            attachPendingSignals[tid] = {sig, 0};
                    }
                    owesSigstop.push_back(tid);
                }
            }
        }

        if(std::find(acquired.begin(), acquired.end(), pid) == acquired.end())
        {
            rollback();
            cbInternalError("cannot attach to pid " + std::to_string(pid) +
                            ": the main thread exited during attach");
            return false;
        }

        if(mDetachRequested.load(std::memory_order_acquire))
        {
            rollback();
            cbInternalError("detached from pid " + std::to_string(pid) +
                            " before the session started");
            cbDetach();
            return false;
        }

        const Arch arch = DetectArchFromProcExe(pid);
        if(arch != Arch::X86_64)
        {
            rollback();
            cbInternalError("cannot attach to pid " + std::to_string(pid) + ": " +
                            ArchRejectMessage(arch));
            return false;
        }

        mMainPid.store(pid, std::memory_order_release);
        createProcessEvent(pid, arch);

        for(const pid_t tid : acquired)
        {
            if(tid != pid)
                createThreadEvent(tid, pid, false);
        }

        for(const pid_t tid : owesSigstop)
        {
            std::unique_lock lock(mProcessMutex);
            const auto it = mProcess->threads.find(tid);
            if(it == mProcess->threads.end())
                continue;

            it->second->SetPendingSigstop(true);

            const auto pending = attachPendingSignals.find(tid);
            if(pending != attachPendingSignals.end())
                it->second->SetPendingSignal(pending->second.signal, pending->second.address, true);
        }

        return true;
    }

    bool Debugger::restopForDetach(const pid_t tid, const pid_t tgid, const bool owed, int & deliver)
    {
        if(!owed && tgkill(tgid, tid, SIGSTOP) == -1)
            return errno == ESRCH;

        int status = 0;
        if(WaitForStop(tid, status) != WaitResult::Stopped)
            return false;
        if(WIFEXITED(status) || WIFSIGNALED(status))
            return true;
        if(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP)
            return true;

        std::vector<pid_t> clones;
        collectClonedChild(tid, status, clones);
        siginfo_t info{};
        const bool haveInfo = ptrace(PTRACE_GETSIGINFO, tid, nullptr, &info) != -1;
        ptr address = 0;
        (void)StopShouldQueue(status, haveInfo, info, deliver, address);
        const bool drained = drainQueuedSigstop(tid, deliver, clones);
        for(const pid_t child : clones)
            releaseForeignClone(child, tgid, false);
        return drained;
    }

    void Debugger::releaseThread(const pid_t tid, const pid_t tgid, const int signal, const int raced)
    {
        const int first = signal != 0 ? signal : raced;
        if(ptrace(PTRACE_DETACH, tid, nullptr, reinterpret_cast<void*>(static_cast<unsigned long>(first))) == -1)
        {
            if(errno != ESRCH)
                cbInternalError("PTRACE_DETACH failed: " + std::string(strerror(errno)));
            return;
        }
        if(signal != 0 && raced != 0 && raced != signal)
            tgkill(tgid, tid, raced);
    }

    void Debugger::releaseForeignClone(const pid_t tid, const pid_t tgid, const bool running)
    {
        int deliver = 0;

        if(running && tgkill(tgid, tid, SIGSTOP) == -1)
        {
            if(errno != ESRCH)
                cbInternalError("tgkill() failed: " + std::string(strerror(errno)));
            return;
        }

        int status = 0;
        if(WaitForStop(tid, status) != WaitResult::Stopped)
        {
            cbInternalError("cloned process " + std::to_string(tid) + " could not be released");
            return;
        }

        if(WIFEXITED(status) || WIFSIGNALED(status))
            return;

        if(WIFSTOPPED(status) && WSTOPSIG(status) != SIGSTOP)
        {
            siginfo_t info{};
            const bool haveInfo = ptrace(PTRACE_GETSIGINFO, tid, nullptr, &info) != -1;
            int signal = 0;
            ptr address = 0;
            if(StopShouldQueue(status, haveInfo, info, signal, address))
                deliver = signal;

            std::vector<pid_t> clones;
            const bool drained = !running || drainQueuedSigstop(tid, deliver, clones);
            for(const pid_t child : clones)
                releaseForeignClone(child, tgid, false);
            if(!drained)
            {
                cbInternalError("cloned process " + std::to_string(tid) + " could not be released");
                return;
            }
        }

        if(ptrace(PTRACE_DETACH, tid, nullptr, reinterpret_cast<void*>(static_cast<long>(deliver))) == -1 &&
                errno != ESRCH)
            cbInternalError("PTRACE_DETACH failed: " + std::string(strerror(errno)));
    }

    void Debugger::detachFromProcess(const pid_t reportedTid)
    {
        mDetachRequested.store(false, std::memory_order_release);

        if(!mProcess)
        {
            mPaused.store(false, std::memory_order_release);
            mIsRunning.store(false, std::memory_order_release);
            cbDetach();
            return;
        }

        const pid_t pid = mProcess->pid;

        mSourceRearms.clear();
        cancelStepOver(pid);
        if(!mProcess->DisarmAllBreakpointBytes())
            cbInternalError("could not restore every breakpoint byte before detaching");

        std::vector<pid_t> acquired;
        std::vector<pid_t> owesSigstop;
        {
            std::shared_lock lock(mProcessMutex);
            for(const auto & [tid, thread] : mProcess->threads)
            {
                acquired.push_back(tid);
                if(thread->PendingSigstop())
                    owesSigstop.push_back(tid);
            }
        }
        for(const pid_t tid : owesSigstop)
        {
            swallowPendingSigstop(tid);
            if(!mProcess)
                break;
        }

        std::vector<std::pair<pid_t, int>> targets;
        std::unordered_set<pid_t> leftRunning;
        std::unordered_set<pid_t> owedSigstop;
        for(const pid_t tid : mUnregisteredRunning)
        {
            const pid_t tgid = ThreadGroupId(tid);
            releaseForeignClone(tid, tgid != 0 ? tgid : pid, true);
        }
        mUnregisteredRunning.clear();

        if(mProcess)
        {
            if(mPendingSignal != 0)
            {
                std::unique_lock lock(mProcessMutex);
                const auto it = mProcess->threads.find(reportedTid);
                if(it != mProcess->threads.end())
                    it->second->SetPendingSignal(mPendingSignal, 0, false);
                mPendingSignal = 0;
            }

            std::shared_lock lock(mProcessMutex);
            for(const auto & [tid, thread] : mProcess->threads)
            {
                targets.emplace_back(tid, thread->PendingSignal());
                if(thread->IsRunning())
                {
                    leftRunning.insert(tid);
                    if(thread->PendingSigstop())
                        owedSigstop.insert(tid);
                }
            }
        }
        else
        {
            mPendingSignal = 0;
            for(const pid_t tid : acquired)
                targets.emplace_back(tid, 0);
        }

        mMainPid.store(0, std::memory_order_release);

        for(const auto & [tid, signal] : targets)
        {
            int raced = 0;
            if(leftRunning.count(tid) > 0 && !restopForDetach(tid, pid, owedSigstop.count(tid) > 0, raced))
                cbInternalError("thread " + std::to_string(tid) +
                                " stays traced: it could not be stopped to release it");
            releaseThread(tid, pid, signal, raced);
        }

        if(!mWasGroupStopped)
        {
            const auto deadline = std::chrono::steady_clock::now() + kReleaseGroupStopTimeout;
            while(std::chrono::steady_clock::now() < deadline)
            {
                if(processState(pid) == 'T')
                {
                    cbInternalError("pid " + std::to_string(pid) + " was group-stopped on release; "
                                    "sending SIGCONT so it keeps running");
                    kill(pid, SIGCONT);
                    break;
                }
                std::this_thread::sleep_for(kPollInterval);
            }
        }

        {
            std::unique_lock lock(mProcessMutex);
            mProcess = nullptr;
            mThread = nullptr;
            mProcesses.clear();
        }

        if(mAttachPid == 0)
            mDetachedChildren.push_back(pid);

        mPaused.store(false, std::memory_order_release);
        mIsRunning.store(false, std::memory_order_release);

        cbDetach();
    }
}
