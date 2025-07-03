# SetExceptionBreakpointFastResume

Sets the fast resume flag of an exception breakpoint. If this flag is set and the break condition doesn't evaluate to break, no GUI, plugin, logging or any other action will be performed, except for incrementing the hit counter.

## arguments

`arg1` The name, exception name or code of the exception breakpoint.

`[arg2]` The fast resume flag. If it is 0 (default), fast resume is disabled, otherwise it is enabled

## result

This command does not set any result variables.
