# analxrefs/analx

Do xrefs analysis in the module selected in the disassembly view, or in the range
given as arguments.

## arguments

`[addr]` Address to analyse. When omitted, the module selected in the disassembly
view is used (the previous behaviour).

`[size]` Optional size in bytes. When omitted, the module containing `addr` is
analysed.

## results

This command does not set any result variables.

## note

Automation (plugins, scripts, `dbg_cmd_exec`) cannot create a disassembly
selection. Without arguments this command therefore resolves the selection to
`MemFindBaseAddr(0)` and reports success while the xref database stays empty.
Passing `addr` makes the analysis work regardless of the GUI state, including
headless setups.
