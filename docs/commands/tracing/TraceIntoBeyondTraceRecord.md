# TraceIntoBeyondTraceCoverage/TraceIntoBeyondTraceRecord/tibt

Perform [StepInto](StepInto.md) until the program reaches somewhere outside the trace coverage. This is similar to `ticnd tr.hitcount(cip)==0&&arg1, arg2` except that it achieves higher performance by avoiding the expression function invocation.

Usage example: If you want to find out the forking point of the program when different inputs are provided, first enable or re-enable trace coverage to clean trace coverage data.
Then you trace while input A is provided. Finally you provide input B and execute `TraceIntoBeyondTraceRecord` command. The program will be paused where the instruction is never executed before.

## arguments

`[arg1]` The break condition of tracing. When this condition is satisfied, tracing will stop regardless of `EIP`/`RIP` location. If this argument is not specified then tracing will be unconditional.

`[arg2]` The maximun steps before the debugger gives up. If this argument is not specified, the default value will be 50000.

## result

This command does not set any result variables.
