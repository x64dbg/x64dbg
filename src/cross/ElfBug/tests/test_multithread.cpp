#include "TestSupport.h"
#include <set>
#include "targets/TargetUtil.h"

TEST_CASE("Multi-threaded: clone events per worker thread", "[thread]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("multi_threaded").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();

    const auto events = dbg.events();
    std::set<pid_t> createdTids;
    std::set<pid_t> exitedTids;
    for(const auto & e : events)
    {
        if(e.type == EventType::CreateThread)
            createdTids.insert(e.pid);
        else if(e.type == EventType::ExitThread)
            exitedTids.insert(e.pid);
    }

    REQUIRE(exit_ev.exitCode == 5);
    REQUIRE(dbg.count(EventType::CreateThread) == 5);
    REQUIRE(dbg.count(EventType::ExitThread) == 5);
    REQUIRE(createdTids.size() == 5);
    REQUIRE(exitedTids == createdTids);
    REQUIRE(dbg.count(EventType::InternalError) == 0);
}

TEST_CASE("Breakpoint on a hot path does not lose thread-creation events", "[multithread]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("multi_threaded");
    REQUIRE(dbg.Init(path.c_str()));

    dbg.OnSystemBreakpoint([&]
    {
        const auto worker = ResolveRuntimeAddress(path, dbg.process()->pid, "_ZL6workerPv");
        if(worker)
            dbg.process()->SetBreakpoint(*worker, false, ElfBug::SoftwareType::ShortInt3);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();

    for(int i = 0; i < 5; ++i)
    {
        dbg.Continue();
        dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));
    }

    dbg.Continue();
    const auto exit_ev = dbg.WaitFor(EventType::ExitProcess, std::chrono::seconds(20));

    std::set<pid_t> createdTids;
    for(const auto & e : dbg.events())
        if(e.type == EventType::CreateThread)
            createdTids.insert(e.pid);

    REQUIRE(exit_ev.exitCode == 5);
    REQUIRE(createdTids.size() == 5);
    REQUIRE(dbg.count(EventType::Breakpoint) == 5);
    REQUIRE(dbg.count(EventType::InternalError) == 0);
}

TEST_CASE("threads_spin starts four long-lived workers", "[multithread]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("threads_spin");
    REQUIRE(dbg.Init(path.c_str()));

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();

    for(int i = 0; i < 4; ++i)
        dbg.WaitFor(EventType::CreateThread, std::chrono::seconds(10));

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(dbg.count(EventType::InternalError) == 0);
}

namespace
{
    std::uint64_t ReadSpinCounters(const ElfBug::Process* process, const ElfBug::ptr base)
    {
        std::uint64_t slots[4] = {};
        if(!process->MemRead(base, slots, sizeof(slots)))
            return 0;
        return slots[0] + slots[1] + slots[2] + slots[3];
    }
}

TEST_CASE("Killing the group leader during a pause always reports the process exit", "[multithread][exit]")
{
    using namespace ElfBug::test;
    for(int attempt = 0; attempt < 8; ++attempt)
    {
        RecordingDebugger dbg;
        REQUIRE(dbg.Init(FIXTURE("clone_relay").c_str()));
        dbg.StartOnThread();
        dbg.WaitForSystemBreakpoint();
        dbg.Continue();
        REQUIRE(dbg.WaitForRunning());
        REQUIRE(dbg.process() != nullptr);
        const pid_t leader = dbg.process()->pid;

        std::vector<pid_t> tids;
        for(int i = 0; i < 200 && tids.size() < 24; ++i)
        {
            ElfBug::ReadTaskList(leader, tids);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        CAPTURE(tids.size());

        std::thread killer([leader, attempt]
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100 + 150 * attempt));
            kill(leader, SIGKILL);
        });
        dbg.Pause();
        killer.join();

        CAPTURE(attempt);
        const auto first = dbg.WaitForAny({EventType::ExitProcess, EventType::Paused},
                                          std::chrono::seconds(10));
        if(first.type == EventType::Paused)
            dbg.Continue();

        const auto exit_ev = first.type == EventType::ExitProcess
                             ? first
                             : dbg.WaitFor(EventType::ExitProcess, std::chrono::seconds(10));
        dbg.JoinThread();
        REQUIRE(exit_ev.exitCode == -SIGKILL);
    }
}

TEST_CASE("All threads freeze when Pause stops the process", "[multithread]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("threads_spin");
    REQUIRE(dbg.Init(path.c_str()));

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();

    for(int i = 0; i < 4; ++i)
        dbg.WaitFor(EventType::CreateThread, std::chrono::seconds(10));

    const auto counters = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_counters");
    REQUIRE(counters.has_value());

    dbg.Pause();
    dbg.WaitForPaused();

    std::uint64_t before[4] = {};
    std::uint64_t after[4] = {};
    REQUIRE(dbg.process()->MemRead(*counters, before, sizeof(before)));
    REQUIRE(ReadSpinCounters(dbg.process(), *counters) == before[0] + before[1] + before[2] + before[3]);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(dbg.process()->MemRead(*counters, after, sizeof(after)));

    for(int i = 0; i < 4; ++i)
        REQUIRE(after[i] == before[i]);

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("Continue resumes every thread", "[multithread]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("threads_spin");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> promise;
    auto future = promise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto started = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_worker_started");
        if(started)
            dbg.process()->SetBreakpoint(*started, true, ElfBug::SoftwareType::ShortInt3);
        promise.set_value(ResolveRuntimeAddress(path, dbg.process()->pid, "ts_counters"));
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto counters = future.get();
    REQUIRE(counters.has_value());

    dbg.Continue();
    dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));

    std::uint64_t stopped[4] = {};
    std::uint64_t stillStopped[4] = {};
    REQUIRE(dbg.process()->MemRead(*counters, stopped, sizeof(stopped)));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(dbg.process()->MemRead(*counters, stillStopped, sizeof(stillStopped)));

    for(int i = 0; i < 4; ++i)
        REQUIRE(stillStopped[i] == stopped[i]);

    dbg.Continue();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::uint64_t running[4] = {};
    REQUIRE(dbg.process()->MemRead(*counters, running, sizeof(running)));

    for(int i = 0; i < 4; ++i)
        REQUIRE(running[i] > stopped[i]);

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("Breakpoints absorbed by the stop sweep are still reported", "[multithread]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("threads_spin");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> promise;
    auto future = promise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto started = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_worker_started");
        if(started)
            dbg.process()->SetBreakpoint(*started, false, ElfBug::SoftwareType::ShortInt3);
        promise.set_value(started);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    REQUIRE(future.get().has_value());

    for(int i = 0; i < 4; ++i)
    {
        dbg.Continue();
        dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));
    }

    REQUIRE(dbg.count(EventType::Breakpoint) == 4);

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(dbg.count(EventType::InternalError) == 0);
}

namespace
{
    enum class StepShape
    {
        Into,
        Over
    };

    void RequireOtherThreadsFrozenAcrossStep(const StepShape shape)
    {
        using namespace ElfBug::test;
        RecordingDebugger dbg;
        const std::string path = FIXTURE("threads_spin");
        REQUIRE(dbg.Init(path.c_str()));

        struct Sites
        {
            std::optional<ElfBug::ptr> tick;
            std::optional<ElfBug::ptr> counters;
        };

        std::promise<Sites> promise;
        auto future = promise.get_future();
        dbg.OnSystemBreakpoint([&]
        {
            Sites s;
            s.tick = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_tick");
            s.counters = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_counters");
            if(s.tick)
                dbg.process()->SetBreakpoint(*s.tick, false, ElfBug::SoftwareType::ShortInt3);
            promise.set_value(s);
        });

        dbg.StartOnThread();
        dbg.WaitForSystemBreakpoint();
        const auto s = future.get();
        REQUIRE(s.tick.has_value());
        REQUIRE(s.counters.has_value());

        std::uint64_t before[4] = {};
        std::uint64_t after[4] = {};

        int spinning = 0;
        for(int round = 0; round < 50 && spinning < 4; ++round)
        {
            dbg.Continue();
            dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));

            spinning = 0;
            if(dbg.process()->MemRead(*s.counters, before, sizeof(before)))
                for(const auto counter : before)
                    if(counter > 0)
                        ++spinning;
        }

        REQUIRE(spinning == 4);

        REQUIRE(dbg.process()->MemRead(*s.counters, before, sizeof(before)));

        if(shape == StepShape::Over)
            dbg.StepOver();
        else
            dbg.StepInto();
        dbg.WaitForStep();

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        REQUIRE(dbg.process()->MemRead(*s.counters, after, sizeof(after)));
        for(int i = 0; i < 4; ++i)
            REQUIRE(after[i] == before[i]);

        dbg.Stop();
        dbg.WaitForExit();
        dbg.JoinThread();
        REQUIRE(dbg.count(EventType::InternalError) == 0);
    }
}

TEST_CASE("Other threads do not run across a step", "[multithread][step]")
{
    RequireOtherThreadsFrozenAcrossStep(StepShape::Into);
}

TEST_CASE("Other threads do not run across a step-over", "[multithread][stepover]")
{
    RequireOtherThreadsFrozenAcrossStep(StepShape::Over);
}

TEST_CASE("A breakpoint hit before the thread's clone event is still a breakpoint", "[multithread][breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("clone_trap");
    REQUIRE(dbg.Init(path.c_str()));

    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "ct_site");
        if(site)
            dbg.process()->SetBreakpoint(*site, false, ElfBug::SoftwareType::ShortInt3);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();

    Event last;
    for(int round = 0; round < 1000; ++round)
    {
        dbg.Continue();
        last = dbg.WaitForAny({EventType::Breakpoint, EventType::ExitProcess, EventType::Exception},
                              std::chrono::seconds(10));
        if(last.type == EventType::ExitProcess)
            break;
    }
    dbg.JoinThread();

    REQUIRE(last.type == EventType::ExitProcess);
    REQUIRE(last.exitCode == 0);
    REQUIRE(dbg.count(EventType::Exception) == 0);
    REQUIRE(dbg.count(EventType::Breakpoint) == 2 * kCloneTrapRounds);
    REQUIRE(dbg.count(EventType::InternalError) == 0);
}

TEST_CASE("A process exit racing the stop sweep is still reported", "[multithread][process]")
{
    using namespace ElfBug::test;

    for(int attempt = 0; attempt < 8; ++attempt)
    {
        RecordingDebugger dbg;
        const std::string path = FIXTURE("exit_race");
        REQUIRE(dbg.Init(path.c_str()));

        struct Sites
        {
            std::optional<ElfBug::ptr> hot;
            std::optional<ElfBug::ptr> exitNow;
        };

        std::promise<Sites> promise;
        auto future = promise.get_future();
        dbg.OnSystemBreakpoint([&]
        {
            Sites s;
            s.hot = ResolveRuntimeAddress(path, dbg.process()->pid, "er_hot");
            s.exitNow = ResolveRuntimeAddress(path, dbg.process()->pid, "er_exit_now");
            if(s.hot)
                dbg.process()->SetBreakpoint(*s.hot, false, ElfBug::SoftwareType::ShortInt3);
            promise.set_value(s);
        });

        dbg.StartOnThread();
        dbg.WaitForSystemBreakpoint();
        const auto s = future.get();
        REQUIRE(s.hot.has_value());
        REQUIRE(s.exitNow.has_value());

        dbg.Continue();
        dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));

        const int go = 1;
        REQUIRE(dbg.process()->MemWrite(*s.exitNow, &go, sizeof(go)));

        Event last;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while(std::chrono::steady_clock::now() < deadline)
        {
            dbg.Continue();
            last = dbg.WaitForAny({EventType::Breakpoint, EventType::ExitProcess, EventType::Exception},
                                  std::chrono::seconds(10));
            if(last.type == EventType::ExitProcess)
                break;
        }

        dbg.JoinThread();

        REQUIRE(last.type == EventType::ExitProcess);
        REQUIRE(last.exitCode == 7);
        REQUIRE(dbg.count(EventType::Exception) == 0);
        REQUIRE(dbg.count(EventType::InternalError) == 0);
    }
}

TEST_CASE("Stop while a hot-path breakpoint is armed still reports the exit", "[multithread][process]")
{
    using namespace ElfBug::test;

    for(int attempt = 0; attempt < 5; ++attempt)
    {
        RecordingDebugger dbg;
        const std::string path = FIXTURE("exit_race");
        REQUIRE(dbg.Init(path.c_str()));

        std::promise<std::optional<ElfBug::ptr>> promise;
        auto future = promise.get_future();
        dbg.OnSystemBreakpoint([&]
        {
            const auto hot = ResolveRuntimeAddress(path, dbg.process()->pid, "er_hot");
            if(hot)
                dbg.process()->SetBreakpoint(*hot, false, ElfBug::SoftwareType::ShortInt3);
            promise.set_value(hot);
        });

        dbg.StartOnThread();
        dbg.WaitForSystemBreakpoint();
        REQUIRE(future.get().has_value());

        dbg.Continue();
        dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));

        REQUIRE(dbg.Stop());
        const auto exit_ev = dbg.WaitFor(EventType::ExitProcess, std::chrono::seconds(10));
        dbg.JoinThread();

        REQUIRE(exit_ev.exitCode == -SIGKILL);
        REQUIRE(dbg.count(EventType::InternalError) == 0);
    }
}

TEST_CASE("The sweep records where a frozen thread was blocked", "[multithread][waitreason]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("threads_spin");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> promise;
    auto future = promise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        promise.set_value(ResolveRuntimeAddress(path, dbg.process()->pid, "ts_worker_tick"));
    });
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto site = future.get();
    REQUIRE(site.has_value());
    const pid_t mainTid = dbg.process()->pid;

    dbg.Continue();
    for(int i = 0; i < 4; ++i)
        dbg.WaitFor(EventType::CreateThread);
    dbg.Pause();
    dbg.WaitForPaused();
    REQUIRE(dbg.process()->SetBreakpoint(*site, false, ElfBug::SoftwareType::ShortInt3));

    std::string mainReason;
    std::string workerReason = "unset";
    for(int attempt = 0; attempt < 10 && mainReason.empty(); ++attempt)
    {
        dbg.Continue();
        const Event hit = dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));
        REQUIRE(hit.pid != mainTid);
        mainReason = dbg.process()->threads.at(mainTid)->WaitReason();
        workerReason = dbg.process()->threads.at(hit.pid)->WaitReason();
    }
    CAPTURE(mainReason);
    REQUIRE(mainReason.find("nanosleep") != std::string::npos);
    REQUIRE(workerReason.empty());

    REQUIRE(dbg.process()->DeleteBreakpoint(*site));
    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("Stepping a thread off a queued breakpoint consumes the hit", "[multithread][breakpoint][step]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("clone_trap");
    REQUIRE(dbg.Init(path.c_str()));

    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "ct_site");
        if(site)
            dbg.process()->SetBreakpoint(*site, false, ElfBug::SoftwareType::ShortInt3);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const pid_t mainTid = dbg.process()->pid;

    pid_t queued = 0;
    Event last;
    for(int round = 0; round < 1000 && queued == 0; ++round)
    {
        dbg.Continue();
        last = dbg.WaitForAny({EventType::Breakpoint, EventType::ExitProcess, EventType::Exception},
                              std::chrono::seconds(10));
        if(last.type != EventType::Breakpoint)
            break;
        for(const auto & [tid, thread] : dbg.process()->threads)
        {
            if(tid != mainTid && tid != last.pid && thread->HasPendingBreakpoint())
            {
                queued = tid;
                break;
            }
        }
    }
    REQUIRE(queued != 0);

    REQUIRE(dbg.SwitchThread(queued));
    dbg.StepInto();
    REQUIRE(dbg.WaitForStep().pid == queued);
    const std::size_t hitsBeforeRun = dbg.count(EventType::Breakpoint);

    for(int round = 0; round < 1000; ++round)
    {
        dbg.Continue();
        last = dbg.WaitForAny({EventType::Breakpoint, EventType::ExitProcess, EventType::Exception},
                              std::chrono::seconds(10));
        if(last.type == EventType::ExitProcess)
            break;
        REQUIRE(last.type == EventType::Breakpoint);
        REQUIRE(last.pid != queued);
    }
    dbg.JoinThread();

    REQUIRE(last.type == EventType::ExitProcess);
    REQUIRE(last.exitCode == 0);
    REQUIRE(dbg.count(EventType::Exception) == 0);
    REQUIRE(dbg.count(EventType::Breakpoint) == 2 * kCloneTrapRounds - 1);
    REQUIRE(dbg.count(EventType::InternalError) == 0);
    REQUIRE(hitsBeforeRun >= 1);
}

TEST_CASE("A clone outside the thread group is not registered as a thread", "[multithread]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("clone_process").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();
    REQUIRE(dbg.WaitForRunning());

    REQUIRE(dbg.process() != nullptr);
    const pid_t inferiorPid = dbg.process()->pid;

    const pid_t child = WaitForClonedChild(inferiorPid);
    CAPTURE(inferiorPid, child);
    REQUIRE(child > 0);

    dbg.Pause();
    dbg.WaitForPaused();

    REQUIRE(dbg.process()->threads.size() == 1);
    REQUIRE(dbg.process()->threads.count(child) == 0);
    REQUIRE(dbg.count(EventType::CreateThread) == 0);

    REQUIRE(ElfBug::TracerPid(child) == 0);

    REQUIRE(dbg.Stop());
    dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(StaysRunning(child));

    kill(child, SIGKILL);
    int status = 0;
    waitpid(child, &status, __WALL);
}
