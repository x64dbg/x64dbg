# setthreadpriority/setprioritythread/threadsetpriority

Set thread priority in the debuggee.

## arguments

`arg1` ThreadId of the thread to change the priority of (see the Threads tab).

`arg2` Priority value, this can be the integer of a valid thread priority (see MSDN) or one of the following values: "Normal", "AboveNormal", "TimeCritical", "Idle", "BelowNormal", "Highest", "Lowest".

## result

This command does not set any result variables.
