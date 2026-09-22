#include "TestSupport.h"

namespace
{
    struct SpinSession
    {
        ElfBug::test::RecordingDebugger dbg;
        std::string path;
        pid_t mainTid = 0;
        std::vector<pid_t> workers;
        ElfBug::ptr counters = 0;

        SpinSession()
            : path(FIXTURE("threads_spin"))
        {
            using namespace ElfBug::test;
            REQUIRE(dbg.Init(path.c_str()));
            std::promise<std::optional<ElfBug::ptr>> promise;
            auto future = promise.get_future();
            dbg.OnSystemBreakpoint([&]
            {
                promise.set_value(ResolveRuntimeAddress(path, dbg.process()->pid, "ts_counters"));
            });
            dbg.StartOnThread();
            dbg.WaitForSystemBreakpoint();
            const auto resolved = future.get();
            REQUIRE(resolved.has_value());
            counters = *resolved;
            mainTid = dbg.process()->pid;

            dbg.Continue();
            for(int i = 0; i < 4; ++i)
                workers.push_back(dbg.WaitFor(EventType::CreateThread).pid);
            dbg.Pause();
            dbg.WaitForPaused();
        }

        std::uint64_t Sum()
        {
            std::uint64_t slots[4] = {};
            REQUIRE(dbg.process()->MemRead(counters, slots, sizeof(slots)));
            return slots[0] + slots[1] + slots[2] + slots[3];
        }

        void RunBriefly()
        {
            dbg.Continue();
            REQUIRE(dbg.WaitForRunning());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            dbg.Pause();
            dbg.WaitForPaused();
        }

        ~SpinSession()
        {
            dbg.Stop();
            try
            {
                dbg.WaitForExit();
            }
            catch(const std::exception &)
            {
            }
            dbg.JoinThread();
        }
    };

    std::optional<ElfBug::ptr> ReadStoppedPc(const pid_t tgid, const pid_t tid)
    {
        std::ifstream file("/proc/" + std::to_string(tgid) + "/task/" + std::to_string(tid) + "/syscall");
        std::string line;
        std::getline(file, line);
        const auto space = line.rfind(' ');
        if(line.empty() || line == "running" || space == std::string::npos)
            return std::nullopt;
        return static_cast<ElfBug::ptr>(std::stoull(line.substr(space + 1), nullptr, 16));
    }
}

TEST_CASE("Suspended threads stay frozen across Continue", "[multithread][suspend]")
{
    SpinSession s;
    for(const pid_t tid : s.workers)
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));

    const std::uint64_t before = s.Sum();
    s.RunBriefly();
    REQUIRE(s.Sum() == before);

    for(const pid_t tid : s.workers)
        REQUIRE(s.dbg.SetThreadSuspended(tid, false));
    s.RunBriefly();
    REQUIRE(s.Sum() > before);
}

TEST_CASE("Continue with every thread suspended reports a pause instead of running nothing", "[multithread][suspend]")
{
    SpinSession s;
    REQUIRE(s.dbg.SetThreadSuspended(s.mainTid, true));
    for(const pid_t tid : s.workers)
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));

    const std::uint64_t before = s.Sum();
    s.dbg.Continue();
    s.dbg.WaitForPaused();
    REQUIRE(s.dbg.IsPaused());
    REQUIRE(s.Sum() == before);
}

TEST_CASE("A step on a suspended thread is ignored", "[multithread][suspend][step]")
{
    SpinSession s;
    const pid_t worker = s.workers.front();
    REQUIRE(s.dbg.SetThreadSuspended(worker, true));
    REQUIRE(s.dbg.SwitchThread(worker));

    s.dbg.StepInto();
    REQUIRE_THROWS_AS(s.dbg.WaitFor(ElfBug::test::EventType::Step, std::chrono::milliseconds(300)), ElfBug::test::WaitTimeout);
    REQUIRE(s.dbg.IsPaused());

    s.dbg.StepOver();
    REQUIRE_THROWS_AS(s.dbg.WaitFor(ElfBug::test::EventType::Step, std::chrono::milliseconds(300)), ElfBug::test::WaitTimeout);
    REQUIRE(s.dbg.IsPaused());
}

TEST_CASE("A suspend landing on a queued step drops the step and reports a pause", "[multithread][suspend][step]")
{
    using namespace ElfBug::test;
    SpinSession s;
    const pid_t worker = s.workers.front();
    REQUIRE(s.dbg.SwitchThread(worker));
    const ElfBug::Thread* thread = s.dbg.process()->threads.at(worker).get();

    bool dropped = false;
    for(int round = 0; round < 50 && !dropped; ++round)
    {
        CAPTURE(round);
        const auto before = ReadStoppedPc(s.mainTid, worker);
        REQUIRE(before.has_value());

        s.dbg.StepInto();
        REQUIRE(s.dbg.SetThreadSuspended(worker, true));

        const Event event = s.dbg.WaitForAny({EventType::Step, EventType::Paused});
        if(event.type == EventType::Paused)
        {
            dropped = true;
            REQUIRE(s.dbg.IsPaused());
            REQUIRE(thread->IsSuspended());
            const auto after = ReadStoppedPc(s.mainTid, worker);
            REQUIRE(after.has_value());
            REQUIRE(*after == *before);
            REQUIRE(s.dbg.SetThreadSuspended(worker, false));
            break;
        }

        REQUIRE(event.pid == worker);
        REQUIRE(s.dbg.SetThreadSuspended(worker, false));
        s.RunBriefly();
        REQUIRE(s.dbg.SwitchThread(worker));
    }
    REQUIRE(dropped);
}

TEST_CASE("SetThreadSuspended refuses an unknown tid", "[multithread][suspend]")
{
    SpinSession s;
    REQUIRE_FALSE(s.dbg.SetThreadSuspended(1, true));
    s.dbg.Continue();
    REQUIRE(s.dbg.WaitForRunning());
    REQUIRE_FALSE(s.dbg.SetThreadSuspended(1, true));
    s.dbg.Pause();
    s.dbg.WaitForPaused();
}

TEST_CASE("Suspending workers while running freezes them", "[multithread][suspend]")
{
    SpinSession s;
    s.dbg.Continue();
    REQUIRE(s.dbg.WaitForRunning());

    for(const pid_t tid : s.workers)
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    s.dbg.Pause();
    s.dbg.WaitForPaused();
    const std::uint64_t before = s.Sum();

    s.RunBriefly();
    REQUIRE(s.Sum() == before);
}

TEST_CASE("Suspending the last running thread reports a pause", "[multithread][suspend]")
{
    SpinSession s;
    s.dbg.Continue();
    REQUIRE(s.dbg.WaitForRunning());

    for(const pid_t tid : s.workers)
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));
    REQUIRE(s.dbg.SetThreadSuspended(s.mainTid, true));

    s.dbg.WaitForPaused();
    REQUIRE(s.dbg.IsPaused());
}

TEST_CASE("Suspending every thread right after Continue still reports a pause", "[multithread][suspend]")
{
    SpinSession s;

    for(int round = 0; round < 30; ++round)
    {
        CAPTURE(round);
        s.dbg.Continue();
        for(const pid_t tid : s.workers)
            REQUIRE(s.dbg.SetThreadSuspended(tid, true));
        REQUIRE(s.dbg.SetThreadSuspended(s.mainTid, true));

        s.dbg.WaitForPaused();
        REQUIRE(s.dbg.IsPaused());
        for(const pid_t tid : s.workers)
            REQUIRE_FALSE(s.dbg.process()->threads.at(tid)->IsRunning());

        for(const pid_t tid : s.workers)
            REQUIRE(s.dbg.SetThreadSuspended(tid, false));
        REQUIRE(s.dbg.SetThreadSuspended(s.mainTid, false));
    }
}

TEST_CASE("Resuming a worker while running unfreezes it", "[multithread][suspend]")
{
    SpinSession s;
    for(const pid_t tid : s.workers)
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));

    s.dbg.Continue();
    REQUIRE(s.dbg.WaitForRunning());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    s.dbg.Pause();
    s.dbg.WaitForPaused();
    const std::uint64_t frozen = s.Sum();

    s.dbg.Continue();
    REQUIRE(s.dbg.WaitForRunning());
    for(const pid_t tid : s.workers)
        REQUIRE(s.dbg.SetThreadSuspended(tid, false));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE_FALSE(s.dbg.IsPaused());

    s.dbg.Pause();
    s.dbg.WaitForPaused();
    REQUIRE(s.Sum() > frozen);
}

TEST_CASE("Resuming a thread suspended at its own breakpoint steps off it first", "[multithread][suspend][breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("threads_spin");
    REQUIRE(dbg.Init(path.c_str()));

    std::optional<ElfBug::ptr> site;
    dbg.OnSystemBreakpoint([&]
    {
        site = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_worker_started");
        if(site)
            dbg.process()->SetBreakpoint(*site, false, ElfBug::SoftwareType::ShortInt3);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    REQUIRE(site.has_value());

    pid_t target = 0;
    for(int i = 0; i < 4; ++i)
    {
        dbg.Continue();
        target = dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10)).pid;
    }

    REQUIRE(dbg.SetThreadSuspended(target, true));
    dbg.Continue();
    REQUIRE(dbg.WaitForRunning());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE_FALSE(dbg.IsPaused());

    REQUIRE(dbg.SetThreadSuspended(target, false));

    REQUIRE_THROWS_AS(dbg.WaitForBreakpointAt(*site, std::chrono::milliseconds(500)), WaitTimeout);

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("A signal parked on a thread suspended at resume is delivered when it resumes running", "[multithread][suspend][exception]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("threads_spin");
    REQUIRE(dbg.Init(path.c_str()));

    struct Sites
    {
        std::optional<ElfBug::ptr> armed;
        std::optional<ElfBug::ptr> handlerTid;
    };
    std::promise<Sites> promise;
    auto future = promise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const pid_t pid = dbg.process()->pid;
        promise.set_value({
            ResolveRuntimeAddress(path, pid, "ts_fault_armed"),
            ResolveRuntimeAddress(path, pid, "ts_fault_handler_tid")});
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto s = future.get();
    REQUIRE(s.armed.has_value());
    REQUIRE(s.handlerTid.has_value());
    const pid_t mainTid = dbg.process()->pid;

    dbg.Continue();
    for(int i = 0; i < 4; ++i)
        dbg.WaitFor(EventType::CreateThread);
    dbg.Pause();
    dbg.WaitForPaused();

    constexpr int armed = 1;
    REQUIRE(dbg.process()->MemWrite(*s.armed, &armed, sizeof(armed)));
    dbg.Continue();
    const Event fault = dbg.WaitForException(SIGSEGV, std::chrono::seconds(10));
    REQUIRE(fault.pid == mainTid);

    constexpr int disarmed = 0;
    REQUIRE(dbg.process()->MemWrite(*s.armed, &disarmed, sizeof(disarmed)));

    REQUIRE(dbg.SetThreadSuspended(mainTid, true));
    dbg.Continue();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE_FALSE(dbg.IsPaused());

    REQUIRE(dbg.SetThreadSuspended(mainTid, false));

    int handlerTid = 0;
    for(int attempt = 0; attempt < 50 && handlerTid == 0; ++attempt)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        REQUIRE(dbg.process()->MemRead(*s.handlerTid, &handlerTid, sizeof(handlerTid)));
    }
    REQUIRE(handlerTid == mainTid);

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("Overlapping suspend requests for a running tid both nest", "[multithread][suspend]")
{
    SpinSession s;
    s.dbg.Continue();
    REQUIRE(s.dbg.WaitForRunning());

    for(const pid_t tid : s.workers)
    {
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    s.dbg.Pause();
    s.dbg.WaitForPaused();
    const std::uint64_t before = s.Sum();

    for(const pid_t tid : s.workers)
        REQUIRE(s.dbg.SetThreadSuspended(tid, false));
    s.RunBriefly();
    REQUIRE(s.Sum() == before);

    for(const pid_t tid : s.workers)
        REQUIRE(s.dbg.SetThreadSuspended(tid, false));
    s.RunBriefly();
    REQUIRE(s.Sum() > before);
}

TEST_CASE("A resume issued right before a running suspend lands is not lost", "[multithread][suspend]")
{
    SpinSession s;
    s.dbg.Continue();
    REQUIRE(s.dbg.WaitForRunning());

    for(const pid_t tid : s.workers)
    {
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));
        REQUIRE(s.dbg.SetThreadSuspended(tid, false));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    s.dbg.Pause();
    s.dbg.WaitForPaused();
    for(const pid_t tid : s.workers)
    {
        CAPTURE(tid);
        REQUIRE_FALSE(s.dbg.process()->threads.at(tid)->IsSuspended());
    }
    std::uint64_t before[4] = {};
    REQUIRE(s.dbg.process()->MemRead(s.counters, before, sizeof(before)));

    s.RunBriefly();
    std::uint64_t after[4] = {};
    REQUIRE(s.dbg.process()->MemRead(s.counters, after, sizeof(after)));
    for(int i = 0; i < 4; ++i)
    {
        CAPTURE(i);
        REQUIRE(after[i] > before[i]);
    }
}

TEST_CASE("Queued breakpoints on suspended threads wait for the resume", "[multithread][suspend][breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("threads_spin");
    REQUIRE(dbg.Init(path.c_str()));

    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_worker_started");
        if(site)
            dbg.process()->SetBreakpoint(*site, false, ElfBug::SoftwareType::ShortInt3);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();

    dbg.Continue();
    const Event first = dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));

    std::vector<pid_t> others;
    for(const auto & e : dbg.events())
        if(e.type == EventType::CreateThread && e.pid != first.pid)
            others.push_back(e.pid);
    for(const pid_t tid : others)
        REQUIRE(dbg.SetThreadSuspended(tid, true));

    for(;;)
    {
        dbg.Continue();
        Event hit{};
        try
        {
            hit = dbg.WaitFor(EventType::Breakpoint, std::chrono::milliseconds(500));
        }
        catch(const WaitTimeout &)
        {
            break;
        }
        REQUIRE(std::find(others.begin(), others.end(), hit.pid) == others.end());
    }

    dbg.Pause();
    dbg.WaitForPaused();
    for(const pid_t tid : others)
        REQUIRE(dbg.SetThreadSuspended(tid, false));

    std::size_t hits = 0;
    for(std::size_t i = 0; i < others.size(); ++i)
    {
        dbg.Continue();
        const Event hit = dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));
        REQUIRE(std::find(others.begin(), others.end(), hit.pid) != others.end());
        ++hits;
    }
    REQUIRE(hits == others.size());

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("The sweep keeps Suspended for a thread whose requested stop has not landed", "[multithread][suspend][waitreason]")
{
    SpinSession s;

    for(int round = 0; round < 30; ++round)
    {
        CAPTURE(round);
        s.dbg.Continue();
        REQUIRE(s.dbg.WaitForRunning());

        const pid_t worker = s.workers[round % 4];
        REQUIRE(s.dbg.SetThreadSuspended(worker, true));
        s.dbg.Pause();
        s.dbg.WaitForPaused();
        REQUIRE(s.dbg.IsPaused());

        const ElfBug::Thread* thread = s.dbg.process()->threads.at(worker).get();
        REQUIRE(thread->IsSuspended());
        CAPTURE(thread->WaitReason());
        REQUIRE(thread->WaitReason() == "Suspended");

        REQUIRE(s.dbg.SetThreadSuspended(worker, false));
    }
}
