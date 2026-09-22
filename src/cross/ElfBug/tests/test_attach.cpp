#include "TestSupport.h"
#include <ElfBug/api/elfbug_api.h>

TEST_CASE("Attach traces every thread of a running process", "[attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(target.WaitForThreads(5));

    ElfBug::test::RecordingDebugger dbg;

    std::vector<pid_t> readable;
    std::vector<pid_t> unreadable;
    dbg.OnAttachBreakpoint([&]
    {
        for(const auto & [tid, thread] : dbg.process()->threads)
        {
            if(thread->registers.Read())
                readable.push_back(tid);
            else
                unreadable.push_back(tid);
        }
    });

    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    REQUIRE(unreadable.empty());
    REQUIRE(readable.size() == 5);
}

TEST_CASE("Attach makes the main thread current, not whichever stopped last", "[attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(target.WaitForThreads(5));

    ElfBug::test::RecordingDebugger dbg;
    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    REQUIRE(dbg.currentThread() != nullptr);
    REQUIRE(dbg.currentThread()->tid == target.pid);
}

TEST_CASE("Attach reports a pause and freezes the debuggee", "[attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(target.WaitForThreads(5));

    ElfBug::test::RecordingDebugger dbg;
    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    REQUIRE(dbg.IsPaused());
    REQUIRE_FALSE(target.Running());
}

TEST_CASE("Attach refuses a process that is already being debugged", "[attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(target.WaitForRunning());

    ElfBug::test::RecordingDebugger first;
    REQUIRE(first.Attach(target.pid));
    first.StartOnThread();
    first.WaitForAttachBreakpoint();

    ElfBug::test::RecordingDebugger second;
    REQUIRE_FALSE(second.Attach(target.pid));

    const auto list = ElfBugProcessList();
    const auto it = std::find_if(list.begin(), list.end(),
    [&](const ElfBugProcessInfo & p) { return p.pid == target.pid; });
    REQUIRE(it != list.end());
    REQUIRE(it->traced);
}

TEST_CASE("Attach refuses an unknown pid and our own pid", "[attach]")
{
    ElfBug::test::RecordingDebugger dbg;
    REQUIRE_FALSE(dbg.Attach(getpid()));
    REQUIRE_FALSE(dbg.Attach(-1));
    REQUIRE_FALSE(dbg.Attach(0x7FFFFFFF));
}

TEST_CASE("Attach and Init refuse to run while a debug loop is active", "[attach][init]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(target.WaitForRunning());

    ElfBug::test::RecordingDebugger dbg;
    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();
    REQUIRE(dbg.IsPaused());

    ElfBug::test::UntracedProcess other(FIXTURE("threads_spin"));
    REQUIRE(other.pid > 0);
    REQUIRE(other.WaitForRunning());

    REQUIRE_FALSE(dbg.Attach(other.pid));
    REQUIRE_FALSE(dbg.Init(FIXTURE("hello_elfbug").c_str()));
    REQUIRE(dbg.IsPaused());

    std::size_t guardErrors = 0;
    for(const auto & e : dbg.events())
    {
        if(e.type == ElfBug::test::EventType::InternalError &&
                e.message.find("debug loop is still running") != std::string::npos)
            ++guardErrors;
    }
    REQUIRE(guardErrors == 2);

    dbg.Stop();
    dbg.JoinThread();
}

TEST_CASE("Attach acquires every live thread of a multithreaded process", "[attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("thread_storm"));
    REQUIRE(target.pid > 0);
    REQUIRE(ElfBug::test::WaitForExeced(target.pid, FIXTURE("thread_storm")));
    REQUIRE(target.WaitForThreads(8));

    ElfBug::test::RecordingDebugger dbg;

    std::vector<pid_t> known;
    std::vector<pid_t> unreadable;
    std::vector<pid_t> live;
    dbg.OnAttachBreakpoint([&]
    {
        for(const auto & [tid, thread] : dbg.process()->threads)
        {
            known.push_back(tid);
            if(!thread->registers.Read())
                unreadable.push_back(tid);
        }
        ElfBug::ReadTaskList(target.pid, live);
    });

    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    REQUIRE(unreadable.empty());
    std::sort(known.begin(), known.end());
    std::sort(live.begin(), live.end());
    REQUIRE(known == live);
}

TEST_CASE("Attach acquires threads the target clones during the sweep", "[attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("clone_relay"));
    REQUIRE(target.pid > 0);
    REQUIRE(ElfBug::test::WaitForExeced(target.pid, FIXTURE("clone_relay")));
    REQUIRE(target.WaitForThreads(50));

    ElfBug::test::RecordingDebugger dbg;

    std::vector<pid_t> known;
    std::vector<pid_t> live;
    dbg.OnAttachBreakpoint([&]
    {
        for(const auto & [tid, thread] : dbg.process()->threads)
            known.push_back(tid);
        ElfBug::ReadTaskList(target.pid, live);
    });

    std::vector<pid_t> before;
    REQUIRE(ElfBug::ReadTaskList(target.pid, before));

    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    std::sort(known.begin(), known.end());
    std::sort(live.begin(), live.end());
    CAPTURE(before.size(), known.size(), live.size());

    REQUIRE(known.size() > before.size());
    REQUIRE(known == live);
}

TEST_CASE("A signal delivered to an attached process is reported and forwarded", "[attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("signal_pending"));
    REQUIRE(target.pid > 0);
    REQUIRE(target.WaitForRunning());
    REQUIRE(ElfBug::test::WaitForExeced(target.pid, FIXTURE("signal_pending")));
    const auto ready = ElfBug::test::ResolveRuntimeAddress(FIXTURE("signal_pending"), target.pid, "sp_ready");
    REQUIRE(ready.has_value());
    REQUIRE(ElfBug::test::WaitForDetachedValue(target.pid, *ready, 1));

    ElfBug::test::RecordingDebugger dbg;
    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    dbg.Continue();

    kill(target.pid, SIGUSR1);

    dbg.WaitForException(SIGUSR1);
    dbg.Continue();

    const auto handled = ElfBug::test::ResolveRuntimeAddress(FIXTURE("signal_pending"),
                         dbg.process()->pid, "sp_handled");
    REQUIRE(handled.has_value());
    REQUIRE(ElfBug::test::WaitForTraceeValue(dbg.process(), *handled, 1));
}

TEST_CASE("Attach reports an error when the target dies before the sweep", "[attach]")
{
    ElfBug::test::RecordingDebugger dbg;
    pid_t pid = 0;
    {
        ElfBug::test::UntracedProcess target(FIXTURE("run_endlessly"));
        REQUIRE(target.pid > 0);
        REQUIRE(target.WaitForRunning());
        pid = target.pid;
        REQUIRE(dbg.Attach(pid));
    }

    dbg.StartOnThread();
    const auto event = dbg.WaitForInternalError();
    REQUIRE(event.message.find(std::to_string(pid)) != std::string::npos);
    dbg.JoinThread();
}

TEST_CASE("A signal already pending at attach is reported before the tracee runs", "[attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("signal_storm"));
    REQUIRE(target.pid > 0);
    REQUIRE(ElfBug::test::WaitForExeced(target.pid, FIXTURE("signal_storm")));
    REQUIRE(target.WaitForThreads(9));

    ElfBug::test::RecordingDebugger dbg;

    bool pending = false;
    dbg.OnAttachBreakpoint([&]
    {
        for(const auto & [tid, thread] : dbg.process()->threads)
        {
            if(thread->PendingSignal() == SIGUSR1)
                pending = true;
        }
    });

    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    REQUIRE(pending);

    const auto handled = ElfBug::test::ResolveRuntimeAddress(FIXTURE("signal_storm"),
                         dbg.process()->pid, "ss_handled");
    REQUIRE(handled.has_value());

    int before = 0;
    REQUIRE(dbg.process()->MemReadRaw(*handled, &before, sizeof(before)));

    dbg.Continue();
    dbg.WaitForException(SIGUSR1);

    int atException = 0;
    REQUIRE(dbg.process()->MemReadRaw(*handled, &atException, sizeof(atException)));

    REQUIRE(atException == before);
}

TEST_CASE("AttachErrorMessage names the yama fix for EPERM", "[attach]")
{
    const auto blocked = ElfBug::AttachErrorMessage(1234, EPERM, 1, false);
    REQUIRE(blocked.find("ptrace_scope") != std::string::npos);
    REQUIRE(blocked.find("setcap cap_sys_ptrace=+eip") != std::string::npos);

    const auto other = ElfBug::AttachErrorMessage(1234, EPERM, 0, false);
    REQUIRE(other.find("setcap") == std::string::npos);

    const auto child = ElfBug::AttachErrorMessage(1234, EPERM, 1, true);
    REQUIRE(child.find("setcap") == std::string::npos);

    const auto gone = ElfBug::AttachErrorMessage(1234, ESRCH, 1, false);
    REQUIRE(gone.find("setcap") == std::string::npos);

    const auto adminOnly = ElfBug::AttachErrorMessage(1234, EPERM, 2, false);
    REQUIRE(adminOnly.find("setcap cap_sys_ptrace=+eip") != std::string::npos);
    REQUIRE(adminOnly.find("own children") == std::string::npos);

    const auto disabled = ElfBug::AttachErrorMessage(1234, EPERM, 3, false);
    REQUIRE(disabled.find("ptrace_scope") != std::string::npos);
    REQUIRE(disabled.find("setcap") == std::string::npos);
}
