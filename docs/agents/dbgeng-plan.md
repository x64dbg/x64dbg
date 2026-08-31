# DbgEng compatibility-first implementation plan

## Goal

Complete the DbgEng-backed `TitanEngine.dll` for live user-mode debugging with
feature parity for every live-process operation in x64dbg's canonical engine
boundary, on x64 and x32. Completion must be demonstrated end to end through
repeatable `headless.exe` tests, while preserving the native TitanEngine,
GleeBug, and StaticEngine backends.

This live-process milestone is a hard gate: do not begin minidump or TTD
implementation until the live test matrix, export/signature checks, teardown
stress tests, and target-access audit all pass. `HideDebugger` is explicitly
excluded. It has been removed from x64dbg's canonical TitanEngine ABI and must
not be reintroduced merely for DbgEng parity; remove any stale adapter-only
export when cleaning the shim.

The migration strategy is deliberately incremental:

1. Keep the TitanEngine API as the engine boundary for now.
2. Extend that API where x64dbg currently calls Win32/NT APIs directly on a
   target process or thread.
3. Implement the new APIs in the native TitanEngine and GleeBug adapters as
   thin forwards to the existing Windows APIs.
4. Fully implement the live semantics in the DbgEng adapter using DbgEng state,
   its internal handle registry, or duplicated native handles where the live
   contract explicitly permits them. Temporary unsupported returns are allowed
   only while a patch is in progress, not at the live-completion gate.
5. Move x64dbg call sites to the TitanEngine API in small groups and add a
   headless regression for each migrated behavior.

A larger process/thread identity abstraction can come later, but the live
implementation must keep the registry authoritative so the following
minidump/TTD work can replace native handles with opaque identities. The
important invariant is that x64dbg must never pass an opaque engine handle
directly to Win32.

## Implementation status

The compatibility-first live backend now builds and runs for x64 and x32 launch
and attach sessions. The implemented surface includes event translation,
continuation, repeatable sessions, short/long-INT3 and UD2 software
breakpoints, hardware slot tracking with DbgEng data breakpoints for data access,
page-guard memory breakpoints with overlap/range/rearm handling, stepping,
memory map and mutation APIs, explicitly filtered `MemoryReadSafe` versus raw
`MemoryReadUnsafe` semantics, x64/x86 register mapping, x87/XMM/YMM transfer,
x64 AVX-512 transfer, non-current thread selection, and common process/thread
controls. Execute hardware breakpoints use an engine-owned native INT3 fallback
because the tested user-mode DbgEng versions accept but do not arm
`DEBUG_BREAK_EXECUTE` data breakpoints.

Standard TitanEngine, GleeBug, StaticEngine, and both DbgEng architectures
export the same required ABI; `scripts/check_titanengine_exports.py` verifies
all eight built DLLs. The x64 and x32 dependency bundles carry matching DbgEng
component sets and root `dbghelp.dll` versions, because Windows may otherwise
bind the backend to x64dbg's already-loaded legacy `dbghelp.dll` and fail DbgEng
loading with `ERROR_PROC_NOT_FOUND`.

The live backend now also maintains an authoritative registry for event,
initialization, and explicitly opened process/thread handles. Export validation
checks the ABI signatures in every adapter header in addition to PE export
names. Runtime AVX-512 mutation remains capability-gated because the validation
host has no AVX-512 support; the state-transfer implementation still builds for
both architectures.

Disable-ASLR is explicitly outside this milestone: x64dbg does not require the
adapter to rewrite target image policy, and the setting remains a compatibility
hint for engines that already support it.

The reproducible gate is `scripts/run_dbgeng_live_gate.py`. Three consecutive
complete DbgEng runs passed 47/47 tests on each of x64 and x32, followed by a
post-race-fix 47/47 run on each architecture and TitanEngine/GleeBug x64/x32
smoke regressions. Each architecture also passed a twenty-session in-process
launch/step/stop stress test. A subsequently exposed page-guard race was fixed
by suspending peer target threads during the hidden unguarded instruction; its
multithread regression then passed ten consecutive runs per architecture.
Export and ABI-signature checks pass for all adapters and architectures.
AVX-512 execution is capability-waived on this validation host, which has no
AVX-512 support. The user explicitly waived a fresh clangd audit run; the
existing source inventory and migrations remain preserved. The live gate is
therefore green and TTD session work may begin.

## Explicit decisions

### Use TitanEngine APIs, not WinAPI hooks

Do not use broad IAT/WinAPI hooks. x64dbg uses APIs such as `CloseHandle`,
`CreateThread`, `VirtualAlloc`, `WaitForSingleObject`, and `ResumeThread` for
both target operations and its own files, events, queues, and worker threads.
Hooking these globally creates an unnecessary classification and recursion
problem.

Instead:

- Add explicit TitanEngine APIs for target operations.
- Replace only call sites known to operate on target process/thread handles.
- Leave debugger-local Win32 operations unchanged.
- Return correct failure values for unsupported target operations.

### The canonical ABI header is in x64dbg

`src/dbg/TitanEngine/TitanEngine.h` is the canonical declaration and type
source. New APIs and types are defined there first.

The following must be kept in lockstep with it:

- `src/dbg/TitanEngine/TitanEngine.def` and the x86/x64 import libraries.
- `src/third_party/TitanEngine` exports.
- `src/third_party/GleeBug/TitanEngineEmulator` exports.
- `src/third_party/GleeBug/StaticEngine` exports where that engine remains a
  selectable backend.
- `../x64dbg-dbgeng/src/TitanEngine/TitanEngine.h` and its exports.

The copied full TitanEngine headers currently present in GleeBug and
StaticEngine are not authoritative. Their x64dbg-facing shim should compile
against the canonical header. GleeBug-only legacy declarations can be moved to
an internal legacy header if they are still needed for extra exports.

### ABI breakage is allowed, but must be explicit

Strict compatibility with external historical TitanEngine binaries is not a
requirement. It is acceptable to add required imports or correct signatures,
but every break must be recorded in the relevant patch and all in-tree engines
must be updated atomically.

In particular, do not add a required export to x64dbg and leave one selectable
engine without it: the delay-loaded DLL will fail when that symbol is first
used.

## Current facts that affect the plan

### Canonical surface

The current canonical x64dbg header and definition file declare 55 required
exports. In addition to the original session, callback, memory, context,
breakpoint, and stepping APIs, the extended surface covers:

- Memory query, allocation, free, and protection.
- Explicit target-handle closure and WOW64 detection.
- Process terminate, break-in, and remote-thread creation.
- Thread suspend, resume, terminate, ID, priority, timing, and cycle queries.

`HideDebugger` is not part of this surface. Static file mapping/dumping helpers
are also not live-process parity requirements unless a current x64dbg live call
site is found to require one.

The 55-export list is not assumed permanently complete. Before declaring live
parity, rerun `scripts/live_target_api_audit.py` against the mandatory
`build/compile_commands.json`, classify every remaining direct target API use,
and either migrate it to an explicit canonical API or prove that it is
host/debugger-local. Regenerate import libraries and rerun both export-name and
signature-conformance checks after every ABI change.

### GleeBug compatibility findings

`src/third_party/GleeBug` is an in-tree submodule with two relevant adapters:

- `GleeBugTitanEngine`, emitted as `GleeBug/TitanEngine.dll`.
- `GleeBugStaticEngine`, emitted as `StaticEngine/TitanEngine.dll`.

Both still carry copied TitanEngine-facing headers rather than directly
consuming the canonical x64dbg header. Their implementations now expose all 55
required names, including explicit unsupported AVX exports in StaticEngine,
and `scripts/check_titanengine_exports.py` validates the built DLLs. The
remaining work is to consume the canonical header or enforce a checked
generated copy and to compile signature-conformance translation units, so
matching export names cannot hide enum, callback, calling-convention, or output
parameter drift.

The main GleeBug emulator already has useful adaptation machinery:

- `processFromHandleCache` and `threadFromHandleCache`.
- `MemoryReadSafe`/`MemoryWriteSafe` dispatch through GleeBug objects when a
  handle belongs to the active debug session.
- A Win32 fallback for handles not owned by GleeBug.
- Existing Titan callback and continuation semantics.

This makes GleeBug a straightforward implementation of the extended surface:
look up a GleeBug object when necessary, otherwise forward to Win32.

StaticEngine is not a normal live debugger and may return unsupported for live
mutation/control operations, but it must still export every required canonical
symbol with the correct signature.

### DbgEng live foundation and remaining blockers

The original event-loop blockers are resolved: initialization handles are
independent, event ordering and the system breakpoint are translated,
continuation is applied, stepping is keyed by thread, non-current general
register reads and writes work, callback locking is safe, and normal unsupported
paths no longer execute `__debugbreak()`.

The remaining blockers to complete live parity are:

1. Hardware/data breakpoints and their Titan DR-slot semantics.
2. Page-based memory breakpoints with persistent rearming and protection
   restoration.
3. Long-INT3 and UD2 software breakpoint encodings in addition to ordinary
   INT3, including running-state mutation and one-shot cleanup.
4. Complete x87, MMX, XMM, YMM, ZMM, opmask, and AVX/AVX-512 context reads and
   writes for current and non-current threads.
5. Full launch/attach engine-option behavior, attach exit policy, and failure
   cleanup.
6. x32 DbgEng build, deployment, register mapping, and tests.
7. Closure of remaining direct target-access call sites and comprehensive
   headless tests for all live behavior.

These issues must be fixed before minidump or TTD implementation begins.

## Handle contract

Use `HANDLE` as the public opaque type for now, but define its engine contract.

### Categories

1. **Initialization handles** returned in `PROCESS_INFORMATION` by
   `InitDebugW`. x64dbg owns these and closes them before `DebugLoop`.
2. **Event/session handles** supplied by create-process/create-thread events.
   They remain valid until the corresponding exit event or session teardown.
3. **Explicitly opened handles** returned by `TitanOpenProcess` and
   `TitanOpenThread`. The caller owns these and closes them through
   `TitanCloseHandle`.
4. **Non-engine native handles**, such as files and x64dbg worker events. These
   remain ordinary Win32 handles and use `CloseHandle`.

### DbgEng registry

The DbgEng implementation should maintain entries similar to:

```cpp
enum class TitanHandleType
{
    Process,
    Thread,
};

struct TitanHandleEntry
{
    TitanHandleType type;
    ULONG dbgengId;
    std::optional<DWORD> systemId;
    HANDLE nativeHandle; // present for a live target when available
    bool callerOwned;
};
```

For a live process, using duplicated native handles is preferable because it
keeps not-yet-migrated x64dbg code operational. The registry remains the source
of truth and permits future dump/TTD entries with no native handle.

`InitDebugW` must return independently owned duplicates. Closing them must not
invalidate the later event/session handles.

### New close API

Add to the canonical surface:

```cpp
__declspec(dllexport) bool TitanCloseHandle(HANDLE hEngineHandle);
```

Semantics:

- Close/release an explicitly opened or caller-owned engine handle.
- Return `false` and set `ERROR_INVALID_HANDLE` for an unknown value.
- Native TitanEngine and GleeBug may forward known native handles to
  `CloseHandle`.
- DbgEng removes the registry entry and closes its native duplicate if present.
- Do not route arbitrary file/event/worker handles through this API.

Target-handle RAII will eventually need a dedicated wrapper so it does not use
the existing generic `Handle` type, whose destructor calls `CloseHandle`.

## Proposed TitanEngine extensions

Use Windows-compatible types and return conventions where possible. This keeps
x64dbg call-site changes mechanical and lets native engines forward directly.
The final names can be adjusted, but one canonical naming scheme must be chosen
before implementation.

### Tier 0: required for the first usable live DbgEng session

```cpp
__declspec(dllexport) bool TitanCloseHandle(HANDLE hEngineHandle);

__declspec(dllexport) SIZE_T MemoryQuerySafe(
    HANDLE hProcess,
    LPCVOID lpAddress,
    PMEMORY_BASIC_INFORMATION lpBuffer,
    SIZE_T dwLength);

__declspec(dllexport) bool ProcessIsWow64(
    HANDLE hProcess,
    PBOOL isWow64);
```

Rationale:

- `MemoryQuerySafe` replaces the `VirtualQueryEx` paths used to build the memory
  map and derive module regions.
- `ProcessIsWow64` prevents startup from depending on the public handle being a
  native Win32 handle.
- `TitanCloseHandle` makes handle ownership explicit.
- Existing `MemoryReadSafe`, `MemoryWriteSafe`, context, software-breakpoint,
  and stepping APIs cover the rest of the basic inspection path.

### Tier 1: semi-feature-parity for common memory commands

```cpp
__declspec(dllexport) LPVOID MemoryAllocSafe(
    HANDLE hProcess,
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD flAllocationType,
    DWORD flProtect);

__declspec(dllexport) bool MemoryFreeSafe(
    HANDLE hProcess,
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD dwFreeType);

__declspec(dllexport) bool MemoryProtectSafe(
    HANDLE hProcess,
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD flNewProtect,
    PDWORD lpflOldProtect);
```

Implementations:

- Native TitanEngine/GleeBug: forward to `VirtualAllocEx`, `VirtualFreeEx`, and
  `VirtualProtectEx` after any engine lookup required.
- DbgEng live: use a DbgEng operation if appropriate; otherwise use the native
  duplicate associated with the opaque process handle.
- Dump/TTD later: return the proper unsupported result.

### Tier 1: common process/thread operations

```cpp
__declspec(dllexport) bool TitanTerminateProcess(
    HANDLE hProcess,
    DWORD exitCode);

__declspec(dllexport) bool TitanDebugBreakProcess(
    HANDLE hProcess);

__declspec(dllexport) HANDLE TitanCreateRemoteThread(
    HANDLE hProcess,
    LPTHREAD_START_ROUTINE start,
    LPVOID argument,
    DWORD creationFlags,
    LPDWORD threadId);

__declspec(dllexport) DWORD TitanSuspendThread(HANDLE hThread);
__declspec(dllexport) DWORD TitanResumeThread(HANDLE hThread);
__declspec(dllexport) bool TitanTerminateThread(HANDLE hThread, DWORD exitCode);
__declspec(dllexport) DWORD TitanGetThreadId(HANDLE hThread);
__declspec(dllexport) DWORD TitanGetProcessIdOfThread(HANDLE hThread);
__declspec(dllexport) int TitanGetThreadPriority(HANDLE hThread);
__declspec(dllexport) bool TitanSetThreadPriority(HANDLE hThread, int priority);
__declspec(dllexport) bool TitanGetThreadTimes(
    HANDLE hThread,
    LPFILETIME creation,
    LPFILETIME exit,
    LPFILETIME kernel,
    LPFILETIME user);
__declspec(dllexport) bool TitanQueryThreadCycleTime(
    HANDLE hThread,
    PULONG64 cycleTime);
```

Return conventions must match the corresponding Win32 API, particularly
`TitanSuspendThread`/`TitanResumeThread`, which return `(DWORD)-1` on failure.

### Tier 2: live closure APIs

Add narrowly scoped APIs when migrating the audited x64dbg caller:

- Process and thread information-class queries currently using `NtQuery*`.
- Mapped/module/image filename queries.
- Remote-handle duplication and object-name/type queries.
- Process-token operations.
- Stack walking and symbol operations where a native handle cannot be part of
  the final engine contract.

These are required live work when the audit classifies the caller as a target
operation; they are not optional merely because duplicated native handles make
them work today. Minidump generation is deliberately not in this tier and
starts only after the live gate.

Do not prematurely move live host state into TitanEngine:

- Window enumeration/control.
- TCP-owner enumeration.
- Process picker enumeration.

Those APIs use system IDs and host state rather than debug-target memory. They
can remain available for live sessions and return empty/unavailable for
non-live targets later.

## Phase 0: make the existing DbgEng event loop correct

This phase should make almost no changes in x64dbg.

### 0.1 Fix process creation

- Build the complete mutable command line from executable and arguments.
- Respect `szCurrentFolder`.
- Use the appropriate `IDebugClient` create-process method and options.
- Produce caller-owned initialization handles distinct from session handles.
- Register both initialization and event/session handles with their DbgEng IDs.

### 0.2 Centralize event dispatch

Create one helper that:

1. Builds and stores `gFakeDebugEvent`.
2. Calls `UE_CH_DEBUGEVENT` first.
3. Calls the event-specific handler with the correct union member.

Use it for:

- Create/exit process.
- Create/exit thread.
- Load/unload DLL.
- Exceptions.
- DbgEng breakpoints.
- Output debug strings if exposed by the chosen DbgEng callback path.

Do not call `gCustomHandlers.at(...)`; missing optional handlers must not throw.

### 0.3 Recognize the system breakpoint

Track the first breakpoint for each newly created process. Translate the first
DbgEng breakpoint exception to `UE_CH_SYSTEMBREAKPOINT`. Subsequent unknown
breakpoint exceptions go through `UE_CH_UNHANDLEDEXCEPTION`; DbgEng breakpoint
objects registered by `SetBPX` invoke their stored Titan callback.

### 0.4 Implement continuation

Store the value from `SetNextDbgContinueStatus` and translate it after x64dbg's
callback returns:

- `DBG_CONTINUE` -> handled/go status.
- `DBG_EXCEPTION_NOT_HANDLED` -> `DEBUG_STATUS_GO_NOT_HANDLED`.
- An explicit step request takes precedence over ordinary go.

A normal Run command only releases `WAITID_RUN`; therefore the DbgEng adapter
must issue the actual `SetExecutionStatus` transition afterward.

Fix `debugStep` to key step callbacks by current thread ID rather than process
ID.

### 0.5 Fix callback queue locking

Do not hold the callback-queue lock while executing x64dbg callbacks. Pop work
under the lock, release it, then execute. DbgEng may deliver another event while
an x64dbg callback invokes a DbgEng method; holding the queue lock across the
callback risks deadlock.

### 0.6 Make cleanup repeatable

On process/thread exit and session end:

- Remove ID/handle mappings.
- Release caller- and engine-owned duplicates according to ownership.
- Clear breakpoint and step callback maps.
- Release DbgEng breakpoint COM objects.
- Reset fake event and context caches.
- Ensure a second launch in the same x64dbg instance works.

## Phase 1: route the minimum x64dbg target accesses through TitanEngine

### 1.1 Memory query

Replace target `VirtualQueryEx` calls in:

- `src/dbg/memory.cpp`
- `src/dbg/module.cpp`

with `MemoryQuerySafe`.

Keep local `VirtualQuery`/allocation operations unchanged.

This is the highest-value API addition because `cbCreateProcess` immediately
calls `MemUpdateMap`, and `cbSystemBreakpoint` refreshes it again.

### 1.2 Startup architecture check

Replace the target-side `IsWow64Process(fdProcessInfo->hProcess, ...)` in
`debugLoopFunction` with `ProcessIsWow64`. The `GetCurrentProcess()` query for
x64dbg itself remains an ordinary Win32 call.

Failure remains fatal because x64dbg cannot safely debug the wrong architecture.
The DbgEng implementation must therefore support this Tier 0 operation rather
than returning unsupported.

### 1.3 Handle closure

Replace only target-engine handle closes with `TitanCloseHandle`, including:

- Initialization process/thread handles returned by `InitDebugW`.
- Handles returned by `TitanOpenProcess` and `TitanOpenThread`.
- Future handles returned by new Titan target APIs.

Do not change file, mapping, event, semaphore, or x64dbg worker-thread closes.

### 1.4 Memory write and contexts

Complete the already-existing APIs before adding more surface:

- `MemoryWriteSafe` using `IDebugDataSpaces4::WriteVirtual` for DbgEng.
- `GetContextDataEx` and `GetFullContextDataEx` for any registered thread, not
  only DbgEng's current thread.
- `SetContextDataEx` and `SetFullContextDataEx`.
- Register cache `Flush()` using DbgEng register writes.

For a non-current thread, save the current DbgEng thread, select the requested
thread under the paused-state lock, perform the operation, then restore and
refresh the previous context.

AVX/AVX-512 can initially return zeroed unsupported portions while preserving
valid general registers, but the return value and GUI behavior must be
consistent.

## Phase 2: common feature parity

### 2.1 Memory mutation

Replace target uses of:

- `VirtualAllocEx`
- `VirtualFreeEx`
- `VirtualProtectEx`

with `MemoryAllocSafe`, `MemoryFreeSafe`, and `MemoryProtectSafe`.

This covers allocation/free/protection commands and page-right changes without
a broad x64dbg refactor.

### 2.2 Thread operations

Move the direct target operations in `thread.cpp`, `stackinfo.cpp`,
`cmd-thread-control.cpp`, and `cmd-debug-control.cpp` to the proposed Titan
thread APIs.

Debugger-owned threads, symbol-loader threads, and task threads remain direct
Win32 operations.

### 2.3 Process control

Move target `DebugBreakProcess` and `TerminateProcess` to
`TitanDebugBreakProcess` and `TitanTerminateProcess`.

Remote thread creation can use `TitanCreateRemoteThread`. DbgEng may implement it using a
native live handle initially or return `ERROR_NOT_SUPPORTED` without affecting
ordinary debugging.

### 2.4 Breakpoints

For semi-feature-parity:

- Software breakpoints must work, including one-shot cleanup and repeated
  set/delete operations.
- Stepping must work.
- Hardware and memory breakpoint functions may initially return `false` with
  `ERROR_NOT_SUPPORTED`, but they must not execute `__debugbreak()`.
- `RemoveAllBreakPoints` must safely remove only breakpoints that exist and
  release associated COM objects.

## Phase 3: complete live-process parity

Phase 3 closes every temporary unsupported or partial live path. It is required
before minidump or TTD work. Every subsection needs focused `headless.exe`
coverage and must preserve the other selectable engines.

### 3.1 Software breakpoint completion

Support every canonical Titan software encoding and lifecycle operation:

- Short INT3, long INT3, and UD2 selected through `SetBPXOptions` or the explicit
  type bits passed to `SetBPX`.
- Persistent and `UE_SINGLESHOOT` breakpoints.
- Set, hit, disable, re-enable, delete, and remove-all sequences.
- Breakpoint changes while paused and safe coordination when x64dbg requests a
  pause or mutation while DbgEng is inside `WaitForEvent`.
- Original-byte ownership, instruction-cache flushing, stale exception
  suppression, and overlapping/repeated requests.
- Correct callback ordering and continuation after an engine breakpoint, a
  native-patch fallback, or an unknown breakpoint exception.

Do not manually release an `IDebugBreakpoint` after giving it back to DbgEng.
Track engine ownership explicitly and clear maps only after removal or session
termination has made the object unreachable.

### 3.2 Hardware/data breakpoints

Implement `GetUnusedHardwareBreakPointRegister`, `SetHardwareBreakPoint`, and
`DeleteHardwareBreakPoint` using DbgEng data breakpoints:

- Create `DEBUG_BREAKPOINT_DATA` objects and call `SetDataParameters`.
- Map Titan execute, write, and read/write modes to the applicable
  `DEBUG_BREAK_*` access flags.
- Map Titan sizes 1, 2, 4, and 8, validating architecture alignment and execute
  constraints before changing DbgEng state.
- Maintain an explicit Titan DR0-DR3 slot registry independent of DbgEng's
  breakpoint object IDs. Reject duplicate/invalid slots and report resource
  exhaustion normally.
- Route hits to the stored `TITANCBHWBP` callback with the correct current
  process/thread and synthetic event state.
- Define behavior for threads created after the breakpoint, thread switches,
  deletion, remove-all, detach, and process exit. Prefer DbgEng's global
  user-process data-breakpoint management rather than manually writing debug
  registers unless testing proves a DbgEng gap.

Tests must cover execute, write, and read/write hits; all valid sizes;
alignment rejection; all four slots; fifth-slot exhaustion; delete/reuse;
multiple threads; and repeat sessions.

### 3.3 Page-based memory breakpoints

Implement `SetMemoryBPXEx` and `RemoveMemoryBPX` with Titan-compatible range
semantics rather than treating them as small hardware data breakpoints:

- Normalize arbitrary byte ranges to pages while retaining the exact requested
  range and access mode.
- Save original protections and maintain per-page reference counts so
  overlapping breakpoints do not restore a page too early.
- Implement `UE_MEMORY`, read, write, and execute filtering from exception
  access metadata.
- On a hit, identify the exact registered range, update `gFakeDebugEvent`, and
  invoke the stored `TITANCBMEMBP` in canonical event order.
- Honor `RestoreOnHit`. For persistent breakpoints, coordinate a hidden
  single-step/rearm cycle without consuming or corrupting a user-requested
  step. For one-shot behavior, remove only the matching registration.
- Preserve unrelated protection flags, guard state, and application-initiated
  protection changes as far as the Win32 model allows. Document and test the
  policy for pre-existing `PAGE_GUARD` pages.
- Restore protections on explicit removal, remove-all, detach, process exit,
  failed setup, and debugger shutdown.

Tests must cover each access type, hit/no-hit filtering, cross-page ranges,
overlapping ranges, persistent repeated hits, one-shot hits, concurrent user
stepping, original-protection restoration, deletion before hit, and teardown.

### 3.4 Complete thread contexts

Make scalar and full context APIs faithful for all supported live
architectures:

- Complete x87 control/status/tag and stack registers, MMX, XMM, YMM, ZMM, and
  AVX-512 opmask transfer in `GetFullContextDataEx`, `SetFullContextDataEx`,
  `GetAVXContext`, `SetAVXContext`, `GetAVX512Context`, and
  `SetAVX512Context`.
- Convert every relevant `DEBUG_VALUE` representation without truncation and
  batch writes through the register cache.
- Preserve unsupported-by-CPU state deterministically: distinguish a CPU/OS
  feature absence from an adapter implementation failure and zero documented
  unavailable output fields.
- Support current and non-current threads by saving, selecting, refreshing,
  flushing, and restoring DbgEng thread state under the paused-state lock.
- Complete x86 register aliases and WOW64 behavior without mapping `rip/rsp`
  unconditionally.
- Verify segment, flags, debug registers, instruction/stack pointers, and SIMD
  edits in the GUI-facing full context as well as scalar APIs.

Tests must edit and read back GPR, flags, x87, XMM, YMM, and available
AVX-512 state on current and non-current threads, then execute instructions that
prove the target observed the change.

### 3.5 Memory, process, and thread operation closure

Validate and complete every canonical live operation, including failure
semantics and output initialization:

- Partial and cross-region memory reads/writes, memory-map queries, allocation,
  reserve/commit, protection changes, instruction-cache effects, and release.
- PEB/TEB lookup for initial and newly created threads.
- Open/close process and thread handles without stale registry entries or
  handle-reuse confusion.
- Process break-in and termination; remote-thread creation and returned handle
  ownership.
- Thread suspend/resume counts, termination, IDs, priority get/set, creation and
  CPU times, and cycle queries.
- Current and non-current stack/context access while another thread exits.
- Predictable `LastError`, byte counts, and zeroed outputs on invalid handles,
  running targets, exited targets, and unsupported host capabilities.

The current source audit also identifies target operations outside the first 54
exports. Add narrowly typed canonical APIs and migrate their live call sites as
needed, including:

- Process/thread `NtQueryInformation*` uses for process cookies, basic
  information, suspend counts, and thread metadata.
- Mapped-file, module-image, and process-image filename queries.
- Target process-token opening and any token lifetime used by debugger startup.
- Remote handle duplication/closure and handle owner/type/name queries used by
  the Handles view and operating-system-control commands.
- Remaining direct `ReadProcessMemory`, `GetThreadContext`, and similar calls in
  undocumented commands, stack walking, exports, and helper headers.

Do not add one catch-all escape hatch. Each API must have explicit ownership,
output, running-state, live-engine, and future non-live semantics.

Rerun the live-target API audit after this work. No remaining direct target
Win32/NT call may be accepted merely because DbgEng currently supplies a native
handle; it must be classified and either routed through the engine boundary or
explicitly documented as debugger-local.

### 3.6 Launch, attach, events, and engine options

Finish behavior that is easy to miss in simple launch tests:

- Preserve executable quoting, arbitrary arguments, Unicode paths, environment,
  and working directory.
- Honor canonical `SetEngineVariable` settings that affect required live
  behavior: no-console-window, debug privilege, safe attach, alternate
  memory-breakpoint behavior, and safe-step. Disable-ASLR is an optional
  compatibility hint and is explicitly outside the live gate.
- Honor `KillOnExit`, explicit detach, detach-on-exit, stop while running, stop
  while paused, and failed attach/launch rollback.
- Preserve callback ordering and synthetic event state for create/exit process,
  create/exit thread, load/unload DLL, output-debug-string, system breakpoint,
  software/data/memory breakpoints, and first/second-chance exceptions.
- Verify handled versus not-handled continuation for representative exception
  classes, including breakpoint, single-step, access violation, guard page,
  illegal instruction, and application-defined exceptions.
- Keep debug output useful while suppressing only proven duplicate state-change
  notifications; expected memory probes and normal teardown must not be logged
  as backend errors.

x64dbg intentionally debugs one selected process with
`DEBUG_ONLY_THIS_PROCESS`; child-process and general multi-process debugging are
not added unless the canonical x64dbg behavior changes. Remote DbgEng and kernel
debugging are also outside this live user-mode milestone.

### 3.7 x32 and deployment parity

The live gate covers both x64dbg and x32dbg:

- Build a 32-bit DbgEng shim and deploy matching-architecture DbgEng components.
- Keep `dbgeng.dll`, `dbghelp.dll`, `dbgcore.dll`, and `dbgmodel.dll` version
  matched in each architecture's distribution.
- Enable the DbgEng engine selector in x32dbg only when its runtime is present.
- Validate native x86 processes and the normal x64dbg architecture-selection
  flow. Do not claim cross-bitness support from a build that only happens to
  open a process handle.
- Run export and signature checks for both DbgEng architectures.

### 3.8 Lifecycle, ownership, and concurrency hardening

Before the live gate:

- Close or release every initialization duplicate, explicit open handle, event
  handle, callback object, and DbgEng breakpoint according to one documented
  owner.
- Remove process/thread IDs, PEB/TEB entries, breakpoints, pending steps,
  synthetic strings, callbacks, and register caches at the corresponding exit
  event and again idempotently at session teardown.
- Ensure callbacks never execute while holding queue or cache locks and ensure
  GUI/script worker reads cannot observe freed synthetic state.
- Make shutdown and initialization failure safe if only a subset of COM
  interfaces, events, or threads was created.
- Stress launch/exit, attach/detach, terminate, failed launch, failed attach,
  and debugger shutdown for many iterations while checking handle counts,
  callback-map sizes, and absence of stale events in the next session.

`HideDebugger` is not a task in this or any later phase. It is absent from the
canonical ABI.

## Graceful failure contract

Every canonical API must be callable in every selectable engine. During
incremental development, or in a non-live backend such as StaticEngine, an
unsupported operation must return a documented value and set `LastError` where
the return shape follows Win32. This contract prevents crashes but is not a
substitute for implementation: at the live gate, DbgEng may use it only for a
genuine host/CPU capability absence or an operation that is explicitly outside
the live user-mode scope above.

| Operation shape | Unsupported result |
|---|---|
| `bool` | `false`, `SetLastError(ERROR_NOT_SUPPORTED)` |
| pointer/handle | `nullptr`, `SetLastError(ERROR_NOT_SUPPORTED)` |
| byte/region count | `0`, zero any output count |
| suspend/resume count | `(DWORD)-1`, `SetLastError(ERROR_NOT_SUPPORTED)` |
| scalar register getter without status | `0`; prefer a future status-returning replacement if ambiguity matters |
| callback registration/configuration | ignore only if documented; never crash |

Output buffers and byte counts must be initialized on failure. Unsupported
features should produce one useful x64dbg error rather than repeated DbgEng log
spam.

Remove `__debugbreak()` from normal unsupported paths. Keep assertions only for
internal invariants whose violation indicates an adapter bug.

## GleeBug workstream

Changes to the canonical surface require a coordinated GleeBug submodule
update.

### Header and signature synchronization

1. Make GleeBug's x64dbg-facing emulator compile against
   `src/dbg/TitanEngine/TitanEngine.h` during the x64dbg build.
2. Move GleeBug-only legacy constants/types needed for extra exports into a
   separate internal header.
3. Update wrapper signatures to exactly match canonical enum and callback
   types, not merely ABI-compatible `DWORD`/`LPVOID` substitutes.
4. Add explicit canonical-signature `GetAVXContext` and `SetAVXContext`
   wrappers to GleeBug; retain StaticEngine's explicit graceful-failure exports.
5. Add every newly required export to both GleeBug and StaticEngine shims.

Because GleeBug is also independently buildable, its upstream repository may
retain a generated copy of the canonical ABI header. Add a synchronization or
hash check so it cannot silently diverge.

### Native forwarding implementations

For GleeBug live debugging:

- Reuse `processFromHandle`/`threadFromHandle` where engine objects provide a
  better operation.
- Otherwise forward the new API to the corresponding Win32 function.
- Preserve exact Win32 return values and `LastError`.
- Implement `TitanCloseHandle` consistently with the handle caches; evict a
  cached entry before closing to prevent stale lookups and handle-reuse bugs.

For StaticEngine:

- Forward operations that are meaningful with a real process/thread handle.
- Return `ERROR_NOT_SUPPORTED` for operations that are not part of its static
  model.
- Export the full canonical surface regardless.

### Export conformance check

Add a build/test script that compares each selected engine DLL against the
canonical required export list:

- Standard `TitanEngine.dll`
- `GleeBug/TitanEngine.dll`
- `StaticEngine/TitanEngine.dll`
- x64 and x32 `DbgEng/TitanEngine.dll`

The check should fail on a missing export before x64dbg starts. Also compile a
small conformance translation unit against the canonical signatures for each
adapter so signature drift is caught at build time, not just by PE export-name
comparison.

## Expected x64dbg changes

Keep changes localized:

1. Extend `src/dbg/TitanEngine/TitanEngine.h` and `.def`.
2. Regenerate/update `TitanEngine_x86.lib` and `TitanEngine_x64.lib`.
3. Replace target API calls primarily in:
   - `memory.cpp`
   - `module.cpp`
   - `thread.cpp`
   - `stackinfo.cpp`
   - `commands/cmd-memory-operations.cpp`
   - `commands/cmd-thread-control.cpp`
   - selected target paths in `commands/cmd-debug-control.cpp`
4. Add a target-handle RAII helper if needed; do not alter the generic native
   `Handle` helper.
5. Add error reporting where an existing direct API failure was ignored.

Do not replace `fdProcessInfo`, `hActiveThread`, `THREADINFO.Handle`, Titan
callback structures, or bridge/plugin handle APIs merely for cosmetic
abstraction. They remain the compatibility representation for live sessions,
but every operation performed through them must obey the engine handle
contract and must be representable by the registry before minidump/TTD work.

## Validation milestones

### Milestone A: event loop

With DbgEng on x64 and, once Phase 3.7 lands, x32:

- Launch the test application with arguments and working directory.
- Receive create-process and module events.
- Receive exactly one system-breakpoint callback.
- Pause at the system breakpoint.
- Run and reach process exit.
- Stop while running and while paused.
- Launch a second process in the same x64dbg instance.

### Milestone B: basic inspection

- Non-empty memory map.
- Disassembly at CIP.
- General register display.
- PEB and TEB expressions.
- Thread list for a multithreaded target.
- Stack for current and non-current threads.
- Module load/unload updates.

### Milestone C: interaction

- Set, hit, disable, enable, and delete software breakpoints.
- One-shot entry breakpoint.
- Step into and step over.
- Read and write memory.
- Edit a general register.
- Allocate, protect, and free target memory and verify the resulting map and
  access rights.
- Exercise all software encodings, hardware/data breakpoints, page-based memory
  breakpoints, and complete scalar/SIMD context mutation.

### Milestone D: cross-engine compatibility

Run applicable `src/tests/run.py` suites for:

- TitanEngine x64 and x32.
- GleeBug x64 and x32.
- StaticEngine tests.
- DbgEng x64 and x32.

During intermediate patches, a not-yet-implemented DbgEng feature must have an
explicit expected-unsupported test rather than being silently skipped. At the
live-completion gate, no applicable canonical live API may remain in that
category.

At minimum, run launch/break/run/step/memory/register/cleanup smoke tests against
both TitanEngine and GleeBug after each canonical ABI change.

### Milestone E: mandatory headless live gate

The gate is a checked test suite, not a manual claim. Add deterministic test
programs, scripts, and Python drivers under `src/tests` and run them through
`src/tests/run.py`/`headless.exe`. The matrix must include:

- Launch with Unicode paths, quoting, arguments, working directory, and relevant
  engine options; normal exit and forced stop.
- Attach, blocked-target break-in, detach, terminate, detach-on-exit, and failed
  attach rollback.
- Event order, system breakpoint count, thread/module churn, debug strings, and
  handled/not-handled first- and second-chance exceptions.
- Short INT3, long INT3, UD2, one-shot, stale-breakpoint suppression,
  set/delete/reuse, and remove-all.
- Hardware execute/write/read-write modes, sizes and alignment, four-slot
  exhaustion, deletion/reuse, and multiple threads.
- Memory read/write/execute filters, persistent and one-shot behavior,
  cross-page and overlapping ranges, stepping interaction, and protection
  restoration.
- Memory query/read/write/allocate/protect/free including partial failures.
- Current and non-current GPR, flags, x87, XMM, YMM, and available AVX-512 edits
  proven by target-side assertions.
- Process break-in/terminate/remote-thread creation and every canonical thread
  control/metadata API.
- Multi-session and lifecycle stress with no stale callbacks, breakpoint
  objects, IDs, native handles, or increasing handle count.
- x64 and x32 DbgEng runs, plus applicable TitanEngine, GleeBug, and StaticEngine
  regressions.

The gate also runs:

- `scripts/check_titanengine_exports.py` for every built engine and
  architecture.
- Canonical ABI-signature conformance for every adapter header, combined with
  normal adapter compilation so implementation/header drift is a build error.
- `scripts/live_target_api_audit.py` using `build/compile_commands.json`, with
  reviewed inventory and reverse call chains showing no unclassified direct
  target access. A fresh run may be explicitly waived when the preserved
  inventory is accepted and no audit-driven ABI change is requested.
- `git diff --check`, Python syntax checks, and clean x64/x32 builds.

Any flaky failure is a gate failure. Preserve artifacts and iterate until the
same complete matrix passes repeatedly from a clean process environment.

## Patch sequence

Keep patches independently reviewable and working:

1. **Canonical ABI conformance:** canonical-header consumption, explicit
   GleeBug AVX wrappers, missing StaticEngine AVX exports, and export checks; no
   new behavior.
2. **DbgEng event-loop correctness:** event dispatch, system breakpoint,
   continuation, queue locking, cleanup, handle ownership.
3. **Tier 0 API:** `TitanCloseHandle`, `MemoryQuerySafe`, `ProcessIsWow64` in all
   engines; migrate only startup/memory-map call sites.
4. **Context completion:** non-current reads, writes, register flush, graceful
   AVX behavior.
5. **Memory parity:** alloc/free/protect APIs and call-site migration.
6. **Thread/process controls:** migrate direct target operations; unsupported
   paths return correct failures.
7. **Software breakpoint completion:** all encodings, one-shot behavior,
   running-state coordination, ownership, and remove-all.
8. **Hardware breakpoint parity:** DbgEng data breakpoints, Titan slot mapping,
   callbacks, lifecycle, and tests.
9. **Memory breakpoint parity:** page/range tracking, access filtering, hidden
   rearm stepping, overlapping protections, lifecycle, and tests.
10. **Context parity:** x87/MMX/SIMD/AVX/AVX-512 reads and writes, non-current
    threads, and x86 mappings.
11. **Live operation closure:** engine options, attach/exit policy, remaining
    process/thread/memory APIs, audit closure, and x32 deployment.
12. **Live gate:** full headless matrix, cross-engine regressions, conformance
    checks, and repeated lifecycle stress.
13. **Only after step 12 is green:** begin the separately planned minidump and
    TTD handle/session work.

Each patch must keep standard TitanEngine and GleeBug usable. During development
a feature may temporarily return a documented unsupported result, but selecting
DbgEng must never cause a missing import, unhandled adapter exception, or
deliberate debug break. Temporary unsupported behavior must be eliminated for
all applicable canonical live APIs by step 12.

## Definition of live-process completion

The DbgEng live milestone is complete only when:

- x64dbg and x32dbg can launch, attach, detach, stop, and repeatedly debug live
  targets through normal UI, command, script, and headless flows.
- All canonical software, hardware/data, and page-based memory breakpoint
  operations work with correct callbacks and cleanup.
- Memory maps and mutation, PEB/TEB, current and non-current stacks, full scalar
  and SIMD contexts, stepping, exception continuation, process controls, and
  thread controls behave correctly.
- All applicable engine variables and launch/attach policies are honored.
- No applicable canonical live API returns `ERROR_NOT_SUPPORTED`; host/CPU
  feature absence is reported distinctly and tested.
- No opaque engine handle is passed directly to Win32, and every remaining
  direct target access from the reproducible audit is migrated or explicitly
  proven debugger-local.
- The complete x64/x32 DbgEng headless matrix passes repeatedly, including
  lifecycle/handle stress, with preserved evidence.
- TitanEngine, GleeBug, and StaticEngine applicable regressions pass.
- Every selectable engine and architecture passes canonical export and
  signature conformance.
- `HideDebugger` remains absent from the canonical ABI and is not treated as a
  missing feature.

The live evidence recorded above is green, with AVX-512 runtime execution and a
fresh audit run explicitly capability/user-waived. Minidump and TTD
implementation may now begin. Their design should reuse the validated event,
context, breakpoint, and authoritative handle-registry boundaries rather than
weakening live-process behavior.
