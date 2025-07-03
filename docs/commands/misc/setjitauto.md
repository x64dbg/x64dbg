# setjitauto/jitsetauto

Set the Auto Just-In-Time Debugger FLAG in Windows. if this flag value its TRUE Windows runs the debugger without user confirmation when a process crash. In WIN64 systems there are two JIT AUTO FLAG entries: one for a x32 debugger and other for a x64 debugger. In a WIN64 system when a x32 process crash with AUTO FLAG = FALSE: Windows confirm before attach the x32 debugger stored in the x32-JIT entry.

Important notes:

- Its possible set the x32-JIT AUTO FLAG entry from the x64 debugger (using the x32 arg).

- Its possible set the x64-JIT AUTO FLAG entry from the x32 debugger ONLY if the x32 debugger its running in a WIN64 System (using the x64 arg).

## arguments

`arg1`
1. 1/ON: Set current JIT entry FLAG as TRUE.

2. 0/FALSE: Set current JIT entry FLAG as FALSE.

3. x32: Set the x32-JIT AUTO FLAG TRUE or FALSE. It needs an arg2: can be ON/1 or OFF/0.

4. x64: Set the x64-JIT AUTO FLAG TRUE or FALSE. It needs an arg2: can be ON/1 or OFF/0.

## result

This command does not set any result variables.
