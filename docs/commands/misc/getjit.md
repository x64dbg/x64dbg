# getjit/jitget

Get the Just-In-Time Debugger in Windows. In WIN64 systems there are two JIT entries: one for a x32 debugger and other for a x64 debugger. In a WIN64 system when a x32 process crash: Windows attach the x32 debugger stored in the x32-JIT entry.

Important notes:

- Its possible get the x32-JIT entry from the x64 debugger (using the x32 arg).

- Its possible get the x64-JIT entry from the x32 debugger ONLY if the x32 debugger its running in a WIN64 System (using the x64 arg).

## arguments

Without arguments: Get the current JIT debugger.

`arg2`

1. *old*: Get the old JIT entry stored.

2. *x32*: Get the x32-JIT entry.x64: Get the x64-JIT entry.

## result

This command does not set any result variables.
