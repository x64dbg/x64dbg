# SetBPX/bp/bpx

Set an INT3 (SHORT/LONG) or UD2 breakpoint and optionally assign a name to it.

## arguments

`arg1` Address to put a breakpoint on. This can be an API name.

`[arg2]` Name of the breakpoint, use quotation marks to include spaces. This name can be used by the EnableBPX, DisableBPX and DeleteBPX functions as alias, but is mainly intended to provide a single line of information about the currently-hit breakpoint. When arg2 equals to a valid type (arg3) the type is used and arg2 is ignored.

`[arg3]` Breakpoint type. Can be one of the following options in random order: "ss" (single shot breakpoint), "long" (CD03), "ud2" (0F0B) and "short" (CC). You can combine the "ss" option with one of the type options in one string. Example: "SetBPX 00401000,"entrypoint",ssud2" will set a single shot UD2 breakpoint at 00401000 with the name "entrypoint". When specifying no type or just the type "ss" the default type will be used. Per default this equals to the "short" type. You can change the default type using the "SetBPXOptions" command.

## result

This command does not any result variables.
