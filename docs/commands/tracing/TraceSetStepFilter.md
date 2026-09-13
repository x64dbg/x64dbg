# TraceSetStepFilter/SetTraceStepFilter

Select the module party included by conditional tracing.

## arguments

`arg1` Filter: `none`, `user`, or `system` (case-insensitive).

`[arg2]` Traversal mode: `step` (default) or `run`.

Omitting the mode always selects `step`. `none` disables filtering and ignores the traversal mode. The settings are cleared when the trace ends. They cannot be changed during a trace.

## behavior

With `step`, excluded code is traversed by the lightweight party-aware step functions. Trace Into follows calls; Trace Over steps over them. Trace conditions, logging, commands, recording, and the trace counter only apply at matching locations, except when manually interrupting the trace.

With `run`, the same step functions are used in included code. On reaching excluded code, the debugger flushes the pending trace record, arms temporary execute-memory breakpoints on currently executable pages of the selected party, and runs until a matching location is reached. The internal hit resumes tracing without a GUI pause.

This is an opt-in speed/behavior tradeoff:

* Breakpoints are process-wide: any thread entering the selected party can resume the trace.
* Running excluded code stops at the first matching execution, including callbacks. For Trace Over, calls in included code are still stepped over, but calls made while running excluded code are not individually stepped over.
* The breakpoint set is a snapshot of currently executable memory, including executable private memory classified as user code. New allocations or protection changes during the run can introduce code not covered by the snapshot. Use `step` for tracing unpackers/JITs or code that changes executable mappings.
* A DLL load/unload invalidates the snapshot and switches that traversal back to stepping.
* Enabled user memory breakpoints, guard pages, missing target pages, or setup failures prevent the fast path. Partial setup is rolled back and the remainder of the trace uses stepping instead.
* Normal debugger breaks and manual pause still interrupt tracing and remove temporary breakpoints.

The trace dialog's **Run through excluded modules (memory breakpoints)** checkbox selects `run`. It is unchecked by default and disabled for **All Modules**.

## examples

```
TraceSetStepFilter user
TraceSetStepFilter user, run
TraceSetStepFilter system, step
TraceSetStepFilter none
```

## results

This command does not set any result variables.

## see also

[RunToParty](RunToParty.md)
