# LibrarianSetBreakpoint/bpdll

Set a singleshoot breakpoint on DLL load/unload.

## arguments

`arg1` DLL Name to break on.

`[arg2]` `a` means on load and unload, `l` means on load, `u` means on unload. When not specified, x64dbg will break on both load and unload.

`[arg3]` When specified, the breakpoint will be singleshoot. When not specified the breakpoint will not be removed after it has been hit.

## result

This command does not set any result variables.
