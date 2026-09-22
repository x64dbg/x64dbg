#include "TestSupport.h"
#include <tuple>

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

TEST_CASE("Continuing past a breakpoint preserves its callback", "[breakpoint]")
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
    REQUIRE(std::equal(b.original + 1, b.original + 4, b.masked + 1));
    REQUIRE(std::equal(b.original + 1, b.original + 4, b.raw + 1));

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

TEST_CASE("An execve reseats breakpoint records onto the new image", "[stepover][exec]")
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

    uint8_t originalByte = 0;
    REQUIRE(dbg.process()->MemRead(*site, &originalByte, 1));

    const auto scratchVar = ResolveRuntimeAddress(path, dbg.process()->pid, "eo_scratch");
    REQUIRE(scratchVar.has_value());
    ElfBug::ptr scratch = 0;
    REQUIRE(dbg.process()->MemRead(*scratchVar, &scratch, sizeof(scratch)));
    REQUIRE(scratch != 0);
    REQUIRE(dbg.process()->SetBreakpoint(scratch, false, ElfBug::SoftwareType::ShortInt3));

    std::promise<std::pair<bool, bool>> scratchPromise;
    auto scratchFuture = scratchPromise.get_future();
    dbg.OnExec([&]
    {
        const bool deleted = dbg.process()->DeleteBreakpoint(scratch);
        scratchPromise.set_value({deleted, dbg.process()->HasBreakpoint(scratch)});
    });

    dbg.StepOver();

    dbg.WaitForExec();

    const auto [scratchDeleted, scratchStillSet] = scratchFuture.get();
    CHECK(scratchDeleted);
    CHECK_FALSE(scratchStillSet);

    const auto second = dbg.WaitForBreakpointAt(*site, std::chrono::seconds(10));
    CAPTURE(second.address);

    uint8_t raw = 0;
    REQUIRE(dbg.process()->MemReadRaw(*site, &raw, 1));
    CHECK(raw == 0xCC);

    uint8_t shown = 0;
    REQUIRE(dbg.process()->MemRead(*site, &shown, 1));
    CHECK(shown == originalByte);

    dbg.Continue();
    const auto exit_ev = dbg.WaitFor(EventType::ExitProcess, std::chrono::seconds(10));
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == 7);
    REQUIRE(dbg.count(EventType::InternalError) == 0);
    REQUIRE(dbg.count(EventType::Exec) == 1);
    REQUIRE(dbg.count(EventType::Breakpoint) == 2);
}

TEST_CASE("An execve into a different image does not carry breakpoints over", "[exec]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    const std::string path = FIXTURE("exec_peer_a");
    const std::string peer = FIXTURE("exec_peer_b");
    const char* argv[] = {path.c_str(), peer.c_str(), nullptr};
    REQUIRE(dbg.Init(path.c_str(), argv, nullptr));

    std::promise<std::optional<ElfBug::ptr>> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const auto site = ResolveRuntimeAddress(path, dbg.process()->pid, "ep_site");
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

    std::promise<std::tuple<std::optional<ElfBug::ptr>, bool, uint8_t, bool>> afterPromise;
    auto afterFuture = afterPromise.get_future();
    dbg.OnExec([&]
    {
        const auto peerSite = ResolveRuntimeAddress(peer, dbg.process()->pid, "ep_site");
        uint8_t raw = 0;
        const bool read = dbg.process()->MemReadRaw(*site, &raw, 1);
        afterPromise.set_value({peerSite, read, raw, dbg.process()->HasBreakpoint(*site)});
    });

    dbg.Continue();
    dbg.WaitForExec();

    const auto [peerSite, rawRead, raw, stillListed] = afterFuture.get();

    REQUIRE(peerSite.has_value());
    REQUIRE(*peerSite == *site);
    REQUIRE(rawRead);

    CHECK(raw != 0xCC);

    CHECK(stillListed);

    dbg.Continue();
    const auto exit_ev = dbg.WaitFor(EventType::ExitProcess, std::chrono::seconds(10));
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == 9);
    REQUIRE(dbg.count(EventType::Breakpoint) == 1);
    REQUIRE(dbg.count(EventType::Exec) == 1);
    REQUIRE(dbg.count(EventType::InternalError) == 0);
}

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
        r.site = ResolveRuntimeAddress(path, dbg.process()->pid, "so_call_site");
        if(r.site)
        {
            auto* process = dbg.process();
            (void)process->MemRead(*r.site, r.original, sizeof(r.original));
            process->SetBreakpoint(*r.site, false, ElfBug::SoftwareType::ShortInt3);

            const std::uint8_t patch[2] = {0x90, 0x90};
            process->MemWrite(*r.site, patch, sizeof(patch));

            (void)process->MemReadRaw(*r.site, &r.rawAfterWrite, 1);
            (void)process->MemRead(*r.site, &r.maskedAfterWrite, 1);
            (void)process->MemReadRaw(*r.site + 1, &r.neighbourAfterWrite, 1);

            process->DeleteBreakpoint(*r.site);
            (void)process->MemReadRaw(*r.site, &r.afterDelete, 1);

            process->MemWrite(*r.site, r.original, sizeof(r.original));
        }
        promise.set_value(r);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto r = future.get();
    REQUIRE(r.site.has_value());

    REQUIRE(r.original[0] == 0xe8);
    REQUIRE(r.rawAfterWrite == 0xCC);
    REQUIRE(r.maskedAfterWrite == 0x90);
    REQUIRE(r.neighbourAfterWrite == 0x90);
    // Deleting restores what the caller wrote (0x90), not what was there before it (0xe8).
    REQUIRE(r.afterDelete == 0x90);

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
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

    dbg.Continue();
    dbg.WaitForBreakpointAt(*site + 2);
    REQUIRE((dbg.currentThread()->registers.Gax() & (1ull << 8)) == 0);

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
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

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(dbg.count(EventType::Breakpoint) == 1);
}

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
