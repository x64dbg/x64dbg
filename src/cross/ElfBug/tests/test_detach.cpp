#include "TestSupport.h"

TEST_CASE("Detaching releases a clone outside the thread group", "[multithread][detach]")
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

    dbg.Detach();
    dbg.WaitForDetach();
    dbg.JoinThread();

    REQUIRE(ElfBug::TracerPid(child) == 0);
    REQUIRE(StaysRunning(child));
    REQUIRE(StaysRunning(inferiorPid));

    kill(child, SIGKILL);
    kill(inferiorPid, SIGKILL);
    int status = 0;
    waitpid(child, &status, __WALL);
    waitpid(inferiorPid, &status, __WALL);
}

TEST_CASE("Detach leaves an attached process running", "[detach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(ElfBug::test::WaitForExeced(target.pid, FIXTURE("threads_spin")));
    REQUIRE(target.WaitForThreads(5));

    ElfBug::test::RecordingDebugger dbg;
    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();
    REQUIRE_FALSE(target.Running());

    dbg.Detach();
    dbg.WaitForDetach();
    dbg.JoinThread();

    REQUIRE(ElfBug::test::StaysRunning(target.pid));
    REQUIRE(ElfBug::TracerPid(target.pid) == 0);
}

TEST_CASE("Detach restores every patched breakpoint byte", "[detach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(ElfBug::test::WaitForExeced(target.pid, FIXTURE("threads_spin")));
    REQUIRE(target.WaitForThreads(5));

    ElfBug::test::RecordingDebugger dbg;
    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    const auto site = ElfBug::test::ResolveRuntimeAddress(FIXTURE("threads_spin"),
                      dbg.process()->pid, "ts_tick");
    REQUIRE(site.has_value());

    std::uint8_t original = 0;
    REQUIRE(dbg.process()->MemReadRaw(*site, &original, 1));
    REQUIRE(original != 0xCC);
    REQUIRE(dbg.process()->SetBreakpoint(*site));

    std::uint8_t patched = 0;
    REQUIRE(dbg.process()->MemReadRaw(*site, &patched, 1));
    REQUIRE(patched == 0xCC);

    dbg.Detach();
    dbg.WaitForDetach();
    dbg.JoinThread();

    std::ifstream mem("/proc/" + std::to_string(target.pid) + "/mem", std::ios::binary);
    REQUIRE(mem);
    mem.seekg(static_cast<std::streamoff>(*site));
    char after = 0;
    REQUIRE(mem.read(&after, 1));
    REQUIRE(static_cast<std::uint8_t>(after) == original);
}

TEST_CASE("Detach leaves a launched process running", "[detach]")
{
    ElfBug::test::RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("run_endlessly").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();

    const pid_t pid = dbg.process()->pid;

    dbg.Detach();
    dbg.WaitForDetach();
    dbg.JoinThread();

    REQUIRE(ElfBug::TracerPid(pid) == 0);
    REQUIRE(ElfBug::test::StaysRunning(pid));
    kill(pid, SIGKILL);
    int status = 0;
    waitpid(pid, &status, __WALL);
}

TEST_CASE("A signal parked at detach is delivered to the process", "[detach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("signal_pending"));
    REQUIRE(target.pid > 0);
    REQUIRE(ElfBug::test::WaitForExeced(target.pid, FIXTURE("signal_pending")));
    const auto ready = ElfBug::test::ResolveRuntimeAddress(FIXTURE("signal_pending"), target.pid, "sp_ready");
    REQUIRE(ready.has_value());
    REQUIRE(ElfBug::test::WaitForDetachedValue(target.pid, *ready, 1));

    ElfBug::test::RecordingDebugger dbg;
    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    const auto handled = ElfBug::test::ResolveRuntimeAddress(FIXTURE("signal_pending"),
                         dbg.process()->pid, "sp_handled");
    REQUIRE(handled.has_value());

    dbg.Continue();

    kill(target.pid, SIGUSR1);
    dbg.WaitForException(SIGUSR1);

    int atException = 0;
    REQUIRE(dbg.process()->MemReadRaw(*handled, &atException, sizeof(atException)));
    REQUIRE(atException == 0);

    dbg.Detach();
    dbg.WaitForDetach();
    dbg.JoinThread();

    REQUIRE(ElfBug::test::WaitForDetachedValue(target.pid, *handled, 1));
}

TEST_CASE("Detach requested while running arrives as a pause and then detaches", "[detach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(ElfBug::test::WaitForExeced(target.pid, FIXTURE("threads_spin")));
    REQUIRE(target.WaitForThreads(5));

    ElfBug::test::RecordingDebugger dbg;
    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    dbg.Continue();
    REQUIRE(dbg.WaitForRunning());

    dbg.Detach();
    dbg.WaitForDetach();
    dbg.JoinThread();

    const auto log = dbg.events();
    const auto paused = std::find_if(log.begin(), log.end(),
    [](const ElfBug::test::Event & e) { return e.type == ElfBug::test::EventType::Paused; });
    const auto detached = std::find_if(log.begin(), log.end(),
    [](const ElfBug::test::Event & e) { return e.type == ElfBug::test::EventType::Detach; });
    REQUIRE(paused != log.end());
    REQUIRE(detached != log.end());
    REQUIRE(paused < detached);

    REQUIRE(ElfBug::test::StaysRunning(target.pid));
    REQUIRE(ElfBug::TracerPid(target.pid) == 0);
}

TEST_CASE("Detach drains a SIGSTOP the attach sweep still owes", "[detach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("signal_storm"));
    REQUIRE(target.pid > 0);
    REQUIRE(ElfBug::test::WaitForExeced(target.pid, FIXTURE("signal_storm")));
    REQUIRE(target.WaitForThreads(9));

    bool owed = false;
    for(int attempt = 0; attempt < 20 && !owed; ++attempt)
    {
        ElfBug::test::RecordingDebugger dbg;
        dbg.OnAttachBreakpoint([&]
        {
            for(const auto & [tid, thread] : dbg.process()->threads)
            {
                if(thread->PendingSigstop())
                    owed = true;
            }
        });

        REQUIRE(dbg.Attach(target.pid));
        dbg.StartOnThread();
        dbg.WaitForAttachBreakpoint();

        dbg.Detach();
        dbg.WaitForDetach();
        dbg.JoinThread();

        REQUIRE(ElfBug::TracerPid(target.pid) == 0);
        REQUIRE(ElfBug::test::StaysRunning(target.pid));
    }

    REQUIRE(owed);
}

TEST_CASE("Detach releases a thread the user suspended", "[detach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(ElfBug::test::WaitForExeced(target.pid, FIXTURE("threads_spin")));
    REQUIRE(target.WaitForThreads(5));

    ElfBug::test::RecordingDebugger dbg;

    pid_t worker = 0;
    dbg.OnAttachBreakpoint([&]
    {
        for(const auto & [tid, thread] : dbg.process()->threads)
        {
            if(tid != dbg.process()->pid)
                worker = tid;
        }
    });

    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    REQUIRE(worker != 0);
    REQUIRE(dbg.SetThreadSuspended(worker, true));

    dbg.Detach();
    dbg.WaitForDetach();
    dbg.JoinThread();

    REQUIRE(ElfBug::TracerPid(target.pid) == 0);
    REQUIRE(ElfBug::test::StaysRunning(target.pid));
}

TEST_CASE("Detach releases a thread suspended while the process runs", "[detach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(ElfBug::test::WaitForExeced(target.pid, FIXTURE("threads_spin")));
    REQUIRE(target.WaitForThreads(5));

    ElfBug::test::RecordingDebugger dbg;

    pid_t worker = 0;
    dbg.OnAttachBreakpoint([&]
    {
        for(const auto & [tid, thread] : dbg.process()->threads)
        {
            if(tid != dbg.process()->pid)
                worker = tid;
        }
    });

    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    REQUIRE(worker != 0);

    dbg.Continue();
    REQUIRE(dbg.WaitForRunning());
    REQUIRE(dbg.SetThreadSuspended(worker, true));

    dbg.Detach();
    dbg.WaitForDetach();
    dbg.JoinThread();

    REQUIRE(ElfBug::TracerPid(target.pid) == 0);
    REQUIRE(ElfBug::test::StaysRunning(target.pid));
}
