#include "TestSupport.h"

namespace
{
    namespace procfs = ElfBug::procfs;

    struct SpinSession
    {
        ElfBug::test::RecordingDebugger dbg;
        std::string path;
        pid_t mainTid = 0;
        // workers[i] owns ts_counters[i].
        std::vector<pid_t> workers;
        ElfBug::ptr counters = 0;

        // Never suspended by the frozen checks.
        static constexpr std::size_t kWitness = 0;
        static constexpr std::uint64_t kWitnessWork = 1u << 22;

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

        ElfBug::test::SpinSlots Read() const
        {
            return ElfBug::test::ReadSpinSlots(dbg.process(), counters);
        }

        std::vector<pid_t> Frozen() const
        {
            return {workers.begin() + kWitness + 1, workers.end()};
        }

        bool WaitForWitness(const ElfBug::test::SpinSlots & since) const
        {
            return ElfBug::test::WaitForSpinSlots(dbg.process(), counters, [&](const ElfBug::test::SpinSlots & now)
            {
                return now[kWitness] >= since[kWitness] + kWitnessWork;
            });
        }

        void RequireFrozenSince(const ElfBug::test::SpinSlots & before) const
        {
            const auto now = Read();
            for(std::size_t i = kWitness + 1; i < now.size(); ++i)
            {
                CAPTURE(i);
                REQUIRE(now[i] == before[i]);
            }
        }

        void RunWitness()
        {
            const auto since = Read();
            dbg.Continue();
            REQUIRE(WaitForWitness(since));
            dbg.Pause();
            dbg.WaitForPaused();
        }

        void RunUntilEveryWorkerMoves()
        {
            const auto since = Read();
            dbg.Continue();
            REQUIRE(ElfBug::test::WaitForEverySlotPast(dbg.process(), counters, since));
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
        const std::string syscall = procfs::ReadLine(procfs::TaskPath(tgid, tid, "syscall"));
        const auto fields = procfs::Split(syscall, ' ');
        if(fields.size() < 3 || !fields.back().starts_with("0x"))
            return std::nullopt;
        return procfs::ParseNumber<ElfBug::ptr>(fields.back().substr(2), 16);
    }

    bool WaitForSigstopTaken(const pid_t tgid, const pid_t tid)
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < std::chrono::seconds(5))
        {
            const std::string status = procfs::ReadFile(procfs::TaskPath(tgid, tid, "status"));
            const auto pending = procfs::ParseNumber<std::uint64_t>(procfs::FindValue(status, "SigPnd:"), 16);
            if(pending && (*pending & (1ull << (SIGSTOP - 1))) == 0)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }
}

TEST_CASE("Suspended threads stay frozen across Continue", "[multithread][suspend]")
{
    SpinSession s;
    for(const pid_t tid : s.Frozen())
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));

    const auto before = s.Read();
    s.RunWitness();
    s.RequireFrozenSince(before);

    for(const pid_t tid : s.Frozen())
        REQUIRE(s.dbg.SetThreadSuspended(tid, false));
    s.RunUntilEveryWorkerMoves();
}

TEST_CASE("Continue with every thread suspended reports a pause instead of running nothing", "[multithread][suspend]")
{
    SpinSession s;
    REQUIRE(s.dbg.SetThreadSuspended(s.mainTid, true));
    for(const pid_t tid : s.workers)
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));

    const auto before = s.Read();
    s.dbg.Continue();
    s.dbg.WaitForPaused();
    REQUIRE(s.dbg.IsPaused());
    REQUIRE(s.Read() == before);
}

TEST_CASE("A step on a suspended thread is ignored", "[multithread][suspend][step]")
{
    using namespace ElfBug::test;
    SpinSession s;
    const pid_t worker = s.workers.front();
    REQUIRE(s.dbg.SetThreadSuspended(worker, true));
    REQUIRE(s.dbg.SwitchThread(worker));

    s.dbg.StepInto();
    REQUIRE(s.dbg.IsPaused());
    s.dbg.StepOver();
    REQUIRE(s.dbg.IsPaused());
    REQUIRE_THROWS_AS(s.dbg.WaitForAny({EventType::Step, EventType::Paused}, std::chrono::milliseconds(50)), WaitTimeout);
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
        s.RunUntilEveryWorkerMoves();
        REQUIRE(s.dbg.SwitchThread(worker));
    }
    REQUIRE(dropped);
}

TEST_CASE("Suspending workers while running freezes them", "[multithread][suspend]")
{
    using namespace ElfBug::test;
    SpinSession s;
    s.dbg.Continue();
    REQUIRE(s.dbg.WaitForRunning());

    for(const pid_t tid : s.Frozen())
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));
    for(const pid_t tid : s.Frozen())
        REQUIRE(WaitForTaskStopped(s.mainTid, tid));

    const auto before = s.Read();
    REQUIRE(s.WaitForWitness(before));
    s.RequireFrozenSince(before);
    REQUIRE_FALSE(s.dbg.IsPaused());
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
    using namespace ElfBug::test;
    SpinSession s;
    for(const pid_t tid : s.workers)
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));
    const auto frozen = s.Read();

    s.dbg.Continue();
    REQUIRE(s.dbg.WaitForRunning());
    for(const pid_t tid : s.workers)
        REQUIRE(s.dbg.SetThreadSuspended(tid, false));

    REQUIRE(WaitForEverySlotPast(s.dbg.process(), s.counters, frozen));
    REQUIRE_FALSE(s.dbg.IsPaused());
}

TEST_CASE("Resuming a thread suspended at its own breakpoint steps off it first", "[multithread][suspend][breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("threads_spin");
    REQUIRE(dbg.Init(path.c_str()));

    std::optional<ElfBug::ptr> site;
    std::optional<ElfBug::ptr> counters;
    dbg.OnSystemBreakpoint([&]
    {
        site = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_worker_started");
        counters = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_counters");
        if(site)
            dbg.process()->SetBreakpoint(*site, false, ElfBug::SoftwareType::ShortInt3);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    REQUIRE(site.has_value());
    REQUIRE(counters.has_value());

    pid_t target = 0;
    for(int i = 0; i < 4; ++i)
    {
        dbg.Continue();
        target = dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10)).pid;
    }

    REQUIRE(dbg.SetThreadSuspended(target, true));
    const auto parked = ReadSpinSlots(dbg.process(), *counters);
    dbg.Continue();
    REQUIRE(WaitForSpinSlots(dbg.process(), *counters, [&](const SpinSlots & now)
    {
        int moved = 0;
        for(std::size_t i = 0; i < now.size(); ++i)
            moved += now[i] > parked[i];
        return moved == 3;
    }));
    REQUIRE_FALSE(dbg.IsPaused());

    REQUIRE(dbg.SetThreadSuspended(target, false));

    REQUIRE(WaitForSpinSlots(dbg.process(), *counters, [](const SpinSlots & now)
    {
        return std::all_of(now.begin(), now.end(), [](const std::uint64_t slot) { return slot > 0; });
    }));
    REQUIRE(dbg.count(EventType::Breakpoint) == 4);

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
        std::optional<ElfBug::ptr> counters;
    };
    std::promise<Sites> promise;
    auto future = promise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const pid_t pid = dbg.process()->pid;
        promise.set_value({
            ResolveRuntimeAddress(path, pid, "ts_fault_armed"),
            ResolveRuntimeAddress(path, pid, "ts_fault_handler_tid"),
            ResolveRuntimeAddress(path, pid, "ts_counters")});
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto s = future.get();
    REQUIRE(s.armed.has_value());
    REQUIRE(s.handlerTid.has_value());
    REQUIRE(s.counters.has_value());
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
    const auto since = ReadSpinSlots(dbg.process(), *s.counters);
    dbg.Continue();
    REQUIRE(WaitForEverySlotPast(dbg.process(), *s.counters, since));
    REQUIRE_FALSE(dbg.IsPaused());

    REQUIRE(dbg.SetThreadSuspended(mainTid, false));
    REQUIRE(WaitForTraceeValue(dbg.process(), *s.handlerTid, mainTid));

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("Overlapping suspend requests for a running tid both nest", "[multithread][suspend]")
{
    using namespace ElfBug::test;
    SpinSession s;
    s.dbg.Continue();
    REQUIRE(s.dbg.WaitForRunning());

    for(const pid_t tid : s.Frozen())
    {
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));
    }
    for(const pid_t tid : s.Frozen())
        REQUIRE(WaitForTaskStopped(s.mainTid, tid));

    s.dbg.Pause();
    s.dbg.WaitForPaused();
    const auto before = s.Read();

    for(const pid_t tid : s.Frozen())
        REQUIRE(s.dbg.SetThreadSuspended(tid, false));
    s.RunWitness();
    s.RequireFrozenSince(before);

    for(const pid_t tid : s.Frozen())
        REQUIRE(s.dbg.SetThreadSuspended(tid, false));
    s.RunUntilEveryWorkerMoves();
}

TEST_CASE("A resume issued right before a running suspend lands is not lost", "[multithread][suspend]")
{
    using namespace ElfBug::test;
    SpinSession s;
    s.dbg.Continue();
    REQUIRE(s.dbg.WaitForRunning());

    for(const pid_t tid : s.workers)
    {
        REQUIRE(s.dbg.SetThreadSuspended(tid, true));
        REQUIRE(s.dbg.SetThreadSuspended(tid, false));
    }

    for(const pid_t tid : s.workers)
        REQUIRE(WaitForSigstopTaken(s.mainTid, tid));
    REQUIRE(WaitForEverySlotPast(s.dbg.process(), s.counters, s.Read()));
    REQUIRE_FALSE(s.dbg.IsPaused());
}

TEST_CASE("Queued breakpoints on suspended threads wait for the resume", "[multithread][suspend][breakpoint]")
{
    using namespace ElfBug::test;

    bool sawQueued = false;
    for(int attempt = 0; attempt < 50 && !sawQueued; ++attempt)
    {
        CAPTURE(attempt);
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
        const pid_t mainTid = dbg.process()->pid;

        dbg.Continue();
        const Event first = dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));

        std::vector<pid_t> others;
        for(const auto & [tid, thread] : dbg.process()->threads)
        {
            if(tid == mainTid || tid == first.pid)
                continue;
            others.push_back(tid);
            sawQueued = sawQueued || thread->HasPendingBreakpoint();
        }
        if(!sawQueued)
        {
            dbg.Stop();
            dbg.WaitForExit();
            dbg.JoinThread();
            continue;
        }

        for(const pid_t tid : others)
            REQUIRE(dbg.SetThreadSuspended(tid, true));

        const std::size_t unregistered = 3 - others.size();
        for(std::size_t i = 0; i < unregistered; ++i)
        {
            dbg.Continue();
            const Event hit = dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));
            REQUIRE(std::find(others.begin(), others.end(), hit.pid) == others.end());
        }
        dbg.Continue();
        REQUIRE_THROWS_AS(dbg.WaitFor(EventType::Breakpoint, std::chrono::milliseconds(50)), WaitTimeout);

        dbg.Pause();
        dbg.WaitForPaused();
        for(const pid_t tid : others)
            REQUIRE(dbg.SetThreadSuspended(tid, false));

        for(std::size_t i = 0; i < others.size(); ++i)
        {
            dbg.Continue();
            const Event hit = dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));
            REQUIRE(std::find(others.begin(), others.end(), hit.pid) != others.end());
        }

        dbg.Stop();
        dbg.WaitForExit();
        dbg.JoinThread();
    }
    REQUIRE(sawQueued);
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
