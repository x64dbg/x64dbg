# memcpy

Copy memory in the context of the debuggee, without applying patches.

## arguments

`arg1` Destination address.

`arg2` Source address.

`arg3` Size to copy.

## result

This command sets `$result` to the total amount of bytes written at the destination. The condition `$result == arg3` is true if all memory was copied.
