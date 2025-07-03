# Command line

x64dbg supports the following command line:

- 1 argument: `x64dbg filename.exe` will debug `filename.exe`.
- 2 arguments: `x64dbg -p PID` will attach to the process with `PID` PID.
- 2 arguments: `x64dbg filename.exe cmdline` will debug `filename.exe` with `cmdline` as command line.
- 3 arguments: `x64dbg filename.exe cmdline currentdir` will debug `filename.exe` with `cmdline` as command line and `currentdir` as current directory.
