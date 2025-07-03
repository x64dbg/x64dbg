# free

Free memory in the debuggee (using VirtualFreeEx).

## arguments

`[arg1]` Address of the memory to free. When not specified, the value at $lastalloc is used.

## result

This command sets $result to 1 if VirtualFreeEx succeeded, otherwise it's set to 0. $lastalloc is set to zero when the address specified is equal to $lastalloc.
