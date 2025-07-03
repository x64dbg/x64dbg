# SetMemoryBPX/membp/bpm

Set a memory breakpoint (GUARD_PAGE) on the whole memory region the provided address is in.

## arguments

`arg1` Address of or inside a memory region that will be watched.

`[arg2]` 1/0 restore the memory breakpoint once it's hit? When this value is not equal to '1' or '0', it's assumed to be arg3. This means "bpm eax,r" would be the same command as: "bpm eax,0,r".

`[arg3]` Breakpoint type, it can be 'a' (read+write+execute) 'r' (read), 'w' (write) or 'x' (execute). Per default, it's 'a' (read+write+execute)

## result

This command does not set any result variables.
