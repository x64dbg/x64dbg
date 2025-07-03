# AttachDebugger/attach

Attach the debugger to a running process.

## arguments

`arg1` Process Identifier (PID) of the running process.

`[arg2]` Handle to an Event Object to signal (this is for internal use only).

`[arg3]` Thread Identifier (TID) of the thread to resume after attaching (this is for internal use only).

## result

This command will give control back to the user after the system breakpoint is reached. It will set `$pid` and `$hp`/`$hProcess` variables.
