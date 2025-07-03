# Trace

Trace view is a view in which you can see history of stepped instructions. This lets you examine the details of each instruction stepped when you are stepping manually or [tracing](../../introduction/ConditionalTracing.md) automatically, or view a trace history previously saved. This functionality must be enabled explicitly from trace view or [CPU view](CPU.rst). It features saving all the instructions, registers and memory accesses during a trace.

You can double-click on columns to perform quick operation:
* Index: follow in disassembly.
* Address: toggle RVA display mode.
* Opcode: toggle breakpoint.
* Disassembly: follow in disassembly.
* Comments: set comments.

## Start trace recording

To enable trace logging into trace view, you first enable it via "**Start trace recording**" menu item. It will pop up a dialog allowing you to save the recorded instructions to a file. The default location of this file is in the database directory.

Once started, every instruction you stepped or traced will appear immediately in Trace view. If you let the application run the executed instructions will not be recorded.

## Stop trace recording

This menu can stop recording instructions.

## Close

Close current trace file and clear the trace view.

## Close and delete

Close current trace file and clear the trace view, and also delete current trace file from disk.

## Open

Open a trace file to view the content of it. It can be used when not debugging, but it is recommended that you debug the corresponding debuggee when viewing a trace, as it will be able to render the instructions with labels from the database of the debuggee. The debugger will show a warning if you want to load a trace file which is not recorded from currently debugged executable.

## Recent files

Open a recent file to view the content of it.

## Search
### Constant

Search for the user-specified constant in the entire recorded trace, and record the occurances in references view.

### Memory Reference

Search for memory accesses to the user-specified address.

## Toggle Auto Disassembly Scroll

When turned on, the disassembly view in the [CPU view](CPU.rst) will automatically follow the EIP or RIP of selected instruction in trace view.
