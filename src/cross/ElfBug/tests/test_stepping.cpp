#include "TestSupport.h"
#include <ElfBug/process/StepOver.h>
#include <zydis_wrapper.h>

namespace
{
    bool ClassifiesAsNone(const ElfBug::uint8* bytes, const std::size_t n)
    {
        ElfBug::ptr next = 0xdead;
        return ElfBug::ClassifyStepOver(bytes, n, 0x1000, next) == ElfBug::StepOverKind::None && next == 0;
    }
}

TEST_CASE("Step fires cbStep on single instruction", "[step]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("hello_elfbug").c_str()));
    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();

    const ElfBug::ptr rip = dbg.currentThread()->registers.Gip();
    std::uint8_t bytes[16] = {};
    REQUIRE(dbg.process()->MemRead(rip, bytes, sizeof(bytes)));
    Zydis zydis(true);
    REQUIRE(zydis.Disassemble(rip, bytes, sizeof(bytes)));
    REQUIRE_FALSE(zydis.IsBranchType(Zydis::BTAny));

    dbg.StepInto();
    const auto step = dbg.WaitForStep();
    REQUIRE(step.instructionPointer == rip + zydis.Size());

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();

    REQUIRE(exit_ev.exitCode == 0);
    REQUIRE(dbg.count(EventType::Step) == 1);
}

TEST_CASE("StepInto lifts an armed breakpoint byte the thread never hit", "[step][breakpoint]")
{
    using namespace ElfBug::test;
    RecordingDebugger dbg;
    REQUIRE(dbg.Init(FIXTURE("hello_elfbug").c_str()));

    std::promise<ElfBug::ptr> sitePromise;
    auto siteFuture = sitePromise.get_future();
    dbg.OnSystemBreakpoint([&]
    {
        const ElfBug::ptr site = dbg.currentThread()->registers.Gip();
        dbg.process()->SetBreakpoint(site, false, ElfBug::SoftwareType::ShortInt3);
        sitePromise.set_value(site);
    });

    dbg.StartOnThread();
    dbg.WaitForSystemBreakpoint();
    const auto site = siteFuture.get();
    REQUIRE(site != 0);

    dbg.StepInto();
    dbg.WaitForStep();

    const ElfBug::ptr stepped = dbg.currentThread()->registers.Gip();
    CAPTURE(site, stepped);
    REQUIRE(stepped != site + 1);
    REQUIRE(stepped != site);
    REQUIRE(dbg.count(EventType::Breakpoint) == 0);

    dbg.Continue();
    const auto exit_ev = dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(exit_ev.exitCode == 0);
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

    // f3 c3 : rep ret
    const uint8 repRet[] = {0xf3, 0xc3};
    REQUIRE(ClassifiesAsNone(repRet, sizeof(repRet)));

    // f3 90 : pause
    const uint8 pause_[] = {0xf3, 0x90};
    REQUIRE(ClassifiesAsNone(pause_, sizeof(pause_)));
}

TEST_CASE("ClassifyStepOver recognises pushfq", "[stepover]")
{
    using namespace ElfBug;
    // 9c : pushfq
    const uint8 bytes[] = {0x9c};
    ptr next = 0;
    REQUIRE(ClassifyStepOver(bytes, sizeof(bytes), 0x1000, next) == StepOverKind::Pushf);
    REQUIRE(next == 0x1001);
}

TEST_CASE("ClassifyStepOver rejects ordinary instructions", "[stepover]")
{
    using namespace ElfBug;

    const uint8 nop[] = {0x90};                          // nop
    REQUIRE(ClassifiesAsNone(nop, sizeof(nop)));

    const uint8 ret[] = {0xc3};                          // ret
    REQUIRE(ClassifiesAsNone(ret, sizeof(ret)));

    const uint8 jmp[] = {0xeb, 0x00};                    // jmp short +0
    REQUIRE(ClassifiesAsNone(jmp, sizeof(jmp)));

    const uint8 mov[] = {0x48, 0x89, 0xd8};              // mov rax, rbx
    REQUIRE(ClassifiesAsNone(mov, sizeof(mov)));

    const uint8 syscall_[] = {0x0f, 0x05};               // syscall
    REQUIRE(ClassifiesAsNone(syscall_, sizeof(syscall_)));
}

TEST_CASE("ClassifyStepOver handles undecodable and empty input", "[stepover]")
{
    using namespace ElfBug;
    REQUIRE(ClassifiesAsNone(nullptr, 0));

    const uint8 garbage[] = {0xff, 0xff, 0xff};          // ff /7: invalid
    REQUIRE(ClassifiesAsNone(garbage, sizeof(garbage)));
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

    REQUIRE(step.instructionPointer == *s.site + 5);

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

    REQUIRE(step.instructionPointer == *s.address + 2);
    REQUIRE(dbg.currentThread()->registers.Gcx() == 0);

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
    REQUIRE(dbg.count(EventType::Step) == 1);
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
    REQUIRE(dbg.process()->DeleteBreakpoint(*site));

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

    dbg.WaitForBreakpointAt(*s.callee);

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

    dbg.StepOver();
    const auto step = dbg.WaitForStep();
    REQUIRE(step.instructionPointer == *site + 5);

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

    REQUIRE(dbg.process()->HasBreakpointCallback(*site));
    REQUIRE(dbg.process()->HasBreakpoint(*site));

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

    dbg.StepOver();
    const auto step = dbg.WaitFor(EventType::Step, std::chrono::seconds(3));
    REQUIRE(step.instructionPointer == *site + 5);

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

    REQUIRE(callbackHits.load() == 1);
    REQUIRE(dbg.count(EventType::Breakpoint) == 2);
    REQUIRE_FALSE(dbg.process()->HasBreakpoint(*site + 5));

    dbg.Continue();
    dbg.WaitForExit();
    dbg.JoinThread();
}

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

    // f2 0f 10 c1 : movsd xmm0, xmm1
    const uint8 movsdXmm[] = {0xf2, 0x0f, 0x10, 0xc1};
    REQUIRE(ClassifiesAsNone(movsdXmm, sizeof(movsdXmm)));

    // f2 0f c2 c1 00 : cmpsd xmm0, xmm1, 0
    const uint8 cmpsdXmm[] = {0xf2, 0x0f, 0xc2, 0xc1, 0x00};
    REQUIRE(ClassifiesAsNone(cmpsdXmm, sizeof(cmpsdXmm)));

    // f3 0f 10 c1 : movss xmm0, xmm1
    const uint8 movssXmm[] = {0xf3, 0x0f, 0x10, 0xc1};
    REQUIRE(ClassifiesAsNone(movssXmm, sizeof(movssXmm)));
}
