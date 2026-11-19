#include "TestSupport.h"
#include <ElfBug/core/SignalFilters.h>
#include <ElfBug/process/ProcessList.h>
#include <zydis_wrapper.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <csignal>
#include <future>
#include <initializer_list>
#include <string>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace
{
    using namespace ElfBug;
    using namespace ElfBug::test;

    constexpr int kRealtimeSignal = 40;
    constexpr SignalFilter kQuiet{false, false, true};
    constexpr SignalFilter kLogged{false, true, true};

    // Stops at flt_ready with breakpoints on the named sites.
    struct FilterTarget
    {
        explicit FilterTarget(std::vector<std::string> mode, std::vector<std::string> sites = {"flt_done"});
        const std::string path = FIXTURE("signal_filter");
        RecordingDebugger dbg;
        ptr Site(const std::string & name) const;
        int Handled(int signal);
        template<typename T> T Read(const std::string & symbol);
        bool WorkerSpun();
        void Release();
        Event Finish();

    private:
        mutable std::unordered_map<std::string, ptr> mSites;
    };

    FilterTarget::FilterTarget(std::vector<std::string> mode, std::vector<std::string> sites)
    {
        std::vector<const char*> argv{path.c_str()};
        for(const auto & arg : mode)
            argv.push_back(arg.c_str());
        argv.push_back(nullptr);
        REQUIRE(dbg.Init(path.c_str(), argv.data()));

        sites.push_back("flt_ready");
        std::promise<bool> planted;
        auto plantedFuture = planted.get_future();
        dbg.OnSystemBreakpoint([&]
        {
            bool all = true;
            for(const auto & name : sites)
            {
                const auto address = ResolveRuntimeAddress(path, dbg.process()->pid, name);
                all = all && address && dbg.process()->SetBreakpoint(*address, false, SoftwareType::ShortInt3);
            }
            planted.set_value(all);
        });

        dbg.StartOnThread();
        dbg.WaitForSystemBreakpoint();
        REQUIRE(plantedFuture.get());
        dbg.Continue();
        dbg.WaitForBreakpointAt(Site("flt_ready"));
    }

    ptr FilterTarget::Site(const std::string & name) const
    {
        const auto known = mSites.find(name);
        if(known != mSites.end())
            return known->second;
        const auto address = ResolveRuntimeAddress(path, dbg.process()->pid, name);
        REQUIRE(address.has_value());
        mSites.emplace(name, *address);
        return *address;
    }

    int FilterTarget::Handled(const int signal)
    {
        int handled = 0;
        REQUIRE(dbg.process()->MemRead(Site("flt_handled") + signal * sizeof(int), &handled, sizeof(handled)));
        return handled;
    }

    template<typename T>
    T FilterTarget::Read(const std::string & symbol)
    {
        T value{};
        REQUIRE(dbg.process()->MemRead(Site(symbol), &value, sizeof(value)));
        return value;
    }

    bool FilterTarget::WorkerSpun()
    {
        return Read<std::array<unsigned long, 2>>("flt_spin")[1] > 0;
    }

    void FilterTarget::Release()
    {
        const int go = 1;
        REQUIRE(dbg.process()->MemWrite(Site("flt_go"), &go, sizeof(go)));
    }

    Event FilterTarget::Finish()
    {
        dbg.Continue();
        const Event exited = dbg.WaitForExit();
        dbg.JoinThread();
        REQUIRE(dbg.count(EventType::InternalError) == 0);
        return exited;
    }

    // vfork_signal blocked in vfork, where a signal sent to its main thread stays pending until the attach.
    struct VforkTarget
    {
        VforkTarget();
        void Attach();
        ptr Symbol(const char* name) const;
        int Read(const char* name) const;
        UntracedProcess process{FIXTURE("vfork_signal")};
        const pid_t pid = process.pid;
        RecordingDebugger dbg;

    private:
        pid_t mChild = 0;
        bool mPending = false;
    };

    VforkTarget::VforkTarget()
    {
        REQUIRE(pid > 0);
        REQUIRE(WaitForExeced(pid, FIXTURE("vfork_signal")));
        mChild = WaitForClonedChild(pid);
        REQUIRE(mChild > 0);
    }

    void VforkTarget::Attach()
    {
        dbg.OnAttachBreakpoint([this]
        {
            const auto & threads = dbg.process()->threads;
            mPending = std::any_of(threads.begin(), threads.end(), [](const auto & entry) { return entry.second->PendingSignal() == SIGUSR1; });
        });

        REQUIRE(dbg.Attach(pid));
        dbg.StartOnThread();

        REQUIRE(WaitUntil([&] { return TracerPid(pid) != 0; }, std::chrono::seconds(2)));
        REQUIRE(kill(mChild, SIGKILL) == 0);
        dbg.WaitForAttachBreakpoint();
        REQUIRE(mPending);
    }

    ptr VforkTarget::Symbol(const char* name) const
    {
        const auto address = ResolveRuntimeAddress(FIXTURE("vfork_signal"), pid, name);
        REQUIRE(address.has_value());
        return *address;
    }

    int VforkTarget::Read(const char* name) const
    {
        const auto value = ReadTraceeValue<int>(pid, Symbol(name));
        REQUIRE(value.has_value());
        return *value;
    }

    ptr NextInstruction(const RecordingDebugger & dbg, const ptr address)
    {
        std::uint8_t bytes[16] = {};
        REQUIRE(dbg.process()->MemRead(address, bytes, sizeof(bytes)));
        Zydis zydis(true);
        REQUIRE(zydis.Disassemble(address, bytes, sizeof(bytes)));
        return address + zydis.Size();
    }

    std::vector<Event> EventsOf(const RecordingDebugger & dbg, const EventType type)
    {
        std::vector<Event> found;
        for(const auto & event : dbg.events())
        {
            if(event.type == type)
                found.push_back(event);
        }
        return found;
    }

    // The wait mode's worker, once it spins; that also means the debugger has registered it.
    pid_t SpinningWorker(FilterTarget & target)
    {
        REQUIRE(WaitUntil([&] { return target.WorkerSpun(); }));
        std::vector<pid_t> tids;
        REQUIRE(ReadTaskList(target.dbg.process()->pid, tids));
        for(const pid_t tid : tids)
        {
            if(tid != target.dbg.process()->pid)
                return tid;
        }
        FAIL("the worker thread is gone");
        return 0;
    }

    // Leaves SIGUSR1 pending on the stopped main thread, so the next resume reports it first.
    void SendPending(FilterTarget & target)
    {
        const pid_t pid = target.dbg.process()->pid;
        REQUIRE(tgkill(pid, pid, SIGUSR1) == 0);
    }

    Event Step(FilterTarget & target, const bool over)
    {
        if(over)
            target.dbg.StepOver();
        else
            target.dbg.StepInto();
        return target.dbg.WaitForStep();
    }

    void StepWithFilteredSignal(const std::string & mode, const bool over)
    {
        FilterTarget target({mode}, {"flt_step_site"});
        const ptr site = target.Site("flt_step_site");
        target.dbg.Signals().Set(SIGUSR1, kQuiet);
        target.dbg.Continue();
        target.dbg.WaitForBreakpointAt(site);

        SECTION("from the user's breakpoint")
        {
        }
        SECTION("with the breakpoint deleted first")
        {
            REQUIRE(target.dbg.process()->DeleteBreakpoint(site));
        }

        SendPending(target);
        REQUIRE(Step(target, over).instructionPointer == site + 1);
        REQUIRE(target.Handled(SIGUSR1) == 1);
        REQUIRE(target.dbg.count(EventType::Step) == 1);
        REQUIRE(target.dbg.count(EventType::Exception) == 0);
        REQUIRE(target.dbg.count(EventType::Breakpoint) == 2);

        REQUIRE(Step(target, over).instructionPointer == site + 2);
        REQUIRE(target.Handled(SIGUSR1) == 1);
        REQUIRE(target.Finish().exitCode == 0);
    }

    std::size_t HitsAt(const RecordingDebugger & dbg, const ptr address)
    {
        const auto hits = EventsOf(dbg, EventType::Breakpoint);
        return std::count_if(hits.begin(), hits.end(), [address](const Event & hit) { return hit.address == address; });
    }

    // Another thread holds a hit the sweep queued, so the next resume reports it instead of running anything.
    bool OthersQueued(const RecordingDebugger & dbg, const Thread* current)
    {
        const auto & threads = dbg.process()->threads;
        return std::any_of(threads.begin(), threads.end(), [current](const auto & entry) { return entry.second.get() != current && entry.second->HasPendingBreakpoint(); });
    }

    // Stops in the hidden handler of a SIGUSR1 that was pending at flt_loop_site.
    void StopInHiddenHandler(FilterTarget & target)
    {
        target.dbg.Signals().Set(SIGUSR1, kQuiet);
        target.dbg.Continue();
        target.dbg.WaitForBreakpointAt(target.Site("flt_loop_site"));

        SendPending(target);
        target.dbg.Continue();
        target.dbg.WaitForBreakpointAt(target.Site("flt_in_handler"));
    }

    void ContinueToDone(FilterTarget & target)
    {
        target.dbg.Continue();
        for(int stop = 0; stop < 4; ++stop)
        {
            if(target.dbg.WaitFor(EventType::Breakpoint).address == target.Site("flt_done"))
                break;
            target.dbg.Continue();
        }
    }

    void ContinueThroughLoop(const std::string & mode, const SignalFilter filter, const bool owed = false)
    {
        FilterTarget target({mode}, {"flt_loop_site", "flt_done"});
        const ptr site = target.Site("flt_loop_site");
        target.dbg.Signals().Set(SIGUSR1, filter);

        target.dbg.Continue();
        for(int hit = 0; hit < 3; ++hit)
        {
            target.dbg.WaitForBreakpointAt(site);
            if(owed)
                target.dbg.currentThread()->SetPendingSignal(OwedSignal(SIGUSR1, ParkedSignal::Filtered, nullptr));
            else
                SendPending(target);
            target.dbg.Continue();
        }
        target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
        REQUIRE(HitsAt(target.dbg, site) == 3);
        REQUIRE(target.Handled(SIGUSR1) == (filter.pass ? 3 : 0));
        REQUIRE(target.dbg.count(EventType::Exception) == 0);
        REQUIRE(target.dbg.count(EventType::Paused) == 0);

        REQUIRE(target.Finish().exitCode == 0);
    }
}

TEST_CASE("SignalFilters starts from the defaults", "[signalfilter]")
{
    const SignalFilters filters;
    const SignalFilter quiet{false, false, true};
    for(const int signal : {SIGCHLD, SIGALRM, SIGVTALRM, SIGPROF, SIGURG, SIGIO, SIGWINCH, 32, 33, 34})
        REQUIRE(filters.Get(signal) == quiet);
    for(const int signal : {SIGSEGV, SIGUSR1, SIGINT, SIGTRAP, SIGPIPE, 35, 64})
        REQUIRE(filters.Get(signal) == SignalFilter{});
    REQUIRE(SignalFilters::Default(SIGCHLD) == quiet);
}

TEST_CASE("SignalFilters loads the Windows filter format", "[signalfilter]")
{
    SignalFilters filters;
    filters.Load("0000000A-0000000A:nobreak:nolog:debugger,00000023-00000025:nobreak:log:debuggee,0000000E-0000000E:first:log:debuggee");
    REQUIRE(filters.Get(SIGUSR1) == SignalFilter{false, false, false});
    for(const int signal : {35, 36, 37})
        REQUIRE(filters.Get(signal) == SignalFilter{false, true, true});
    REQUIRE(filters.Get(38) == SignalFilter{});
    REQUIRE(filters.Get(SIGALRM) == SignalFilter{true, true, true});
    REQUIRE(filters.Get(SIGCHLD) == SignalFilter{false, false, true});
}

TEST_CASE("SignalFilters skips what it cannot use", "[signalfilter]")
{
    SignalFilters filters;
    filters.Load("garbage,,0000000C-0000000C,0000000B-0000000A:nobreak:nolog:debuggee,00000009-00000009:nobreak:nolog:debugger,"
                 "00000013-00000013:nobreak:nolog:debugger,00000000-00000000:nobreak:nolog:debuggee,00000041-000000FF:nobreak:nolog:debuggee,"
                 "0000000c-0000000c:second:nolog:debuggee");
    REQUIRE(filters.Get(SIGKILL) == SignalFilter{});
    REQUIRE(filters.Get(SIGSTOP) == SignalFilter{});
    REQUIRE(filters.Get(SIGSEGV) == SignalFilter{});
    REQUIRE(filters.Get(SIGUSR2) == SignalFilter{true, false, true});
    REQUIRE(filters.Get(0) == SignalFilter{});
    REQUIRE(filters.Get(65) == SignalFilter{});
}

TEST_CASE("SignalFilters skips an invalid entry whole", "[signalfilter]")
{
    SignalFilters filters;
    filters.Load("0000000F-0000000F:nobreak:nolog,0000003F-00000041:nobreak:nolog:debuggee,00000000-00000003:nobreak:nolog:debuggee");
    REQUIRE(filters.Get(SIGTERM) == SignalFilter{});
    REQUIRE(filters.Get(63) == SignalFilter{});
    REQUIRE(filters.Get(64) == SignalFilter{});
    for(const int signal : {1, 2, 3})
        REQUIRE(filters.Get(signal) == SignalFilter{});
}

TEST_CASE("SignalFilters resets to the defaults on every load", "[signalfilter]")
{
    SignalFilters filters;
    filters.Set(SIGUSR1, {false, false, false});
    filters.Load("");
    REQUIRE(filters.Get(SIGUSR1) == SignalFilter{});
    filters.Set(SIGKILL, {false, false, false});
    filters.Set(99, {false, false, false});
    REQUIRE(filters.Get(SIGKILL) == SignalFilter{});
}

TEST_CASE("A signal set not to stop reaches its handler without a stop", "[signalfilter][exception]")
{
    FilterTarget target({"raise"});
    target.dbg.Signals().Set(SIGUSR1, kQuiet);

    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
    REQUIRE(target.Handled(SIGUSR1) == 1);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
    REQUIRE(target.dbg.count(EventType::SignalFiltered) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A signal set not to pass never reaches its handler", "[signalfilter][exception]")
{
    FilterTarget target({"raise"});

    SECTION("stopping")
    {
        target.dbg.Signals().Set(SIGUSR1, {true, true, false});
        target.dbg.Continue();
        target.dbg.WaitForException(SIGUSR1);
        target.dbg.Continue();
        target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
        REQUIRE(target.Handled(SIGUSR1) == 0);
        REQUIRE(target.dbg.count(EventType::Exception) == 1);
    }

    SECTION("not stopping")
    {
        target.dbg.Signals().Set(SIGUSR1, {false, true, false});
        target.dbg.Continue();
        target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
        REQUIRE(target.Handled(SIGUSR1) == 0);
        REQUIRE(target.dbg.count(EventType::Exception) == 0);
        const auto filtered = EventsOf(target.dbg, EventType::SignalFiltered);
        REQUIRE(filtered.size() == 1);
        REQUIRE(!filtered[0].passed);
    }

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A filtered signal is logged only when asked", "[signalfilter][exception]")
{
    FilterTarget target({"raise"});
    target.dbg.Signals().Set(SIGUSR1, kLogged);

    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
    const auto filtered = EventsOf(target.dbg, EventType::SignalFiltered);
    REQUIRE(filtered.size() == 1);
    REQUIRE(filtered[0].signal == SIGUSR1);
    REQUIRE(filtered[0].pid == target.dbg.process()->pid);
    REQUIRE(filtered[0].passed);
    REQUIRE(target.Handled(SIGUSR1) == 1);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("The default filters skip SIGCHLD and stop on SIGUSR1", "[signalfilter][exception]")
{
    SECTION("SIGCHLD")
    {
        FilterTarget target({"child"});
        target.dbg.Continue();
        target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
        REQUIRE(target.dbg.count(EventType::Exception) == 0);
        REQUIRE(target.Handled(SIGCHLD) == 1);
        REQUIRE(target.Finish().exitCode == 0);
    }

    SECTION("SIGUSR1")
    {
        FilterTarget target({"raise"});
        target.dbg.Continue();
        target.dbg.WaitForException(SIGUSR1);
        target.dbg.Continue();
        target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
        REQUIRE(target.dbg.count(EventType::Exception) == 1);
        REQUIRE(target.Finish().exitCode == 0);
    }
}

TEST_CASE("A filter changed while the program runs applies to the next signal", "[signalfilter][exception]")
{
    FilterTarget target({"wait"});
    const pid_t pid = target.dbg.process()->pid;

    target.dbg.Continue();
    REQUIRE(WaitUntil([&]
    {
        const auto spin = target.Read<std::array<unsigned long, 2>>("flt_spin");
        return spin[0] > 0 && spin[1] > 0;
    }));

    target.dbg.Signals().Set(SIGUSR1, kQuiet);
    REQUIRE(kill(pid, SIGUSR1) == 0);
    REQUIRE(WaitUntil([&] { return target.Handled(SIGUSR1) == 1; }));
    REQUIRE(kill(pid, SIGUSR2) == 0);

    target.dbg.WaitForException(SIGUSR2);
    REQUIRE(target.Handled(SIGUSR1) == 1);
    target.Release();

    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
    REQUIRE(target.dbg.count(EventType::Exception) == 1);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A fast timer set not to stop never stops the program", "[signalfilter][exception]")
{
    FilterTarget target({"timer"});

    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_done"), std::chrono::seconds(20));
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
    REQUIRE(target.dbg.count(EventType::Paused) == 0);
    REQUIRE(target.Handled(SIGALRM) >= 300);
    REQUIRE(target.WorkerSpun());

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A fast timer never breaks a continue or a step from a breakpoint in a hot loop", "[signalfilter][stepping][breakpoint]")
{
    constexpr int kRounds = 30;
    FilterTarget target({"timerloop"}, {"flt_loop_site", "flt_done"});
    const ptr site = target.Site("flt_loop_site");
    const std::initializer_list<EventType> stops{EventType::Breakpoint, EventType::Step, EventType::Paused, EventType::Exception};

    const ptr next = NextInstruction(target.dbg, site);
    std::size_t steps = 0;
    target.dbg.Continue();
    for(int round = 0; round <= kRounds; ++round)
    {
        const Event hit = target.dbg.WaitForAny(stops);
        REQUIRE(hit.type == EventType::Breakpoint);
        REQUIRE(hit.address == site);
        if(round == kRounds)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        if(round % 3 != 0)
        {
            if(round % 3 == 1)
                target.dbg.StepInto();
            else
                target.dbg.StepOver();
            const Event step = target.dbg.WaitForAny(stops);
            REQUIRE(step.type == EventType::Step);
            REQUIRE(step.pid == hit.pid);
            REQUIRE(step.instructionPointer == next);
            ++steps;
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
        target.dbg.Continue();
    }
    REQUIRE(steps == 20);
    REQUIRE(target.Handled(SIGALRM) > 0);
    REQUIRE(target.WorkerSpun());

    target.Release();
    REQUIRE(target.dbg.DeleteBreakpoint(site));
    target.dbg.Continue();
    REQUIRE(target.dbg.WaitForAny(stops).address == target.Site("flt_done"));
    REQUIRE(HitsAt(target.dbg, site) == kRounds + 1);
    REQUIRE(target.dbg.count(EventType::Step) == steps);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
    REQUIRE(target.dbg.count(EventType::Paused) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A job-control stop for a signal set not to stop is not reported", "[signalfilter][exception]")
{
    FilterTarget target({"tstp"});
    target.dbg.Signals().Set(SIGTSTP, kQuiet);

    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
    REQUIRE(target.dbg.count(EventType::Exception) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A job-control signal set not to stop is logged once and not again for its group-stop", "[signalfilter][exception]")
{
    FilterTarget target({"tstp"});
    target.dbg.Signals().Set(SIGTSTP, kLogged);

    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
    const auto filtered = EventsOf(target.dbg, EventType::SignalFiltered);
    REQUIRE(filtered.size() == 1);
    REQUIRE(filtered[0].signal == SIGTSTP);
    REQUIRE(filtered[0].passed);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A fault set not to stop is passed even when set not to pass", "[signalfilter][exception]")
{
    FilterTarget target({"fault"});
    target.dbg.Signals().Set(SIGSEGV, {false, false, false});

    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
    REQUIRE(target.Handled(SIGSEGV) == 1);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("Queued real-time signals set not to stop all arrive", "[signalfilter][exception]")
{
    FilterTarget target({"burst"});
    target.dbg.Signals().Set(kRealtimeSignal, kQuiet);

    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
    REQUIRE(target.Handled(kRealtimeSignal) == 16);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("An unhandled fatal signal set not to stop ends the program", "[signalfilter][exception]")
{
    FilterTarget target({"nohandler"});
    target.dbg.Signals().Set(SIGSEGV, kQuiet);

    REQUIRE(target.Finish().exitCode == -SIGSEGV);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
}

TEST_CASE("A signal set not to stop that is pending at attach is delivered without a report", "[signalfilter][attach]")
{
    VforkTarget target;
    REQUIRE(tgkill(target.pid, target.pid, SIGUSR1) == 0);
    target.dbg.Signals().Set(SIGUSR1, kQuiet);
    target.Attach();

    const ptr handled = target.Symbol("vs_handled");
    target.dbg.Continue();
    REQUIRE(WaitForTraceeValue(target.pid, handled, 1));
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
}

TEST_CASE("A filtered signal for a suspended thread waits for its resume", "[signalfilter][exception]")
{
    FilterTarget target({"wait"});
    const pid_t pid = target.dbg.process()->pid;
    target.dbg.Signals().Set(SIGUSR1, kQuiet);

    target.dbg.Continue();
    const pid_t worker = SpinningWorker(target);

    REQUIRE(target.dbg.SetThreadSuspended(worker, true));
    REQUIRE(tgkill(pid, worker, SIGUSR1) == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
    REQUIRE(target.dbg.count(EventType::Paused) == 0);
    REQUIRE(target.Handled(SIGUSR1) == 0);

    REQUIRE(target.dbg.SetThreadSuspended(worker, false));
    REQUIRE(WaitUntil([&] { return target.Handled(SIGUSR1) == 1; }));

    target.Release();
    target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
    REQUIRE(target.dbg.count(EventType::Exception) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A filtered signal parked at detach still reaches the program", "[signalfilter][detach]")
{
    FilterTarget target({"wait"});
    const pid_t pid = target.dbg.process()->pid;
    const ptr handled = target.Site("flt_handled") + SIGUSR1 * sizeof(int);
    target.dbg.Signals().Set(SIGUSR1, kLogged);

    bool suspended = false;
    target.dbg.OnSignalFiltered([&](const pid_t tid) { suspended = target.dbg.SetThreadSuspended(tid, true); });

    target.dbg.Continue();
    const pid_t worker = SpinningWorker(target);

    REQUIRE(tgkill(pid, worker, SIGUSR1) == 0);
    REQUIRE(WaitUntil([&] { return target.dbg.count(EventType::SignalFiltered) == 1; }));
    REQUIRE(suspended);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(target.Handled(SIGUSR1) == 0);

    target.dbg.Detach();
    target.dbg.WaitForDetach();
    target.dbg.JoinThread();
    const ProcessReaper reaper(pid);

    REQUIRE(WaitForTraceeValue(pid, handled, 1));
}

TEST_CASE("A parked signal reaches its handler with the siginfo the kernel gave it", "[signalfilter][exception]")
{
    FilterTarget target({"sites"}, {"flt_step_site", "flt_done"});
    const pid_t pid = target.dbg.process()->pid;
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_step_site"));

    sigval value{};
    value.sival_int = 1234;

    SECTION("stopping")
    {
        target.dbg.Signals().Set(kRealtimeSignal, {true, true, true});
        REQUIRE(sigqueue(pid, kRealtimeSignal, value) == 0);
        target.dbg.Continue();
        target.dbg.WaitForException(kRealtimeSignal);
        target.dbg.StepInto();
        target.dbg.WaitForStep();
        target.dbg.Continue();
        REQUIRE(target.dbg.WaitFor(EventType::Breakpoint).address == target.Site("flt_step_site"));
        target.dbg.Continue();
        REQUIRE(target.dbg.WaitFor(EventType::Breakpoint).address == target.Site("flt_done"));
    }

    SECTION("not stopping")
    {
        target.dbg.Signals().Set(kRealtimeSignal, kQuiet);
        REQUIRE(sigqueue(pid, kRealtimeSignal, value) == 0);
        target.dbg.Continue();
        REQUIRE(target.dbg.WaitFor(EventType::Breakpoint).address == target.Site("flt_done"));
        REQUIRE(target.dbg.count(EventType::Exception) == 0);
        REQUIRE(target.dbg.count(EventType::Paused) == 0);
        REQUIRE(target.dbg.count(EventType::Breakpoint) == 3);
    }

    REQUIRE(target.Handled(kRealtimeSignal) == 1);
    REQUIRE(target.Read<int>("flt_code") == SI_QUEUE);
    REQUIRE(target.Read<int>("flt_pid") == getpid());
    REQUIRE(target.Read<int>("flt_value") == 1234);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A signal pending at attach keeps its siginfo when a step runs before it is delivered", "[signalfilter][attach]")
{
    VforkTarget target;

    siginfo_t queued{};
    queued.si_signo = SIGUSR1;
    queued.si_code = SI_QUEUE;
    queued.si_pid = getpid();
    queued.si_uid = getuid();
    queued.si_value.sival_int = 1234;
    REQUIRE(syscall(SYS_rt_tgsigqueueinfo, target.pid, target.pid, SIGUSR1, &queued) == 0);

    bool stops = true;
    SECTION("stopping")
    {
    }
    SECTION("not stopping")
    {
        stops = false;
        target.dbg.Signals().Set(SIGUSR1, kQuiet);
    }

    target.Attach();

    target.dbg.StepInto();
    target.dbg.WaitForStep();
    target.dbg.Continue();
    if(stops)
    {
        target.dbg.WaitForException(SIGUSR1);
        target.dbg.Continue();
    }

    REQUIRE(WaitForTraceeValue(target.pid, target.Symbol("vs_handled"), 1));
    REQUIRE(target.Read("vs_code") == SI_QUEUE);
    REQUIRE(target.Read("vs_pid") == getpid());
    REQUIRE(target.Read("vs_value") == 1234);
    REQUIRE(target.dbg.count(EventType::Exception) == (stops ? 1 : 0));
}

TEST_CASE("A step with a filtered signal pending finishes at the next instruction", "[signalfilter][stepping]")
{
    StepWithFilteredSignal("sites", false);
}

TEST_CASE("A step with a filtered signal pending finishes at the next instruction with the handler on an alternate stack", "[signalfilter][stepping]")
{
    StepWithFilteredSignal("altstack", false);
}

TEST_CASE("A step over with a filtered signal pending finishes at the next instruction", "[signalfilter][stepping]")
{
    StepWithFilteredSignal("sites", true);
}

TEST_CASE("A step over a call survives a filtered signal", "[signalfilter][stepping]")
{
    FilterTarget target({"sites"}, {"flt_call_site", "flt_done"});
    const ptr site = target.Site("flt_call_site");
    target.dbg.Signals().Set(SIGUSR1, kQuiet);
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(site);

    SECTION("from the user's breakpoint")
    {
    }
    SECTION("with the breakpoint deleted first")
    {
        REQUIRE(target.dbg.process()->DeleteBreakpoint(site));
    }

    SendPending(target);
    target.dbg.StepOver();
    REQUIRE(target.dbg.WaitForStep().instructionPointer == site + 5);
    REQUIRE(target.Handled(SIGUSR1) == 1);
    REQUIRE(target.dbg.count(EventType::Step) == 1);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
    REQUIRE(target.dbg.count(EventType::Paused) == 0);

    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
    REQUIRE(HitsAt(target.dbg, site) == 1);
    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A step over a call passes a filtered signal the thread already owes", "[signalfilter][stepping]")
{
    FilterTarget target({"sites"}, {"flt_call_site"});
    const ptr site = target.Site("flt_call_site");
    target.dbg.Signals().Set(SIGUSR1, kQuiet);
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(site);

    SECTION("from the user's breakpoint")
    {
    }
    SECTION("with the breakpoint deleted first")
    {
        REQUIRE(target.dbg.process()->DeleteBreakpoint(site));
    }

    target.dbg.currentThread()->SetPendingSignal(OwedSignal(SIGUSR1, ParkedSignal::Unreported, nullptr));
    target.dbg.StepOver();
    REQUIRE(target.dbg.WaitForStep().instructionPointer == site + 5);
    REQUIRE(target.Handled(SIGUSR1) == 1);
    REQUIRE(target.dbg.count(EventType::Step) == 1);
    REQUIRE(target.dbg.count(EventType::Breakpoint) == 2);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("Stepping after attach with a filtered signal pending runs its handler first", "[signalfilter][attach][stepping]")
{
    VforkTarget target;
    REQUIRE(tgkill(target.pid, target.pid, SIGUSR1) == 0);
    target.dbg.Signals().Set(SIGUSR1, kQuiet);
    target.Attach();

    target.dbg.StepInto();
    target.dbg.WaitForStep();

    REQUIRE(target.Read("vs_handled") == 1);
    REQUIRE(target.dbg.count(EventType::Step) == 1);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
}

TEST_CASE("A reported signal is still stepped into", "[signalfilter][stepping]")
{
    FilterTarget target({"sites"}, {"flt_step_site"});
    const ptr site = target.Site("flt_step_site");
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(site);

    SendPending(target);
    target.dbg.Continue();
    target.dbg.WaitForException(SIGUSR1);
    REQUIRE(target.dbg.process()->SetBreakpoint(target.Site("flt_in_handler"), false, SoftwareType::ShortInt3));

    target.dbg.StepInto();
    target.dbg.WaitForStep();
    target.dbg.Continue();
    REQUIRE(target.dbg.WaitFor(EventType::Breakpoint).address == target.Site("flt_in_handler"));

    target.dbg.Continue();
    REQUIRE(target.dbg.WaitFor(EventType::Breakpoint).address == site);
    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("Continuing from a breakpoint on a faulting instruction set not to stop runs its handler without a stop", "[signalfilter][breakpoint]")
{
    FilterTarget target({"fault"}, {"flt_fault_site", "flt_done"});
    const ptr site = target.Site("flt_fault_site");
    target.dbg.Signals().Set(SIGSEGV, kQuiet);
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(site);

    target.dbg.Continue();
    REQUIRE(target.dbg.WaitForAny({EventType::Breakpoint, EventType::Paused, EventType::Exception}).address == target.Site("flt_done"));
    REQUIRE(HitsAt(target.dbg, site) == 1);
    REQUIRE(target.Handled(SIGSEGV) == 1);
    REQUIRE(target.dbg.count(EventType::Paused) == 0);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A fault set not to stop ends the step in its handler", "[signalfilter][stepping]")
{
    FilterTarget target({"fault"}, {"flt_fault_site"});
    target.dbg.Signals().Set(SIGSEGV, kQuiet);
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_fault_site"));

    target.dbg.StepInto();
    REQUIRE(target.dbg.WaitForStep().instructionPointer == target.Site("flt_on_fault"));
    REQUIRE(target.dbg.count(EventType::Exception) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A breakpoint inside a hidden handler is reported and ends the step", "[signalfilter][stepping]")
{
    FilterTarget target({"sites"}, {"flt_step_site", "flt_in_handler", "flt_done"});
    const ptr site = target.Site("flt_step_site");
    target.dbg.Signals().Set(SIGUSR1, kQuiet);
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(site);

    REQUIRE(target.dbg.process()->DeleteBreakpoint(site));
    SendPending(target);
    target.dbg.StepInto();
    REQUIRE(target.dbg.WaitFor(EventType::Breakpoint).address == target.Site("flt_in_handler"));
    REQUIRE(target.dbg.count(EventType::Step) == 0);
    REQUIRE(ReadProcessByte(target.dbg.process(), site) == 0x90);

    target.dbg.Continue();
    REQUIRE(target.dbg.WaitFor(EventType::Breakpoint).address == target.Site("flt_done"));
    REQUIRE(target.dbg.count(EventType::Step) == 0);
    REQUIRE(target.Handled(SIGUSR1) == 1);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A hidden handler running the interrupted code itself does not end the step early", "[signalfilter][stepping]")
{
    FilterTarget target({"raise"}, {"flt_in_handler"});
    const pid_t pid = target.dbg.process()->pid;
    const ptr site = target.Site("flt_in_handler");
    target.dbg.Signals().Set(SIGUSR1, kQuiet);
    target.dbg.Signals().Set(SIGUSR2, kQuiet);
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(site);
    const ptr rsp = target.dbg.currentThread()->registers.Gsp();

    REQUIRE(target.dbg.process()->DeleteBreakpoint(site));
    REQUIRE(tgkill(pid, pid, SIGUSR2) == 0);
    target.dbg.StepInto();
    REQUIRE(target.dbg.WaitForStep().instructionPointer == NextInstruction(target.dbg, site));
    REQUIRE(rsp - target.dbg.currentThread()->registers.Gsp() <= 16);
    REQUIRE(target.Handled(SIGUSR2) == 1);
    REQUIRE(target.dbg.count(EventType::Step) == 1);
    REQUIRE(target.dbg.count(EventType::Breakpoint) == 2);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("The program exiting inside a hidden handler ends the session cleanly", "[signalfilter][stepping]")
{
    FilterTarget target({"exitinhandler"}, {"flt_step_site"});
    target.dbg.Signals().Set(SIGUSR1, kQuiet);
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_step_site"));

    SendPending(target);
    target.dbg.StepInto();
    REQUIRE(target.dbg.WaitForExit().exitCode == 7);
    REQUIRE(target.dbg.count(EventType::InternalError) == 0);
    target.dbg.JoinThread();
}

TEST_CASE("Stepping the exit of a multithreaded process with the other thread held reports the exit at once", "[signalfilter][stepping][multithread]")
{
    bool byWorker = false;
    bool over = false;
    SECTION("the worker exits with step into")
    {
        byWorker = true;
    }
    SECTION("the worker exits with step over")
    {
        byWorker = true;
        over = true;
    }
    SECTION("the main thread exits with step into")
    {
    }
    SECTION("the main thread exits with step over")
    {
        over = true;
    }

    FilterTarget target({"groupexit", byWorker ? "worker" : "main"}, {"flt_group_exit_site"});
    const pid_t pid = target.dbg.process()->pid;
    target.dbg.Continue();
    const Event hit = target.dbg.WaitForBreakpointAt(target.Site("flt_group_exit_site"));
    REQUIRE((hit.pid != pid) == byWorker);
    REQUIRE(target.dbg.process()->threads.size() == 2);

    if(over)
        target.dbg.StepOver();
    else
        target.dbg.StepInto();
    const Event exited = target.dbg.WaitForAny({EventType::ExitProcess, EventType::Paused, EventType::Step, EventType::Exception});
    REQUIRE(exited.type == EventType::ExitProcess);
    REQUIRE(exited.exitCode == 5);
    target.dbg.JoinThread();
    REQUIRE(target.dbg.count(EventType::Paused) == 0);
    REQUIRE(target.dbg.count(EventType::InternalError) == 0);
}

TEST_CASE("Continuing from a breakpoint with a filtered signal pending reports each hit once", "[signalfilter][breakpoint]")
{
    ContinueThroughLoop("sites", kQuiet);
}

TEST_CASE("Continuing from a breakpoint with a filtered signal pending reports each hit once with the handler on an alternate stack", "[signalfilter][breakpoint]")
{
    ContinueThroughLoop("altstack", kQuiet);
}

TEST_CASE("A filtered signal set not to pass does not disturb a step off a breakpoint", "[signalfilter][breakpoint]")
{
    ContinueThroughLoop("sites", {false, false, false});
}

TEST_CASE("Continuing from a breakpoint that already owes a filtered signal reports each hit once", "[signalfilter][breakpoint]")
{
    ContinueThroughLoop("sites", kQuiet, true);
}

TEST_CASE("Threads stopped on one breakpoint with a filtered signal each report every hit once", "[signalfilter][breakpoint][multithread]")
{
    constexpr std::size_t kHits = 3 * 5;
    FilterTarget target({"threads"}, {"flt_loop_site", "flt_done"});
    const pid_t pid = target.dbg.process()->pid;
    const ptr site = target.Site("flt_loop_site");
    const ptr done = target.Site("flt_done");
    target.dbg.Signals().Set(SIGUSR1, kQuiet);

    int sent = 0;
    target.dbg.Continue();
    for(std::size_t stop = 0; stop <= kHits; ++stop)
    {
        const Event hit = target.dbg.WaitFor(EventType::Breakpoint);
        if(hit.address == done)
            break;
        REQUIRE(hit.address == site);
        std::vector<pid_t> tids;
        REQUIRE(ReadTaskList(pid, tids));
        for(const pid_t tid : tids)
        {
            if(tgkill(pid, tid, SIGUSR1) == 0)
                ++sent;
        }
        target.dbg.Continue();
    }
    REQUIRE(HitsAt(target.dbg, site) == kHits);
    REQUIRE(HitsAt(target.dbg, done) == 1);
    REQUIRE(target.Handled(SIGUSR1) >= 1);
    REQUIRE(target.Handled(SIGUSR1) <= sent);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
    REQUIRE(target.dbg.count(EventType::Paused) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("Removing the breakpoint while its handler runs forgets the silent step", "[signalfilter][breakpoint]")
{
    FilterTarget target({"sites"}, {"flt_loop_site", "flt_in_handler", "flt_done"});
    const ptr site = target.Site("flt_loop_site");
    StopInHiddenHandler(target);
    REQUIRE(target.dbg.DeleteBreakpoint(site));

    std::size_t hits = 1;
    SECTION("deleted")
    {
    }
    SECTION("deleted and set again, so the return to it is a new hit")
    {
        REQUIRE(target.dbg.process()->SetBreakpoint(site, false, SoftwareType::ShortInt3));
        hits = 4;
    }

    for(std::size_t hit = 1; hit < hits; ++hit)
    {
        target.dbg.Continue();
        target.dbg.WaitForBreakpointAt(site);
    }
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
    REQUIRE(HitsAt(target.dbg, site) == hits);
    REQUIRE(target.Handled(SIGUSR1) == 1);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A hit on the same breakpoint inside the hidden handler is still reported", "[signalfilter][breakpoint]")
{
    FilterTarget target({"raise"}, {"flt_in_handler", "flt_done"});
    const pid_t pid = target.dbg.process()->pid;
    const ptr site = target.Site("flt_in_handler");
    target.dbg.Signals().Set(SIGUSR1, kQuiet);
    target.dbg.Signals().Set(SIGUSR2, kQuiet);
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(site);
    const ptr rsp = target.dbg.currentThread()->registers.Gsp();

    REQUIRE(tgkill(pid, pid, SIGUSR2) == 0);
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(site);
    REQUIRE(target.dbg.currentThread()->registers.Gsp() < rsp);
    REQUIRE(target.Handled(SIGUSR2) == 1);

    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(target.Site("flt_done"));
    REQUIRE(HitsAt(target.dbg, site) == 2);
    REQUIRE(target.Handled(SIGUSR1) == 1);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
    REQUIRE(target.dbg.count(EventType::Paused) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("Stepping out of a hidden handler and on past its breakpoint keeps the later hits", "[signalfilter][breakpoint][stepping]")
{
    FilterTarget target({"sites"}, {"flt_loop_site", "flt_in_handler", "flt_done"});
    const ptr site = target.Site("flt_loop_site");
    StopInHiddenHandler(target);
    REQUIRE(target.dbg.DeleteBreakpoint(target.Site("flt_in_handler")));

    bool back = false;
    for(int steps = 0; steps < 400 && !back; ++steps)
        back = Step(target, false).instructionPointer == site;
    REQUIRE(back);

    bool over = false;
    SECTION("step into")
    {
    }
    SECTION("step over")
    {
        over = true;
    }
    REQUIRE(Step(target, over).instructionPointer == NextInstruction(target.dbg, site));

    ContinueToDone(target);
    REQUIRE(HitsAt(target.dbg, site) == 3);
    REQUIRE(HitsAt(target.dbg, target.Site("flt_done")) == 1);
    REQUIRE(target.Handled(SIGUSR1) == 1);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
    REQUIRE(target.dbg.count(EventType::Paused) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A step with a filtered signal inside a hidden handler keeps the silent return to the breakpoint it left", "[signalfilter][breakpoint][stepping]")
{
    FilterTarget target({"sites"}, {"flt_loop_site", "flt_in_handler", "flt_done"});
    const pid_t pid = target.dbg.process()->pid;
    const ptr inHandler = target.Site("flt_in_handler");
    target.dbg.Signals().Set(SIGUSR2, kQuiet);
    StopInHiddenHandler(target);
    REQUIRE(target.dbg.DeleteBreakpoint(inHandler));

    REQUIRE(tgkill(pid, pid, SIGUSR2) == 0);
    REQUIRE(Step(target, false).instructionPointer == NextInstruction(target.dbg, inHandler));
    REQUIRE(target.Handled(SIGUSR2) == 1);

    ContinueToDone(target);
    REQUIRE(HitsAt(target.dbg, target.Site("flt_loop_site")) == 3);
    REQUIRE(HitsAt(target.dbg, target.Site("flt_done")) == 1);
    REQUIRE(target.Handled(SIGUSR1) == 1);
    REQUIRE(target.dbg.count(EventType::Step) == 1);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
    REQUIRE(target.dbg.count(EventType::Paused) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A signal picked up while stepping off a breakpoint runs its handler before the next hit", "[signalfilter][breakpoint]")
{
    FilterTarget target({"sites"}, {"flt_loop_site", "flt_done"});
    const pid_t pid = target.dbg.process()->pid;
    const ptr site = target.Site("flt_loop_site");
    target.dbg.Signals().Set(SIGUSR1, kLogged);
    target.dbg.Continue();
    target.dbg.WaitForBreakpointAt(site);

    REQUIRE(tgkill(pid, pid, SIGSTOP) == 0);
    target.dbg.currentThread()->SetPendingSigstop(true);
    SendPending(target);
    target.dbg.Continue();
    REQUIRE(target.dbg.WaitFor(EventType::Breakpoint).address == site);
    REQUIRE(target.Handled(SIGUSR1) == 1);
    REQUIRE(target.dbg.currentThread()->PendingSignal() == 0);
    REQUIRE(target.dbg.count(EventType::SignalFiltered) == 1);

    target.dbg.Continue();
    REQUIRE(target.dbg.WaitFor(EventType::Breakpoint).address == site);
    target.dbg.Continue();
    REQUIRE(target.dbg.WaitFor(EventType::Breakpoint).address == target.Site("flt_done"));
    REQUIRE(HitsAt(target.dbg, site) == 3);
    REQUIRE(target.Handled(SIGUSR1) == 1);
    REQUIRE(target.dbg.count(EventType::Exception) == 0);
    REQUIRE(target.dbg.count(EventType::Paused) == 0);

    REQUIRE(target.Finish().exitCode == 0);
}

TEST_CASE("A signal picked up while a queued thread steps off its breakpoint is delivered when it resumes", "[signalfilter][breakpoint][multithread]")
{
    bool probed = false;
    for(int attempt = 0; attempt < 10 && !probed; ++attempt)
    {
        FilterTarget target({"threads"}, {"flt_loop_site", "flt_done"});
        const pid_t pid = target.dbg.process()->pid;
        const ptr done = target.Site("flt_done");
        target.dbg.Signals().Set(SIGUSR1, kLogged);

        bool finished = false;
        target.dbg.Continue();
        for(int stop = 0; stop < 16 && !finished; ++stop)
        {
            finished = target.dbg.WaitFor(EventType::Breakpoint).address == done;
            if(finished)
                break;
            Thread* thread = target.dbg.currentThread();
            if(probed || !thread->PendingSigstop() || OthersQueued(target.dbg, thread))
            {
                target.dbg.Continue();
                continue;
            }

            const pid_t tid = thread->tid;
            REQUIRE(tgkill(pid, tid, SIGUSR1) == 0);
            target.dbg.Continue();
            finished = target.dbg.WaitFor(EventType::Breakpoint).address == done;
            const auto & threads = target.dbg.process()->threads;
            const auto owner = threads.find(tid);
            REQUIRE((owner == threads.end() || owner->second->PendingSignal() != SIGUSR1));
            REQUIRE(target.dbg.count(EventType::SignalFiltered) == 1);
            probed = true;
            if(!finished)
                target.dbg.Continue();
        }
        if(!probed)
            continue;

        REQUIRE(finished);
        REQUIRE(target.Handled(SIGUSR1) == 1);
        REQUIRE(HitsAt(target.dbg, target.Site("flt_loop_site")) == 15);
        REQUIRE(target.dbg.count(EventType::Exception) == 0);
        REQUIRE(target.dbg.count(EventType::Paused) == 0);
        REQUIRE(target.Finish().exitCode == 0);
    }
    REQUIRE(probed);
}
