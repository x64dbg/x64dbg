# bpgoto

Configure the breakpoint so that when the program reaches it, the program will be directed to a new location. It is equivallent to the following commands:

```
SetBreakpointCondition arg1, 0
SetBreakpointCommand arg1, "CIP=arg2"
SetBreakpointCommandCondition arg1, 1
SetBreakpointFastResume arg1, 0
```

## arguments

`arg1` The address of the breakpoint.

`arg2` The new address to execute if the breakpoint is reached.

## results

This command does not set any result variables.
