# InitDebug/initdbg/init

Initializes the debugger. This command will load the executable (do some basic checks), set breakpoints on TLS callbacks (if present), set a breakpoint at the process entry point and break at the system breakpoint before giving back control to the user.

## arguments

`arg1` Path to the executable file to debug. If no full path is given, the `GetCurrentDirectory` API will be called to retrieve a full path. Use quotation marks to include spaces in your path.

`[arg2]` Commandline to create the process with.

`[arg3]` Current folder (passed to the `CreateProcess` API) (this is also sometimes called 'working directory' or 'current directory')

## result

This command will give control back to the user after the system breakpoint is reached. It will set `$pid` and `$hp`/`$hProcess` variables.
