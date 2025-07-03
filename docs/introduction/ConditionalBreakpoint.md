# Conditional Breakpoints

This section describes the conditional breakpoint capability in x64dbg.

## Operations overview

When a breakpoint is hit, x64dbg will do the following things:

- Set the system variable `$breakpointexceptionaddress` to the address that triggered the breakpoint (memory location);
- Increment the *hit counter*;
- Set the system variable `$breakpointcounter` to the value of *hit counter*;
- If *break condition* is set, evaluate the [expression](./Expressions.md) (defaults to `1`);
- If *fast resume* is set and *break condition* evaluated to `0`:
  - Resume execution of the debuggee (skip the next steps). This will also skip executing plugin callbacks and GUI updates.
- If *log condition* is set, evaluate the [expression](./Expressions.md) (defaults to `1`);
- If *command condition* is set, evaluate the [expression](./Expressions.md) (defaults to `1`);
- If *break condition* evaluated to `1` (or any value other than '0'):
  - Print the standard log message; (if [the breakpoint is set to be silent](../commands/conditional-breakpoint-control/SetBreakpointSilent.md), standard log message is supressed.)
  - Execute plugin callbacks.
- If *log text* is set and *log condition* evaluated to `1` (or any value other than '0'):
  - Format and print the *log text* (see [String Formatting](./Formatting.md)).
- If *command text* is set and *command condition* evaluated to `1`:
  - Set the system variable `$breakpointcondition` to the *break condition*;
  - Set the system variable `$breakpointlogcondition` to the *log condition*;
  - Execute the command in *command text*;
  - The *break condition* will be set to the value of `$breakpointcondition`. So if you modify this system variable in the script, you will be able to control whether the debuggee would break.
- If *break condition* evaluated to `1` (or any value other than '0'):
  - Break the debuggee and wait for the user to resume.
  
If any expression is invalid, the condition will be triggered (That is, an invalid expression as condition will cause the breakpoint to always break, log and execute command).

**Warning: All numbers in expressions are interpreted as hex by default!** For decimal use `.123`.

## Hit counter

A hit counter records how many times a breakpoint has been reached. It will be incremented unconditionally, even if fast resume is enabled on this breakpoint. It may be viewed at breakpoint view and reset with [ResetBreakpointHitCount](../commands/conditional-breakpoint-control/ResetBreakpointHitCount.md).

## Logging

The log can be formatted by x64dbg to log the current state of the program. See [formatting](./Formatting.md) on how to format the log string.

## Notes

You can set a conditional breakpoint with GUI by setting a software breakpoint(key F2) first, then right-click on the instruction and select "Edit breakpoint" command from the context menu. Fill in the conditional expression and/or other information as necessary, then confirm and close the dialog.

You should not use commands that can change the running state of the debuggee (such as `run`) inside the breakpoint command, because these commands are unstable when used here. You can use *break condition*, *command condition* or `$breakpointcondition` instead.

## Examples

**A conditional breakpoint which never breaks**

*break condition*: `0` (Useful if you only want to execute command or log data, but not pause the debuggee)

**A conditional breakpoint which breaks only if EAX and ECX both equal to 1**

*break condition*: `EAX==1 && ECX==1`

**A conditional breakpoint which breaks only if the first argument is 1**

*break condition*: `arg.get(0)==1`

**A conditional breakpoint which breaks only if EAX is a valid address**

*break condition*: `mem.valid(EAX)`

**A conditional breakpoint which breaks on the third hit**

*break condition*: `$breakpointcounter==3` or `($breakpointcounter%3)==0`

**A conditional breakpoint which breaks only if executed by the thread 1C0**

*break condition*: `tid()==1C0`

**A conditional breakpoint which breaks only if ECX points to a UTF-16 string containing "foo"**

*break condition*: `strstr(utf16(ECX), "foo")`

## Comparison with Conditional Tracing

A conditional breakpoint can only pause the debuggee when it is executed. It cannot pause the debuggee when the breakpoint is not hit even if the condition is satisfied. If you don't know where the condition will become true, try [conditional tracing](./ConditionalTracing.md) instead!

## See also

- [Conditional Breakpoint Control](../commands/conditional-breakpoint-control/index.rst)
- [Expressions](./Expressions.md)
- [Expression Functions](./Expression-functions.md)
- [String Formatting](./Formatting.md)
