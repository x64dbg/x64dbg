# DbgEng compatibility-first implementation plan

## Goal

Get the DbgEng-backed `TitanEngine.dll` working for live x64 processes at
semi-feature-parity with x64dbg's current engines, while making the smallest
practical changes in `src/dbg`.

The migration strategy is deliberately incremental:

1. Keep the TitanEngine API as the engine boundary for now.
2. Extend that API where x64dbg currently calls Win32/NT APIs directly on a
   target process or thread.
3. Implement the new APIs in the native TitanEngine and GleeBug adapters as
   thin forwards to the existing Windows APIs.
4. Implement them in the DbgEng adapter using DbgEng state, an internal handle
   lookup, or a documented unsupported result.
5. Move x64dbg call sites to the TitanEngine API in small groups.

A larger process/thread identity abstraction can come later. `HANDLE` is
allowed to be opaque at this stage. The important invariant is that x64dbg must
not pass an opaque engine handle directly to Win32.

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

The current canonical x64dbg header declares 37 exports. It already covers:

- Debug session startup, attach/detach, stop, and the debug loop.
- Generic and event-specific callbacks.
- Memory reads and writes.
- PEB/TEB lookup.
- Full/scalar/AVX/AVX-512 contexts.
- Software, memory, and hardware breakpoints.
- Stepping and continuation status.
- Opening process and thread handles.

It does not cover memory-map queries, memory allocation/protection, most thread
metadata/control, target handle closure, or several process operations that
x64dbg invokes directly.

### GleeBug compatibility findings

`src/third_party/GleeBug` is an in-tree submodule with two relevant adapters:

- `GleeBugTitanEngine`, emitted as `GleeBug/TitanEngine.dll`.
- `GleeBugStaticEngine`, emitted as `StaticEngine/TitanEngine.dll`.

Both use their own copied TitanEngine header instead of the canonical x64dbg
header. The currently built GleeBug DLL exports all 37 canonical names,
including `GetAVXContext` and `SetAVXContext`; those two exports are produced
indirectly through declarations/helpers rather than obvious wrappers in
`TitanEngineEmulator.cpp`, so signature conformance should still be made
explicit. The currently built StaticEngine DLL lacks `GetAVXContext` and
`SetAVXContext` while exporting the AVX-512 pair. This is an existing
compatibility defect and must be fixed as part of ABI synchronization.

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

### DbgEng POC blockers

The POC has reached a DbgEng breakpoint, but several compatibility details still
prevent a normal x64dbg session:

1. `InitDebugW` and the synthetic create-process event reuse handle values with
   unclear ownership, while x64dbg closes the handles returned by `InitDebugW`.
2. `UE_CH_DEBUGEVENT` is never invoked.
3. The first breakpoint is not translated to `UE_CH_SYSTEMBREAKPOINT`.
4. `LoadModule` does not update the synthetic `DEBUG_EVENT` before invoking the
   x64dbg callback.
5. Normal Run does not reliably transition to `DEBUG_STATUS_GO*` after the
   callback releases `WAITID_RUN`.
6. `SetNextDbgContinueStatus` only logs and does not control DbgEng exception
   disposition.
7. `debugStep` obtains a process ID where it appears to require a thread ID.
8. Register reads only work for DbgEng's current thread.
9. Register cache `Flush()` does not write changed values.
10. Many unsupported APIs execute `__debugbreak()` instead of failing cleanly.

These issues should be fixed before broad API migration because they affect the
basic event loop regardless of how memory/thread helpers are exposed.

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

### Tier 2: specialized features

Candidates to add only when their x64dbg caller is migrated:

- Process and thread information-class queries currently using `NtQuery*`.
- Mapped/module/image filename queries.
- Remote-handle duplication and object-name/type queries.
- Process-token operations.
- Minidump generation.
- Stack walking and symbol operations if native handles cannot remain a valid
  transitional path.

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

## Graceful failure contract

Every canonical API must be callable in every selectable engine. Unsupported
operations must return a documented value and set `LastError` where the return
shape follows Win32.

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
   wrappers to GleeBug, and add the currently missing exports to StaticEngine.
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
- `DbgEng/TitanEngine.dll`

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

Do not initially replace `fdProcessInfo`, `hActiveThread`, `THREADINFO.Handle`,
Titan callback structures, or bridge/plugin handle APIs. They remain the
compatibility representation for this milestone.

## Validation milestones

### Milestone A: event loop

With DbgEng on x64:

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
- Allocate, protect, and free target memory, or report unsupported without
  destabilizing the session.

### Milestone D: cross-engine compatibility

Run applicable `src/tests/run.py` suites for:

- TitanEngine x64 and x32.
- GleeBug x64 and x32.
- StaticEngine tests.
- DbgEng x64.

Add DbgEng to the test runner's engine mapping. Features not yet implemented by
DbgEng should have explicit expected-unsupported tests rather than being
silently skipped.

At minimum, run launch/break/run/step/memory/register/cleanup smoke tests against
both TitanEngine and GleeBug after each canonical ABI change.

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
7. **Breakpoint parity:** software/step stabilization, then memory/hardware
   breakpoints.
8. **Specialized APIs:** handles, tokens, minidump, symbols/stacks only as needed
   for the chosen semi-parity target.

Each patch must keep standard TitanEngine and GleeBug usable. A feature may be
unsupported in DbgEng, but selecting DbgEng must never cause a missing import,
unhandled exception, or deliberate debug break.

## Definition of the current goal being complete

The compatibility-first goal is complete when:

- DbgEng can launch and debug a live x64 process through normal x64dbg UI and
  command flows.
- Core memory/register/thread/module views are useful.
- Run, stop, software breakpoints, and stepping work.
- Common mutation commands either work or fail cleanly.
- No target opaque handle is passed directly to Win32 from a migrated x64dbg
  path.
- TitanEngine and GleeBug still pass their applicable regression suites.
- All selectable engine DLLs conform to the canonical x64dbg TitanEngine ABI.

Minidump and TTD support are explicitly outside this immediate milestone, but
the handle registry and Titan API routing must not preclude them.
