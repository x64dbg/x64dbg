# SetMemoryRangeBPX/memrangebp/bpmrange

Set a memory breakpoint (GUARD_PAGE) on a specific memory range.

## arguments

`start` Start of the memory range.

`size` Size of the memory range.

`[type]` Breakpoint type, it can be 'a' (read+write+execute) 'r' (read), 'w' (write) or 'x' (execute). Per default, it's 'a' (read+write+execute). Append `ss` for a singleshot breakpoint (you can also use [`SetMemoryBreakpointSingleshoot`](../conditional-breakpoint-control/SetMemoryBreakpointSingleshoot.md) to do this).

## result

This command does not set any result variables.
