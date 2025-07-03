# seStepOver/sestep/sesto/sest

Step over calls, **swallowing the current exception, skipping exception dispatching in the debuggee**. When the instruction at EIP/RIP isn't a call, a `eStepInto` is performed.

## arguments

`[arg1]` The number of steps to take. If not specified `1` is used.

## result

This command does not set any result variables.
