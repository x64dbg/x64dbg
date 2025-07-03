# alloc

Allocate memory in the debuggee (using VirtualAllocEx). The memory is allocated with PAGE_EXECUTE_READWRITE protection.

## arguments

`[arg1]` Size of the memory to allocate. When not specified, a default size of 0x1000 is used.

`[arg2]` Address to allocate the memory at. Unspecified or zero means a random address.

## result

This command sets $result to the allocated memory address. It also sets the $lastalloc variable to the allocated memory address when VirtualAllocEx succeeded.
