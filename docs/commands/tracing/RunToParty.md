# RunToParty

Run until any thread executes code belonging to the requested module party. Temporary execute-memory breakpoints are placed on currently committed executable pages with that party, including executable private memory (classified as user code).

Windows-managed CFG/SCP image-extension trampolines (Windows 11 24H2+) are omitted from the temporary breakpoint set. Windows reports these executable pages but does not allow changing their protections. They may run through without stopping, even if their module-party classification matches. Only ranges explicitly reported by the OS as CFG/SCP extensions are omitted; ordinary private/JIT code remains covered. Normal stepping and module-party classification are unchanged.

The breakpoint set is a snapshot. New executable allocations and protection changes during the run may not be covered. A DLL load/unload switches to single-stepping until the requested party is reached. Use `StepUser` or `StepSystem` when executable memory changes need to be followed reliably.

The command fails without resuming if another party run is active, no target pages exist, or the full breakpoint set cannot be installed. Enabled user memory breakpoints and target guard pages prevent setup; user breakpoints are not removed or replaced. Partial setup is rolled back.

On a hit the temporary breakpoints are removed and the debugger pauses. Manual pause, other debugger breaks, and process termination also clear the temporary state.

## arguments

`arg1` The party number. This value cannot be an expression. Note: `0` is user module, `1` is system module.

## results

This command does not set any result variables.

## see also

[RunToUserCode](RunToUserCode.md)

[RunToSystemCode](RunToSystemCode.md)

[TraceSetStepFilter](TraceSetStepFilter.md)