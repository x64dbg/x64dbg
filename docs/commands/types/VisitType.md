# VisitType/DisplayType/dt

Display a type and print its members in the struct view.

## arguments

`arg1` The type to display.

`[arg2]` Address to print from. If not specified (or zero) the type will be printed without values.

`[arg3]` Maximum pointer resolution depth. This can be used to also display structures (and values) pointed to by members of the type you are visiting. If not specified or negative, it will default to 2 (configurable with `[Engine].DefaultTypePtrDepth`).

`[arg4]` Name of the variable. If not specified it will default to an empty string.

## result

This command does not set any result variables.
