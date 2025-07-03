# RunToUserCode/rtu

Run until user code is reached. It is equivallent to `RunToParty 0`.

This command sets temporary memory breakpoints on all user code pages, rather than single stepping. It fails when another RunToUserCode command is already executing, because the temporary memory breakpoints are already set.

## arguments

This command has no arguments.

## results

This command does not set any result variables.
