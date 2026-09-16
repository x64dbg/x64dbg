#include <catch2/catch_test_macros.hpp>
#include "TestHarness.h"
#include "SymbolHelper.h"
#include <ElfBug/process/StepOver.h>
#include <ElfBug/process/ProcessList.h>
#include <ElfBug/api/elfbug_api.h>
#include <condition_variable>
#include <mutex>
#include <string>
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <optional>
#include <set>
#include <thread>
#include <unistd.h>
#include <sched.h>
#include <sys/resource.h>

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

    // Raw read: MemRead hides the patch byte these assertions are about.
    std::optional<std::uint8_t> ReadProcessByte(const ElfBug::Process* process, const ElfBug::ptr address)
    {
        if(!process)
            return std::nullopt;

        std::uint8_t byte = 0;
        if(!process->MemReadRaw(address, &byte, 1))
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

    bool WaitForTraceeValue(const ElfBug::Process* process, const ElfBug::ptr address,
                            const int expected,
                            const std::chrono::milliseconds timeout = std::chrono::seconds(2))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            int value = 0;
            if(process && process->MemReadRaw(address, &value, sizeof(value)) && value == expected)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    // The session's Process is gone once the detach completes, so read through /proc directly.
    bool WaitForDetachedValue(const pid_t pid, const ElfBug::ptr address, const int expected,
                              const std::chrono::milliseconds timeout = std::chrono::seconds(2))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            std::ifstream mem("/proc/" + std::to_string(pid) + "/mem", std::ios::binary);
            if(mem)
            {
                mem.seekg(static_cast<std::streamoff>(address));
                int value = 0;
                if(mem.read(reinterpret_cast<char*>(&value), sizeof(value)) && value == expected)
                    return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    // Running() goes true before execl() replaces the image; this waits for the real exec.
    bool WaitForExeced(const pid_t pid, const std::string & path,
                       const std::chrono::milliseconds timeout = std::chrono::seconds(2))
    {
        std::error_code ec;
        const auto want = std::filesystem::canonical(path, ec);
        if(ec)
            return false;
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            std::error_code linkEc;
            const auto have = std::filesystem::canonical("/proc/" + std::to_string(pid) + "/exe", linkEc);
            if(!linkEc && have == want)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    pid_t WaitForClonedChild(const pid_t parent,
                             const std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        const auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < timeout)
        {
            for(const auto & entry : std::filesystem::directory_iterator("/proc"))
            {
                const auto name = entry.path().filename().string();
                if(name.find_first_not_of("0123456789") != std::string::npos)
                    continue;
                const auto pid = static_cast<pid_t>(std::stol(name));
                if(pid != parent && ElfBug::ParentPid(pid) == parent)
                    return pid;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return 0;
    }
}

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

TEST_CASE("ClassifyStepOver recognises call", "[stepover]")
{
    using namespace ElfBug;
    // e8 00 00 00 00 : call rel32 (+0)
    const uint8 bytes[] = {0xe8, 0x00, 0x00, 0x00, 0x00};
    ptr next = 0;
    REQUIRE(ClassifyStepOver(bytes, sizeof(bytes), 0x1000, next) == StepOverKind::Call);
    REQUIRE(next == 0x1005);
}

TEST_CASE("ClassifyStepOver recognises indirect call", "[stepover]")
{
    using namespace ElfBug;
    // ff d0 : call rax
    const uint8 bytes[] = {0xff, 0xd0};
    ptr next = 0;
    REQUIRE(ClassifyStepOver(bytes, sizeof(bytes), 0x1000, next) == StepOverKind::Call);
    REQUIRE(next == 0x1002);
}

TEST_CASE("ClassifyStepOver recognises rep-prefixed string ops", "[stepover]")
{
    using namespace ElfBug;
    // f3 a4 : rep movsb
    const uint8 movsb[] = {0xf3, 0xa4};
    ptr next = 0;
    REQUIRE(ClassifyStepOver(movsb, sizeof(movsb), 0x1000, next) == StepOverKind::Rep);
    REQUIRE(next == 0x1002);

    // f3 48 ab : rep stosq
    const uint8 stosq[] = {0xf3, 0x48, 0xab};
    next = 0;
    REQUIRE(ClassifyStepOver(stosq, sizeof(stosq), 0x2000, next) == StepOverKind::Rep);
    REQUIRE(next == 0x2003);

    // f2 ae : repne scasb
    const uint8 scasb[] = {0xf2, 0xae};
    next = 0;
    REQUIRE(ClassifyStepOver(scasb, sizeof(scasb), 0x3000, next) == StepOverKind::Rep);
    REQUIRE(next == 0x3002);
}

TEST_CASE("ClassifyStepOver ignores rep prefixes on non-string instructions", "[stepover]")
{
    using namespace ElfBug;
    ptr next = 0;

    // f3 c3 : rep ret (AMD branch padding) - never falls through to a planted trap
    const uint8 repRet[] = {0xf3, 0xc3};
    REQUIRE(ClassifyStepOver(repRet, sizeof(repRet), 0x1000, next) == StepOverKind::None);
    REQUIRE(next == 0);

    // f3 90 : pause - the F3 is a hint, nothing repeats
    const uint8 pause_[] = {0xf3, 0x90};
    REQUIRE(ClassifyStepOver(pause_, sizeof(pause_), 0x1000, next) == StepOverKind::None);
}

TEST_CASE("ClassifyStepOver recognises pushfq", "[stepover]")
{
    using namespace ElfBug;
    // 9c : pushfq (in 64-bit mode)
    const uint8 bytes[] = {0x9c};
    ptr next = 0;
    REQUIRE(ClassifyStepOver(bytes, sizeof(bytes), 0x1000, next) == StepOverKind::Pushf);
    REQUIRE(next == 0x1001);
}

TEST_CASE("ClassifyStepOver rejects ordinary instructions", "[stepover]")
{
    using namespace ElfBug;
    ptr next = 0;

    const uint8 nop[] = {0x90};                          // nop
    REQUIRE(ClassifyStepOver(nop, sizeof(nop), 0x1000, next) == StepOverKind::None);
    REQUIRE(next == 0);

    const uint8 ret[] = {0xc3};                          // ret
    REQUIRE(ClassifyStepOver(ret, sizeof(ret), 0x1000, next) == StepOverKind::None);

    const uint8 jmp[] = {0xeb, 0x00};                    // jmp short +0
    REQUIRE(ClassifyStepOver(jmp, sizeof(jmp), 0x1000, next) == StepOverKind::None);

    const uint8 mov[] = {0x48, 0x89, 0xd8};              // mov rax, rbx
    REQUIRE(ClassifyStepOver(mov, sizeof(mov), 0x1000, next) == StepOverKind::None);

    const uint8 syscall_[] = {0x0f, 0x05};               // syscall
    REQUIRE(ClassifyStepOver(syscall_, sizeof(syscall_), 0x1000, next) == StepOverKind::None);
}

TEST_CASE("ClassifyStepOver handles undecodable and empty input", "[stepover]")
{
    using namespace ElfBug;
    ptr next = 0;
    REQUIRE(ClassifyStepOver(nullptr, 0, 0x1000, next) == StepOverKind::None);
    REQUIRE(next == 0);

    const uint8 garbage[] = {0xff, 0xff, 0xff};
    (void)ClassifyStepOver(garbage, sizeof(garbage), 0x1000, next);  // must not crash
}

TEST_CASE("HasBreakpoint reflects set and delete", "[stepover]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("hello_elfbug");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> addrPromise;
    auto addrFuture = addrPromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto resolved = ResolveRuntimeAddress(path, dbg.process()->pid, "hit_me");
        if(resolved)
        {
            REQUIRE_FALSE(dbg.process()->HasBreakpoint(*resolved));
            dbg.process()->SetBreakpoint(*resolved, false, ElfBug::SoftwareType::ShortInt3);
            REQUIRE(dbg.process()->HasBreakpoint(*resolved));
            dbg.process()->DeleteBreakpoint(*resolved);
            REQUIRE_FALSE(dbg.process()->HasBreakpoint(*resolved));
        }
        addrPromise.set_value(resolved);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    REQUIRE(addrFuture.get().has_value());

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(exit_ev.exitCode == 0);
}

TEST_CASE("StepOver steps over a call", "[stepover]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    struct CallSite
    {
        std::optional<ElfBug::ptr> site;
        std::optional<ElfBug::ptr> ranFlag;
    };

    std::promise<CallSite> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        CallSite s;
        s.site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_call_site");
        s.ranFlag = ResolveRuntimeAddress(path, dbg.process()->pid, "so_callee_ran");
        if(s.site)
            dbg.process()->SetBreakpoint(*s.site, false, ElfBug::SoftwareType::ShortInt3);
        sitePromise.set_value(s);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto s = siteFuture.get();
    REQUIRE(s.site.has_value());
    REQUIRE(s.ranFlag.has_value());

    dbg.Continue();
    dbg.WaitForBreakpointAt(*s.site);

    std::int32_t ranBefore = -1;
    REQUIRE(dbg.process()->MemRead(*s.ranFlag, &ranBefore, sizeof(ranBefore)));
    REQUIRE(ranBefore == 0);

    dbg.StepOver();
    const auto step = dbg.WaitForStep();

    // `call so_callee` is e8 rel32 == 5 bytes.
    REQUIRE(step.instructionPointer == *s.site + 5);

    // Stepped over, not skipped: the callee really ran.
    std::int32_t ranAfter = -1;
    REQUIRE(dbg.process()->MemRead(*s.ranFlag, &ranAfter, sizeof(ranAfter)));
    REQUIRE(ranAfter == 1);

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(exit_ev.exitCode == 0);
}

TEST_CASE("StepOver runs a rep-prefixed instruction to completion", "[stepover]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    struct RepSite
    {
        std::optional<ElfBug::ptr> address;
        std::uint8_t bytes[2] = {};
    };

    std::promise<RepSite> promise;
    auto future = promise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        RepSite s;
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_rep_insn");
        if(site)
        {
            s.address = *site;
            (void)dbg.process()->MemRead(*s.address, s.bytes, sizeof(s.bytes));
            dbg.process()->SetBreakpoint(*s.address, false, ElfBug::SoftwareType::ShortInt3);
        }
        promise.set_value(s);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto s = future.get();
    REQUIRE(s.address.has_value());
    REQUIRE(s.bytes[0] == 0xf3);
    REQUIRE(s.bytes[1] == 0xa4);

    dbg.Continue();
    dbg.WaitForBreakpointAt(*s.address);
    REQUIRE(dbg.currentThread()->registers.Gcx() == 64);

    dbg.StepOver();
    const auto step = dbg.WaitForStep();

    // One event past the whole instruction, not one per iteration.
    REQUIRE(step.instructionPointer == *s.address + 2);
    REQUIRE(dbg.currentThread()->registers.Gcx() == 0);

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(dbg.count(EventType::Step) == 1);
}

TEST_CASE("StepOver falls back to single-step on a plain instruction", "[stepover]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_plain_site");
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

    dbg.StepOver();
    const auto step = dbg.WaitForStep();

    // `nop` is 1 byte; behaviour must be identical to StepInto.
    REQUIRE(step.instructionPointer == *site + 1);

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("StepOver of pushfq does not leak the trap flag", "[stepover]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_pushf_site");
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

    // so_pushf_site's first instruction IS pushfq (1 byte, 0x9c).
    dbg.StepOver();
    const auto step = dbg.WaitForStep();
    REQUIRE(step.instructionPointer == *site + 1);

    const ElfBug::ptr rsp = dbg.currentThread()->registers.Gsp();
    std::uint64_t pushed = 0;
    REQUIRE(dbg.process()->MemRead(rsp, &pushed, sizeof(pushed)));
    REQUIRE((pushed & (1ull << 8)) == 0);

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

// A plain PTRACE_SINGLESTEP would push EFLAGS with TF set.
TEST_CASE("StepInto of pushfq does not leak the trap flag", "[step]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_pushf_site");
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

    dbg.StepInto();
    const auto step = dbg.WaitForStep();
    REQUIRE(step.instructionPointer == *site + 1);

    const ElfBug::ptr rsp = dbg.currentThread()->registers.Gsp();
    std::uint64_t pushed = 0;
    REQUIRE(dbg.process()->MemRead(rsp, &pushed, sizeof(pushed)));
    REQUIRE((pushed & (1ull << 8)) == 0);

    // The breakpoint byte must be re-armed afterwards.
    REQUIRE(WaitForProcessByte(dbg.process(), *site, 0xCC));

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("StepOver respects the stack frame under recursion", "[stepover]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_recurse_site");
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
    // Out of the way: only the step-over's own breakpoint at site + 5 is in play now.
    REQUIRE(dbg.process()->DeleteBreakpoint(*site));

    // The callee reaches site + 5 at deeper frames first; those hits must be skipped.
    const ElfBug::ptr rspBefore = dbg.currentThread()->registers.Gsp();

    dbg.StepOver();
    const auto step = dbg.WaitForStep();

    REQUIRE(step.instructionPointer == *site + 5);
    REQUIRE(dbg.currentThread()->registers.Gsp() >= rspBefore);
    REQUIRE(dbg.count(EventType::Breakpoint) == 1);

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("A user breakpoint at the step-over target still fires for inner frames", "[stepover]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_recurse_site");
        if(site)
        {
            // Singleshot: the recursion must not stop at the call site again.
            dbg.process()->SetBreakpoint(*site, true, ElfBug::SoftwareType::ShortInt3);
            dbg.process()->SetBreakpoint(*site + 5, false, ElfBug::SoftwareType::ShortInt3);
        }
        sitePromise.set_value(site);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto site = siteFuture.get();
    REQUIRE(site.has_value());

    dbg.Continue();
    dbg.WaitForBreakpointAt(*site);
    const ElfBug::ptr rspBefore = dbg.currentThread()->registers.Gsp();

    // The step-over rides the user's breakpoint at the return address. Deeper frames
    // reach it first: not the completion, but still the user's breakpoint.
    dbg.StepOver();
    const auto inner = dbg.WaitForBreakpointAt(*site + 5);
    REQUIRE(inner.instructionPointer == *site + 5);
    REQUIRE(dbg.currentThread()->registers.Gsp() < rspBefore);

    dbg.Continue();
    const auto middle = dbg.WaitForBreakpointAt(*site + 5);
    REQUIRE(middle.instructionPointer == *site + 5);

    dbg.Continue();
    const auto outer = dbg.WaitForBreakpointAt(*site + 5);
    REQUIRE(outer.instructionPointer == *site + 5);
    REQUIRE(dbg.currentThread()->registers.Gsp() >= rspBefore);

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(exit_ev.exitCode == 0);
    // Reporting the inner hit cancelled the step-over, so no Step ever completes.
    REQUIRE(dbg.count(EventType::Step) == 0);
    REQUIRE(dbg.count(EventType::Breakpoint) == 4);
}

TEST_CASE("A breakpoint inside the callee cancels the step-over", "[stepover]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    struct Sites
    {
        std::optional<ElfBug::ptr> site;
        std::optional<ElfBug::ptr> callee;
        std::uint8_t returnByte = 0;
    };

    std::promise<Sites> promise;
    auto future = promise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        Sites s;
        s.site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_call_site");
        s.callee = ResolveRuntimeAddress(path, dbg.process()->pid, "so_callee");
        if(s.site)
        {
            const auto byte = ReadProcessByte(dbg.process(), *s.site + 5);
            if(byte)
                s.returnByte = *byte;
            dbg.process()->SetBreakpoint(*s.site, false, ElfBug::SoftwareType::ShortInt3);
        }
        if(s.callee)
            dbg.process()->SetBreakpoint(*s.callee, false, ElfBug::SoftwareType::ShortInt3);
        promise.set_value(s);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto s = future.get();
    REQUIRE(s.site.has_value());
    REQUIRE(s.callee.has_value());

    dbg.Continue();
    dbg.WaitForBreakpointAt(*s.site);

    dbg.StepOver();

    const auto hit = dbg.WaitForBreakpointAt(*s.callee);
    REQUIRE(hit.address == *s.callee);

    // The step-over was cancelled, so its INT3 must be gone from the return address.
    REQUIRE(WaitForProcessByte(dbg.process(), *s.site + 5, s.returnByte));

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("StepOver rides an existing breakpoint without deleting it", "[stepover]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_call_site");
        if(site)
        {
            dbg.process()->SetBreakpoint(*site, false, ElfBug::SoftwareType::ShortInt3);
            dbg.process()->SetBreakpoint(*site + 5, false, ElfBug::SoftwareType::ShortInt3);
        }
        sitePromise.set_value(site);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto site = siteFuture.get();
    REQUIRE(site.has_value());

    dbg.Continue();
    dbg.WaitForBreakpointAt(*site);

    dbg.StepOver();

    const auto step = dbg.WaitForStep();
    REQUIRE(step.instructionPointer == *site + 5);

    // We did not plant it, so we must not have removed it.
    REQUIRE(dbg.process()->HasBreakpoint(*site + 5));

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("StepOver re-arms the breakpoint it stepped off", "[stepover]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_call_site");
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

    // Arming lifts the 0xCC under RIP so the real `call` executes.
    dbg.StepOver();
    const auto step = dbg.WaitForStep();
    REQUIRE(step.instructionPointer == *site + 5);

    // Completion must re-arm it.
    REQUIRE(dbg.process()->HasBreakpoint(*site));
    REQUIRE(WaitForProcessByte(dbg.process(), *site, 0xCC));

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("StepOver re-arms the breakpoint it stepped off via the single-step fallback", "[stepover]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    // so_plain_site is a `nop`, so the step-over classifies as None and falls back to a
    // single step - a different restore path from the completion one above.
    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_plain_site");
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

    dbg.StepOver();
    const auto step = dbg.WaitForStep();
    REQUIRE(step.instructionPointer == *site + 1);

    REQUIRE(dbg.process()->HasBreakpoint(*site));
    REQUIRE(WaitForProcessByte(dbg.process(), *site, 0xCC));

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("Continuing past a breakpoint preserves its callback", "[breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    // so_recurse is entered three times. Every resume in between lifts and restores its
    // patch byte, which used to take the registered callback with it.
    std::atomic<int> callbackHits{0};
    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_recurse");
        if(site)
        {
            const ElfBug::BreakpointCallback onHit = [&](const ElfBug::BreakpointInfo &) { ++callbackHits; };
            dbg.process()->SetBreakpoint(*site, onHit, false, ElfBug::SoftwareType::ShortInt3);
        }
        sitePromise.set_value(site);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto site = siteFuture.get();
    REQUIRE(site.has_value());

    for(int expected = 1; expected <= 3; ++expected)
    {
        dbg.Continue();
        dbg.WaitForBreakpointAt(*site);
        REQUIRE(callbackHits.load() == expected);
        REQUIRE(WaitForProcessByte(dbg.process(), *site, 0xCC));
    }

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(callbackHits.load() == 3);
}

TEST_CASE("StepOver preserves the callback of the breakpoint it stepped off", "[stepover][breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::atomic<int> callbackHits{0};
    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_call_site");
        if(site)
        {
            const ElfBug::BreakpointCallback onHit = [&](const ElfBug::BreakpointInfo &) { ++callbackHits; };
            dbg.process()->SetBreakpoint(*site, onHit, false, ElfBug::SoftwareType::ShortInt3);
        }
        sitePromise.set_value(site);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto site = siteFuture.get();
    REQUIRE(site.has_value());

    dbg.Continue();
    dbg.WaitForBreakpointAt(*site);
    REQUIRE(callbackHits.load() == 1);

    dbg.StepOver();
    const auto step = dbg.WaitForStep();
    REQUIRE(step.instructionPointer == *site + 5);

    // so_call_site runs once; assert the registration directly instead of waiting for
    // a second hit.
    REQUIRE(dbg.process()->HasBreakpointCallback(*site));
    REQUIRE(dbg.process()->HasBreakpoint(*site));

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("MemRead hides breakpoint patches, MemReadRaw does not", "[breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    struct Bytes
    {
        std::optional<ElfBug::ptr> site;
        std::uint8_t original[4] = {};
        std::uint8_t masked[4] = {};
        std::uint8_t raw[4] = {};
    };

    std::promise<Bytes> promise;
    auto future = promise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        Bytes b;
        b.site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_call_site");
        if(b.site)
        {
            (void)dbg.process()->MemRead(*b.site, b.original, sizeof(b.original));
            dbg.process()->SetBreakpoint(*b.site, false, ElfBug::SoftwareType::ShortInt3);
            (void)dbg.process()->MemRead(*b.site, b.masked, sizeof(b.masked));
            (void)dbg.process()->MemReadRaw(*b.site, b.raw, sizeof(b.raw));
            // Drop it again so the target runs to exit without stopping.
            dbg.process()->DeleteBreakpoint(*b.site);
        }
        promise.set_value(b);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto b = future.get();
    REQUIRE(b.site.has_value());

    // `call rel32` - the opcode is what the patch replaces.
    REQUIRE(b.original[0] == 0xe8);
    REQUIRE(b.raw[0] == 0xCC);
    REQUIRE(b.masked[0] == 0xe8);
    // Bytes outside the patch are untouched either way.
    REQUIRE(std::equal(b.original + 1, b.original + 4, b.masked + 1));
    REQUIRE(std::equal(b.original + 1, b.original + 4, b.raw + 1));

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("StepOver a call to the next instruction (get-PC idiom)", "[stepover]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_getpc_site");
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

    // `call 1f` is e8 rel32 == 5 bytes, and its destination IS site+5.
    dbg.StepOver();
    const auto step = dbg.WaitFor(EventType::Step, std::chrono::seconds(3));
    REQUIRE(step.instructionPointer == *site + 5);

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("StepInto from a breakpointed instruction executes exactly one instruction", "[step]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_plain_site");
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

    // so_plain_site is a 1-byte nop followed by ret.
    dbg.StepInto();
    const auto step = dbg.WaitFor(EventType::Step, std::chrono::seconds(3));
    REQUIRE(step.instructionPointer == *site + 1);

    REQUIRE(WaitForProcessByte(dbg.process(), *site, 0xCC));

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

// The image is replaced mid-step-over; nothing may be written back afterwards. With
// ASLR off, a re-exec puts real code exactly where a stray 0xCC would land.
TEST_CASE("Step-over across an execve does not write into the new image", "[stepover][exec]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("exec_target");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "eo_exec_site");
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

    // The callee never returns - it execs. The planted return breakpoint and the lifted
    // source byte both have to be dropped without a poke.
    dbg.StepOver();

    const auto exit_ev = dbg.WaitFor(EventType::ExitProcess, std::chrono::seconds(10));
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == 7);
    REQUIRE(dbg.count(EventType::InternalError) == 0);
    // A 0xCC left behind would trap the second pass against a stale record.
    REQUIRE(dbg.count(EventType::Breakpoint) == 1);
}

// MemRead hides the patch byte, so a caller that reads, edits and writes back would
// otherwise hand us the original byte and silently disarm the breakpoint.
TEST_CASE("MemWrite over an armed breakpoint keeps the trap and retargets the restore", "[breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    struct Result
    {
        std::optional<ElfBug::ptr> site;
        std::uint8_t original[2] = {};
        std::uint8_t rawAfterWrite = 0;
        std::uint8_t maskedAfterWrite = 0;
        std::uint8_t neighbourAfterWrite = 0;
        std::uint8_t afterDelete = 0;
    };

    std::promise<Result> promise;
    auto future = promise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        Result r;
        // `call rel32` - the 0xe8 opcode differs from the 0x90 written over it, which is
        // what separates "restored the write" from "restored the pre-write original".
        r.site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_call_site");
        if(r.site)
        {
            auto* process = dbg.process();
            (void)process->MemRead(*r.site, r.original, sizeof(r.original));
            process->SetBreakpoint(*r.site, false, ElfBug::SoftwareType::ShortInt3);

            // Two bytes: one lands on the breakpoint, one next to it.
            const std::uint8_t patch[2] = {0x90, 0x90};
            process->MemWrite(*r.site, patch, sizeof(patch));

            (void)process->MemReadRaw(*r.site, &r.rawAfterWrite, 1);
            (void)process->MemRead(*r.site, &r.maskedAfterWrite, 1);
            (void)process->MemReadRaw(*r.site + 1, &r.neighbourAfterWrite, 1);

            process->DeleteBreakpoint(*r.site);
            (void)process->MemReadRaw(*r.site, &r.afterDelete, 1);

            // Put the real instruction back so the target still runs to a clean exit.
            process->MemWrite(*r.site, r.original, sizeof(r.original));
        }
        promise.set_value(r);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto r = future.get();
    REQUIRE(r.site.has_value());

    REQUIRE(r.original[0] == 0xe8);
    // The trap survives the write...
    REQUIRE(r.rawAfterWrite == 0xCC);
    // ...while readers see what was written, not the trap and not the stale original.
    REQUIRE(r.maskedAfterWrite == 0x90);
    // A byte next to the breakpoint is written through untouched.
    REQUIRE(r.neighbourAfterWrite == 0x90);
    // Deleting restores what the caller wrote (0x90), not what was there before it (0xe8).
    REQUIRE(r.afterDelete == 0x90);

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

// Breakpoint bytes are poked while a worker, not the leader, is the stopped thread.
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

    // Every worker must report: all-stop closes the window where one could run past a
    // breakpoint another thread has lifted.
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

TEST_CASE("Continue from a breakpoint on pushfq does not leak the trap flag", "[breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_pushf_site");
        if(site)
        {
            // pushfq (1 byte), popq %rax (1 byte), ret
            dbg.process()->SetBreakpoint(*site, false, ElfBug::SoftwareType::ShortInt3);
            dbg.process()->SetBreakpoint(*site + 2, false, ElfBug::SoftwareType::ShortInt3);
        }
        sitePromise.set_value(site);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto site = siteFuture.get();
    REQUIRE(site.has_value());

    dbg.Continue();
    dbg.WaitForBreakpointAt(*site);

    // Stepping off the breakpoint executes pushfq under the hardware trap flag.
    dbg.Continue();
    dbg.WaitForBreakpointAt(*site + 2);
    REQUIRE((dbg.currentThread()->registers.Gax() & (1ull << 8)) == 0);

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("StepOver completing on a singleshot user breakpoint consumes it", "[stepover][breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::atomic<int> callbackHits{0};
    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_call_site");
        if(site)
        {
            dbg.process()->SetBreakpoint(*site, false, ElfBug::SoftwareType::ShortInt3);
            const ElfBug::BreakpointCallback onHit = [&](const ElfBug::BreakpointInfo &) { ++callbackHits; };
            dbg.process()->SetBreakpoint(*site + 5, onHit, true, ElfBug::SoftwareType::ShortInt3);
        }
        sitePromise.set_value(site);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto site = siteFuture.get();
    REQUIRE(site.has_value());

    dbg.Continue();
    dbg.WaitForBreakpointAt(*site);

    dbg.StepOver();
    const auto step = dbg.WaitForStep();
    REQUIRE(step.instructionPointer == *site + 5);

    // The user's breakpoint fired: its callback ran, it was reported, and being
    // singleshot it is gone.
    REQUIRE(callbackHits.load() == 1);
    REQUIRE(dbg.count(EventType::Breakpoint) == 2);
    REQUIRE_FALSE(dbg.process()->HasBreakpoint(*site + 5));

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
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

    // The instruction under the breakpoint faults while being stepped off.
    dbg.Continue();
    const auto exc = dbg.WaitForException(SIGSEGV);
    REQUIRE(exc.address == 0);
    REQUIRE(exc.instructionPointer == *site);

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(exit_ev.exitCode == -SIGSEGV);
    REQUIRE(dbg.count(EventType::Breakpoint) == 1);
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

    // Neither request may latch and turn the next Continue into a step.
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

// The call-site breakpoint is stepped off synchronously, so it is armed again while the
// callee runs. Lifting it for the whole callee would hide it from the recursion and from
// every other thread.
TEST_CASE("A breakpoint on the stepped call fires again for inner frames", "[stepover][breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_recurse_site");
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
    const ElfBug::ptr rspBefore = dbg.currentThread()->registers.Gsp();

    dbg.StepOver();
    const auto inner = dbg.WaitForBreakpointAt(*site);
    REQUIRE(inner.instructionPointer == *site);
    REQUIRE(dbg.currentThread()->registers.Gsp() < rspBefore);
    REQUIRE(WaitForProcessByte(dbg.process(), *site, 0xCC));

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(exit_ev.exitCode == 0);
    REQUIRE(dbg.count(EventType::Step) == 0);
}

TEST_CASE("ClassifyStepOver does not treat SSE scalar ops as repeated", "[stepover]")
{
    using namespace ElfBug;
    ptr next = 0;

    // f2 0f 10 c1 : movsd xmm0, xmm1 - F2 is a mandatory prefix, not a rep
    const uint8 movsdXmm[] = {0xf2, 0x0f, 0x10, 0xc1};
    REQUIRE(ClassifyStepOver(movsdXmm, sizeof(movsdXmm), 0x1000, next) == StepOverKind::None);
    REQUIRE(next == 0);

    // f2 0f c2 c1 00 : cmpsd xmm0, xmm1, 0
    const uint8 cmpsdXmm[] = {0xf2, 0x0f, 0xc2, 0xc1, 0x00};
    REQUIRE(ClassifyStepOver(cmpsdXmm, sizeof(cmpsdXmm), 0x1000, next) == StepOverKind::None);

    // f3 0f 10 c1 : movss xmm0, xmm1
    const uint8 movssXmm[] = {0xf3, 0x0f, 0x10, 0xc1};
    REQUIRE(ClassifyStepOver(movssXmm, sizeof(movssXmm), 0x1000, next) == StepOverKind::None);
}

TEST_CASE("Continue from a breakpoint on a rep instruction reports one hit", "[breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_rep_insn");
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
    REQUIRE(dbg.currentThread()->registers.Gcx() == 64);

    // A single step runs one iteration and leaves RIP on the instruction, so a naive
    // step-off re-traps once per iteration.
    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(dbg.count(EventType::Breakpoint) == 1);
}

// The raw read and the masking step must see the same breakpoint state, or a read that
// observed an armed 0xCC can be masked against a record that is already gone.
TEST_CASE("MemRead never returns a breakpoint byte while breakpoints change", "[breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("step_over_targets");
    REQUIRE(dbg.Init(path.c_str()));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        sitePromise.set_value(ResolveRuntimeAddress(path, dbg.process()->pid, "so_call_site"));
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto site = siteFuture.get();
    REQUIRE(site.has_value());

    auto* process = dbg.process();
    std::atomic<bool> stop{false};
    std::atomic<int> leaked{0};

    // so_call_site is `call rel32`, so 0xe8. A 0xCC there is always the debugger's.
    std::thread reader([&]
    {
        while(!stop.load(std::memory_order_relaxed))
        {
            std::uint8_t byte = 0;
            if(process->MemRead(*site, &byte, 1) && byte == 0xCC)
                leaked.fetch_add(1, std::memory_order_relaxed);
        }
    });

    for(int i = 0; i < 3000; ++i)
    {
        process->SetBreakpoint(*site, false, ElfBug::SoftwareType::ShortInt3);
        process->DeleteBreakpoint(*site);
    }

    stop.store(true, std::memory_order_relaxed);
    reader.join();

    REQUIRE(leaked.load() == 0);

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
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
    // Sum of the fixture's per-worker counters. Each worker writes only its own slot.
    std::uint64_t ReadSpinCounters(const ElfBug::Process* process, const ElfBug::ptr base)
    {
        std::uint64_t slots[4] = {};
        if(!process->MemRead(base, slots, sizeof(slots)))
            return 0;
        return slots[0] + slots[1] + slots[2] + slots[3];
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

    // Pause() signals the whole process, so any thread can take the SIGSTOP, but that one
    // sits in signal-delivery-stop and stopAllThreads freezes the rest: no counter may move.
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

    // All-stop is in effect: the reporting thread is stopped and the sweep froze the
    // rest, so not one worker's counter may move.
    std::uint64_t stopped[4] = {};
    std::uint64_t stillStopped[4] = {};
    REQUIRE(dbg.process()->MemRead(*counters, stopped, sizeof(stopped)));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(dbg.process()->MemRead(*counters, stillStopped, sizeof(stillStopped)));

    for(int i = 0; i < 4; ++i)
        REQUIRE(stillStopped[i] == stopped[i]);

    // The singleshot breakpoint is gone, so nothing stops the tracee again. Every slot
    // has to advance on its own: a sum grows even when a single thread was resumed.
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

// Four workers reach the same breakpoint at once. Whichever the sweep freezes mid-hit
// must still be reported rather than absorbed.
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

    // ts_worker_started runs exactly once per worker, so exactly four hits exist.
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

    // Freeze on the main thread with every worker spinning, take one step, prove no worker
    // moved. Stepping a worker instead would advance the counter the guard reads.
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

        // Each round gives the workers real running time. Stepping at the first stop would
        // prove nothing: the counters are all zero there.
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

        // Without threads that were running, the comparison below proves nothing.
        REQUIRE(spinning == 4);

        // A failed read looks exactly like a frozen process, so require the read too.
        REQUIRE(dbg.process()->MemRead(*s.counters, before, sizeof(before)));

        if(shape == StepShape::Over)
            dbg.StepOver();
        else
            dbg.StepInto();
        dbg.WaitForStep();

        // Nothing lifts the freeze when a step reports, so the counters must not have moved.
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

// Freeze-on-step policy: a failure here means stepping now resumes the whole process.
TEST_CASE("Other threads do not run across a step", "[multithread][step]")
{
    RequireOtherThreadsFrozenAcrossStep(StepShape::Into);
}

// Same policy through the StepOver entry point. ts_tick's prologue classifies as
// StepOverKind::None, so this covers the request path, not StepOverArm::Armed.
TEST_CASE("Other threads do not run across a step-over", "[multithread][stepover]")
{
    RequireOtherThreadsFrozenAcrossStep(StepShape::Over);
}

// abandonFreeze must not fire on a step that is still in flight: the fault is reported from
// inside a step-off, and answering it with StepInto holds the freeze on purpose.
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

    // The fault is still disarmed, so these rounds only stop on ts_tick.
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

    // Arm the fault and move the breakpoint onto the faulting store itself.
    const int armed = 1;
    REQUIRE(dbg.process()->MemWrite(*s.armed, &armed, sizeof(armed)));
    REQUIRE(dbg.process()->DeleteBreakpoint(*s.tick));
    REQUIRE(dbg.process()->SetBreakpoint(*s.fault, false, ElfBug::SoftwareType::ShortInt3));

    dbg.Continue();
    dbg.WaitForBreakpointAt(*s.fault, std::chrono::seconds(10));

    // Stepping off the byte executes the store, which faults from inside
    // stepPastBreakpointByte and returns false to a caller that must not read it as stranded.
    dbg.Continue();
    dbg.WaitForException(SIGSEGV, std::chrono::seconds(10));

    REQUIRE(dbg.process()->MemRead(*s.counters, before, sizeof(before)));

    // Answer the fault with a step, not a continue. A continue routes through
    // resumeAllThreads and clears the freeze before the abandon path is ever reached.
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

// A thread that already reported PTRACE_EVENT_EXIT owes the sweep no stop, so treating it
// as running costs a waitpid that never returns. Regressions hang this test, not fail it.
// waitpid can report the new thread's own stops before the parent's clone event, so the
// first thing the core hears from a thread may be its breakpoint hit.
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
    REQUIRE(dbg.count(EventType::Breakpoint) == 64);
    REQUIRE(dbg.count(EventType::InternalError) == 0);
}

TEST_CASE("A process exit racing the stop sweep is still reported", "[multithread][process]")
{
    using namespace ElfBug::test;

    // A race by nature: the sweep only meets the dying leader if a spinner's trap is
    // dispatched while the exit is in flight, so run several passes to make that land.
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

        // Stop on a spinner first, so the exit below starts from a frozen process with
        // the leader registered, running, and about to be swept.
        dbg.Continue();
        dbg.WaitFor(EventType::Breakpoint, std::chrono::seconds(10));

        const int go = 1;
        REQUIRE(dbg.process()->MemWrite(*s.exitNow, &go, sizeof(go)));

        // The spinners keep trapping while the exit runs, so each of these rounds is
        // another chance for a sweep to land on the dying leader.
        Event last;
        for(int round = 0; round < 400; ++round)
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
        // The fixture raises nothing: a swept int3 must never come back as a signal.
        REQUIRE(dbg.count(EventType::Exception) == 0);
        REQUIRE(dbg.count(EventType::InternalError) == 0);
    }
}

namespace
{
    struct SignalRaceResult
    {
        int raised = 0;
        int handled = 0;
        std::size_t reported = 0;
    };

    // The raiser sits in signal-delivery-stop almost constantly, so the sweep run by every
    // spinner hit absorbs a share of its raises that the main loop would otherwise report.
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

        // Stop on a spinner first, so every raise below happens under a sweeping process.
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
        // raise() is not a fault: si_addr aliases the sender there and must not leak.
        REQUIRE(withAddress == 0);

        // Checked before Stop so a wedged debugger cannot hide the numbers.
        REQUIRE(r.raised == quota);
        REQUIRE(r.handled == r.raised);
        REQUIRE(r.reported == static_cast<std::size_t>(r.raised));

        // Stop() races an in-flight stop: a spinner trapping after the kill is sent would
        // pause a tracer that then never returns to waitpid. Disarm the site first.
        REQUIRE(dbg.process()->DeleteBreakpoint(*s.hot));
        dbg.Stop();
        dbg.WaitForExit();
        dbg.JoinThread();
        REQUIRE(dbg.count(EventType::InternalError) == 0);
        return r;
    }
}

// A queued signal the sweep absorbs is delivered on the resume, but nothing reports it, so
// whether the user sees a SIGUSR1 depends on which waitpid happened to win.
TEST_CASE("Signals absorbed by the stop sweep are still reported", "[multithread][exception]")
{
    RunSignalRace(SIGUSR1, 200);
}

// A hardware fault re-raises itself when the instruction runs again, so dropping it is free.
// A raised SIGSEGV does not, so dropping it on the resume loses it for good.
TEST_CASE("Raised fatal signals absorbed by the stop sweep are still delivered", "[multithread][exception]")
{
    RunSignalRace(SIGSEGV, 200);
}

// int3 and single-step traps are the debugger's own, but a SIGTRAP the tracee raised at
// itself is a plain signal, and continuing it with signal 0 loses it exactly like SIGSEGV.
TEST_CASE("A raised SIGTRAP is delivered and reported", "[multithread][exception]")
{
    RunSignalRace(SIGTRAP, 200);
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

// The stop that was switched away from still owes the signal it reported. A step on the
// switched thread must not carry it: the handler has to run on the thread that faulted.
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

namespace
{
    struct SpinSession
    {
        ElfBug::test::RecordingDebugger dbg;
        std::string path;
        pid_t mainTid = 0;
        std::vector<pid_t> workers;
        ElfBug::ptr counters = 0;

        // Starts threads_spin, waits for its four workers, and pauses the process.
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

        // Runs the process for a moment and pauses it again.
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
            dbg.WaitForExit();
            dbg.JoinThread();
        }
    };

    // User-mode pc of a stopped thread from /proc, so a test thread can see where the
    // tracee is parked without being its tracer. Empty while the thread runs.
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
    REQUIRE_THROWS_AS(s.dbg.WaitFor(ElfBug::test::EventType::Step, std::chrono::milliseconds(300)), std::runtime_error);
    REQUIRE(s.dbg.IsPaused());

    s.dbg.StepOver();
    REQUIRE_THROWS_AS(s.dbg.WaitFor(ElfBug::test::EventType::Step, std::chrono::milliseconds(300)), std::runtime_error);
    REQUIRE(s.dbg.IsPaused());
}

// StepInto refuses a suspended thread, but the request only raises a flag the tracer
// consumes once it wakes. A suspend that lands in between must not let the step run.
TEST_CASE("A suspend landing on a queued step drops the step and reports a pause", "[multithread][suspend][step]")
{
    using namespace ElfBug::test;
    SpinSession s;
    const pid_t worker = s.workers.front();
    REQUIRE(s.dbg.SwitchThread(worker));
    const ElfBug::Thread* thread = s.dbg.process()->threads.at(worker).get();

    // The caller normally wins mPauseMutex against the tracer the notify just woke, but
    // a round where the step ran first proves nothing and is retried. Such a round can
    // leave the worker holding the suspend's SIGSTOP; a Continue/Pause cycle drains it.
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
            REQUIRE(thread->isSuspended());
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

    // No WaitForRunning: the requests race the loop's resume of the threads it snapshotted.
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
            REQUIRE_FALSE(s.dbg.process()->threads.at(tid)->isRunning());

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

// A thread parked exactly on its own armed breakpoint byte must step off it when
// resumed while running, or a bare PTRACE_CONT replays the INT3 and reports the same
// hit again.
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

    // ts_worker_started fires exactly once per worker; draining all four leaves the
    // last hitter parked, unconsumed, right on the byte, so any later hit there can
    // only be a re-triggered breakpoint.
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

    REQUIRE_THROWS_AS(dbg.WaitForBreakpointAt(*site, std::chrono::milliseconds(500)), std::runtime_error);

    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
}

// pauseAndResume stashes a reported signal on mThread when mThread turns out to be
// suspended, meaning to deliver it once that thread runs again. drainPendingResumes must
// honor that, not silently drop the signal on its own final continue.
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

    // Disarm before delivery, so the fixed handler's own retry does not immediately
    // re-arm and fault again.
    constexpr int disarmed = 0;
    REQUIRE(dbg.process()->MemWrite(*s.armed, &disarmed, sizeof(disarmed)));

    // Suspending the reporting thread right after its report, then resuming the process,
    // is what makes pauseAndResume stash the signal instead of delivering it directly.
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

    // Two requests land on the same running tid before the loop can service the first,
    // so the suspend count must reach 2 rather than coalescing into 1.
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

    // The resume arrives while the suspend's SIGSTOP is still in flight.
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
        REQUIRE_FALSE(s.dbg.process()->threads.at(tid)->isSuspended());
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

    // A worker not yet created when `others` was taken could not have been suspended
    // and may still legitimately reach the breakpoint on its own; nothing in `others` may.
    // Each hit needs its own Continue, the same as any other stop.
    for(;;)
    {
        dbg.Continue();
        Event hit{};
        try
        {
            hit = dbg.WaitFor(EventType::Breakpoint, std::chrono::milliseconds(500));
        }
        catch(const std::runtime_error &)
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

// Main sleeps one millisecond per loop, so once the workers exist a worker's stop almost
// always finds it parked in nanosleep. The reporting worker was running, so it has no reason.
TEST_CASE("The sweep keeps Suspended for a thread whose requested stop has not landed", "[multithread][suspend][waitreason]")
{
    SpinSession s;

    // The Pause races the worker's own requested SIGSTOP. When the sweep wins it still
    // sees the worker running and samples /proc for it; the caller's reason must survive.
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
        REQUIRE(thread->isSuspended());
        CAPTURE(thread->waitReason());
        REQUIRE(thread->waitReason() == "Suspended");

        REQUIRE(s.dbg.SetThreadSuspended(worker, false));
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

    // Arm the hot site only once every worker exists and main is in its sleep loop.
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
        mainReason = dbg.process()->threads.at(mainTid)->waitReason();
        workerReason = dbg.process()->threads.at(hit.pid)->waitReason();
    }
    CAPTURE(mainReason);
    REQUIRE(mainReason.find("nanosleep") != std::string::npos);
    REQUIRE(workerReason.empty());

    REQUIRE(dbg.process()->DeleteBreakpoint(*site));
    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
}

// A hit the sweep queued on a child is consumed when the user steps that child off the
// address. Only the parent revisits ct_site, so a child reporting it twice is the defect.
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
            if(tid != mainTid && tid != last.pid && thread->hasPendingBreakpoint())
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
    REQUIRE(dbg.count(EventType::Breakpoint) == 63);
    REQUIRE(dbg.count(EventType::InternalError) == 0);
    REQUIRE(hitsBeforeRun >= 1);
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
    REQUIRE_THROWS_AS(dbg.WaitFor(EventType::Paused, std::chrono::milliseconds(500)), std::runtime_error);

    dbg.Pause();
    dbg.WaitForPaused();
    dbg.Stop();
    dbg.WaitForExit();
    dbg.JoinThread();
}

namespace
{
    // Records what the C API hands its callbacks.
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

    // Drives a fixture through the C API on its own loop thread. A test that fails
    // mid-run still kills the tracee and joins the loop.
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

        // Takes over an already running pid instead of launching the fixture.
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
        REQUIRE(s.events.exceptionAddress == 0);
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

    // No delay between the two: the queue must be applied before the resume.
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
    // Four spinners ran for 200 ms; even on two cores that is well past one 10 ms tick.
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
        CAPTURE(i);
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

    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, true));
    REQUIRE(ElfBugGetThreadList(s.dbg, all, 5) == 5);
    for(const auto & t : all)
    {
        if(t.tid == worker)
            REQUIRE(std::string(t.wait_reason) == "Suspended");
    }

    REQUIRE(ElfBugSetThreadSuspended(s.dbg, worker, false));
    REQUIRE(ElfBugGetThreadList(s.dbg, all, 5) == 5);
    for(const auto & t : all)
    {
        if(t.tid == worker)
            REQUIRE(std::string(t.wait_reason) != "Suspended");
    }
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

    const auto list = ElfBugEnumProcessesList();
    REQUIRE(!list.empty());

    const auto it = std::find_if(list.begin(), list.end(),
    [&](const ElfBugProcessInfo & p) { return p.pid == target.pid; });
    REQUIRE(it != list.end());
    REQUIRE(std::string(it->name) == "run_endlessly");
    // The kernel resolves /proc/<pid>/exe, so canonicalise both sides.
    REQUIRE(std::filesystem::canonical(it->path) == std::filesystem::canonical(FIXTURE("run_endlessly")));
    REQUIRE(it->arch == ElfBugArch_X86_64);
    REQUIRE(it->traced == false);
}

TEST_CASE("EnumProcesses truncates at capacity and still reports the total", "[api][attach]")
{
    std::vector<ElfBugProcessInfo> list(2);
    list[1].pid = -1234;

    // Comparing against a separate scan would race any process starting or exiting; the
    // contract is that the total is the real one and nothing is written past capacity.
    const uint32_t total = ElfBugEnumProcesses(list.data(), 1);
    REQUIRE(total > 1);
    REQUIRE(list[1].pid == -1234);
}

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

    // Four workers plus main. Reading registers on each is what proves they are traced.
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

    // The same fact the dialog greys the row on.
    const auto list = ElfBugEnumProcessesList();
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

TEST_CASE("Attach acquires every live thread of a multithreaded process", "[attach]")
{
    // Waits for real threads before attaching, so this does not isolate the rescan loop.
    ElfBug::test::UntracedProcess target(FIXTURE("thread_storm"));
    REQUIRE(target.pid > 0);
    REQUIRE(WaitForExeced(target.pid, FIXTURE("thread_storm")));
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
    // Every thread the process actually has is one we acquired.
    REQUIRE(known == live);
}

TEST_CASE("Attach acquires threads the target clones during the sweep", "[attach]")
{
    // clone_relay clones from its newest thread, so the pass that froze everything it
    // listed leaves a cloner running. Only the rescan loop picks the new threads up.
    ElfBug::test::UntracedProcess target(FIXTURE("clone_relay"));
    REQUIRE(target.pid > 0);
    REQUIRE(WaitForExeced(target.pid, FIXTURE("clone_relay")));
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

    // The precondition: the target really did clone across the sweep. Without this the
    // case degenerates into the single-pass test above.
    REQUIRE(known.size() > before.size());
    // A single pass would leave the threads cloned after its readdir running and untraced.
    REQUIRE(known == live);
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
    // The precondition: the target really did clone. Without it the case proves nothing.
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

TEST_CASE("A signal delivered to an attached process is reported and forwarded", "[attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("signal_pending"));
    REQUIRE(target.pid > 0);
    REQUIRE(target.WaitForRunning());
    // Exec must finish before we attach, or the sweep sees the exec's SIGTRAP instead of SIGSTOP.
    REQUIRE(WaitForExeced(target.pid, FIXTURE("signal_pending")));

    ElfBug::test::RecordingDebugger dbg;
    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    const auto ready = ElfBug::test::ResolveRuntimeAddress(FIXTURE("signal_pending"),
                       dbg.process()->pid, "sp_ready");
    REQUIRE(ready.has_value());

    // Run until sigaction() is installed, or SIGUSR1 below hits the default disposition instead.
    dbg.Continue();
    REQUIRE(WaitForTraceeValue(dbg.process(), *ready, 1));

    kill(target.pid, SIGUSR1);

    // A freshly seen signal is reported before it is forwarded, same as any other exception.
    dbg.WaitForException(SIGUSR1);
    dbg.Continue();

    // The handler increments sp_handled. If the loop dropped the signal it never moves.
    const auto handled = ElfBug::test::ResolveRuntimeAddress(FIXTURE("signal_pending"),
                         dbg.process()->pid, "sp_handled");
    REQUIRE(handled.has_value());
    REQUIRE(WaitForTraceeValue(dbg.process(), *handled, 1));
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
        // Attach passes its checks here, and the destructor kills the target before Start.
        REQUIRE(dbg.Attach(pid));
    }

    dbg.StartOnThread();
    // An internal error, not a hang and not a crash.
    const auto event = dbg.WaitForInternalError();
    REQUIRE(event.message.find(std::to_string(pid)) != std::string::npos);
    dbg.JoinThread();
}

TEST_CASE("A signal already pending at attach is reported before the tracee runs", "[attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("signal_storm"));
    REQUIRE(target.pid > 0);
    REQUIRE(WaitForExeced(target.pid, FIXTURE("signal_storm")));
    REQUIRE(target.WaitForThreads(9));

    ElfBug::test::RecordingDebugger dbg;

    bool pending = false;
    dbg.OnAttachBreakpoint([&]
    {
        for(const auto & [tid, thread] : dbg.process()->threads)
        {
            if(thread->pendingSignal() == SIGUSR1)
                pending = true;
        }
    });

    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    // The precondition this case exists for: the sweep caught a thread already stopped on
    // its own SIGUSR1 and parked it. Without one there is nothing here to report.
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

    // Nothing ran between attach and the report: the signal came from a stop the sweep
    // captured, not from a tracee that was allowed to resume first.
    REQUIRE(atException == before);
}

TEST_CASE("AttachErrorMessage names the yama fix for EPERM", "[attach]")
{
    // scope 1, not our child: the case that actually blocks people.
    const auto blocked = ElfBug::AttachErrorMessage(1234, EPERM, 1, false);
    REQUIRE(blocked.find("ptrace_scope") != std::string::npos);
    REQUIRE(blocked.find("setcap cap_sys_ptrace=+eip") != std::string::npos);

    // scope 0: EPERM is about something else, so do not send them down the yama path.
    const auto other = ElfBug::AttachErrorMessage(1234, EPERM, 0, false);
    REQUIRE(other.find("setcap") == std::string::npos);

    // Our own child under scope 1 is allowed, so EPERM here is also something else.
    const auto child = ElfBug::AttachErrorMessage(1234, EPERM, 1, true);
    REQUIRE(child.find("setcap") == std::string::npos);

    const auto gone = ElfBug::AttachErrorMessage(1234, ESRCH, 1, false);
    REQUIRE(gone.find("setcap") == std::string::npos);

    // Scope 2 blocks even our own children, so the "your own children" wording would mislead.
    const auto adminOnly = ElfBug::AttachErrorMessage(1234, EPERM, 2, false);
    REQUIRE(adminOnly.find("setcap cap_sys_ptrace=+eip") != std::string::npos);
    REQUIRE(adminOnly.find("own children") == std::string::npos);

    // Scope 3 disables ptrace entirely, so setcap would not help.
    const auto disabled = ElfBug::AttachErrorMessage(1234, EPERM, 3, false);
    REQUIRE(disabled.find("ptrace_scope") != std::string::npos);
    REQUIRE(disabled.find("setcap") == std::string::npos);
}


TEST_CASE("Detach leaves an attached process running", "[detach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(WaitForExeced(target.pid, FIXTURE("threads_spin")));
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
    REQUIRE(WaitForExeced(target.pid, FIXTURE("threads_spin")));
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

    // Read through a fresh descriptor: the session's Process is gone.
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

    // PTRACE_O_EXITKILL stops applying once we are not the tracer, so it survives us.
    // kill(pid, 0) and TracerPid both read fine for a zombie, so check the process state.
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
    REQUIRE(WaitForExeced(target.pid, FIXTURE("signal_pending")));

    ElfBug::test::RecordingDebugger dbg;
    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    const auto ready = ElfBug::test::ResolveRuntimeAddress(FIXTURE("signal_pending"),
                       dbg.process()->pid, "sp_ready");
    REQUIRE(ready.has_value());
    const auto handled = ElfBug::test::ResolveRuntimeAddress(FIXTURE("signal_pending"),
                         dbg.process()->pid, "sp_handled");
    REQUIRE(handled.has_value());

    // Run until sigaction() is installed, or SIGUSR1 hits the default terminate disposition.
    dbg.Continue();
    REQUIRE(WaitForTraceeValue(dbg.process(), *ready, 1));

    // The loop reports this signal and parks it; only the next resume forwards it.
    kill(target.pid, SIGUSR1);
    dbg.WaitForException(SIGUSR1);

    int atException = 0;
    REQUIRE(dbg.process()->MemReadRaw(*handled, &atException, sizeof(atException)));
    REQUIRE(atException == 0);

    // No second Continue: the detach is the only thing left that can deliver the signal.
    dbg.Detach();
    dbg.WaitForDetach();
    dbg.JoinThread();

    // If detach passed 0 instead of the parked signal, the handler never runs.
    REQUIRE(WaitForDetachedValue(target.pid, *handled, 1));
}

TEST_CASE("Detach requested while running arrives as a pause and then detaches", "[detach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(WaitForExeced(target.pid, FIXTURE("threads_spin")));
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

    // The name's claim: the request arrived as a pause, and only then as a detach.
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
    REQUIRE(WaitForExeced(target.pid, FIXTURE("signal_storm")));
    REQUIRE(target.WaitForThreads(9));

    ElfBug::test::RecordingDebugger dbg;

    bool owed = false;
    dbg.OnAttachBreakpoint([&]
    {
        for(const auto & [tid, thread] : dbg.process()->threads)
        {
            if(thread->pendingSigstop())
                owed = true;
        }
    });

    REQUIRE(dbg.Attach(target.pid));
    dbg.StartOnThread();
    dbg.WaitForAttachBreakpoint();

    // The precondition this case exists for: a thread stopped on its own SIGUSR1, so the
    // attach SIGSTOP is still queued behind it.
    REQUIRE(owed);

    dbg.Detach();
    dbg.WaitForDetach();
    dbg.JoinThread();

    REQUIRE(ElfBug::TracerPid(target.pid) == 0);
    // A SIGSTOP left queued at detach group-stops the process we just released.
    REQUIRE(ElfBug::test::StaysRunning(target.pid));
}

TEST_CASE("Detach releases a thread the user suspended", "[detach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(WaitForExeced(target.pid, FIXTURE("threads_spin")));
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
    // A suspend count means nothing once we are not the tracer, and a thread left in
    // ptrace-stop would be killed by PTRACE_O_EXITKILL when the loop thread exits.
    REQUIRE(ElfBug::test::StaysRunning(target.pid));
}

// Suspending a running thread goes out as a tgkill SIGSTOP the loop has not observed yet,
// a different path from suspending one that is already stopped.
TEST_CASE("Detach releases a thread suspended while the process runs", "[detach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(WaitForExeced(target.pid, FIXTURE("threads_spin")));
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

TEST_CASE("C API attach hands the session over with threads and registers", "[api][attach]")
{
    ElfBug::test::UntracedProcess target(FIXTURE("threads_spin"));
    REQUIRE(target.pid > 0);
    REQUIRE(WaitForExeced(target.pid, FIXTURE("threads_spin")));
    REQUIRE(target.WaitForThreads(5));

    ApiSession s(FIXTURE("threads_spin"), target.pid);
    REQUIRE(s.Started());
    REQUIRE(s.WaitForAttachBreakpoint());

    REQUIRE(ElfBugIsPaused(s.dbg));
    REQUIRE(ElfBugGetPid(s.dbg) == target.pid);
    REQUIRE(ElfBugGetCurrentTid(s.dbg) != 0);

    // The GUI reads the thread list the moment this callback lands, so by then it holds
    // every thread the sweep acquired, each with registers read rather than a zero rip.
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

    // Nothing is stopped and accepting Continue any more, so IsPaused must say so.
    REQUIRE_FALSE(ElfBugIsPaused(s.dbg));
    REQUIRE(ElfBugGetPid(s.dbg) == 0);
    REQUIRE(ElfBugGetThreadList(s.dbg, nullptr, 0) == 0);

    REQUIRE(ElfBug::TracerPid(pid) == 0);
    // A killed debuggee would be an unreaped zombie here, which kill(pid, 0) accepts.
    REQUIRE(ElfBug::test::StaysRunning(pid));
    kill(pid, SIGKILL);
    int status = 0;
    waitpid(pid, &status, __WALL);
}
