# DbgEng minidump and TTD replay implementation plan

## Goal

Add read-only/replay-oriented DbgEng sessions to x64dbg and x32dbg for:

1. user-mode minidumps (`.dmp`/`.mdmp`) as immutable snapshots; and
2. Time Travel Debugging traces (`.run`) as navigable replay timelines.

The work must reuse the live DbgEng adapter's event translation, context,
memory, module, opaque-handle, callback, and teardown boundaries without
weakening the now-green live-debugging gate.

"Replay only" is a product boundary:

- Do not add TTD recording, process launching under the TTD recorder, trace
  packaging, or trace trimming.
- Do not add minidump creation as part of this phase. x64dbg's existing
  `minidump` command for a live target is independent and remains unchanged.
- Do not pretend that replay targets are mutable live processes.
- Remote DbgEng, process servers, kernel dumps, kernel debugging, and general
  multi-process debugging remain out of scope.

## Required end state

A user can open a supported minidump or TTD trace from the GUI, command line,
drag-and-drop, or recent-files list and receive the normal x64dbg memory,
disassembly, register, stack, thread, module, symbol, expression, and database
experience appropriate to the artifact.

For TTD, forward and reverse replay, stepping, exact position queries/seeks,
logical breakpoints, thread changes, module timeline changes, and repeated
open/close cycles must work deterministically. For minidumps, unsupported
execution and mutation commands must be visibly disabled or fail normally,
without attempting Win32 target operations.

Completion is verified by architecture-matched `headless.exe` suites, replay
fixtures with known expected state, lifecycle stress, ABI/export checks, and a
full rerun of the live DbgEng and cross-engine gates.

## Preconditions and accepted live baseline

The preceding live plan is `docs/agents/dbgeng-plan.md`. Its accepted baseline
includes:

- x64 and x32 DbgEng launch/attach sessions;
- deterministic event translation and continuation;
- scalar, x87, XMM, YMM, and capability-gated AVX-512 contexts;
- software, hardware/data, and page/range breakpoints;
- filtered `MemoryReadSafe` and unfiltered `MemoryReadUnsafe` semantics;
- repeatable sessions and an authoritative process/thread handle registry;
- 55-export canonical ABI conformance across TitanEngine, GleeBug,
  StaticEngine, and DbgEng; and
- `scripts/run_dbgeng_live_gate.py` as the preserved live regression gate.

Replay changes must not alter live behavior merely to make a dump or trace
appear live.

## Current implementation status

The minidump implementation and the first complete TTD replay slice are implemented:

- `replay_probe` opens and inventories x64/x86 user minidumps directly through
  the deployed DbgEng interfaces.
- `shim_replay_probe` validates the TitanEngine-compatible minidump boundary,
  including session capabilities, opaque handles, contexts, PEB/TEB, memory
  reads/queries, immutable writes, and teardown.
- The canonical ABI now has 64 exports. Session, path, exact-position,
  directional run, and directional step APIs are implemented or explicitly
  unsupported across every selectable engine.
- DbgEng uses generation-tagged synthetic process/thread handles for minidumps;
  no native process exists and mutation/control capabilities are absent.
- `initreplay` works through commands, command-line startup, GUI File/Open,
  recent files, and drag-and-drop extension dispatch.
- Minidump bootstrap synthesizes process, thread, module, stored-exception, and
  system-breakpoint state in deterministic order.
- DbgEng-backed memory maps, captured/partial reads, scalar and available
  extended contexts, thread selection, module paths, and repeated teardown are
  active in x64dbg and x32dbg.
- `src/tests/replay_minidump` generates a deterministic full-memory fixture with
  worker threads and a stored access violation. Its DbgEng x64 and x32 gates
  each pass 11 assertions across two sessions in one headless process.
- TTD Phase 0 proved that `.run` files are opened through the typed TTD replay
  engine API rather than `IDebugClient::OpenDumpFileWide`. The adapter loads
  architecture-matched `TTDReplay.dll`/`TTDReplayCPU.dll` and supports x64 and
  x86 replay ABIs, including their different x86 structure-return conventions.
  The x86 call/return callback uses `__fastcall` with a pointer-sized callback
  value; this is required when engine-owned step-over observes a recorded call.
- TTD sessions provide synthetic immutable identities, contexts, PEB/TEB,
  memory reads/maps, module paths, exact seeks, forward/reverse stepping,
  forward/reverse runs, trace exceptions, timeline state diffs, and logical
  code/data breakpoints without patching trace memory. Reverse watchpoint runs
  use a finite replay bound because this runtime treats an all-ones reverse
  count as zero work; GUI Pause and Stop call the cursor interruption API so a
  long replay cannot strand teardown or leave the next open `ERROR_BUSY`.
  Recorded process exit is published as a mandatory synthetic replay boundary:
  the GUI pauses and retains cursor, memory, register, and timeline state until
  the user seeks/runs backward or explicitly stops the session. The cursor is
  parked at the final executable instruction before exit because the runtime's
  post-exit position exposes only a sparse stack. Active NT_TIB stack/TEB ranges
  are synthesized into the replay memory map, and partial TTD memory ranges are
  accumulated without claiming unread bytes as valid. The GUI queries one
  bridge-level `DbgCanReplayBackwards()` boolean (no TitanEngine enums) and
  conditionally exposes `DebugRunBackward` and `DebugStepIntoBackward` actions
  in backward/forward order in the Debug menu and toolbar. Forward and reverse
  single-step ignore TTD's stale zero-step re-notification of an execute
  watchpoint at the current cursor, so stepping from a reverse-hit logical code
  breakpoint advances one recorded execution instead of resuming to the next
  exception. Forward step-over always follows x64dbg's ordinary
  `StepOverWrapper` -> TitanEngine `StepOver` path. The DbgEng adapter detects
  TTD calls through the replay engine's call/return callback and owns the
  current-thread one-shot return watchpoint internally; cursor-global callbacks
  ignore matching executions from peer threads, while ordinary user
  breakpoints remain process-global. No replay-only breakpoint flag is exposed
  through the canonical TitanEngine ABI. Once a forward run publishes the retained
  pseudo-exit boundary,
  ordinary forward step cannot cross into TTD's sparse raw post-exit cursor;
  reverse step and a subsequent forward step back to the boundary remain valid.
- `src/tests/replay_ttd` supplies a deterministic target with three worker
  threads, repeated known functions, known memory writes, helper-DLL
  load/unload, and a handled `0xE0424242` exception. The x64 and x32 DbgEng
  gates each pass 11 assertions. The transition coverage includes first-position
  rejection, rapid alternating forward/reverse steps, stepping onto and away
  from persistent code and data breakpoints in both directions, forward and
  reverse run round trips through the handled exception, current-thread
  step-over at the shared loader initialization call, step-over after reverse
  run, switching directly between step and run modes, long reverse execution to
  the early image entry point, pseudo-exit boundary navigation, explicit replay
  interruption, and twenty sessions in one headless process.
- The 47-test selected live DbgEng matrix passes on both x64 and x32 after the
  TTD changes. TitanEngine/GleeBug x64/x32 memory and multi-session smoke tests
  also pass.

Both deterministic fixtures are recorded against their final target/helper
binaries. The manifest records trace, target, and helper hashes plus exact
extent, first-milestone, and handled-exception positions for each architecture.

## Session model

### Explicit session kinds

Introduce an engine-visible session classification rather than inferring
semantics from null native handles:

```cpp
enum TitanSessionKind
{
    UE_SESSION_NONE,
    UE_SESSION_LIVE,
    UE_SESSION_MINIDUMP,
    UE_SESSION_TTD,
};
```

Expose capabilities separately from the kind. Proposed capability bits include:

- memory read and query;
- register/context read;
- forward run/step;
- reverse run/step;
- exact replay seek;
- logical code/data breakpoint evaluation;
- memory write overlay;
- register write overlay;
- live process/thread control;
- native process/thread handles;
- exception continuation; and
- timeline module/thread updates.

x64dbg must branch on capabilities, not on engine number, extension, or guessed
handle values. A minidump and TTD trace are both non-live but have different
execution capabilities.

### Semantics matrix

| Operation | Live | Minidump | TTD replay |
|---|---|---|---|
| Memory/context read | yes | yes, where captured | yes, at cursor |
| Run/step forward | yes | no | yes |
| Run/step reverse | no | no | yes |
| Exact position seek | no | no | yes |
| Process/thread control | yes | no | no |
| Target memory/register mutation | yes | no by default | no by default |
| Software/data breakpoints | live breakpoints | analytical only | logical replay breakpoints |
| Exception continuation | yes | no | replay navigation only |
| Native target handles | live registry entries | never required | never required |

DbgEng-supported overlays may be considered later, but immutable artifact state
is the default for this milestone. No command may silently mutate only x64dbg's
cache while presenting the result as trace state.

## Phase 0: capability and deployment spike

Do not freeze a TTD-specific ABI before proving the deployed DbgEng stack.
Create a small standalone probe in `../x64dbg-dbgeng` that uses the same
`IDebugClient5` baseline and runtime bundle as the shim.

The probe must establish, for x64 and x86 separately where available:

1. Whether `IDebugClient4::OpenDumpFileWide` opens ordinary user minidumps.
2. Whether the same API opens `.run` traces and which `GetDebuggeeType`
   class/qualifier identifies them (expected qualifier candidates include
   `DEBUG_DUMP_TRACE_LOG`; do not assume this without evidence).
3. Which additional replay components are loaded, including
   `TTDReplay.dll`, `TTDReplayCPU.dll`, extensions, and Data Model providers.
4. Whether forward and reverse statuses work through
   `DEBUG_STATUS_GO`, `DEBUG_STATUS_STEP_INTO`, `DEBUG_STATUS_STEP_OVER`,
   `DEBUG_STATUS_REVERSE_GO`, `DEBUG_STATUS_REVERSE_STEP_INTO`, and
   `DEBUG_STATUS_REVERSE_STEP_OVER`.
5. How exact TTD positions are read and changed. Prefer a typed DbgEng/Data
   Model interface. Do not build the product around parsing localized WinDbg
   console output. If an extension command is the only available mechanism,
   isolate it behind one adapter method and prove a machine-readable round trip.
6. Event callback behavior for open, seek, forward/reverse step, breakpoint,
   first/last position, and session close.
7. Whether memory/register writes are overlays, cursor-local, persistent across
   seek, or rejected. The product remains immutable regardless unless a later
   explicit feature changes the policy.
8. TTD architecture support and the exact error returned for an incompatible
   host/trace combination.

Preserve probe logs with runtime file versions, loaded module paths,
`GetDebuggeeType`, initial event information, initial/final positions, and every
HRESULT. Turn successful probe operations into adapter-level tests before
removing or retiring the probe.

### Runtime packaging rule

The x64 and x32 replay bundles must be version-coherent. At minimum validate:

- `dbgeng.dll`;
- `dbghelp.dll`;
- `dbgcore.dll`;
- `dbgmodel.dll`;
- required TTD replay engine/CPU DLLs; and
- any required extension or Data Model provider.

Validate missing-component and version-mismatch failures with precise runtime
diagnostics. Minidump replay must continue to work when optional TTD components
are absent.

## Phase 1: canonical replay ABI

Add narrow APIs to `src/dbg/TitanEngine/TitanEngine.h` first, then update the
canonical `.def`, import libraries, native TitanEngine, GleeBug, StaticEngine,
and DbgEng atomically.

The exact names can change during Phase 0, but the required concepts are:

```cpp
struct TITAN_SESSION_INFO
{
    TitanSessionKind kind;
    ULONG64 capabilities;
    DWORD machineType;
    DWORD processId;
    DWORD threadId;
};

struct TITAN_REPLAY_POSITION
{
    ULONG64 sequence;
    ULONG64 steps;
};

PROCESS_INFORMATION* InitReplayW(
    const wchar_t* artifactPath,
    TitanSessionKind expectedKind);

bool GetSessionInfo(TITAN_SESSION_INFO* info);
bool ReplayGetPosition(TITAN_REPLAY_POSITION* position);
bool ReplayGetExtent(
    TITAN_REPLAY_POSITION* first,
    TITAN_REPLAY_POSITION* last);
bool ReplaySetPosition(const TITAN_REPLAY_POSITION* position);
bool ReplayRun(bool reverse);
bool ReplayStep(bool reverse, bool stepOver, TITANCBSTEP callback);
```

Rules:

- `InitReplayW` must reject live executables and reject a kind mismatch.
- Detection results come from DbgEng after opening, not only from extensions.
- Position structures use stable numeric fields only after Phase 0 proves the
  mapping. Otherwise use an opaque versioned byte/string representation with
  strict round-trip validation.
- TitanEngine and GleeBug return `ERROR_NOT_SUPPORTED` for replay initialization
  and navigation while preserving all live behavior.
- StaticEngine may identify itself as static but does not impersonate a dump.
- Every output is zero-initialized on failure.
- Export and ABI-signature validation must cover the enlarged surface in both
  architectures before x64dbg imports any new symbol.

Do not add a catch-all DbgEng command execution API.

## Phase 2: opaque replay handles and identity

### Registry entries

Extend the DbgEng registry so a process/thread token can exist without a native
Windows handle:

```cpp
struct TitanHandleEntry
{
    TitanHandleType type;
    ULONG dbgengId;
    std::optional<DWORD> systemId;
    HANDLE nativeHandle;       // null for replay
    bool callerOwned;
    uint64_t sessionGeneration;
};
```

Public `HANDLE` values are opaque tokens. Replay tokens must not be values that
could plausibly be accepted by Win32. Validate type and session generation on
every operation so a token from a closed trace cannot alias a later session.

Required behavior:

- Return independently owned initialization process/thread tokens from
  `InitReplayW`.
- Create distinct event/session tokens during synthetic bootstrap.
- `TitanOpenProcess`/`TitanOpenThread` may return replay tokens only for IDs
  present at the current cursor.
- `TitanCloseHandle` releases caller-owned tokens without calling
  `CloseHandle` when no native handle exists.
- Cursor changes update ID/index mappings but do not invalidate unrelated
  caller-owned tokens until their represented entity leaves the timeline or
  the session closes. Operations on inactive entities fail predictably.
- Teardown invalidates every generation entry idempotently.

### Eliminate automatic Win32 use

Before a replay session reaches normal x64dbg initialization, capability-gate
all automatic operations that assume a live process, including:

- process token opening;
- `CheckRemoteDebuggerPresent` and WOW64/native-handle probes;
- process/thread terminate, suspend, resume, priority, timing, and cycle APIs;
- remote thread creation and handle duplication;
- `NtQueryInformationProcess/Thread` on public engine handles;
- working-set and mapped-file queries that bypass the engine; and
- minidump generation from the replay target.

A disabled feature must not receive an opaque handle and merely rely on Win32
to reject it.

## Phase 3: file detection and user entry points

### Artifact detection

Add an explicit artifact classifier before `GetPeArch`:

- PE executable/DLL;
- user minidump, using the minidump signature and system-info stream where
  practical;
- TTD trace candidate;
- unsupported/kernel dump; and
- unknown file.

Extensions are a hint, not authority. DbgEng performs the final replay-kind and
machine validation. Kernel dumps must be rejected with a clear message rather
than entering a partially working user-mode session.

### Commands and GUI

Add a dedicated command such as:

```text
initreplay "artifact.dmp"
initreplay "trace.run"
```

`init` may auto-dispatch recognized replay artifacts after the dedicated path
is tested, but command scripts must retain an unambiguous replay entry point.

Update:

- File/Open filters to include minidumps and TTD traces;
- drag-and-drop and recent-file restart behavior;
- architecture handoff between x32dbg and x64dbg;
- window title/session status to show `Minidump` or `TTD Replay`;
- error reporting for missing replay dependencies, corrupt artifacts,
  unsupported architecture, and kernel dumps; and
- command enablement based on session capabilities.

Arguments and working-directory fields have no replay meaning and must not be
silently passed as process-launch options.

### Database identity

Use the artifact path and replay kind for database identity so a dump/trace does
not overwrite the database for its original executable. Preserve module-relative
labels/comments where possible while keeping trace-specific timeline metadata
in the artifact database.

## Phase 4: replay bootstrap and event synthesis

`OpenDumpFileWide` does not guarantee the live `CREATE_PROCESS_DEBUG_EVENT`
sequence x64dbg currently expects. Build one deterministic bootstrap path from
DbgEng state after the initial `WaitForEvent`.

The adapter must query and cache:

- debuggee class/qualifier and effective machine;
- selected process and thread;
- all available processes (reject unsupported general multi-process traces);
- thread IDs, TEBs where captured, and current-thread selection;
- executable identity, PEB where captured, image base, and entry/current IP;
- loaded and unloaded module state available at the cursor;
- initial exception/last event information; and
- first/current/last TTD positions.

Synthesize x64dbg-facing callbacks in a documented order:

1. `UE_CH_DEBUGEVENT` + create-process;
2. create-thread for additional threads;
3. load-module callbacks in deterministic base order;
4. initial exception information when meaningful; and
5. one session-ready/system-breakpoint callback.

Never expose borrowed callback buffers after the callback returns. Do not run
x64dbg callbacks under the DbgEng, queue, registry, module, or position lock.

### Replay-aware image paths

Replay events cannot depend on a valid `hFile`. Add narrow canonical process
image/module path queries and migrate `cbCreateProcess`/`cbLoadDll` fallback
logic to them. DbgEng obtains paths from its module/system interfaces; live
TitanEngine and GleeBug forward to their existing path logic. Missing local
module files must not prevent the module from appearing from captured DbgEng
metadata.

## Phase 5: memory and module state

### Reads

Implement replay memory through DbgEng data spaces at the current cursor:

- `MemoryReadUnsafe` returns captured bytes without adapter breakpoint
  substitution.
- `MemoryReadSafe` applies only adapter-owned logical filtering required by the
  canonical contract.
- Partial/captured-memory holes return exact byte counts and
  `ERROR_PARTIAL_COPY` rather than fabricated zero-filled success.
- Reads from GUI/analysis worker threads use a stable cursor snapshot and cannot
  race a seek or teardown.

### Queries and map construction

Implement `MemoryQuerySafe` from DbgEng virtual-memory information and convert
64-bit descriptions safely for x32. Build the x64dbg memory map without
`VirtualQueryEx`, working-set APIs, or native process handles.

Minidumps may omit regions/pages. TTD memory availability may change with the
cursor. Cache keys therefore include session generation and, for TTD, cursor
generation. A successful seek invalidates memory, module, stack, expression,
and disassembly caches before notifying the GUI.

### Mutation

For this phase:

- `MemoryWriteSafe`, allocation/free/protect, `Fill`, remote-thread creation,
  and process/thread mutation return a normal unsupported result in replay.
- x64dbg disables commands and menu actions when the capability is absent.
- Breakpoint installation must not use target memory writes.

## Phase 6: replay contexts, stacks, threads, and modules

### Contexts

Remove the live-only `GetThreadContext`/XSTATE path from replay branches.
Populate `TITAN_ENGINE_CONTEXT_t` and AVX/AVX-512 structures from DbgEng
register values for the selected replay thread.

Requirements:

- current and non-current thread scalar/control contexts;
- segment, debug, x87/MMX, XMM, YMM, and available AVX-512 state;
- clear capability/error reporting for state absent from the artifact;
- no stale register cache after thread switch or TTD seek; and
- one bulk context fetch for hot consumers rather than repeated cross-engine
  scalar lookups.

Register setters are disabled unless a separately tested replay-overlay
capability is deliberately enabled.

### Stack walking

Use the already migrated engine context and memory callbacks. Ensure DbgHelp
never receives replay tokens as native process/thread handles. If DbgHelp needs
identity values, supply documented local/synthetic callback context rather than
casting tokens into Win32.

### Timeline diffs

After every successful TTD movement:

1. query current process/thread/module state;
2. diff it against the previous cursor generation;
3. remove departed thread/module registry and cache entries;
4. add newly visible entries;
5. select the DbgEng event thread/current thread correctly; and
6. publish callbacks and one GUI refresh only after internal state is coherent.

Seeking backward must invert module/thread lifecycle changes correctly; do not
assume IDs or bases are monotonic.

## Phase 7: execution, reverse execution, positions, and breakpoints

### Execution mapping

Map normal x64dbg operations only when supported:

- Run -> forward replay go.
- Step into/over -> forward replay step.
- Stop -> stop/close replay session, not terminate process.
- Pause -> interrupt replay movement, not `DebugBreakProcess`.

Add explicit replay commands:

```text
ttdposition
ttdseek sequence:steps
ttdfirst
ttdlast
ttdstepback
ttdstepoverback
ttdrunback
```

Names and position syntax are finalized only after Phase 0. Commands must be
scriptable and return deterministic errors at timeline boundaries.

### Logical breakpoints

Do not patch trace memory or use page guards.

- TTD software breakpoints use DbgEng logical code breakpoints.
- Data/memory replay breakpoints use native DbgEng replay facilities only where
  their access/range semantics are proven. Exact-range filtering remains in the
  adapter when DbgEng reports a coarser hit.
- Minidump breakpoints may be stored as analytical database markers but cannot
  claim runnable breakpoint behavior.
- Existing live DR-slot and page-guard state is separate and always empty in a
  replay session.
- Breakpoint callbacks include the exact replay position and selected thread in
  adapter state before x64dbg is called.

Test breakpoint set/delete/reuse, forward and reverse hits, one-shot behavior,
range false-positive filtering, callback mutation, and seek across a hit.

### Position transaction rule

A seek or step is a transaction:

1. serialize against other movement;
2. request movement from DbgEng;
3. wait for completion/event;
4. verify the resulting position;
5. rebuild cursor-dependent state;
6. release internal locks; and
7. invoke callbacks/GUI refresh.

On failure, retain the last verified cursor and invalidate any partially read
cache. Worker reads must observe either the old or new generation, never a mix.

## Phase 8: teardown, errors, and coexistence

Use one session state machine:

```text
Empty -> Opening -> Bootstrapping -> Paused/Replaying -> Closing -> Empty
```

Cover failure at every transition:

- corrupt/truncated dump;
- corrupt/incomplete TTD trace;
- missing or mismatched replay DLL;
- unsupported architecture or dump class;
- failure before/after the initial event;
- seek/step failure and timeline boundary;
- close during worker reads;
- repeated minidump -> TTD -> live -> minidump sessions; and
- application shutdown with a replay session open.

Replay close uses the correct DbgEng end-session operation; never call
`TerminateProcess`, `DetachProcesses`, or restore live page protections.
Release Data Model objects, callbacks, breakpoints, synthetic handles, module
metadata, position objects, and COM interfaces according to documented owners.

Expected missing-page, unavailable-register, and end-of-timeline conditions are
normal diagnostics, not backend error spam.

## Testing and fixtures

### Minidump fixtures

Create deterministic x64 and x86 user-mode fixtures containing:

- known GPR/flags and XMM/YMM values where available;
- at least three threads with known stacks/TEBs;
- loaded and unloaded test modules;
- readable, reserved, inaccessible, and intentionally omitted memory;
- a known exception and debug string metadata where representable; and
- symbols/PDBs for target-side assertions.

Fixture generation is test infrastructure only and is not a product recording
feature. Record generator source, tool version, dump flags, hashes, and expected
values. Keep fixture size bounded.

### TTD fixture

Use a small deterministic trace with these milestones:

- multiple threads;
- module load/unload;
- repeated execution of a known function;
- memory reads/writes at known positions;
- an exception;
- register values checked before and after a milestone; and
- enough history to test forward/reverse run and exact seek.

If a trace cannot be committed, the gate accepts an explicitly configured
fixture directory, but release qualification must run with the fixture present.
A missing TTD fixture is not a passing TTD gate.

### Headless suites

Add replay support to `src/tests/run.py`, with engine/session selection that is
independent of the live `DebugEngine=3` setting. Required suites include:

#### Minidump

- open/bootstrap/event ordering;
- architecture mismatch and corrupt/kernel-dump rejection;
- process/thread/module enumeration and paths;
- partial memory reads and memory-map holes;
- current/non-current contexts and stacks;
- symbols, expressions, comments, labels, analysis, and database reload;
- mutation/control commands disabled with stable errors; and
- twenty-session open/close stress on x64 and x32.

#### TTD

- open/bootstrap and first/current/last positions;
- exact seek round trips;
- forward run, step-into, step-over;
- reverse run, step-into, step-over;
- first/last boundary behavior;
- current-thread changes and non-current contexts;
- module/thread lifecycle while moving both directions;
- memory/register values at known positions;
- logical code/data/range breakpoint hits in both directions;
- callback mutation and one-shot cleanup;
- stop during movement and repeated sessions; and
- x64/x32 runs only where Phase 0 proves architecture support.

#### Regression and performance

Every replay gate also runs:

- `scripts/check_titanengine_exports.py` for all adapters/architectures;
- ABI-signature conformance;
- `scripts/run_dbgeng_live_gate.py` with its required repeats;
- TitanEngine and GleeBug live smoke suites;
- `git diff --check` and Python compilation checks;
- clean x64/x32 builds and dependency deployment checks; and
- handle/COM object count stress.

Track opening, first memory-map construction, exact seek, single-step, bulk
context read, and teardown latency. Reject designs that perform one DbgEng/COM
round trip per register, byte, memory page, or module when a bulk operation is
available. Keep explicit budgets in the fixture manifest once the Phase 0 probe
establishes a baseline.

## Patch sequence

1. **Replay probe and dependency report**: prove minidump/TTD opening,
   identification, movement, positions, and runtime dependencies.
2. **Session/capability ABI**: add kind/capability queries to all adapters with
   export/signature checks.
3. **Opaque replay registry**: generation-checked process/thread tokens and
   replay-safe close/open semantics.
4. **Minidump bootstrap**: open, classify, synthesize process/thread/module
   events, and close cleanly.
5. **Replay-safe paths and startup**: image/module path APIs, artifact
   detection, `initreplay`, GUI filters, and database identity.
6. **Minidump memory/context parity**: memory map, partial reads, contexts,
   stacks, symbols, and disabled mutation.
7. **Minidump gate**: repeated x64/x32 headless and lifecycle tests.
8. **TTD open and position ABI**: typed current/extent/seek with cursor
   generations.
9. **TTD forward/reverse execution**: run/step/pause/stop and boundary errors.
10. **TTD timeline state**: thread/module diffs, cache invalidation, contexts,
    stacks, and memory at cursor.
11. **TTD logical breakpoints**: forward/reverse code and proven data/range
    semantics without memory patching.
12. **TTD gate and live regression**: repeated fixture matrix, dependency
    validation, leak stress, and full live/cross-engine gates.

Keep each patch buildable. New canonical exports are atomic across all engines.
Minidump completion does not wait for TTD, but no TTD shortcut may regress the
completed minidump or live paths.

## Definition of completion

Replay support is complete only when:

- minidumps and supported TTD traces open through normal x64dbg entry points;
- the artifact kind and architecture are verified by content/DbgEng;
- no replay token is passed to Win32 as a native process/thread handle;
- memory, modules, symbols, threads, contexts, and stacks reflect the snapshot
  or exact TTD cursor with correct partial-data behavior;
- minidump execution/mutation is disabled without crashes or fake success;
- TTD moves forward and backward, seeks exactly, and reports boundaries;
- TTD cursor changes transactionally invalidate and rebuild dependent state;
- replay breakpoints are logical and never patch trace memory or use page
  guards;
- callbacks execute outside internal locks and carry coherent process/thread/
  position state;
- replay teardown is leak-free and repeatable across mixed live/dump/TTD
  sessions;
- all replay headless suites pass repeatedly with preserved fixtures/artifacts;
- all eight adapters pass canonical export/signature checks; and
- the existing live DbgEng, TitanEngine, and GleeBug gates remain green.

Recording support, kernel dumps, remote debugging, mutable replay overlays, and
general multi-process replay require separate plans after this milestone.
