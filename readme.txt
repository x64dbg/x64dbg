[This is a new version of this repository. The old version can be found ]
[here: https://bitbucket.org/mrexodia/x64_dbg_old                       ]

>Installation guide:
1) Download the latest 'qt_base_XXX.rar'
2) Download the latest 'bin_base_XXX.rar'
3) Download the latest 'release_xxx.rar'
4) (Optional) Download the latest 'help_XXX.rar'
5) Extract all in the same directory
6) Run 'bin\x64\x64_dbg.exe' or 'bin\x32\x32_dbg.exe'

>Overview:
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

>Features:
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
- user databases for labels/comments/breakpoints/bookmarks (*.dd64 or *.dd32 files)
- easy context menu in disassembly (to set breakpoints etc)
- plugin support
- (manual) function analysis
- easily follow calls/jumps/ret (press ENTER in when selecting)
- (buggy) dynamic commenting
- scripting support (using the debugger commands)!
- simple dump
- symbols (+ exports) view with search

>Known bugs:
- memory breakpoints sometimes fail (TitanEngine bug)

>Last words:
The debugger core is based on TitanEngine (an updated version,
https://bitbucket.org/mrexodia/titanengine-update)

Disassembly powered by BeaEngine (http://beaengine.org/).

The icon is taken from VisualPharm (http://www.visualpharm.com/)

>Special thanks:
- acidflash
- Ahmadmansoor
- cyberbob
- Teddy Rogers
- EXETools community
- Tuts4You community
- DMichael
- Sorry if I forgot you!

>Lead developers:
- Mr. eXoDia
- Sigma