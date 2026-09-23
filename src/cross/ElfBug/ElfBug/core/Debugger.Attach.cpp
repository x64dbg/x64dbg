#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/ProcessArch.h>
#include <ElfBug/process/ProcessList.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstddef>
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

        void addReleaseSignal(std::vector<int> & signals, const int signal)
        {
            if(signal != 0 && std::find(signals.begin(), signals.end(), signal) == signals.end())
                signals.push_back(signal);
        }

        // Our int3 the thread hit before the bytes were disarmed: step it back onto the restored instruction.
        bool rewindOwnTrap(const pid_t tid, const Process* process)
        {
            if(!process)
                return false;
            constexpr auto ripOffset = offsetof(user_regs_struct, rip);
            errno = 0;
            const long rip = ptrace(PTRACE_PEEKUSER, tid, reinterpret_cast<void*>(ripOffset), nullptr);
            if(errno != 0 || !process->HasBreakpoint(static_cast<ptr>(rip) - 1))
                return false;
            return ptrace(PTRACE_POKEUSER, tid, reinterpret_cast<void*>(ripOffset),
                          reinterpret_cast<void*>(rip - 1)) != -1;
        }

        void keepReleaseSignal(const pid_t tid, const int status, const Process* process, std::vector<int> & signals)
        {
            if(((status >> 16) & 0xffff) != 0)
                return;
            siginfo_t info{};
            const bool haveInfo = ptrace(PTRACE_GETSIGINFO, tid, nullptr, &info) != -1;
            int signal = 0;
            ptr address = 0;
            if(StopShouldQueue(status, haveInfo, info, signal, address))
                addReleaseSignal(signals, signal);
            else if(haveInfo && ForeignTrapShouldQueue(WSTOPSIG(status), info) && !rewindOwnTrap(tid, process))
                addReleaseSignal(signals, SIGTRAP);
        }

        WaitResult drainQueuedSigstop(const pid_t tid, const Process* process, std::vector<int> & signals,
                                      std::vector<pid_t> & clones)
        {
            for(;;)
            {
                if(ptrace(PTRACE_CONT, tid, nullptr, nullptr) == -1)
                    return WaitResult::Gone;

                int status = 0;
                const WaitResult waited = WaitForStop(tid, status);
                if(waited != WaitResult::Stopped)
                    return waited;

                if(WIFEXITED(status) || WIFSIGNALED(status))
                    return WaitResult::Gone;
                if(!WIFSTOPPED(status))
                    continue;

                if(WSTOPSIG(status) == SIGSTOP)
                    return WaitResult::Stopped;

                collectClonedChild(tid, status, clones);
                keepReleaseSignal(tid, status, process, signals);
            }
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
        std::vector<pid_t> unstopped;
        std::unordered_map<pid_t, PendingAttachSignal> attachPendingSignals;

        auto rollback = [&]
        {
            // The sweep's own SIGSTOPs are still queued on these threads; detaching
            // without taking them back freezes the process we failed to attach to.
            std::unordered_set<pid_t> leftRunning(unstopped.begin(), unstopped.end());
            std::unordered_map<pid_t, std::vector<int>> signals;
            std::vector<pid_t> clones;
            for(const pid_t tid : owesSigstop)
            {
                auto & kept = signals[tid];
                const auto pending = attachPendingSignals.find(tid);
                if(pending != attachPendingSignals.end())
                    addReleaseSignal(kept, pending->second.signal);
                if(drainQueuedSigstop(tid, nullptr, kept, clones) == WaitResult::TimedOut)
                    leftRunning.insert(tid);
            }
            for(const pid_t tid : attached)
            {
                if(leftRunning.count(tid) > 0)
                    releaseRunningThread(tid, pid, true, signals[tid]);
                else
                    releaseThread(tid, pid, signals[tid]);
            }
            for(const pid_t child : clones)
                releaseForeignClone(child, pid, false);
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
                    unstopped.push_back(tid);
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

    WaitResult Debugger::restopForDetach(const pid_t tid, const pid_t tgid, const bool owed, std::vector<int> & signals)
    {
        if(!owed && tgkill(tgid, tid, SIGSTOP) == -1)
            return WaitResult::Gone;

        int status = 0;
        const WaitResult waited = WaitForStop(tid, status);
        if(waited != WaitResult::Stopped)
            return waited;
        if(WIFEXITED(status) || WIFSIGNALED(status))
            return WaitResult::Gone;
        if(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP)
            return WaitResult::Stopped;

        std::vector<pid_t> clones;
        collectClonedChild(tid, status, clones);
        keepReleaseSignal(tid, status, mProcess, signals);
        const WaitResult drained = drainQueuedSigstop(tid, mProcess, signals, clones);
        for(const pid_t child : clones)
            releaseForeignClone(child, tgid, false);
        return drained;
    }

    void Debugger::releaseRunningThread(const pid_t tid, const pid_t tgid, bool owed, std::vector<int> signals)
    {
        bool reported = false;
        for(;;)
        {
            const WaitResult result = restopForDetach(tid, tgid, owed, signals);
            if(result == WaitResult::Gone)
                return;
            if(result == WaitResult::Stopped)
                break;
            owed = true;
            if(!reported)
            {
                cbDebugString("thread " + std::to_string(tid) + " has not stopped yet, "
                              "it may be in uninterruptible I/O; waiting for it before releasing it");
                reported = true;
            }
        }
        releaseThread(tid, tgid, signals);
    }

    void Debugger::releaseThread(const pid_t tid, const pid_t tgid, const std::vector<int> & signals)
    {
        const int first = signals.empty() ? 0 : signals.front();
        if(ptrace(PTRACE_DETACH, tid, nullptr, reinterpret_cast<void*>(static_cast<unsigned long>(first))) == -1)
        {
            if(errno != ESRCH)
                cbInternalError("PTRACE_DETACH failed: " + std::string(strerror(errno)));
            return;
        }
        for(std::size_t i = 1; i < signals.size(); i++)
            tgkill(tgid, tid, signals[i]);
    }

    void Debugger::releaseForeignClone(const pid_t tid, const pid_t tgid, const bool running)
    {
        releaseRunningThread(tid, tgid, !running, {});
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
            std::vector<int> signals;
            addReleaseSignal(signals, signal);
            if(leftRunning.count(tid) > 0)
                releaseRunningThread(tid, pid, owedSigstop.count(tid) > 0, std::move(signals));
            else
                releaseThread(tid, pid, signals);
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
