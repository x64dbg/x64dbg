# Live process/thread access audit for DbgEng, dumps, and TTD

This audit describes the `dbgeng` branch at source HEAD
`e16576ce8af951de6763e853c2f82ea7a68f5d80`. It is intended to identify where
x64dbg currently assumes a live Win32 process/thread and where an engine-neutral
abstraction can be inserted.

## Reproducing the evidence

Run from the x64dbg repository root:

```powershell
uv run scripts/live_target_api_audit.py
```

The output is written to `build/live-target-api-audit/`:

- `summary.md`: every API occurrence, source line, enclosing function, and dispatch form.
- `inventory.json`: imports, source sites, definitions, categories, and POC comparison.
- `callgraph.json`: normalized functions, reverse caller edges, and shortest root-to-API stacks.

The script fails if `build/compile_commands.json` is absent or has no `BUILD_DBG`
translation units. It launches clangd with:

```text
--compile-commands-dir=<repository>/build
--background-index
```

It also enables clangd's `textDocument/inactiveRegions` extension. This matters:
without it, x86-only branches such as the `NtWow64*` paths in
`commands/cmd-misc.cpp` appear to be x64 call sites. On this run clangd used 119
`BUILD_DBG` translation units and supplied inactive-region data for 124 files.
Comments and string literals are masked before candidate extraction.

MultiLSPy 0.0.15 on PyPI does not yet contain its C/C++ adapter. The uv script
therefore pins upstream MultiLSPy commit
`b9a0ef1bc221e5575787be80493059b01f03c37a`, which contains the clangd adapter,
and overrides its launch command so that the build compile database is explicit.
It does not let clangd infer stock commands.

`bin/x64/x64dbg.dll` is only an import seed and cross-check. Source and clangd are
the authority for call sites. The binary used in this run has SHA-256
`05eecad66335b3d0573b9636990707b4bf25c2f99dbd0355e6311f883a1b3076`.
Its 553 imports across 29 modules yielded 91 process/thread-looking seed imports.

## Inventory result

The x64 configuration contains **575 catalogued occurrences in 237 functions**:

| Surface | API symbols | Occurrences | Meaning |
|---|---:|---:|---|
| TitanEngine boundary | 37 | 329 | Existing partial engine abstraction |
| Windows/NT/helper APIs | 76 | 246 | Calls outside TitanEngine |
| Handle/window context-sensitive subset | 19 | 115 | Includes target and non-target uses; inspect arguments |
| Unambiguously target-oriented Windows subset | 57 | 131 | Direct migration work outside TitanEngine |

The call graph contains 2,409 source functions and 3,984 reverse caller edges.
Of those edges, 3,194 are ordinary calls and 790 are references/callback
registrations. The latter are labelled `reference`, not represented as direct
calls.

Counts are call-expression occurrences. Two calls on one source line count as
two. For dynamically resolved APIs, the resolution and known pointer invocation
are separately labelled.

### Memory, process, and handle APIs

| Category | APIs and x64 source occurrence counts |
|---|---|
| Memory read | `MemoryReadSafe` (4), `NtReadVirtualMemory` (1), `ReadProcessMemory` (2) |
| Memory write | `MemoryWriteSafe` (1), `NtWriteVirtualMemory` (1) |
| Memory query | `GetMappedFileNameW` (2), dynamic `QueryWorkingSetEx` (resolution + call), `VirtualQueryEx` (7) |
| Memory allocation/protection | `VirtualAllocEx` (1), `VirtualFreeEx` (2), `VirtualProtectEx` (1) |
| Process open/query | `OpenProcess` (1), `TitanOpenProcess` (3), `CheckRemoteDebuggerPresent` (1), `GetModuleFileNameExW` (4), `GetPEBLocation` (5), `GetProcessId` (2), `GetProcessImageFileNameW` (2), `IsWow64Process` (7), `NtQueryInformationProcess` (3) |
| Process control | `DebugBreakProcess` (1), `TerminateProcess` (1) |
| Remote handles | `DuplicateHandle` (4), `NtQueryObject` (5) |
| Debuggee token | `OpenProcessToken` (1), `GetTokenInformation` (2), `LookupPrivilegeValueW` (3), `AdjustTokenPrivileges` (2) |
| System process data | `NtQuerySystemInformation` (4), `GetTcpTable2` (2), `GetTcp6Table2` (2) |
| Process/thread discovery | `CreateToolhelp32Snapshot` (2), `Process32First/Next` (1 each), `Thread32First/Next` (1 each), `GetWindowThreadProcessId` (5) |
| Dump | `MiniDumpWriteDump` (1) |

The context-sensitive handle list is `CloseHandle` (65),
`GetHandleInformation` (1), `WaitForSingleObject` (21), and
`WaitForMultipleObjects` (1). Many operate on debugger worker events, files, or
semaphores rather than target objects. They are retained because the target
handle ownership sites—especially `debugLoopFunction`, `_dbg_sendmessage`,
`HandlesGetName`, and remote-thread creation—must be separated before fake or
absent handles are safe.

### Thread APIs

| Category | APIs and x64 source occurrence counts |
|---|---|
| Open/identity | `OpenThread` (2), `TitanOpenThread` (1), `GetThreadId` (4), `GetProcessIdOfThread` (2) |
| Metadata | `GetThreadDescription` (resolution + call at two sites), `GetThreadPriority` (1), `GetThreadTimes` (2), `QueryThreadCycleTime` (1), `NtQueryInformationThread` (2), `GetTEBLocation` (10) |
| Context | `GetThreadContext` (4), `GetFullContextDataEx` (4), `GetContextDataEx` (115), `GetAVXContext` (3), `GetAVX512Context` (12), `SetFullContextDataEx` (3), `SetContextDataEx` (35), `SetAVXContext` (2), `SetAVX512Context` (8) |
| Control | `NtSuspendThread` (1), `SuspendThread` (5), `ResumeThread` (12), `SetThreadPriority` (1), `TerminateThread` (3), `PostThreadMessageA` (1) |
| Creation | `CreateRemoteThread` (1) |

### Debug session, execution, and breakpoint APIs

| Category | APIs and x64 source occurrence counts |
|---|---|
| Session | `InitDebugW` (2), `AttachDebugger` (1), `DetachDebuggerEx` (1), `DebugLoop` (1), `StopDebug` (2), `IsFileBeingDebugged` (1) |
| Event compatibility | `GetDebugData` (32), `SetCustomHandler` (10), `SetNextDbgContinueStatus` (1), `NtFsControlFile` on `LOAD_DLL_DEBUG_INFO.hFile` (1) |
| Execution | `StepInto` (1), `StepOver` (1) |
| Software breakpoints | `SetBPX` (12), `DeleteBPX` (11), `IsBPXEnabled` (4), `SetBPXOptions` (4) |
| Memory breakpoints | `SetMemoryBPXEx` (7), `RemoveMemoryBPX` (9) |
| Hardware breakpoints | `SetHardwareBreakPoint` (4), `DeleteHardwareBreakPoint` (8), `GetUnusedHardwareBreakPointRegister` (4) |
| General cleanup | `RemoveAllBreakPoints` (1) |
| Engine configuration | `SetEngineVariable` (5), `EngineCheckStructAlignment` (1) |

### Stack and symbol APIs

`StackWalk64`, `SymGetModuleInfoW64`, `SymGetSearchPathW`, `SymLoadModuleExW`,
`SymSetSearchPathW`, and `SymUnloadModule64` each have one direct site, all in
`dbghelp_safe.cpp`. Although these are already serialized behind `Safe*`
wrappers, the wrappers still take native process/thread handles. DbgEng can own
both stack walking and symbols for live, dump, and TTD targets, so this is a
separate provider boundary rather than merely a locking wrapper.

### Live host/window APIs

The source has 27 uses of live USER32 window state: `EnumWindows`,
`EnumChildWindows`, `GetClassLongPtrA/W`, `GetClassNameW`, `GetForegroundWindow`,
`GetParent`, `GetWindow`, `GetWindowRect`, `GetWindowTextW`, `IsWindow`,
`IsWindowEnabled`, `IsWindowUnicode`, `IsWindowVisible`, and `EnableWindow`.
Some inspect or control debuggee windows, while others are debugger UI state.
These should be an optional **live host service**, not methods every dump/TTD
backend must fake. TCP-owner and token operations belong in the same live-only
layer.

## Where the current abstraction already helps

### Memory

Most consumers already call `MemRead`, `MemReadUnsafe`, `MemWrite`,
`MemAllocRemote`, `MemFreeRemote`, `MemGetProtect`, or `MemSetProtect` from
`memory.h`. This is the best first cut because a large caller fan-out converges
on a small number of functions:

```text
_dbg_memread / scripts / expressions / analysis
  -> MemRead
  -> MemoryReadSafePage
  -> TitanEngine!MemoryReadSafe
```

Representative graph roots for `MemAllocRemote` are:

```text
Script::Memory::RemoteAlloc -> MemAllocRemote -> VirtualAllocEx
_dbg_dbginit -> registercommands -> cbDebugAlloc -> MemAllocRemote -> VirtualAllocEx
```

The important bypasses are:

- `commands/cmd-memory-operations.cpp:76`: direct `VirtualFreeEx`.
- `commands/cmd-undocumented.cpp:455`: direct `ReadProcessMemory`.
- `commands/cmd-misc.cpp:148,170`: direct `NtReadVirtualMemory` / `NtWriteVirtualMemory` for PEB hiding.
- `memory.cpp`: seven `VirtualQueryEx`-class query paths and direct alloc/free/protect.
- `module.cpp:1083`: direct `VirtualQueryEx` while deriving module regions.

These are few enough to migrate early.

### Thread records

`thread.cpp` already owns the thread map and provides `ThreadGetHandle`,
`ThreadGetId`, `ThreadGetList`, TEB lookup, suspend-count, priority, and timing
helpers. It is a useful façade, but its public API and `THREADINFO` still expose
`HANDLE`, and several callers bypass it.

The context surface is more fragmented. Titan context APIs are called 186 times,
mostly from `value.cpp`, register/script APIs, debugger callbacks, and general
purpose commands. `GetThreadContext` is also called directly by stack walking,
minidump creation, and `_dbg_isjumpgoingtoexecute`.

### DbgHelp

`dbghelp_safe.cpp` is already a choke point for symbols and stack walking. Its
interface should change from native handles to target/process/thread keys, or be
replaced by an engine symbol/stack provider.

## Why the TitanEngine ABI is not the final abstraction

TitanEngine is useful as a compatibility adapter, but its present ABI encodes
live Windows assumptions:

1. `PROCESS_INFORMATION*`, `HANDLE`, and `DEBUG_EVENT*` are public currency.
2. Process/thread identity is conflated with native handle identity.
3. Memory only has read/write. Query, enumerate, allocate, free, and protect are
   outside the ABI.
4. Thread metadata, listing, suspend/resume/terminate, and priority are outside
   the ABI.
5. Context APIs use Titan register enums and assume a live thread handle.
6. Events are callback registrations plus a synthetic global `DEBUG_EVENT`.
7. There is no capability model for read-only dumps, TTD timeline navigation,
   reverse execution, absent native handles, or unsupported mutation.
8. There is no explicit snapshot/time-position generation for cache invalidation.

The POC demonstrates the mismatch. It maps DbgEng process/thread indices to
values presented as `HANDLE`, builds fake `PROCESS_INFORMATION` and
`DEBUG_EVENT` structures, and caches PEB/TEB values by those handles. That gets
existing code running but preserves the exact assumption that breaks for dumps
and TTD.

The script's mechanical POC classification finds 43 Titan exports: 13 without
obvious stub markers, 12 partial/TODO or debug-break paths, and 18 obvious
stubs. Important current stubs include memory writes, context writes, memory and
hardware breakpoints, attach/detach, and `TitanOpenProcess`/`TitanOpenThread`.
`MemoryReadSafe` and context reads currently require a paused engine and the
selected DbgEng process/thread; non-current target selection is not implemented.

## Recommended abstraction boundary

Use a versioned engine interface with opaque identities, then keep TitanEngine
as a compatibility shim during migration.

### Core identities

Do not use an OS PID/TID as the sole key. DbgEng has engine IDs and system IDs,
and dump/TTD targets may have historical or synthetic objects.

```cpp
struct TargetProcessKey { uint64_t value; };
struct TargetThreadKey  { uint64_t value; };

struct ProcessIdentity
{
    TargetProcessKey key;
    std::optional<uint32_t> systemId;
};
```

A native handle should be an optional, explicitly borrowed compatibility value:

```cpp
std::optional<BorrowedNativeHandle> nativeProcessHandle(TargetProcessKey);
```

It must be capability-gated and must define ownership. A fake DbgEng value must
never be passed to Win32 merely because its C type is `HANDLE`.

### Suggested provider split

1. **Session/control**: create, attach, detach, stop, wait/events, execution,
   stepping, reverse execution, and timeline seek.
2. **Target state**: current process/thread, process/thread enumeration,
   architecture, PEB/TEB, modules, and state-generation number.
3. **Memory**: read, write, query regions, allocation, free, and protection.
4. **Registers**: complete context and scalar register reads/writes for an
   opaque thread key.
5. **Breakpoints**: code/data breakpoint CRUD and capabilities.
6. **Symbols/stacks**: symbol loading/query and stack enumeration without
   exposing DbgHelp handles.
7. **Live host services**: process picker, windows, TCP ownership, tokens,
   arbitrary remote handles, remote thread creation, and other operations that
   are meaningless for dumps/TTD.

Capabilities should be granular—for example `ReadMemory`, `WriteMemory`,
`QueryMemory`, `AllocateMemory`, `ReadRegisters`, `WriteRegisters`,
`ForwardExecution`, `ReverseExecution`, `NativeHandles`, and `LiveHostState`.
A minidump backend should naturally report read-only state capabilities instead
of returning fake handles or generic failures.

Because the selected engine is a DLL, expose a versioned C ABI/vtable such as
`DebugEngineQueryInterface(version, &api)` and wrap it in C++ inside `dbg`.
Legacy Titan/GleeBug/StaticEngine can initially use an x64dbg-side adapter backed
by the current handles and Titan calls. The DbgEng engine can implement the new
interface directly. Existing Titan exports remain compatibility shims until all
callers move.

## Migration order

1. **Introduce the façade with a native/Titan backend and no behavior change.**
   Centralize current target identity and add a temporary, audited native-handle
   escape hatch.
2. **Move memory first.** Route the existing `Mem*` choke points through the
   memory provider, then remove the direct bypasses listed above. Route memory
   map construction and module region queries through `queryMemory`.
3. **Replace handle-based thread context.** Add key-based complete-context and
   scalar-register APIs. Migrate `value.cpp`, `_dbg_getregdump`, stack walking,
   register scripts, and command callbacks. Support selecting/restoring a
   non-current DbgEng thread inside the backend.
4. **Move thread/process metadata and enumeration.** Change `THREADINFO.Handle`
   and global `hActiveThread` users to opaque keys. Keep system IDs as metadata.
5. **Move stacks, symbols, and modules.** This unlocks the same DbgEng path for
   live, minidump, and TTD targets.
6. **Replace synthetic Win32 debug events.** Publish engine-neutral event
   records. Keep a translation layer only for legacy Titan callbacks.
7. **Move execution and breakpoints.** Add reverse/timeline operations without
   trying to encode them as Titan's forward-only step callbacks.
8. **Quarantine live host operations.** Window, token, TCP, arbitrary-handle,
   remote-thread, and process-picker functionality should check live-host
   capabilities rather than leak into target-state code.
9. **Retain bridge/plugin ABI compatibility deliberately.** APIs such as
   `DbgGetProcessHandle`, `DbgGetThreadHandle`, and `_dbg_getProcessInformation`
   cannot be made engine-neutral internally. Return null/unsupported when no
   native handle exists, document borrowed ownership, and add new key/state APIs
   before deprecating the old ones.

## Representative reverse call chains

The complete graph is in `callgraph.json`. Useful examples are:

```text
_dbg_dbginit
  -> registercommands [callback registration]
  -> cbDebugInit
  -> dbgcreatedebugthread
  -> debugLoopFunction
  -> AttachDebugger / DebugLoop / InitDebugW / SetCustomHandler

_dbg_dbginit
  -> registercommands [callback registration]
  -> cbDebugPause
  -> SuspendThread / ResumeThread / GetThreadId / PostThreadMessageA

_dbg_dbginit
  -> dbgfunctionsinit
  -> stackgetcallstackbythread
  -> SafeStackWalk64
  -> StackWalk64

Script::Memory::SetProtect
  -> MemSetProtect
  -> VirtualProtectEx

_dbg_memread
  -> MemRead
  -> MemoryReadSafePage
  -> TitanEngine!MemoryReadSafe

_dbg_dbginit
  -> registercommands [callback registration]
  -> cbDebugHide
  -> HideDebuggerPebOnly
  -> HideNativePeb / HideWow64Peb32
  -> NtQueryInformationProcess / NtReadVirtualMemory / NtWriteVirtualMemory
```

## Accuracy boundaries

- This is a static source call graph, not a claim about one runtime stack.
- clangd resolves ordinary calls and references; virtual dispatch and arbitrary
  function-pointer propagation remain explicit limitations.
- Callback/reference edges are labelled and must not be read as synchronous calls.
- Context-sensitive handle/window sites intentionally include non-target uses.
- The audit covers the `src/dbg` x64 compile configuration. Other binaries and
  x32-only branches require running against their own compile database and seed binary.
- The binary import cross-check still lists broad candidates that were reviewed
  as non-target operations, including debugger `CreateThread`, current-process
  helpers, local `VirtualAlloc/VirtualFree`, `NtSetInformationFile`, and local
  unwind helpers. They remain visible in `inventory.json` rather than being
  silently discarded.
