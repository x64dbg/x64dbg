# GUI

This section describes various settings in the GUI tab.

## Show FPU registers as little endian

Some FPU registers, especially SSE and AVX registers, are usually used to perform parallel computation.
Using little endian helps to correspond floating point numbers to their index in memory arrays. However, big
endian representation are more familiar to most users. This option can set whether FPU registers are shown
as little endian or as big endian. You also edit the FPU registers in the endianness set here.

## Save GUI layout and column orders

Allow column order, width and layout of some views, to be saved in the config file. Note that not all views support
this option. Currently, this option has not been implemented in the CPU view.

## Don't show close dialog

Do not show the close dialog when the debugger exits.

## Show PID in HEX

Show PID in hexadecimal in the attach dialog. If not set, it will use decimal, just like in the Task Manager.

## Enable Load/Save Tab Order

Allow x64dbg to load and save tab order. If not set, x64dbg will always use the default tab order.

## Show Watch Labels in Side Bar

When you add a watched variable in the watch view, a label with the name of the watched variable can appear in
the side bar of the disassembly view if the address is in the sight. They just look like labels for registers. This label
might help you understand the operation and progress of a self modifying routine. If disabled, no labels will be added
in the side bar for watched variables.

## Do not call SetForegroundWindow

When a debug event occurs, x64dbg will focus itself so you can view the state of the debuggee. In some circumstances
this might not be desired. This option can be used to tell x64dbg not to focus itself when a debug event occurs.

## Show RVA addresses in graph view
When enabled in graph view the offset addresses are shown in front of the disassembly.

## Graph zoom mode

Allows to zoom graph view by holding <ctrl> and turning the mousewheel up and down.
Note: Press G on the CPU tab press G to open graph view 

## Show exit confirmation dialog

When enabled the dialog
"The debuggee is still running and will be terminated if you exit. Do you really want to exit?"
is shown when you exit x64dbg but it's still attached to some process for debugging.

## Disable auto completion in goto dialog

Here you can turn of the auto completion that will suggest words as soon you start typing in the "Enter expression" dialog.  Go to/expression from the right click context menu  will bring up the "Enter expression" dialog. Note: That stting will be handy in case there are delays when typing.

## Show ASCII/Unicode in address dump mode
When enabled the column "ASCII" is added to the dump panels.
Furthermore the view mode(right click) must be set to "Address" to see this effect.
Note: The column "ASCII" must not be hidden.
Right click on column header to bring up the 'Edit column' dialog to check.
^^
Note from the doc contributor: 
However please don't ask why there is an option here
when there is also that 'Edit column' dialog that does nearly the same thing.
