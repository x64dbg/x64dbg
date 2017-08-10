# x64dbg

[![BountySource](https://www.bountysource.com/badge/team?team_id=18188&style=raised)](https://www.bountysource.com/teams/x64dbg?utm_source=x64dbg&utm_medium=shield&utm_campaign=raised) [![Build status](https://ci.appveyor.com/api/projects/status/h1j489qa1mx67e0h?svg=true)](https://ci.appveyor.com/project/mrexodia/x64dbg) [![coverity](https://scan.coverity.com/projects/7478/badge.svg?flat=1)](https://scan.coverity.com/projects/7478) [![Crowdin](https://d322cqt584bo4o.cloudfront.net/x64dbg/localized.svg)](http://translate.x64dbg.com)

[![Telegram](https://img.shields.io/badge/chat-%20on%20Telegram-blue.svg)](http://telegram.x64dbg.com) [![Join the chat at Gitter](https://badges.gitter.im/x64dbg/x64dbg.svg)](http://gitter.x64dbg.com) [![freenode](https://img.shields.io/badge/chat-%20on%20freenode-brightgreen.svg)](http://webchat.freenode.net/?channels=x64dbg) [![Download x64dbg](https://img.shields.io/sourceforge/dm/x64dbg.svg)](https://sourceforge.net/projects/x64dbg/files/latest/download)

## Note

Please run `install.bat` before you start committing code, this ensures your code is auto-formatted to the *x64dbg* [standards](https://github.com/x64dbg/x64dbg/wiki/Coding-Guidelines).

## Compiling

For a complete guide on compiling *x64dbg* read [this](https://github.com/x64dbg/x64dbg/wiki/Compiling-the-whole-project).

## Downloads

Releases of *x64dbg* can be found [here](http://releases.x64dbg.com).

## Overview

*x64dbg* is an open-source x32/x64 debugger for Windows.

## Activity Graph

[![Throughput Graph](https://graphs.waffle.io/x64dbg/x64dbg/throughput.svg)](https://waffle.io/x64dbg/x64dbg/metrics/throughput)

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
- Source code view
- Content-sensitive register view
- Fully customizable color scheme
- Dynamically recognize modules and strings
- Import reconstructor integrated (Scylla)
- Fast disassembler (Capstone)
- User database (JSON) for comments, labels, bookmarks, etc.
- Plugin support with growing API
- Extendable, debuggable scripting language for automation
- Multi-datatype memory dump
- Basic debug symbol (PDB) support
- Dynamic stack view
- Built-in assembler (XEDParse/Keystone/asmjit)
- Executable patching
- Yara Pattern Matching
- Decompiler (Snowman)
- Analysis

## License

*x64dbg* is licensed under GPLv3, which means you can freely distribute and/or modify the source of *x64dbg*, as long as you share your changes with us. The only exception is that plugins you write do not have to comply with the GPLv3 license. They do not have to be open-source and they can be commercial and/or private. The only exception to this is when your plugin uses code copied from *x64dbg*. In that case you would still have to share the changes to *x64dbg* with us.

## Credits

- Debugger core by [TitanEngine Community Edition](https://bitbucket.org/titanengineupdate/titanengine-update)
- Disassembly powered by [Capstone](http://capstone-engine.org)
- Assembly powered by [XEDParse](https://github.com/x64dbg/XEDParse), [Keystone](http://keystone-engine.org) and [asmjit](https://github.com/asmjit)
- Import reconstruction powered by [Scylla](https://github.com/NtQuery/Scylla)
- JSON powered by [Jansson](http://www.digip.org/jansson)
- Database compression powered by [lz4](https://bitbucket.org/mrexodia/lz4)
- Advanced pattern matching powered by [yara](http://virustotal.github.io/yara)
- Decompilation powered by [snowman](https://derevenets.com)
- Bug icon by [VisualPharm](http://www.visualpharm.com)
- Interface icons by [Fugue](http://p.yusukekamiyamane.com)
- Website by [tr4ceflow](http://tr4ceflow.com)

## Special Thanks

- All the donators!
- Everybody adding issues!
- People I forgot to add to this list
- [EXETools community](http://forum.exetools.com)
- [Tuts4You community](http://forum.tuts4you.com)
- [ReSharper](https://www.jetbrains.com/resharper)
- [Coverity](http://www.coverity.com)
- acidflash
- cyberbob
- cypher
- Teddy Rogers
- TEAM DVT
- DMichael
- Artic
- ahmadmansoor
- \_pusher\_
- firelegend
- [kao](http://lifeinhex.com)
- sstrato
- [kobalicek](https://github.com/kobalicek)

## Developers

- [mrexodia](http://mrexodia.cf)
- Sigma
- [tr4ceflow](http://blog.tr4ceflow.com)
- [Dreg](http://www.fr33project.org)
- [Nukem](https://github.com/Nukem9)
- [Herz3h](https://github.com/Herz3h)
- [torusrxxx](https://github.com/torusrxxx)

## Contributors

- [blaquee](https://github.com/blaquee)
- [wk-952](https://github.com/wk-952)
- [RaMMicHaeL](http://rammichael.com)
- [lovrolu](https://github.com/lovrolu)
- [fileoffset](https://github.com/fileoffset)
- [SmilingWolf](https://github.com/SmilingWolf)
- [ApertureSecurity](https://github.com/ApertureSecurity)
- [mrgreywater](https://github.com/mrgreywater)
- [Dither](https://github.com/Dither)
- [zerosum0x0](https://github.com/zerosum0x0)
- [RadicalRaccoon](https://github.com/RadicalRaccoon)
- [fetzerms](https://github.com/fetzerms)
- [muratsu](https://github.com/muratsu)
- [ForNeVeR](https://github.com/ForNeVeR)
- [wynick27](https://github.com/wynick27)
- [Atvaark](https://github.com/Atvaark)
- [Avin](https://github.com/Avinm)
- [mrfearless](https://github.com/mrfearless)
- [Storm Shadow](https://github.com/techbliss)
- [shamanas](https://github.com/shamanas)
- [joesavage](https://github.com/joesavage)
- [justanotheranonymoususer](https://github.com/justanotheranonymoususer)
- [gushromp](https://github.com/gushromp)
- [Forsari0](https://github.com/Forsari0)

See [here](https://github.com/x64dbg/x64dbg/graphs/contributors) for a more up-to-date list of contributers.