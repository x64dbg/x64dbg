# SetHardwareBreakpoint/bph/bphws

Set a hardware breakpoint (using debug registers).

## arguments

`arg1` Address of the hardware breakpoint.

`[arg2]` Hardware breakpoint type. Can be either 'r' (readwrite), 'w' (write) or 'x' (execute). When not specified, 'x' is assumed.

`[arg3]` Hardware breakpoint size. Can be either '1', '2', '4' or '8' (x64 only). Per default, '1' is assumed. The address you're putting the hardware breakpoint on must be aligned to the specified size.

## result

This command does not set any result variables.
