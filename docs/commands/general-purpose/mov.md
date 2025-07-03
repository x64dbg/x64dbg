# mov/set

Set a variable.

## arguments

`arg1` Variable name (optionally prefixed with a $) to set. When the variable does not exist, it will be created. Note that SSE registers are not supported (Instead use [movdqu](movdqu.md) for SSE registers).

`arg2` Value to store in the variable. If you use `#11 22 33#` it will write the bytes `11 22 33` in the process memory at `arg1`.

## result

This command does not set any result variables.
