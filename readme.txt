This is a x32/x64 debugger that is currently in active development.

The debugger has (currently) three parts:
- DBG
- GUI
- Bridge

DBG is the debugging part of the debugger. It handles debugging (using
TitanEngine) and will provide data for the GUI.

GUI is the graphical part of the debugger. It is built on top of QT and it
provides the user interaction, the dump window (not yet implemented), the
disassembly, the register window (not yet implemented), the memory map
view (not yet implemented) etc.

Bridge is the communication library for the DBG and GUI part (and maybe in
the future more parts). The bridge can be used to work on new features,
without having to update the code of the other parts.

Right now the debugger supports the following features:
- variables (with regard to the upcoming script feature)
- basic calculations (var*@401000+.45^4A)
- hide debugger (very basic)
- software breakpoints (INT3, LONG INT3, UD2)
- memory breakpoints (read, write, execute)
- hardware breakpoints (access, write, execute)
- stepping (into, over, n instructions)
- rtr (return from function)
- memory allocation/deallocation in the debuggee
- quickly accessing API addresses (GetProcAddress->76E13620)
- highlighting (not yet customizable, but really helpful)

The debugger core is based on TitanEngine (an updated version) and the
disassembly is powered by BeaEngine. The icon is taken from VisualPharm.