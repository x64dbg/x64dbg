#pragma once

#include <ElfBug/core/Debugger.h>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace ElfBug::test
{
    enum class EventType
    {
        CreateProcess,
        ExitProcess,
        CreateThread,
        ExitThread,
        SystemBreakpoint,
        Breakpoint,
        Step,
        Paused,
        Exception,
        InternalError,
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

    class RecordingDebugger : public Debugger
    {
    public:
        void StartOnThread()
        {
            mLoopThread = std::thread([this] { Start(); });
        }

        void JoinThread()
        {
            if(mLoopThread.joinable())
                mLoopThread.join();
        }

        ~RecordingDebugger() override
        {
            if(mLoopThread.joinable())
            {
                Stop();
                mLoopThread.join();
            }
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

        // Runs on the tracer thread, before the event is published.
        void OnSystemBreakpoint(std::function<void()> fn)
        {
            mOnSystemBreakpoint = std::move(fn);
        }

        // /proc/<pid>/stat state char: R/S = scheduled, t/T = ptrace-stopped.
        bool WaitForRunning(const std::chrono::milliseconds timeout = std::chrono::seconds(5)) const
        {
            const pid_t pid = mProcess ? mProcess->pid : 0;
            if(pid <= 0) return false;
            const auto start = std::chrono::steady_clock::now();
            while(std::chrono::steady_clock::now() - start < timeout)
            {
                throwIfAnyInternalError();

                std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");
                std::string data;
                std::getline(stat, data);
                const auto lastParen = data.rfind(')');
                if(lastParen != std::string::npos && lastParen + 2 < data.size())
                {
                    const char state = data[lastParen + 2];
                    if(state == 'R' || state == 'S') return true;
                }
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
        Event WaitForExit()                { return WaitFor(EventType::ExitProcess); }
        Event WaitForPaused()              { return WaitFor(EventType::Paused); }
        Event WaitForStep()                { return WaitFor(EventType::Step); }
        Event WaitForInternalError()       { return WaitFor(EventType::InternalError); }

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
        void cbCreateProcessEvent(const pid_t pid, const ptr entryPoint) override
        {
            push({EventType::CreateProcess, {}, pid, 0, entryPoint, 0, {}});
        }

        void cbExitProcessEvent(const int exitCode) override
        {
            push({EventType::ExitProcess, {}, 0, exitCode, 0, 0, {}});
        }

        void cbCreateThreadEvent(const pid_t tid) override
        {
            push({EventType::CreateThread, {}, tid, 0, 0, 0, {}});
        }

        void cbExitThreadEvent(const pid_t tid) override
        {
            push({EventType::ExitThread, {}, tid, 0, 0, 0, {}});
        }

        void cbSystemBreakpoint() override
        {
            if(mOnSystemBreakpoint)
                mOnSystemBreakpoint();
            push({EventType::SystemBreakpoint, {}, 0, 0, 0, 0, {}});
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

        void cbExceptionEvent(const int signal, const ptr address) override
        {
            push({EventType::Exception, {}, mThread ? mThread->tid : 0, 0, address, signal, {},
                  mThread ? mThread->registers.Gip() : 0
                 });
        }

        void cbInternalError(const std::string & error) override
        {
            push({EventType::InternalError, {}, 0, 0, 0, 0, error});
        }

    private:
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
                    throw std::runtime_error(timeoutMessage);

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
        std::function<void()> mOnSystemBreakpoint;
    };
}
