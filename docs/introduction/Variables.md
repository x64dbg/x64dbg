# Variables

This program supports variables. There are three types of variables:

- USER: Variables created by the user using the `var`/`mov` commands. These variables have no access restrictions. You generally only deal with these.
- SYSTEM: Variables created by x64dbg, that can be read and written, but cannot be deleted.
- READONLY: Variables created by x64dbg, that can be read, but not written or deleted.

## Setting variables

You can set variables in the following ways:

```
mov myvar, 1234
mov $myvar, 1234
myvar = 1234
$myvar = 1234
```

All of the above set a USER variable `myvar` to the [value](./Values.md) `0x1234`. You can also use the C-style assignment operators (see the [expression](./Expressions.md) documentation for a full list of supported operators):

```
myvar += 0x10
myvar |= 0x10
myvar++
myvar--
```

## Reserved Variables

There are a few reserved variables:

- `$res`/`$result`: General result variable.
- `$resN`/`$resultN`: Optional other result variables (N= 1-4).
- `$pid`: Process ID of the debugged executable.
- `$hp`/`$hProcess`: Debugged executable handle.
- `$lastalloc`: Last result of the `alloc` command.
- `$breakpointcondition` : Controls the pause behaviour in the conditional breakpoint command.
- `$breakpointcounter` : The hit counter of the breakpoint, set before the condition of the conditional breakpoint is evaluated.
- `$breakpointlogcondition` : The log condition of the conditional breakpoint. It cannot be used to control the logging behavoiur.
