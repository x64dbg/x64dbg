#include "TestSupport.h"

TEST_CASE("Init fails cleanly for missing binary", "[init]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE_FALSE(dbg.Init("/nonexistent/elfbug_missing_fixture"));
    REQUIRE(dbg.count(EventType::InternalError) >= 1);
}

TEST_CASE("Init explains a missing execute bit", "[init]")
{
    using namespace ElfBug::test;
    namespace fs = std::filesystem;

    const fs::path target = fs::path(ELFBUG_TESTS_TARGETS_DIR) / "elfbug_no_exec_bit";
    fs::copy_file(FIXTURE("end_immediately"), target, fs::copy_options::overwrite_existing);
    fs::permissions(target, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);

    RecordingDebugger dbg;
    const bool started = dbg.Init(target.c_str());

    std::string message;
    for(const auto & e : dbg.events())
    {
        if(e.type == EventType::InternalError)
            message = e.message;
    }
    fs::remove(target);

    REQUIRE_FALSE(started);
    CAPTURE(message);
    REQUIRE(message.find("not executable") != std::string::npos);
    REQUIRE(message.find("chmod +x") != std::string::npos);
}

TEST_CASE("Start without Init reports internal error", "[init]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    dbg.StartOnThread();
    const auto err = dbg.WaitForInternalError();
    dbg.JoinThread();
    REQUIRE(err.message.find("without Init") != std::string::npos);
}

TEST_CASE("Launch setup failure from child is reported cleanly", "[process]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("end_immediately").c_str(), nullptr, "/nonexistent/elfbug_bad_cwd"));
    dbg.StartOnThread();
    const auto err = dbg.WaitForInternalError();
    dbg.JoinThread();
    REQUIRE(err.message.find("chdir failed") != std::string::npos);
}

TEST_CASE("Exit code is propagated", "[process]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("exit_code_42").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(exit_ev.exitCode == 42);
}

TEST_CASE("SystemBreakpoint fires exactly once", "[process]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("end_immediately").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(exit_ev.exitCode == 0);
    REQUIRE(dbg.count(EventType::SystemBreakpoint) == 1);
}

TEST_CASE("Attach rejects invalid pid cleanly", "[process]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE_FALSE(dbg.Attach(-1));
    REQUIRE(dbg.count(EventType::InternalError) >= 1);
}

TEST_CASE("Pause interrupts running process", "[control]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("run_endlessly").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();

    REQUIRE(dbg.WaitForRunning());
    dbg.Pause();
    dbg.WaitForPaused();

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(dbg.count(EventType::Paused) >= 1);
}

TEST_CASE("Inferior is launched in its own process group", "[control]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("run_endlessly").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();
    REQUIRE(dbg.WaitForRunning());

    REQUIRE(dbg.process() != nullptr);
    const auto inferiorPid = dbg.process()->pid;
    REQUIRE(inferiorPid > 0);
    REQUIRE(getpgid(inferiorPid) == inferiorPid);
    REQUIRE(getpgid(inferiorPid) != getpgrp());

    REQUIRE(dbg.Stop());
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(exit_ev.exitCode == -SIGKILL);
}

TEST_CASE("Stop kills running process cleanly", "[control]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("run_endlessly").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();

    REQUIRE(dbg.WaitForRunning());
    REQUIRE(dbg.Stop());
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == -SIGKILL);
}

TEST_CASE("Stop returns false after process already exited", "[control]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("end_immediately").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == 0);
    REQUIRE_FALSE(dbg.Stop());
}

TEST_CASE("Pause after process exit is a no-op", "[control]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("end_immediately").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == 0);

    dbg.Pause();

    REQUIRE(dbg.count(EventType::InternalError) == 0);
    REQUIRE(dbg.count(EventType::Paused) == 0);
}

TEST_CASE("Reuse Debugger instance after exit", "[init]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;

    REQUIRE(dbg.Init(FIXTURE("end_immediately").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();
    auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(exit_ev.exitCode == 0);

    REQUIRE(dbg.Init(FIXTURE("exit_code_42").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();
    exit_ev = dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(exit_ev.exitCode == 42);

    REQUIRE(dbg.count(EventType::CreateProcess) == 2);
    REQUIRE(dbg.count(EventType::ExitProcess) == 2);
    REQUIRE(dbg.count(EventType::SystemBreakpoint) == 2);
    REQUIRE(dbg.count(EventType::InternalError) == 0);
}

TEST_CASE("Step requests while running are ignored", "[control]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("run_endlessly").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();
    REQUIRE(dbg.WaitForRunning());

    dbg.StepOver();
    dbg.StepInto();

    dbg.Pause();
    dbg.WaitForPaused();
    dbg.Continue();
    REQUIRE(dbg.WaitForRunning());

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(dbg.count(EventType::Step) == 0);
}

TEST_CASE("Pause while already paused does not queue a stop", "[control]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("run_endlessly");
    REQUIRE(dbg.Init(path.c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();

    dbg.Pause();
    dbg.Continue();
    REQUIRE(dbg.WaitForRunning());
    REQUIRE_THROWS_AS(dbg.WaitFor(EventType::Paused, std::chrono::milliseconds(500)), WaitTimeout);

    dbg.Pause();
    dbg.WaitForPaused();
    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
}
