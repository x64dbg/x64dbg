# SetBreakpointCondition/bpcond/bpcnd

Sets the software breakpoint condition. When this condition is set, it is evaluated every time the breakpoint hits and the debugger would stop only if condition is not 0.

## arguments

`arg1` The address of the breakpoint.

`[arg2]` The condition expression. Quote and escape the command argument when the expression contains commas or string literals. For example, the following condition compares against a path ending in a backslash:

```
SetBreakpointCondition 401000, "streq(utf8(rax), \"C:\directory\\\")"
```

## result

This command does not set any result variables.
