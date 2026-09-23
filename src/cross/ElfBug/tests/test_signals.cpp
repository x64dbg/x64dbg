#include "TestSupport.h"
#include "targets/TargetUtil.h"

TEST_CASE("SIGSEGV pauses and forwards on continue", "[exception]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("segfault").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();

    const auto exc_ev = dbg.WaitForException(SIGSEGV);
    REQUIRE(exc_ev.address == kSegfaultAddress);

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == -SIGSEGV);
}

TEST_CASE("A trap the program raised itself is reported and still delivered", "[exception]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("int3_target");
    REQUIRE(dbg.Init(path.c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();

    dbg.WaitForException(SIGTRAP);

    const auto handled = ResolveRuntimeAddress(path, dbg.process()->pid, "i3_handled");
    REQUIRE(handled);
    uint32_t before = 1;
    REQUIRE(dbg.process()->MemRead(*handled, &before, sizeof(before)));
    REQUIRE(before == 0);

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == 0);
    REQUIRE(dbg.count(EventType::InternalError) == 0);
}

TEST_CASE("Continue from a breakpoint on a faulting instruction reports the fault", "[exception][breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("segfault");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "sf_fault_site");
        if(site)
            dbg.process()->SetBreakpoint(*site, false, ElfBug::SoftwareType::ShortInt3);
        sitePromise.set_value(site);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto site = siteFuture.get();
    REQUIRE(site.has_value());

    dbg.Continue();
    dbg.WaitForBreakpointAt(*site);

    dbg.Continue();
    const auto exc = dbg.WaitForException(SIGSEGV);
    REQUIRE(exc.address == kSegfaultAddress);
    REQUIRE(exc.instructionPointer == *site);

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(exit_ev.exitCode == -SIGSEGV);
    REQUIRE(dbg.count(EventType::Breakpoint) == 1);
}

TEST_CASE("A step answering a fault reported mid-step-off keeps the other threads frozen",
          "[multithread][step][exception]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("threads_spin");
    REQUIRE(dbg.Init(path.c_str()));

    struct Sites
    {
        std::optional<ElfBug::ptr> tick;
        std::optional<ElfBug::ptr> fault;
        std::optional<ElfBug::ptr> armed;
        std::optional<ElfBug::ptr> counters;
    };

    std::promise<Sites> promise;
    auto future = promise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        Sites s;
        s.tick = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_tick");
        s.fault = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_fault_site");
        s.armed = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_fault_armed");
        s.counters = ResolveRuntimeAddress(path, dbg.process()->pid, "ts_counters");
        if(s.tick)
            dbg.process()->SetBreakpoint(*s.tick, false, ElfBug::SoftwareType::ShortInt3);
        promise.set_value(s);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto s = future.get();
    REQUIRE(s.tick.has_value());
    REQUIRE(s.fault.has_value());
    REQUIRE(s.armed.has_value());
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

    const int armed = 1;
    REQUIRE(dbg.process()->MemWrite(*s.armed, &armed, sizeof(armed)));
    REQUIRE(dbg.process()->DeleteBreakpoint(*s.tick));
    REQUIRE(dbg.process()->SetBreakpoint(*s.fault, false, ElfBug::SoftwareType::ShortInt3));

    dbg.Continue();
    dbg.WaitForBreakpointAt(*s.fault, std::chrono::seconds(10));

    dbg.Continue();
    dbg.WaitForException(SIGSEGV, std::chrono::seconds(10));

    REQUIRE(dbg.process()->MemRead(*s.counters, before, sizeof(before)));

    dbg.StepInto();
    dbg.WaitForStep();

    REQUIRE(dbg.process()->MemRead(*s.counters, after, sizeof(after)));
    for(int i = 0; i < 4; ++i)
        REQUIRE(after[i] == before[i]);

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(dbg.count(EventType::InternalError) == 0);
}

namespace
{
    struct SignalRaceResult
    {
        int raised = 0;
        int handled = 0;
        std::size_t reported = 0;
    };

    SignalRaceResult RunSignalRace(const int signal, const int quota)
    {
        using namespace ElfBug::test;
        RecordingDebugger dbg;
        const std::string path = FIXTURE("signal_race");
        REQUIRE(dbg.Init(path.c_str()));

        struct Sites
        {
            std::optional<ElfBug::ptr> hot;
            std::optional<ElfBug::ptr> signal;
            std::optional<ElfBug::ptr> quota;
            std::optional<ElfBug::ptr> go;
            std::optional<ElfBug::ptr> raised;
            std::optional<ElfBug::ptr> handled;
            std::optional<ElfBug::ptr> done;
        };

        std::promise<Sites> promise;
        auto future = promise.get_future();
        dbg.OnSystemBreakpoint([&]
        {
            Sites s;
            const pid_t pid = dbg.process()->pid;
            s.hot = ResolveRuntimeAddress(path, pid, "sr_hot");
            s.signal = ResolveRuntimeAddress(path, pid, "sr_signal");
            s.quota = ResolveRuntimeAddress(path, pid, "sr_quota");
            s.go = ResolveRuntimeAddress(path, pid, "sr_go");
            s.raised = ResolveRuntimeAddress(path, pid, "sr_raised");
            s.handled = ResolveRuntimeAddress(path, pid, "sr_handled");
            s.done = ResolveRuntimeAddress(path, pid, "sr_done");
            if(s.hot)
                dbg.process()->SetBreakpoint(*s.hot, false, ElfBug::SoftwareType::ShortInt3);
            promise.set_value(s);
        });

        dbg.StartOnThread();
        dbg.WaitForSystemBreakpoint();
        const auto s = future.get();
        REQUIRE(s.hot.has_value());
        REQUIRE(s.signal.has_value());
        REQUIRE(s.quota.has_value());
        REQUIRE(s.go.has_value());
        REQUIRE(s.raised.has_value());
        REQUIRE(s.handled.has_value());
        REQUIRE(s.done.has_value());

        REQUIRE(dbg.process()->MemWrite(*s.signal, &signal, sizeof(signal)));
        REQUIRE(dbg.process()->MemWrite(*s.quota, &quota, sizeof(quota)));

        dbg.Continue();
        dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));
        constexpr int go = 1;
        REQUIRE(dbg.process()->MemWrite(*s.go, &go, sizeof(go)));

        int done = 0;
        for(int round = 0; round < 4000 && done == 0; ++round)
        {
            dbg.Continue();
            dbg.WaitForAny({EventType::Breakpoint, EventType::Exception}, std::chrono::seconds(10));
            REQUIRE(dbg.process()->MemRead(*s.done, &done, sizeof(done)));
        }

        SignalRaceResult r;
        REQUIRE(dbg.process()->MemRead(*s.raised, &r.raised, sizeof(r.raised)));
        REQUIRE(dbg.process()->MemRead(*s.handled, &r.handled, sizeof(r.handled)));
        CAPTURE(r.raised, r.handled);
        REQUIRE(done == 1);
        std::size_t withAddress = 0;
        for(const auto & e : dbg.events())
        {
            if(e.type == EventType::Exception && e.signal == signal)
            {
                ++r.reported;
                if(e.address != 0)
                    ++withAddress;
            }
        }
        REQUIRE(withAddress == 0);

        REQUIRE(r.raised == quota);
        REQUIRE(r.handled == r.raised);
        REQUIRE(r.reported == static_cast<std::size_t>(r.raised));

        REQUIRE(dbg.process()->DeleteBreakpoint(*s.hot));
        dbg.Stop();
        dbg.WaitForExit();
        dbg.JoinThread();
        REQUIRE(dbg.count(EventType::InternalError) == 0);
        return r;
    }
}

TEST_CASE("Signals absorbed by the stop sweep are still reported", "[multithread][exception]")
{
    RunSignalRace(SIGUSR1, 200);
}

TEST_CASE("Raised fatal signals absorbed by the stop sweep are still delivered", "[multithread][exception]")
{
    RunSignalRace(SIGSEGV, 200);
}

TEST_CASE("A raised SIGTRAP is delivered and reported", "[multithread][exception]")
{
    RunSignalRace(SIGTRAP, 200);
}

TEST_CASE("A step after a thread switch keeps the reported signal on its thread", "[multithread][exception]")
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
    pid_t workerTid = 0;
    for(int i = 0; i < 4; ++i)
        workerTid = dbg.WaitFor(EventType::CreateThread).pid;
    dbg.Pause();
    dbg.WaitForPaused();

    constexpr int armed = 1;
    REQUIRE(dbg.process()->MemWrite(*s.armed, &armed, sizeof(armed)));
    dbg.Continue();
    const Event fault = dbg.WaitForException(SIGSEGV, std::chrono::seconds(10));
    REQUIRE(fault.pid == mainTid);

    SECTION("stepping the switched thread")
    {
        REQUIRE(dbg.SwitchThread(workerTid));
        REQUIRE(dbg.currentThread()->tid == workerTid);
        dbg.StepInto();
        REQUIRE(dbg.WaitForStep().pid == workerTid);
    }

    SECTION("stepping the reporting thread after switching away and back")
    {
        REQUIRE(dbg.SwitchThread(workerTid));
        dbg.StepInto();
        REQUIRE(dbg.WaitForStep().pid == workerTid);
        REQUIRE(dbg.SwitchThread(mainTid));
        REQUIRE(dbg.currentThread()->tid == mainTid);
        dbg.StepInto();
        REQUIRE(dbg.WaitForStep().pid == mainTid);
    }

    constexpr int disarmed = 0;
    REQUIRE(dbg.process()->MemWrite(*s.armed, &disarmed, sizeof(disarmed)));
    dbg.Continue();
    REQUIRE(dbg.WaitForRunning());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    dbg.Pause();
    dbg.WaitForPaused();
    int handlerTid = 0;
    REQUIRE(dbg.process()->MemRead(*s.handlerTid, &handlerTid, sizeof(handlerTid)));
    REQUIRE(handlerTid == mainTid);
    REQUIRE(dbg.count(EventType::Exception) == 1);

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("StopShouldQueue keeps kernel-raised non-fault signals", "[signal]")
{
    const auto stopped = [](const int sig, const int event = 0)
    {
        return (event << 16) | (sig << 8) | 0x7f;
    };
    const auto ask = [&](const int status, const bool haveInfo, const int code,
                         const void* addr = nullptr)
    {
        siginfo_t info{};
        info.si_code = code;
        info.si_addr = const_cast<void*>(addr);
        int signal = -1;
        ElfBug::ptr address = 0xdeadbeef;
        const bool keep = ElfBug::StopShouldQueue(status, haveInfo, info, signal, address);
        return std::make_tuple(keep, signal, address);
    };

    auto [keepChld, sigChld, addrChld] = ask(stopped(SIGCHLD), true, CLD_EXITED);
    REQUIRE(keepChld);
    REQUIRE(sigChld == SIGCHLD);
    REQUIRE(addrChld == 0);

    REQUIRE_FALSE(std::get<0>(ask(stopped(SIGTRAP), true, SI_KERNEL)));
    REQUIRE_FALSE(std::get<0>(ask(stopped(SIGTRAP), true, TRAP_TRACE)));
    REQUIRE(std::get<0>(ask(stopped(SIGTRAP), true, SI_TKILL)));

    int page = 0;
    REQUIRE_FALSE(std::get<0>(ask(stopped(SIGSEGV), true, SEGV_MAPERR, &page)));
    REQUIRE(std::get<0>(ask(stopped(SIGSEGV), true, SI_TKILL)));
    REQUIRE_FALSE(std::get<0>(ask(stopped(SIGSTOP), true, SI_USER)));

    REQUIRE_FALSE(std::get<0>(ask(stopped(SIGTRAP, PTRACE_EVENT_CLONE), true, SI_USER)));
    REQUIRE_FALSE(std::get<0>(ask(stopped(SIGCHLD), false, CLD_EXITED)));

    REQUIRE(std::get<0>(ask(stopped(SIGIO), true, POLL_IN)));
    REQUIRE(std::get<0>(ask(stopped(SIGALRM), true, SI_TIMER)));

    auto [keepNo, sigNo, addrNo] = ask(stopped(SIGTRAP), true, SI_KERNEL);
    REQUIRE_FALSE(keepNo);
    REQUIRE(sigNo == -1);
    REQUIRE(addrNo == 0xdeadbeef);
}
