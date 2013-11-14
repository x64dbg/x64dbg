[This is a new version of this repository. The old version can be found ]
[here: https://bitbucket.org/mrexodia/x64_dbg_old                       ]

This is a x64/x32 debugger that is currently in active development.

The debugger has (currently) three parts:
- DBG
- GUI
- Bridge

DBG is the debugging part of the debugger. It handles debugging (using
TitanEngine) and will provide data for the GUI.

GUI is the graphical part of the debugger. It is built on top of Qt and it
provides the user interaction, the dump window (not yet implemented), the
disassembly, the register window, the memory map view, the log view etc.

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
- memory map
- basic module labeling
- import reconstruction (plugin using Scylla)
- drag&drop files
- goto window
- register/flags view with editing support
- quite fast working in really big code pages (tested up to 5GB)
- GUI hotkeys
- dynamic jump arrow (just like OllyDbg)

Known bugs are:
- hardware breakpoints do not work properly on x64 (TitanEngine bug)
- sometimes the disassembly view is not updated (click anywhere to solve)
- ??? (please report)

The debugger core is based on TitanEngine (an updated version,
https://bitbucket.org/mrexodia/titanengine-update)

Disassembly powered by BeaEngine (http://beaengine.org/).

The icon is taken from VisualPharm (http://www.visualpharm.com/)

Special thanks:
- acidflash
- Ahmadmansoor
- EXETools community
- Tuts4You community

Greetings,

Mr. eXoDia & Sigma


