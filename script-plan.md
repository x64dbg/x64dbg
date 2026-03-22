# Script engine plan

## Context

There are two related problems in the current scripting/debugger interaction:

1. **Lifecycle detection is too coarse**
   - `DbgIsDebugging()` is currently not a fully truthful primitive during debugger startup/teardown.
   - Issue: https://github.com/x64dbg/x64dbg/issues/3699
   - This makes it unsafe to use as a hard signal for whether a live debug session exists.

2. **Script interruption is too coarse**
   - `bScriptAbort` is currently used for multiple meanings:
     - real abort
     - temporary yield for breakpoint/debug-event handling
     - shutdown/assertion-style cancellation
   - This causes synchronous script execution to fail early, then resume asynchronously later.

The resulting symptom is that some scripts fail with `script_failed` even though the same commands work when run manually.

---

## Key design decisions

### 1. Make `DbgIsDebugging()` truthful

`DbgIsDebugging()` should become a reliable primitive meaning:

- there is a live, usable debug session
- code may safely treat the debugger as active

It should not return `true` merely because the debug loop thread has started.

### 2. Detect session ownership by observing transitions, not command names

Do **not** infer script ownership from command text such as `init`, `attach`, `stop`, `detach`, etc.

Instead, for each script command:

- sample `DbgIsDebugging()` before execution
- execute the command
- wait for run-state changes as needed
- sample `DbgIsDebugging()` after execution

Then infer lifecycle transitions from reality:

- `false -> true`: command started a session
- `true -> false`: command ended a session

This automatically supports:

- plugin commands that initialize/debug a process
- aliases/custom commands
- future command changes

### 3. Replace `bScriptAbort` with an interrupt reason model

Current `bScriptAbort` conflates multiple situations.

Replace it with something like:

```cpp
enum class SCRIPTINTERRUPTREASON
{
    None = 0,
    YieldDebugEvent,
    AbortUser,
    AbortAssertion,
    AbortShutdown,
};
```

Suggested name:

- `gScriptInterruptReason`

This allows the script engine to distinguish:

- a **real failure/abort**
- a **temporary yield** needed for breakpoint/debug-event handling

---

## High-level behavior goals

### A. Session lifecycle policy

When a script command causes `DbgIsDebugging()` to change:

- if the script caused `false -> true`, mark the session as script-created
- if the script later causes `true -> false`, continuation may be allowed if the session was script-created
- if the script started in an already existing session and that session disappears, pausing/stopping may still be desired depending on final policy

Important: this should be based on observed transitions, not hardcoded command lists.

### B. Yield vs abort

When a debug event interrupts a running script (for example during `run`):

- the script should **yield**
- the synchronous caller should **not** immediately fail
- breakpoint/debug-event handling should run
- if the script was previously running, it should resume in the same logical execution

Only true abort reasons should make `ScriptRunAwait()` / `ScriptExecAwait()` fail.

---

## Current bug mechanism

### `membp/range-execute`

Current flow:

1. script executes `run`
2. script thread waits for the debuggee to pause
3. memory breakpoint hits
4. breakpoint handling calls `ScriptAbortAwait()`
5. this sets `bScriptAbort = true`
6. script loop exits as if aborted
7. `ScriptRunAwait()` returns failure before the script reaches the next line / final `ret`
8. later breakpoint code resumes the script asynchronously with `ScriptRunAsync()`
9. too late: test harness has already recorded `script_failed`

Root cause:

- temporary yield is implemented as a real abort

---

## Proposed implementation plan

### Step 1: Fix `DbgIsDebugging()`

Files to inspect:

- `src/dbg/_exports.cpp`
- `src/dbg/debugger.cpp`
- call sites that assume `DbgIsDebugging()` implies valid `fdProcessInfo->hProcess`

Goal:

- `DbgIsDebugging()` should only be true when the debugger session is actually usable
- this should address the #3699 class of bugs as well

Notes:

- The issue discussion suggests `_dbg_isdebugging()` returning `bIsDebugging && fdProcessInfo != nullptr`
- That may not be sufficient by itself in all paths; the final definition should reflect a truly usable session

### Step 2: Introduce script interrupt reasons

Files to inspect:

- `src/dbg/simplescript.cpp`
- `src/dbg/simplescript.h`
- `src/dbg/debugger.cpp`
- `src/dbg/testing.cpp`

Replace:

- `bScriptAbort`

With:

- `gScriptInterruptReason`

Behavior:

- `AbortUser`, `AbortAssertion`, `AbortShutdown` => true abort/failure behavior
- `YieldDebugEvent` => non-failing temporary interruption

### Step 3: Add a proper yield/resume handshake

A reason enum alone is not enough.

Need a handshake so the debugger thread can:

1. request yield
2. wait until the script thread has acknowledged yield
3. process breakpoint/debug-event logic
4. resume the script if it was previously running

Possible implementation pieces:

- interrupt reason atomic
- yielded event/flag
- resume event/flag

Goal:

- `ScriptRunAwait()` remains logically in progress across debug-event yields
- only real aborts cause it to return failure

### Step 4: Move script lifecycle inference to observed transitions

Within script command execution:

- sample `DbgIsDebugging()` before command
- execute command
- wait if debugger is running
- sample `DbgIsDebugging()` after command
- detect `false -> true` / `true -> false`

This should replace command-name based ownership logic.

---

## Tests

### Keep / add these tests

#### 1. `script_run_exit`

Purpose:

- verify script continues after `init` + `run` when the debuggee exits normally

Minimal script shape:

```txt
settingset Events, EntryBreakpoint, 0
init tests/script_run_exit.exe
testassert 1, "before run executed"
run
testassert 1, "after run executed"
```

Expected:

- pass
- proves the script can continue after process exit in the simple case

#### 2. `membp/range-execute`

Purpose:

- verify that a breakpoint hit during scripted `run` does not falsely become `script_failed`

Expected after fix:

- either reaches the assertion line and passes/fails normally
- but does **not** fail prematurely due to abort-vs-yield confusion

### Optional extra regression

A dedicated script/breakpoint-yield test would be useful:

- script does `run`
- breakpoint callback fires while script is active
- script continues after breakpoint handling
- final assertion proves the synchronous script flow remained intact

---

## Non-goals for the first pass

- do not solve lifecycle policy using command-name whitelists/blacklists
- do not keep the previous ownership patch that relied on command detection
- do not conflate debugger session state with script interrupt state

---

## Summary

The intended architecture is:

1. `DbgIsDebugging()` becomes a truthful lifecycle primitive
2. script session changes are inferred from before/after state transitions
3. script interruption distinguishes **yield** from **abort**
4. breakpoint/debug-event handling temporarily yields script execution instead of aborting it

This should solve both:

- the `script_failed` behavior seen in scripted runs that hit breakpoints
- the broader lifecycle correctness issues highlighted by #3699
