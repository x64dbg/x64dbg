# Breakpoints in unloaded modules

This test sets a breakpoint in `winmm.dll`, unloads the DLL and then disables and enables the breakpoint, both by name and with the enable/disable all commands.

The test fails if enabling the breakpoint fails while the module is not loaded, or if the breakpoint is not armed when the DLL is loaded again.
