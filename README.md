# x64_dbg

## Note
**This is a new version of this repository. The old version can be found [here](https://bitbucket.org/mrexodia/x64_dbg_old).**

Also please run install.bat before you start committing code, this ensures your code is auto-formatted to the *x64_dbg* standards.

## Downloads
Releases of *x64_dbg* can be found on [here](http://download.x64dbg.com).

## Overview
*x64_dbg* is an open-source x32/x64 debugger for Windows.

## Features
- Open-source
- Intuitive and familiar, yet new user interface
- C-like expression parser
- Full-featured debugging of DLL and EXE files (TitanEngine)
- IDA-like sidebar with jump arrows
- IDA-like instruction token highlighter (highlight registers, etc.)
- Memory map
- Symbol view
- Thread view
- Content-sensitive register view
- Fully customizable color scheme
- Dynamically recognize modules and strings
- Import reconstructor integrated (Scylla)
- Fast disassembler (BeaEngine)
- User database (JSON) for comments, labels, bookmarks, etc.
- Plugin support with growing API
- Extendable, debuggable scripting language for automation
- Multi-datatype memory dump
- Basic debug symbol (PDB) support
- Dynamic stack view
- Built-in assembler (XEDParse)

## Known bugs
- Memory breakpoints sometimes fail (TitanEngine bug)

## License
*x64_dbg* is licensed under GLPv3, which means you can freely distribute and/or modify the source of *x64_dbg*, as long as you share your changes with us. The only exception is that plugins you write do not have to comply with the GLPv3 license. They do not have to be open-source and they can be commercial and/or private. The only exception to this is when your plugin uses code copied from *x64_dbg*. In that case you would still have to share the changes to *x64_dbg* with us.

## Credits
- Debugger core by TitanEngine Community Edition
- Disassembly powered by BeaEngine (Updated)
- Assembly powered by XEDParse
- Import reconstruction powered by Scylla
- JSON powered by Jansson
- Database compression powered by lz4
- Bug icon by VisualPharm
- Interface icons by Fugue
- Website by tr4ceflow

## Special Thanks
- EXETools community
- Tuts4You community
- acidflash
- cyberbob
- Teddy Rogers
- TEAM DVT
- DMichael
- Artic
- Nukem
- ahmadmansoor

## Developers (in order of joining)
- [Mr. eXoDia](http://mrexodia.cf)
- Sigma
- [tr4ceflow](http://blog.tr4ceflow.com)
- [Dreg](http://www.fr33project.org)