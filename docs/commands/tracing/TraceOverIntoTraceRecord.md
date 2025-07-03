# TraceOverIntoTraceCoverage/TraceOverIntoTraceRecord/toit

Perform StepOver until the program reaches somewhere inside the trace coverage.

## arguments

`[arg1]` The break condition of tracing. When this condition is satisfied, tracing will stop regardless of `EIP`/`RIP` location. If this argument is not specified then tracing will be unconditional.

`[arg2]` The maximun steps before the debugger gives up. If this argument is not specified, the default value will be 50000.

## result

This command does not set any result variables.
