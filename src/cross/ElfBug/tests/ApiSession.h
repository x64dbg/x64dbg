#pragma once

#include "TestSupport.h"
#include <ElfBug/api/elfbug_api.h>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace ElfBug::test
{
    struct ApiEvents
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool systemBreakpoint = false;
        bool attachBreakpoint = false;
        bool paused = false;
        std::optional<std::uint64_t> breakpointAddress;
        std::optional<int> exceptionSignal;
        std::uint64_t exceptionAddress = 0;
        pid_t pid = 0;
        bool detached = false;
        std::optional<int> exitCode;
        std::vector<pid_t> createdTids;
        std::vector<pid_t> exitedTids;
        ElfBugDebugger* dbg = nullptr;
        std::function<void(ElfBugDebugger*)> atSystemBreakpoint;

        template<class Pred>
        bool WaitFor(Pred pred, const std::chrono::milliseconds timeout = std::chrono::seconds(5))
        {
            std::unique_lock lock(mutex);
            return cv.wait_for(lock, timeout, pred);
        }
    };

    inline ElfBugCallbacks MakeApiCallbacks(ApiEvents & events)
    {
        ElfBugCallbacks cb = {};
        cb.userdata = &events;
        cb.onCreateProcess = [](const pid_t pid, std::uint64_t, void* userdata)
        {
            auto* ev = static_cast<ApiEvents*>(userdata);
            std::lock_guard lock(ev->mutex);
            ev->pid = pid;
        };
        cb.onPaused = [](void* userdata)
        {
            auto* ev = static_cast<ApiEvents*>(userdata);
            std::lock_guard lock(ev->mutex);
            ev->paused = true;
            ev->cv.notify_all();
        };
        cb.onSystemBreakpoint = [](void* userdata)
        {
            auto* ev = static_cast<ApiEvents*>(userdata);
            if(ev->atSystemBreakpoint)
                ev->atSystemBreakpoint(ev->dbg);
            std::lock_guard lock(ev->mutex);
            ev->systemBreakpoint = true;
            ev->cv.notify_all();
        };
        cb.onAttachBreakpoint = [](void* userdata)
        {
            auto* ev = static_cast<ApiEvents*>(userdata);
            std::lock_guard lock(ev->mutex);
            ev->attachBreakpoint = true;
            ev->cv.notify_all();
        };
        cb.onBreakpoint = [](const std::uint64_t address, void* userdata)
        {
            auto* ev = static_cast<ApiEvents*>(userdata);
            std::lock_guard lock(ev->mutex);
            ev->breakpointAddress = address;
            ev->cv.notify_all();
        };
        cb.onException = [](const int signal, const std::uint64_t address, void* userdata)
        {
            auto* ev = static_cast<ApiEvents*>(userdata);
            std::lock_guard lock(ev->mutex);
            ev->exceptionSignal = signal;
            ev->exceptionAddress = address;
            ev->cv.notify_all();
        };
        cb.onCreateThread = [](const pid_t tid, void* userdata)
        {
            auto* ev = static_cast<ApiEvents*>(userdata);
            std::lock_guard lock(ev->mutex);
            ev->createdTids.push_back(tid);
            ev->cv.notify_all();
        };
        cb.onExitThread = [](const pid_t tid, void* userdata)
        {
            auto* ev = static_cast<ApiEvents*>(userdata);
            std::lock_guard lock(ev->mutex);
            ev->exitedTids.push_back(tid);
            ev->cv.notify_all();
        };
        cb.onDetach = [](void* userdata)
        {
            auto* ev = static_cast<ApiEvents*>(userdata);
            std::lock_guard lock(ev->mutex);
            ev->detached = true;
            ev->cv.notify_all();
        };
        cb.onExitProcess = [](const int exitCode, void* userdata)
        {
            auto* ev = static_cast<ApiEvents*>(userdata);
            std::lock_guard lock(ev->mutex);
            ev->exitCode = exitCode;
            ev->cv.notify_all();
        };
        return cb;
    }

    struct ApiSession
    {
        std::string path;
        ApiEvents events;
        ElfBugDebugger* dbg = nullptr;
        std::thread loop;

        explicit ApiSession(std::string fixturePath, std::function<void(ElfBugDebugger*)> atSystemBreakpoint = {})
            : path(std::move(fixturePath))
        {
            events.atSystemBreakpoint = std::move(atSystemBreakpoint);
            const ElfBugCallbacks cb = MakeApiCallbacks(events);
            dbg = ElfBugCreate(&cb);
            events.dbg = dbg;
            if(dbg && ElfBugInit(dbg, path.c_str()))
                loop = std::thread([this] { ElfBugStart(dbg); });
        }

        ApiSession(std::string fixturePath, const pid_t attachPid)
            : path(std::move(fixturePath))
        {
            const ElfBugCallbacks cb = MakeApiCallbacks(events);
            dbg = ElfBugCreate(&cb);
            if(dbg && ElfBugAttach(dbg, attachPid))
                loop = std::thread([this] { ElfBugStart(dbg); });
        }

        [[nodiscard]] bool Started() const
        {
            return loop.joinable();
        }

        ~ApiSession()
        {
            if(loop.joinable())
            {
                ElfBugStop(dbg);
                loop.join();
            }
            ElfBugDestroy(dbg);
        }

        bool WaitForSystemBreakpoint()
        {
            return events.WaitFor([this] { return events.systemBreakpoint; });
        }

        bool WaitForAttachBreakpoint()
        {
            return events.WaitFor([this] { return events.attachBreakpoint; });
        }

        bool WaitForExit(const std::chrono::milliseconds timeout = std::chrono::seconds(5))
        {
            if(!events.WaitFor([this] { return events.exitCode.has_value(); }, timeout))
                return false;
            loop.join();
            return true;
        }

        bool WaitForDetach(const std::chrono::milliseconds timeout = std::chrono::seconds(5))
        {
            if(!events.WaitFor([this] { return events.detached; }, timeout))
                return false;
            loop.join();
            return true;
        }

        bool Pause()
        {
            {
                std::lock_guard lock(events.mutex);
                events.paused = false;
            }
            ElfBugPause(dbg);
            return events.WaitFor([this] { return events.paused; });
        }

        [[nodiscard]] ptr Symbol(const std::string & symbol)
        {
            std::lock_guard lock(events.mutex);
            const auto address = ResolveRuntimeAddress(path, events.pid, symbol);
            REQUIRE(address.has_value());
            return *address;
        }
    };
}
