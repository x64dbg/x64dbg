#include <catch2/catch_test_macros.hpp>
#include "TestHarness.h"
#include "SymbolHelper.h"
#include <ElfBug/api/elfbug_api.h>
#include <string>
#include <chrono>
#include <cinttypes>
#include <condition_variable>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <future>
#include <mutex>
#include <optional>
#include <set>
#include <thread>
#include <vector>
#include <unistd.h>

#define FIXTURE(name) (std::string(ELFBUG_TESTS_TARGETS_DIR "/") + (name))

namespace
{
    struct ResolvedBreakpoint
    {
        std::optional<ElfBug::ptr> address;
        std::uint8_t originalByte = 0;
    };

    struct BreakpointPatchRoundTrip
    {
        std::optional<ElfBug::ptr> address;
        std::uint8_t originalByte = 0;
        std::optional<std::uint8_t> patchedByte;
        std::optional<std::uint8_t> restoredByte;
        bool setSucceeded = false;
        bool deleteSucceeded = false;
    };

    std::optional<std::uint8_t> ReadProcessByte(const ElfBug::Process* process, const ElfBug::ptr address)
    {
        if(!process)
            return std::nullopt;

        std::uint8_t byte = 0;
        if(!process->MemRead(address, &byte, 1))
            return std::nullopt;
        return byte;
    }

    bool WaitForProcessByte(const ElfBug::Process* process, const ElfBug::ptr address, const std::uint8_t expected,
                            const std::chrono::milliseconds timeout = std::chrono::seconds(1))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            const auto byte = ReadProcessByte(process, address);
            if(byte && *byte == expected)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    // Reference parse of /proc/<pid>/maps, normalized the same way the engine
    // normalizes pathnames, so an exact line-by-line comparison is meaningful.
    std::vector<ElfBugMemRegion> ParseProcMaps(const pid_t pid)
    {
        std::vector<ElfBugMemRegion> out;
        std::ifstream f("/proc/" + std::to_string(pid) + "/maps");
        std::string line;
        while(std::getline(f, line))
        {
            uint64_t start = 0, end = 0;
            char perms[8] = {};
            int pathOffset = 0;
            if(sscanf(line.c_str(), "%" SCNx64 "-%" SCNx64 " %4s %*x %*x:%*x %*u %n",
                      &start, &end, perms, &pathOffset) < 3)
                continue;

            ElfBugMemRegion r{};
            r.start = start;
            r.end = end;
            r.read = perms[0] == 'r';
            r.write = perms[1] == 'w';
            r.execute = perms[2] == 'x';
            r.shared = perms[3] == 's';

            std::string pathname;
            if(pathOffset > 0 && pathOffset < static_cast<int>(line.size()))
            {
                const char* p = line.c_str() + pathOffset;
                while(*p == ' ' || *p == '\t') ++p;
                size_t len = strlen(p);
                while(len > 0 && (p[len - 1] == '\n' || p[len - 1] == '\r' || p[len - 1] == ' '))
                    --len;
                if(len > 0 && p[0] != '[')
                    pathname.assign(p, len);

                const std::string deletedSuffix = " (deleted)";
                if(pathname.size() >= deletedSuffix.size() &&
                        pathname.compare(pathname.size() - deletedSuffix.size(), deletedSuffix.size(), deletedSuffix) == 0)
                    pathname.resize(pathname.size() - deletedSuffix.size());
            }

            std::snprintf(r.path, sizeof(r.path), "%s", pathname.c_str());
            out.push_back(r);
        }
        return out;
    }
}

TEST_CASE("Init fails cleanly for missing binary", "[init]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE_FALSE(dbg.Init("/nonexistent/elfbug_missing_fixture"));
    REQUIRE(dbg.count(EventType::InternalError) >= 1);
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

TEST_CASE("Init succeeds for valid binary", "[init]")
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
}

TEST_CASE("Launch and clean exit code 0", "[process]")
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

TEST_CASE("Software breakpoint: persistent hits twice", "[breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("hello_elfbug");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<ResolvedBreakpoint> bpPromise;
    auto bpFuture = bpPromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        ResolvedBreakpoint bp;
        const auto resolved = ResolveRuntimeAddress(path, dbg.process()->pid, "hit_me");
        if(resolved)
        {
            bp.address = resolved;
            const auto originalByte = ReadProcessByte(dbg.process(), *resolved);
            if(originalByte)
                bp.originalByte = *originalByte;
            dbg.process()->SetBreakpoint(*resolved, /*singleshot=*/false, ElfBug::SoftwareType::ShortInt3);
        }
        bpPromise.set_value(bp);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto bp = bpFuture.get();
    REQUIRE(bp.address.has_value());
    REQUIRE(WaitForProcessByte(dbg.process(), *bp.address, 0xCC));

    dbg.Continue();
    const auto firstHit = dbg.WaitForBreakpointAt(*bp.address);
    REQUIRE(firstHit.address == *bp.address);
    REQUIRE(firstHit.pid != 0);
    REQUIRE(firstHit.instructionPointer == *bp.address);
    REQUIRE(WaitForProcessByte(dbg.process(), *bp.address, 0xCC));

    dbg.Continue();
    const auto secondHit = dbg.WaitForBreakpointAt(*bp.address, std::chrono::seconds(5));
    REQUIRE(secondHit.address == *bp.address);
    REQUIRE(secondHit.pid != 0);
    REQUIRE(secondHit.instructionPointer == *bp.address);
    REQUIRE(WaitForProcessByte(dbg.process(), *bp.address, 0xCC));

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == 0);
    REQUIRE(dbg.count(EventType::Breakpoint) == 2);
}

TEST_CASE("Software breakpoint patches and restores instruction byte", "[breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("hello_elfbug");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<BreakpointPatchRoundTrip> bpPromise;
    auto bpFuture = bpPromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        BreakpointPatchRoundTrip bp;
        const auto resolved = ResolveRuntimeAddress(path, dbg.process()->pid, "hit_me");
        if(resolved)
        {
            bp.address = resolved;
            const auto originalByte = ReadProcessByte(dbg.process(), *resolved);
            if(originalByte)
                bp.originalByte = *originalByte;
            bp.setSucceeded = dbg.process()->SetBreakpoint(*resolved, /*singleshot=*/false, ElfBug::SoftwareType::ShortInt3);
            bp.patchedByte = ReadProcessByte(dbg.process(), *resolved);
            bp.deleteSucceeded = dbg.process()->DeleteBreakpoint(*resolved);
            bp.restoredByte = ReadProcessByte(dbg.process(), *resolved);
        }
        bpPromise.set_value(bp);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto bp = bpFuture.get();
    REQUIRE(bp.address.has_value());
    REQUIRE(bp.originalByte != 0xCC);
    REQUIRE(bp.setSucceeded);
    REQUIRE(bp.patchedByte.has_value());
    REQUIRE(*bp.patchedByte == 0xCC);
    REQUIRE(bp.deleteSucceeded);
    REQUIRE(bp.restoredByte.has_value());
    REQUIRE(*bp.restoredByte == bp.originalByte);

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == 0);
}

TEST_CASE("Software breakpoint: singleshot hits once", "[breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("hello_elfbug");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<ResolvedBreakpoint> bpPromise;
    auto bpFuture = bpPromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        ResolvedBreakpoint bp;
        const auto resolved = ResolveRuntimeAddress(path, dbg.process()->pid, "hit_me");
        if(resolved)
        {
            bp.address = resolved;
            const auto originalByte = ReadProcessByte(dbg.process(), *resolved);
            if(originalByte)
                bp.originalByte = *originalByte;
            dbg.process()->SetBreakpoint(*resolved, /*singleshot=*/true, ElfBug::SoftwareType::ShortInt3);
        }
        bpPromise.set_value(bp);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto bp = bpFuture.get();
    REQUIRE(bp.address.has_value());
    REQUIRE(WaitForProcessByte(dbg.process(), *bp.address, 0xCC));

    dbg.Continue();
    const auto hit = dbg.WaitForBreakpointAt(*bp.address);
    REQUIRE(hit.address == *bp.address);
    REQUIRE(hit.pid != 0);
    REQUIRE(hit.instructionPointer == *bp.address);
    REQUIRE(WaitForProcessByte(dbg.process(), *bp.address, bp.originalByte));

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == 0);
    REQUIRE(dbg.count(EventType::Breakpoint) == 1);
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

TEST_CASE("SIGSEGV pauses and forwards on continue", "[exception]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("segfault").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    dbg.Continue();

    const auto exc_ev = dbg.WaitForException(SIGSEGV);
    REQUIRE(exc_ev.address == 0);

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == -SIGSEGV);
}

TEST_CASE("Step fires cbStep on single instruction", "[step]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("hello_elfbug").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();

    dbg.StepInto();
    dbg.WaitForStep();

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == 0);
    REQUIRE(dbg.count(EventType::Step) >= 1);
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

TEST_CASE("Memory map matches /proc/<pid>/maps with permissions", "[memmap]")
{
    struct MemMapSync
    {
        std::mutex m;
        std::condition_variable cv;
        bool systemBreakpoint = false;
    } sync;

    ElfBugCallbacks cb = {};
    cb.userdata = &sync;
    cb.onSystemBreakpoint = [](void* userdata)
    {
        auto* s = static_cast<MemMapSync*>(userdata);
        {
            std::lock_guard<std::mutex> lock(s->m);
            s->systemBreakpoint = true;
        }
        s->cv.notify_all();
    };

    ElfBugDebugger* dbg = ElfBugCreate(&cb);
    REQUIRE(dbg != nullptr);
    REQUIRE(ElfBugInit(dbg, FIXTURE("hello_elfbug").c_str()));

    std::thread loop([&] { ElfBugStart(dbg); });

    {
        std::unique_lock<std::mutex> lock(sync.m);
        REQUIRE(sync.cv.wait_for(lock, std::chrono::seconds(5),
                                 [&] { return sync.systemBreakpoint; }));
    }

    // The tracee is stopped at the system breakpoint, so its maps cannot change
    // between the engine's snapshot and our own read of /proc below.
    const pid_t pid = ElfBugGetPid(dbg);
    REQUIRE(pid > 0);

    const size_t count = ElfBugGetMemoryMap(dbg, nullptr, 0);
    REQUIRE(count > 0);

    std::vector<ElfBugMemRegion> regions(count);
    REQUIRE(ElfBugGetMemoryMap(dbg, regions.data(), regions.size()) == count);

    const auto expected = ParseProcMaps(pid);
    REQUIRE(regions.size() == expected.size());

    bool sawExecutable = false;
    bool sawWritable = false;
    bool sawFixture = false;
    for(size_t i = 0; i < regions.size(); ++i)
    {
        INFO("region " << i);
        REQUIRE(regions[i].start == expected[i].start);
        REQUIRE(regions[i].end == expected[i].end);
        REQUIRE(regions[i].read == expected[i].read);
        REQUIRE(regions[i].write == expected[i].write);
        REQUIRE(regions[i].execute == expected[i].execute);
        REQUIRE(regions[i].shared == expected[i].shared);
        REQUIRE(std::string(regions[i].path) == std::string(expected[i].path));

        sawExecutable = sawExecutable || regions[i].execute;
        sawWritable = sawWritable || regions[i].write;
        sawFixture = sawFixture || std::string(regions[i].path).find("hello_elfbug") != std::string::npos;
    }
    REQUIRE(sawExecutable);
    REQUIRE(sawWritable);
    REQUIRE(sawFixture);

    // Capacity contract: a short buffer still reports the full total and fills only what fits.
    std::vector<ElfBugMemRegion> partial(1);
    REQUIRE(ElfBugGetMemoryMap(dbg, partial.data(), partial.size()) == count);
    REQUIRE(partial[0].start == regions[0].start);

    ElfBugContinue(dbg);
    loop.join();

    // The map is cleared once the inferior exits.
    REQUIRE(ElfBugGetMemoryMap(dbg, nullptr, 0) == 0);

    ElfBugDestroy(dbg);
}
