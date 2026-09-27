#pragma once

#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/ProcessList.h>
#include <ElfBug/process/ProcFs.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <string>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace ElfBug::test
{
    // R or S, so a zombie does not count.
    [[nodiscard]] inline bool ProcessIsRunning(const pid_t pid)
    {
        if(pid <= 0)
            return false;
        const std::string stat = procfs::ReadFile(procfs::Path(pid, "stat"));
        const auto fields = procfs::StatFields(stat);
        return fields.size() > procfs::kStatState &&
               (fields[procfs::kStatState] == "R" || fields[procfs::kStatState] == "S");
    }

    [[nodiscard]] inline bool WaitForProcessRunning(const pid_t pid,
            const std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            if(ProcessIsRunning(pid))
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    // A dying process reads R briefly, so sample again after a settle.
    [[nodiscard]] inline bool StaysRunning(const pid_t pid,
                                           const std::chrono::milliseconds settle = std::chrono::milliseconds(20))
    {
        if(!WaitForProcessRunning(pid))
            return false;
        std::this_thread::sleep_for(settle);
        return ProcessIsRunning(pid);
    }

    enum class EventType
    {
        CreateProcess,
        ExitProcess,
        CreateThread,
        ExitThread,
        SystemBreakpoint,
        AttachBreakpoint,
        Breakpoint,
        Step,
        Paused,
        Exception,
        InternalError,
        Detach,
        Exec,
    };

    struct Event
    {
        EventType type{};
        std::chrono::steady_clock::time_point when{};
        pid_t pid = 0;
        int exitCode = 0;
        ptr address = 0;
        int signal = 0;
        std::string message;
        ptr instructionPointer = 0;
    };

    // Separate from InternalError, so a test can expect silence.
    struct WaitTimeout : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    class RecordingDebugger : public Debugger
    {
    public:
        void StartOnThread()
        {
            std::packaged_task<void()> task([this] { Start(); });
            mLoopDone = task.get_future();
            mLoopThread = std::thread(std::move(task));
        }

        void JoinThread(const std::chrono::milliseconds timeout = std::chrono::seconds(10))
        {
            if(!mLoopThread.joinable())
                return;

            std::string stuck;
            if(mLoopDone.valid() && mLoopDone.wait_for(timeout) != std::future_status::ready)
            {
                stuck = loopState();
                forceStop();
            }
            mLoopThread.join();
            if(!stuck.empty())
                throw std::runtime_error("debug loop did not exit on its own: " + stuck);
        }

        ~RecordingDebugger() override
        {
            if(mLoopThread.joinable())
            {
                forceStop();
                mLoopThread.join();
            }
        }

        bool Attach(const pid_t processId)
        {
            if(!Debugger::Attach(processId))
                return false;
            mTargetPid.store(processId);
            return true;
        }

        std::vector<Event> events() const
        {
            std::lock_guard lock(mMutex);
            return mEvents;
        }

        std::size_t count(const EventType type) const
        {
            std::lock_guard lock(mMutex);
            std::size_t n = 0;
            for(const auto & e : mEvents)
                if(e.type == type) ++n;
            return n;
        }

        Process* process() const { return mProcess; }
        Thread* currentThread() const { return mThread; }

        // Runs on the tracer thread.
        void OnSystemBreakpoint(std::function<void()> fn)
        {
            mOnSystemBreakpoint = std::move(fn);
        }

        // Runs on the tracer thread at the exec stop.
        void OnExec(std::function<void()> fn)
        {
            mOnExec = std::move(fn);
        }

        // Runs on the tracer thread.
        void OnAttachBreakpoint(std::function<void()> fn)
        {
            mOnAttachBreakpoint = std::move(fn);
        }

        // Runs on the tracer thread.
        void OnDetach(std::function<void()> fn)
        {
            mOnDetach = std::move(fn);
        }

        bool WaitForRunning(const std::chrono::milliseconds timeout = std::chrono::seconds(5)) const
        {
            const pid_t pid = mProcess ? mProcess->pid : 0;
            if(pid <= 0) return false;
            const auto start = std::chrono::steady_clock::now();
            while(std::chrono::steady_clock::now() - start < timeout)
            {
                throwIfAnyInternalError();

                if(ProcessIsRunning(pid))
                    return true;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return false;
        }

        Event WaitFor(EventType type, std::chrono::milliseconds timeout = std::chrono::seconds(5))
        {
            return waitForPredicate(
            [type](const Event & e) { return e.type == type; },
            timeout, "WaitFor timeout");
        }

        Event WaitForSystemBreakpoint()    { return WaitFor(EventType::SystemBreakpoint); }
        Event WaitForAttachBreakpoint()    { return WaitFor(EventType::AttachBreakpoint); }
        Event WaitForExit()                { return WaitFor(EventType::ExitProcess); }
        Event WaitForPaused()              { return WaitFor(EventType::Paused); }
        Event WaitForStep()                { return WaitFor(EventType::Step); }
        Event WaitForInternalError()       { return WaitFor(EventType::InternalError); }
        Event WaitForDetach()              { return WaitFor(EventType::Detach); }
        Event WaitForExec()                { return WaitFor(EventType::Exec); }

        Event WaitForAny(const std::initializer_list<EventType> types, const std::chrono::milliseconds timeout = std::chrono::seconds(5))
        {
            const std::vector<EventType> wanted(types);
            return waitForPredicate(
            [wanted](const Event & e) { return std::find(wanted.begin(), wanted.end(), e.type) != wanted.end(); },
            timeout, "WaitForAny timeout");
        }

        Event WaitForException(int sig, const std::chrono::milliseconds timeout = std::chrono::seconds(5))
        {
            return waitForPredicate(
            [sig](const Event & e) { return e.type == EventType::Exception && e.signal == sig; },
            timeout, "WaitForException timeout");
        }

        Event WaitForBreakpointAt(ptr addr, const std::chrono::milliseconds timeout = std::chrono::seconds(5))
        {
            return waitForPredicate(
            [addr](const Event & e) { return e.type == EventType::Breakpoint && e.address == addr; },
            timeout, "WaitForBreakpointAt timeout");
        }

    protected:
        void cbCreateProcess(const pid_t pid, const ptr entryPoint) override
        {
            mTargetPid.store(pid);
            push({EventType::CreateProcess, {}, pid, 0, entryPoint, 0, {}});
        }

        void cbExitProcess(const int exitCode) override
        {
            push({EventType::ExitProcess, {}, 0, exitCode, 0, 0, {}});
        }

        void cbCreateThread(const pid_t tid) override
        {
            push({EventType::CreateThread, {}, tid, 0, 0, 0, {}});
        }

        void cbExitThread(const pid_t tid) override
        {
            push({EventType::ExitThread, {}, tid, 0, 0, 0, {}});
        }

        void cbSystemBreakpoint() override
        {
            if(mOnSystemBreakpoint)
                mOnSystemBreakpoint();
            push({EventType::SystemBreakpoint, {}, 0, 0, 0, 0, {}});
        }

        void cbAttachBreakpoint() override
        {
            if(mOnAttachBreakpoint)
                mOnAttachBreakpoint();
            push({EventType::AttachBreakpoint, {}, 0, 0, 0, 0, {}});
        }

        void cbBreakpoint(const BreakpointInfo & info) override
        {
            push({EventType::Breakpoint, {}, mThread ? mThread->tid : 0, 0, info.address, 0, {},
                  mThread ? mThread->registers.Gip() : 0
                 });
        }

        void cbStep() override
        {
            push({EventType::Step, {}, mThread ? mThread->tid : 0, 0, 0, 0, {},
                  mThread ? mThread->registers.Gip() : 0
                 });
        }

        void cbPaused() override
        {
            push({EventType::Paused, {}, 0, 0, 0, 0, {}});
        }

        void cbException(const int signal, const ptr address) override
        {
            push({EventType::Exception, {}, mThread ? mThread->tid : 0, 0, address, signal, {},
                  mThread ? mThread->registers.Gip() : 0
                 });
        }

        void cbInternalError(const std::string & error) override
        {
            push({EventType::InternalError, {}, 0, 0, 0, 0, error});
        }

        void cbDetach() override
        {
            if(mOnDetach)
                mOnDetach();
            push({EventType::Detach, {}, 0, 0, 0, 0, {}});
        }

        void cbExec() override
        {
            if(mOnExec)
                mOnExec();
            push({EventType::Exec, {}, 0, 0, 0, 0, {}});
        }

    private:
        // Stop() is a no-op without a main pid.
        void forceStop()
        {
            if(!Stop() && mTargetPid.load() > 0)
                kill(mTargetPid.load(), SIGKILL);
        }

        std::string loopState() const
        {
            std::string state = IsPaused() ? "paused" : "running";
            std::shared_lock lock(mProcessMutex);
            if(!mProcess)
                return state + " process=gone";

            std::string runningTids;
            std::size_t total = 0;
            for(const auto & [tid, thread] : mProcess->threads)
            {
                ++total;
                if(thread->IsRunning())
                    runningTids += (runningTids.empty() ? "" : ",") + std::to_string(tid);
            }
            return state + " threads=" + std::to_string(total) +
                   " stillRunning=[" + runningTids + "]";
        }

        [[noreturn]] static void throwInternalError(const Event & e)
        {
            throw std::runtime_error("InternalError: " + e.message);
        }

        void throwIfAnyInternalError() const
        {
            std::lock_guard lock(mMutex);
            for(const auto & e : mEvents)
                if(e.type == EventType::InternalError)
                    throwInternalError(e);
        }

        template<typename Pred>
        Event waitForPredicate(Pred pred, const std::chrono::milliseconds timeout, const char* timeoutMessage)
        {
            std::unique_lock lock(mMutex);
            const auto start = std::chrono::steady_clock::now();

            while(true)
            {
                for(std::size_t i = mConsumedUpto; i < mEvents.size(); ++i)
                {
                    if(pred(mEvents[i]))
                    {
                        mConsumedUpto = i + 1;
                        return mEvents[i];
                    }

                    if(mEvents[i].type == EventType::InternalError)
                    {
                        mConsumedUpto = i + 1;
                        throwInternalError(mEvents[i]);
                    }
                }

                const auto remaining = timeout - (std::chrono::steady_clock::now() - start);
                if(remaining <= std::chrono::milliseconds(0))
                    throw WaitTimeout(timeoutMessage);

                mCv.wait_for(lock, remaining);
            }
        }

        void push(Event e)
        {
            e.when = std::chrono::steady_clock::now();
            {
                std::lock_guard lock(mMutex);
                mEvents.push_back(std::move(e));
            }
            mCv.notify_all();
        }

        mutable std::mutex mMutex;
        std::condition_variable mCv;
        std::vector<Event> mEvents;
        std::size_t mConsumedUpto = 0;
        std::thread mLoopThread;
        std::future<void> mLoopDone;
        std::function<void()> mOnSystemBreakpoint;
        std::function<void()> mOnAttachBreakpoint;
        std::function<void()> mOnExec;
        std::function<void()> mOnDetach;
        std::atomic<pid_t> mTargetPid{0};
    };

    // Forked by the test, so yama ptrace_scope=1 allows attaching.
    class UntracedProcess
    {
    public:
        explicit UntracedProcess(const std::string & path)
        {
            const pid_t parent = getpid();
            pid = fork();
            if(pid == 0)
            {
                prctl(PR_SET_PDEATHSIG, SIGKILL);
                if(getppid() != parent)
                    _exit(127);
                execl(path.c_str(), path.c_str(), nullptr);
                _exit(127);
            }
        }

        ~UntracedProcess()
        {
            if(pid > 0)
            {
                kill(pid, SIGKILL);
                int status = 0;
                waitpid(pid, &status, __WALL);
            }
        }

        UntracedProcess(const UntracedProcess &) = delete;
        UntracedProcess & operator=(const UntracedProcess &) = delete;

        [[nodiscard]] bool Running() const
        {
            return ProcessIsRunning(pid);
        }

        [[nodiscard]] bool WaitForRunning(const std::chrono::milliseconds timeout = std::chrono::seconds(5)) const
        {
            return WaitForProcessRunning(pid, timeout);
        }

        [[nodiscard]] bool WaitForThreads(const std::size_t count,
                                          const std::chrono::milliseconds timeout = std::chrono::seconds(5)) const
        {
            const auto start = std::chrono::steady_clock::now();
            while(std::chrono::steady_clock::now() - start < timeout)
            {
                std::vector<pid_t> tids;
                if(ReadTaskList(pid, tids) && tids.size() >= count)
                    return true;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return false;
        }

        pid_t pid = -1;
    };
}
