#include "TestSupport.h"
#include <set>
#include <mutex>
#include <condition_variable>
#include <sched.h>
#include <sys/resource.h>
#include <ElfBug/api/elfbug_api.h>
#include "targets/TargetUtil.h"

namespace
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

        template<class Pred>
        bool WaitFor(Pred pred, const std::chrono::milliseconds timeout = std::chrono::seconds(5))
        {
            std::unique_lock lock(mutex);
            return cv.wait_for(lock, timeout, pred);
        }
    };

    ElfBugCallbacks MakeApiCallbacks(ApiEvents & events)
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

        explicit ApiSession(std::string fixturePath)
            : path(std::move(fixturePath))
        {
            const ElfBugCallbacks cb = MakeApiCallbacks(events);
            dbg = ElfBugCreate(&cb);
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

        std::optional<ElfBug::ptr> Resolve(const std::string & symbol)
        {
            std::lock_guard lock(events.mutex);
            return ElfBug::test::ResolveRuntimeAddress(path, events.pid, symbol);
        }
    };
}

TEST_CASE("C API reports a signal stop with the faulting registers", "[api][exception]")
{
    ApiSession s(FIXTURE("segfault"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());
    const auto site = s.Resolve("sf_fault_site");
    REQUIRE(site.has_value());

    ElfBugContinue(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.exceptionSignal.has_value(); }));

    ElfBugRegisters regs = {};
    REQUIRE(ElfBugGetRegisters(s.dbg, &regs));
    {
        std::lock_guard lock(s.events.mutex);
        REQUIRE(*s.events.exceptionSignal == SIGSEGV);
        REQUIRE(s.events.exceptionAddress == kSegfaultAddress);
    }
    REQUIRE(regs.rip == *site);
    REQUIRE(ElfBugMemIsCodePtr(s.dbg, regs.rip));

    ElfBugContinue(s.dbg);
    REQUIRE(s.WaitForExit());
    REQUIRE(*s.events.exitCode == -SIGSEGV);
}

TEST_CASE("C API arms a breakpoint queued right before Continue", "[api][breakpoint]")
{
    ApiSession s(FIXTURE("segfault"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());
    const auto site = s.Resolve("sf_fault_site");
    REQUIRE(site.has_value());

    REQUIRE(ElfBugSetBreakpoint(s.dbg, *site));
    ElfBugContinue(s.dbg);

    REQUIRE(s.events.WaitFor([&] { return s.events.breakpointAddress.has_value() || s.events.exceptionSignal.has_value(); }));
    {
        std::lock_guard lock(s.events.mutex);
        REQUIRE(s.events.breakpointAddress == site);
        REQUIRE_FALSE(s.events.exceptionSignal.has_value());
    }

    ElfBugContinue(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.exceptionSignal.has_value(); }));
    ElfBugContinue(s.dbg);
    REQUIRE(s.WaitForExit());
}

TEST_CASE("C API reports no current thread while the debuggee runs", "[api][thread]")
{
    ApiSession s(FIXTURE("run_endlessly"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());
    REQUIRE(ElfBugGetCurrentTid(s.dbg) == ElfBugGetPid(s.dbg));

    ElfBugContinue(s.dbg);
    REQUIRE(ElfBugGetCurrentTid(s.dbg) == 0);

    ElfBugPause(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.paused; }));
    REQUIRE(ElfBugGetCurrentTid(s.dbg) == ElfBugGetPid(s.dbg));
}

TEST_CASE("C API reports thread creation and exit", "[api][thread]")
{
    ApiSession s(FIXTURE("multi_threaded"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());
    ElfBugContinue(s.dbg);
    REQUIRE(s.WaitForExit());

    REQUIRE(*s.events.exitCode == 5);
    REQUIRE(ElfBugGetThreadList(s.dbg, nullptr, 0) == 0);
    const std::set<pid_t> created(s.events.createdTids.begin(), s.events.createdTids.end());
    const std::set<pid_t> exited(s.events.exitedTids.begin(), s.events.exitedTids.end());
    REQUIRE(s.events.createdTids.size() == 5);
    REQUIRE(s.events.exitedTids.size() == 5);
    REQUIRE(created.size() == 5);
    REQUIRE(exited == created);
    REQUIRE(created.count(s.events.pid) == 0);
}

TEST_CASE("C API reports which thread stopped", "[api][thread]")
{
    ApiSession s(FIXTURE("threads_spin"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());
    REQUIRE(ElfBugGetCurrentTid(s.dbg) == ElfBugGetPid(s.dbg));

    const auto site = s.Resolve("ts_worker_started");
    REQUIRE(site.has_value());
    REQUIRE(ElfBugSetBreakpoint(s.dbg, *site));

    ElfBugContinue(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.breakpointAddress.has_value(); }, std::chrono::seconds(10)));

    {
        const pid_t stoppedTid = ElfBugGetCurrentTid(s.dbg);
        std::lock_guard lock(s.events.mutex);
        REQUIRE(*s.events.breakpointAddress == *site);
        REQUIRE(stoppedTid != s.events.pid);
        REQUIRE(std::find(s.events.createdTids.begin(), s.events.createdTids.end(), stoppedTid) != s.events.createdTids.end());
    }

    REQUIRE(ElfBugStop(s.dbg));
    REQUIRE(s.WaitForExit(std::chrono::seconds(10)));
}

TEST_CASE("C API lists every thread of a paused process", "[api][thread]")
{
    const auto nowMs = []
    {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::system_clock::now().time_since_epoch()).count());
    };
    const std::uint64_t before = nowMs();
    ApiSession s(FIXTURE("threads_spin"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());

    ElfBugThreadInfo main = {};
    REQUIRE(ElfBugGetThreadList(s.dbg, &main, 1) == 1);
    REQUIRE(main.tid == ElfBugGetPid(s.dbg));
    REQUIRE(main.number == 0);
    ElfBugRegisters regs = {};
    REQUIRE(ElfBugGetRegisters(s.dbg, &regs));
    REQUIRE(main.rip == regs.rip);
    REQUIRE(main.fs_base == regs.fs_base);
    REQUIRE(std::string(main.name) == "threads_spin");
    REQUIRE(main.policy == sched_getscheduler(0));
    REQUIRE(main.nice == getpriority(PRIO_PROCESS, 0));
    REQUIRE(main.rt_priority == 0);
    REQUIRE(main.start_time_ms + 2000 >= before);
    REQUIRE(main.start_time_ms <= nowMs() + 2000);

    ElfBugContinue(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.createdTids.size() == 4; }, std::chrono::seconds(10)));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ElfBugPause(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.paused; }));

    REQUIRE(ElfBugGetThreadList(s.dbg, nullptr, 0) == 5);
    ElfBugThreadInfo three[3] = {};
    three[2].tid = -1;
    REQUIRE(ElfBugGetThreadList(s.dbg, three, 2) == 5);
    REQUIRE(three[0].number == 0);
    REQUIRE(three[1].number == 1);
    REQUIRE(three[2].tid == -1);

    ElfBugThreadInfo all[5] = {};
    REQUIRE(ElfBugGetThreadList(s.dbg, all, 5) == 5);
    std::uint64_t workerCpuMs = 0;
    std::lock_guard lock(s.events.mutex);
    for(uint32_t i = 0; i < 5; ++i)
    {
        CAPTURE(i);
        REQUIRE(all[i].number == i);
        REQUIRE(all[i].rip != 0);
        REQUIRE(all[i].fs_base != 0);
        REQUIRE(std::string(all[i].name) == "threads_spin");
        REQUIRE(all[i].policy == sched_getscheduler(0));
        REQUIRE(all[i].start_time_ms >= all[0].start_time_ms);
        if(i == 0)
            REQUIRE(all[i].tid == s.events.pid);
        else
        {
            REQUIRE(all[i].tid == s.events.createdTids[i - 1]);
            workerCpuMs += all[i].user_time_ms + all[i].kernel_time_ms;
        }
    }
    REQUIRE(workerCpuMs > 0);
}

TEST_CASE("C API switches the current thread", "[api][thread]")
{
    ApiSession s(FIXTURE("threads_spin"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());
    ElfBugContinue(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.createdTids.size() == 4; }, std::chrono::seconds(10)));
    ElfBugPause(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.paused; }));

    ElfBugThreadInfo all[5] = {};
    REQUIRE(ElfBugGetThreadList(s.dbg, all, 5) == 5);
    const pid_t before = ElfBugGetCurrentTid(s.dbg);
    const ElfBugThreadInfo* other = nullptr;
    for(const auto & t : all)
    {
        if(t.tid != before)
        {
            other = &t;
            break;
        }
    }
    REQUIRE(other != nullptr);

    REQUIRE(ElfBugSwitchThread(s.dbg, other->tid));
    REQUIRE(ElfBugGetCurrentTid(s.dbg) == other->tid);
    ElfBugRegisters regs = {};
    REQUIRE(ElfBugGetRegisters(s.dbg, &regs));
    REQUIRE(regs.rip == other->rip);

    REQUIRE_FALSE(ElfBugSwitchThread(s.dbg, 1));

    ElfBugContinue(s.dbg);
    REQUIRE_FALSE(ElfBugSwitchThread(s.dbg, before));
}

TEST_CASE("C API suspends and resumes a thread", "[api][thread][suspend]")
{
    ApiSession s(FIXTURE("threads_spin"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());
    ElfBugContinue(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.createdTids.size() == 4; }, std::chrono::seconds(10)));
    ElfBugPause(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.paused; }));

    ElfBugThreadInfo all[5] = {};
    REQUIRE(ElfBugGetThreadList(s.dbg, all, 5) == 5);
    const pid_t worker = all[1].tid;

    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, true));
    REQUIRE(ElfBugGetThreadList(s.dbg, all, 5) == 5);
    for(const auto & t : all)
        REQUIRE(t.suspend_count == (t.tid == worker ? 1u : 0u));

    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, false));
    REQUIRE(ElfBugGetThreadList(s.dbg, all, 5) == 5);
    for(const auto & t : all)
        REQUIRE(t.suspend_count == 0);

    REQUIRE_FALSE(ElfBugSetThreadSuspended(s.dbg, 1, true));
    ElfBugContinue(s.dbg);
    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, true));
    REQUIRE_FALSE(ElfBugSetThreadSuspended(s.dbg, 1, true));
}

TEST_CASE("C API nests suspend counts", "[api][thread][suspend]")
{
    ApiSession s(FIXTURE("threads_spin"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());
    ElfBugContinue(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.createdTids.size() == 4; }, std::chrono::seconds(10)));
    ElfBugPause(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.paused; }));

    ElfBugThreadInfo all[5] = {};
    REQUIRE(ElfBugGetThreadList(s.dbg, all, 5) == 5);
    const pid_t worker = all[1].tid;

    const auto countOf = [&](const pid_t tid)
    {
        ElfBugThreadInfo list[5] = {};
        REQUIRE(ElfBugGetThreadList(s.dbg, list, 5) == 5);
        for(const auto & t : list)
        {
            if(t.tid == tid)
                return t.suspend_count;
        }
        FAIL("tid missing from the thread list");
        return 0u;
    };

    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, true));
    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, true));
    REQUIRE(countOf(worker) == 2u);

    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, false));
    REQUIRE(countOf(worker) == 1u);

    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, false));
    REQUIRE(countOf(worker) == 0u);

    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, false));
    REQUIRE(countOf(worker) == 0u);
}

TEST_CASE("C API reports the wait reason at a pause", "[api][thread][waitreason]")
{
    ApiSession s(FIXTURE("threads_spin"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());
    ElfBugContinue(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.createdTids.size() == 4; }, std::chrono::seconds(10)));

    std::string mainReason;
    ElfBugThreadInfo all[5] = {};
    for(int attempt = 0; attempt < 10 && mainReason.find("nanosleep") == std::string::npos; ++attempt)
    {
        if(attempt > 0)
        {
            {
                std::lock_guard lock(s.events.mutex);
                s.events.paused = false;
            }
            ElfBugContinue(s.dbg);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        ElfBugPause(s.dbg);
        REQUIRE(s.events.WaitFor([&] { return s.events.paused; }));
        REQUIRE(ElfBugGetThreadList(s.dbg, all, 5) == 5);
        mainReason = all[0].wait_reason;
    }
    CAPTURE(mainReason);
    REQUIRE(mainReason.find("nanosleep") != std::string::npos);
    for(uint32_t i = 1; i < 5; ++i)
    {
        const std::string workerReason = all[i].wait_reason;
        CAPTURE(i, workerReason);
        REQUIRE(all[i].wait_reason[0] == '\0');
    }
}

TEST_CASE("C API shows Suspended as the wait reason", "[api][thread][suspend][waitreason]")
{
    ApiSession s(FIXTURE("threads_spin"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());
    ElfBugContinue(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.createdTids.size() == 4; }, std::chrono::seconds(10)));
    ElfBugPause(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.paused; }));

    ElfBugThreadInfo all[5] = {};
    REQUIRE(ElfBugGetThreadList(s.dbg, all, 5) == 5);
    const pid_t worker = all[1].tid;

    const auto reasonFor = [&](const pid_t tid)
    {
        ElfBugThreadInfo list[5] = {};
        REQUIRE(ElfBugGetThreadList(s.dbg, list, 5) == 5);
        for(const auto & t : list)
        {
            if(t.tid == tid)
                return std::string(t.wait_reason);
        }
        FAIL("tid missing from the thread list");
        return std::string();
    };

    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, true));
    REQUIRE(reasonFor(worker) == "Suspended");

    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, false));
    REQUIRE(reasonFor(worker) != "Suspended");
}

TEST_CASE("C API shows Suspended as the wait reason for a thread suspended while running", "[api][thread][suspend][waitreason]")
{
    ApiSession s(FIXTURE("threads_spin"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());
    ElfBugContinue(s.dbg);
    REQUIRE(s.events.WaitFor([&] { return s.events.createdTids.size() == 4; }, std::chrono::seconds(10)));

    ElfBugThreadInfo all[5] = {};
    REQUIRE(ElfBugGetThreadList(s.dbg, all, 5) == 5);
    const pid_t worker = all[1].tid;

    const auto reasonFor = [&](const pid_t tid) -> std::string
    {
        ElfBugThreadInfo list[5] = {};
        REQUIRE(ElfBugGetThreadList(s.dbg, list, 5) == 5);
        for(const auto & t : list)
        {
            if(t.tid == tid)
                return t.wait_reason;
        }
        return {};
    };

    const auto waitForReason = [&](const pid_t tid, const std::string & expected,
                                   const std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            if(reasonFor(tid) == expected)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    };

    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, true));
    REQUIRE(waitForReason(worker, "Suspended"));

    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, false));
    REQUIRE(waitForReason(worker, ""));
}

TEST_CASE("EnumProcesses reports a spawned process with its name, path and arch", "[api][attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("run_endlessly"));
    REQUIRE(target.pid > 0);
    REQUIRE(target.WaitForRunning());

    const auto list = ElfBugProcessList();
    REQUIRE(!list.empty());

    const auto it = std::find_if(list.begin(), list.end(),
    [&](const ElfBugProcessInfo & p) { return p.pid == target.pid; });
    REQUIRE(it != list.end());
    REQUIRE(std::string(it->name) == "run_endlessly");
    REQUIRE(std::filesystem::canonical(it->path) == std::filesystem::canonical(FIXTURE("run_endlessly")));
    REQUIRE(it->arch == ElfBugArch_X86_64);
    REQUIRE(it->traced == false);
}

TEST_CASE("EnumProcesses truncates at capacity and still reports the total", "[api][attach]")
{
    std::vector<ElfBugProcessInfo> list(2);
    list[1].pid = -1234;

    const uint32_t total = ElfBugEnumProcesses(list.data(), 1);
    REQUIRE(total > 1);
    REQUIRE(list[1].pid == -1234);
}

TEST_CASE("ElfBugSetRegister reaches the tracee", "[api][registers]")
{
    using namespace ElfBug::test;
    ApiSession s(FIXTURE("run_endlessly"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());

    ElfBugRegisters before{};
    REQUIRE(ElfBugGetRegisters(s.dbg, &before));

    constexpr uint64_t kValue = 0x1234567890abcdefULL;
    REQUIRE(kValue != before.r15);
    REQUIRE(ElfBugSetRegister(s.dbg, "r15", kValue));

    ElfBugRegisters after{};
    REQUIRE(ElfBugGetRegisters(s.dbg, &after));
    CHECK(after.r15 == kValue);

    CHECK_FALSE(ElfBugSetRegister(s.dbg, "nonesuch", 1));
}

TEST_CASE("C API attach hands the session over with threads and registers", "[api][attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(ElfBug::test::WaitForExeced(target.pid, FIXTURE("threads_spin")));
    REQUIRE(target.WaitForThreads(5));

    ApiSession s(FIXTURE("threads_spin"), target.pid);
    REQUIRE(s.Started());
    REQUIRE(s.WaitForAttachBreakpoint());

    REQUIRE(ElfBugIsPaused(s.dbg));
    REQUIRE(ElfBugGetPid(s.dbg) == target.pid);
    REQUIRE(ElfBugGetCurrentTid(s.dbg) != 0);

    const auto count = ElfBugGetThreadList(s.dbg, nullptr, 0);
    REQUIRE(count >= 5);
    std::vector<ElfBugThreadInfo> threads(count);
    REQUIRE(ElfBugGetThreadList(s.dbg, threads.data(), count) == count);
    for(const auto & thread : threads)
    {
        REQUIRE(thread.tid > 0);
        REQUIRE(ElfBugMemIsCodePtr(s.dbg, thread.rip));
    }

    ElfBugRegisters regs = {};
    REQUIRE(ElfBugGetRegisters(s.dbg, &regs));
    REQUIRE(ElfBugMemIsCodePtr(s.dbg, regs.rip));
}

TEST_CASE("C API detach clears the session and leaves the process running", "[api][detach]")
{
    ApiSession s(FIXTURE("run_endlessly"));
    REQUIRE(s.Started());
    REQUIRE(s.WaitForSystemBreakpoint());
    REQUIRE(ElfBugIsPaused(s.dbg));

    const pid_t pid = ElfBugGetPid(s.dbg);
    REQUIRE(pid > 0);
    REQUIRE(ElfBugGetThreadList(s.dbg, nullptr, 0) > 0);

    ElfBugDetach(s.dbg);
    REQUIRE(s.WaitForDetach());

    REQUIRE_FALSE(ElfBugIsPaused(s.dbg));
    REQUIRE(ElfBugGetPid(s.dbg) == 0);
    REQUIRE(ElfBugGetThreadList(s.dbg, nullptr, 0) == 0);

    REQUIRE(ElfBug::TracerPid(pid) == 0);
    REQUIRE(ElfBug::test::StaysRunning(pid));
    kill(pid, SIGKILL);
    int status = 0;
    waitpid(pid, &status, __WALL);
}
