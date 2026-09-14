# Tracing and RunToParty end-to-end tests

This bundle uses a **real debuggee and the normal debugger commands**. It has no
plugin, custom test command, private debugger API call, fake trace writer, or
CIP redirection into fixture routines.

The debuggee warms up `InitOnceExecuteOnce`, then naturally calls it three times.
Windows calls back into user code each time. Exported functions are ordinary
breakpoint locations; exported counters and a scratch allocation are observable
fixture state, not alternate implementations of debugger behavior.

## Coverage

| Test id suffix | Live behavior exercised |
| --- | --- |
| (primary), `user-run` | User Only / Trace Into, default and optional run traversal; all three callbacks |
| `system-step`, `system-run` | System Only starts in excluded user code; repeated transitions through callbacks |
| `over-user-step`, `over-user-run` | Included calls are still stepped over, including their callbacks |
| `over-system-step`, `over-system-run` | System Only / Trace Over, starting in a real Windows API; exact maximum count |
| `recording-active` | Existing file history survives filtering an excluded pending instruction |
| `commands` | `RunUser`, `RunSystem`, `rtu`, `rts`, `RunToParty`, and all user/system step aliases |
| `conditions` | Maximum count, command condition, false log condition, and clearing trace settings between traces |
| `fallback` | Real user memory breakpoint prevents fast setup, survives fallback, and subsequently fires |
| `breakpoint` | Ordinary software breakpoint interrupts a fast trace; another trace can start afterward |
| `exception` | Real exception breakpoint interrupts tracing; `erun` dispatches it to the debuggee handler |
| `module-change` | Actual dependency-free fixture DLL load/unload refreshes an active fast snapshot; subsequent tracing still works |
| `module-refresh` | Two loader cycles, User Only then System Only / Trace Into; requires actual snapshot re-arming rather than a stepping fallback |
| `pause-step`, `pause-run` | Real `pause` while trapped in an excluded user-code loop, then resume and complete |
| `cancel-into`, `cancel-over`, `cancel-run` | Configured DLL pause cancels a pending party step or refreshed fast traversal; ordinary Resume must not crash or restart tracing |
| `initial-over-run` | Trace Over starts on an excluded call and catches its first system callee before any callback executes |
| `pause-breakin` | Repeated Pause interrupts a blocked syscall; resume and externally release the original wait without hitting a stale Pause INT3 |

The fixture checks original page protections after tracing/running. Tests assert
stop locations, trace counters, command counts, actual callback side effects,
and correct continuation rather than just checking that commands returned true.

### Recording oracle

`driver.py` only substitutes per-run absolute paths into the checked-in scripts,
launches headless, and inspects results. `testassert` inside the debugger remains
mandatory for ordinary script tests. A driver check cannot turn a failed script
into a pass.

For recording cases the driver independently decodes the actual binary trace
using `docs/developers/tracefile.md` and compares every instruction address,
in order, against the debugger's text trace log. It accounts for the queued
starting instruction and the unexecuted stop instruction; it also checks any
explicitly logged pre-existing history. Thus an excluded initial record, a lost
record across a skipped region, a broken delta/thread reset, or a corrupt/truncated
record fails even when all script assertions pass. Files must be nonempty.

The run cases reject an unexpected fallback diagnostic. The fallback case requires
exactly one such diagnostic and then exercises the retained breakpoint with a
real write by the debuggee.

### Pause driver

A startup script awaiting a trace cannot concurrently issue its own Pause on the
same command thread. `pause_driver.py` therefore sends the checked-in script's
**ordinary commands** over headless stdin. `WAIT` comments only synchronize the
external user with running/paused/stopped notifications and a target progress
counter read through a debugger expression. They do not implement any stepping,
filtering, breakpoint, or pause logic.

The loop tests request Pause only after observed progress inside the excluded
loop, not because an arbitrary sleep expired. The driver validates actual expression
results afterward and writes the runner's final status. It waits for a fresh
running-to-paused transition so duplicate GUI state notifications cannot make
it proceed before a real stop.

`pause-breakin` exercises the shared native Pause path under ordinary Run. It
stops at the actual native wait entry, suspends unrelated threads, resumes into
the wait, and lets debug events settle. The first Pause must install its pending
breakpoint without stopping; the second must break in. Only after Resume does
the driver signal the fixture's named Windows event. The next stop must be
`Finished`, not the original syscall-return breakpoint. Signaling the event is
normal external debuggee input; it does not implement any debugger behavior.

## Build and run

```powershell
cmake --build build64 --config Release --target headless dbg test_trace_party_target
py src/tests/run.py --arch x64 --engine TitanEngine trace_party trace_party/system-run trace_party/pause-step
py src/tests/run.py --arch x64 --engine GleeBug trace_party trace_party/system-run trace_party/pause-run
```

All 23 variants are also auto-discovered in a normal full-suite run, including CI's
x86/x64 and TitanEngine/GleeBug matrix. `x86` is an alias for `x32`.

If the regular GUI is running, use an isolated build with
`-DX64DBG_BUILD_IN_TREE=OFF` and explicit runtime/archive output directories;
deploy its normal runtime dependencies and select that host with `--headless`.
The driver uses an absolute debuggee path, so it does not require the fixture to
be copied beside the isolated host.

Each artifact directory retains the expanded script, debugger/stdout logs, binary
trace, text trace, and `runtime.json` (SHA-256 of the host/core/bridge/engine/target).
Only a test's own child process tree is terminated on timeout.

The loader fixture is `trace_party_module.dll`, built without an entry point or
CRT imports, but with an executable export so snapshot updates also exercise
newly mapped/unmapped target code pages. This keeps the real loader events while avoiding assumptions about
system DLL residency, dependencies, and initialization across Windows versions.

`module-refresh` exercises both parties because WOW64 enters user-classified
transition code before loader events. A User Only trace can therefore have already
completed its fast traversal before the event on x86, unlike x64. The second,
System Only phase ensures a fast snapshot spans a loader event there too. The
driver requires at least one successful refresh and rejects refresh failures on
either architecture; there is no architecture-specific assertion skip. The
System Only phase stops at the first selected location after the DLL is mapped,
rather than tracing the platform-dependent remainder of the Windows loader.
Normal Run then completes the second load/unload cycle and all callback/protection
assertions still apply. The hermetic tests additionally assert the actual installed ranges and **zero** step
calls for successful refreshes, including new/unloaded pages and both saved modes.

## Negative controls and limits

Restoring the old unconditional loader-event stepping fallback makes the new
`module-refresh` regression fail. The refresh oracle rejects the missing re-arm
even when the ordinary script assertions pass; the original longer x86 scenario
also exposed an incorrect stopping point. The mutation was reverted.

Development validation deliberately disabled the production startup-filter call:
`system-step` failed at binary record zero with one extra user instruction.
Disabling the party-step abort in `cbDebugPause` made `pause-step` time out after
observing progress in the excluded loop.

Review-fix negative controls were also run against a real x64 TitanEngine host:
removing the Trace Over cancellation guard made `cancel-over` crash with
`0xC0000005`; restoring the unconditional initial step made `initial-over-run`
fail because its first callback had already executed; disabling owned Pause
breakpoint deletion made `pause-breakin` stop before `Finished` when the original
syscall returned. All mutations were reverted, then all 22 variants passed on
both x86/x64 and TitanEngine/GleeBug (88 case runs).

This is not exhaustive coverage. The Qt checkbox interaction itself, trace-coverage
into/beyond stop policies, live partial-setup failures, explicit multithreaded race
schedules, and a trace blocked inside a kernel wait still need dedicated E2E cases.
Those are not silently replaced with test-only implementations here. The existing
hermetic tests retain value for failure injection, installation/cancellation
interleavings, user-breakpoint ownership, and the active-trace repeated-Pause branch.

On Windows 11 24H2+, the fast path intentionally omits the single-page CFG/SCP
image-extension layout recognized by the memory map. These pages cannot accept
memory breakpoints. The run tests still reject fallback to stepping: this
platform exception must not disable ordinary target coverage.
`tests/run_to_party_test.py` checks exact exclusion, adjacent private/JIT code,
older Windows, mismatched allocation/size/protection, guard ownership, and
setup-error rollback.

The fast path's documented executable-memory snapshot limitations still apply;
these tests do not promise to catch new executable allocations/protection changes
that happen without a DLL event. They are correctness tests, not timing benchmarks.
