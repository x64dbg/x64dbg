# Conditional Tracing

This section describes the [conditional tracing](../commands/tracing/index.rst) capability in x64dbg.

## Operations overview

When a trace step is hit, x64dbg will do the following things:

- Increment the *trace counter*;
- Set the system variable `$tracecounter` to the value of *trace counter*;
- If *break condition* is set, evaluate the [expression](Expressions.md) (defaults to `0`);
- Execute plugin callbacks (allowing plugins to change the *break condition*);
- If *log condition* is set, evaluate the [expression](Expressions.md) (defaults to `1`);
- If *command condition* is set, evaluate the [expression](Expressions.md) (defaults to *break condition*);
- If *log text* is set and *log condition* evaluated to `1`:
  - Format and print the *log text* (see [String Formatting](Formatting.md)). To redirect the log to a file use [TraceSetLogFile](../commands/tracing/TraceSetLogFile.md).
- If *command text* is set and *command condition* evaluated to `1`:
  - Set the system variable `$tracecondition` to the *break condition*;
  - Set the system variable `$tracelogcondition` to the *log condition*;
  - Execute the command in *command text*;
  - The *break condition* will be set to the value of `$tracecondition`. So if you modify this system variable in the script, you will be able to control whether the debuggee would break.
- If *break condition* evaluated to `1`:
  - Print the standard log message; 
  - Break the debuggee and wait for the user to resume.

In addition to the above operations, x64dbg also has the ability to record traced instructions to the trace view and to update the trace coverage. This happens every time the debugger steps or pauses, also if you do it manually.

**Warning: All numbers in expressions are interpreted as hex by default!** For decimal use `.123`.

## Logging

The log can be formatted by x64dbg to log the current state of the program. See [formatting](./Formatting.md) on how to format the log string. If you are looking for logging the address and disassembly of all instructions traced you can use `{p:cip} {i:cip}`. To redirect the log to a file use [TraceSetLogFile](../commands/tracing/TraceSetLogFile.md), or use the graphical interface.

## Trace coverage

If you use one of the trace coverage-based tracing options such as [TraceIntoBeyondTraceCoverage](../commands/tracing/TraceIntoBeyondTraceRecord.md), the initial evaluation of *break condition* includes the type of trace coverage tracing that you specified. The normal *break condition* can be used to break before the trace coverage condition is satisfied. If you want to include trace coverage in your condition for full control, you can use the [expression functions](./Expression-functions.md).

## Notes

You can start a conditional tracing by "Trace over until condition"/"Trace into until condition" commands in the [Debug menu](../gui/menus/Debug.rst).

You should not use commands that can change the running state of the debuggee (such as `run`) inside the breakpoint command, because these commands are unstable when used here. You can use *break condition*, *command condition* or `$tracecondition` instead.

When you use *Trace Over*, the debuggee would not be paused inside the calls that has been stepped over, even if the condition is true. Also other operations, such as logging and trace record, are not executed. This makes tracing faster, but if these operations are desired you should use *Trace Into*.

## See also

- [Tracing](../commands/tracing/index.rst)
- [Expressions](Expressions.md)
- [Expression Functions](Expression-functions.md)
- [String Formatting](Formatting.md)