# x64dbg

[![BountySource](https://www.bountysource.com/badge/team?team_id=18188&style=raised)](https://www.bountysource.com/teams/x64dbg?utm_source=x64dbg&utm_medium=shield&utm_campaign=raised) [![Telegram](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAHQAAAAUCAYAAABcbhl9AAAABHNCSVQICAgIfAhkiAAAAAlwSFlzAAAOxAAADsQBlSsOGwAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAAjsSURBVGiB7Zl7cFTlGcZ/3zl7zW52s5vdXMlSEoKEgoyipdJBOrFSB7QVGBgwiB3FadXJ9B/Ajraoo4M6046OwQ5oxzik1HakzkiVYKnY6qgBahCMoCYEEsid7Cawl+zlnK9/bLJJmiuOTkinz8zZy3fey/O9z/nO+eY9ory83Nzb2/sksBHIZYrw1rUPTVXq/wW0CajqDtp/YwgEAk8KIbZONSMp5VRTmM7IlbDNZQtKA3D31VDMq4HDdIdE3mOQUuZMNRH4v6DfEHIMV0shrxYe0x3KlTosXrz42+Bx1eGOOW5UIaaaxhXjildoeXk5NTU1V+Tj8/lYunQpe/fuHdPm21yhT98yi5XFbgBy00309CWIxHUAbn+tjube6AifXStmU1RxlERiet05xhVUURTcbjfBYJC+vr7UuJSStLQ0hBCEQqFhPk6nE0VRCAQCqTGTyYTP5xtXtIkEVYUgN91ERzBGXB9p67IaiGuSYEwbcW7bnw6z9bIfgA8eXceOvx2h+mRTMq5vHjkuB5eiGuH4oK+UOug6A7QUAXnpZjpDMWLa8Pwuq4FoQg7zHwqf08zFcIJwXCPNqOJJM9IejI6IA2A3qZhVhe5IHIAcu4lgTBt1XqNhTEFvvvlm1q1bx4ULF3C73ezZs4e6ujoANmzYQFFREXl5ebz55pscPHgQgO3bt6PrySvf4XDwzDPP4Pf7KSsrIz8/n0ceeYTW1lYqKytH5BtP0NJZLipWzOZMd5hij41H3z3LG6e7yLaZqNl8Pfu/ukixy0KJ18bWQ43sO9U1zF94ZyK8M/v/KIiMHJRCNyXeNF756Vy6glEKnBZeP9XFjg+ah3GSEn5U6OJ3y4to9IeZ7bHxxD/Pse9UFwLYubKYm/IdtFyO0hWOU+K1cdMfalmQZaNqdQnnevqwqvDbjy5QkmVnTYmHrlCUeVl2tr93LsX1zC8Xc7DBT2GGhdluK89+eJ7rcu0UOs1c47Xx0Nv1VDf4v56g2dnZlJWVsXnzZgKBALqu43a7UVUVgBMnTvDCCy/g9XrZvXs31dXVAGzdupVoNHn7WrNmDStWrKCqqordu3ezadMmtmzZgsFgwOl0TlpQoyLYtXI2ZS/up6bZT7HLwr9+fRf/aPQjEzG8NiN73qvl6JfNLPa5ePG+Fbz+eee4k5Yyme+llcX86o+HOFzfjkFP8PFjG9n/ZRqfdYRSdnaTQsVtRdz69J9puhQl0yyoeWIT1fXdLClwssBlYNGjr5AQBp5cfRNzHflIKdFjYWZlWLjr92/x2fkuROYM3tejPPeXTlAN5FgVPnr87kGuiTjHTjfyi/dO8N1sO0ce38jal96l+ng9t16TzbZVyzhQ3/31BC0uLqa2tjYl5EDBB2yPHz+O2+0mFothNBoxGo3EYjFKS0tZtmwZLpcLi8VCa2srUkqsVitCiGGxJiuoz2FGanFqglZEfgkNsQiNHX7mZ5qpvxiiOxjhaLdEFMzjlBYh32Ee/9be/+kwqyzIsVO6aD6l318ESDQtzvUeEyfbgylOC71WjELn/jt+CIoBpIaia8xxmbnBa+DgybNoM+YjhOCtz9tYXpIUFAmNnQHqojZEQRYAeRlmtqxfypxMKxZV4LGZcJpVevoSALxzxo8omEd9IoIQcOh8CFEwj68SMfLtxkntM0YVNB6PoygKiqIMCzLwe2BcURR0XUdKic/nY/369Tz88MO0tLSwZMkSVq9enfIZekGMWugxzumxaFIFS3rSxmgBoaDF+pBSEk/oYLEnV4XBgiLEBBNP8lD0BPGEzhvHvkg9J/dFglzQrUjVOcgp1sfFyxH++vHnqQj7DvXQoHi4JdeAzWJFEgIpMdgdg34SQrEE0pTGQIK9a+bx7IF/88DRL9C0BF0VP8eIluIbM9qQUYmmWtB1ScLsACnRVROKmNzGURko9NCjrq6OhQsX4na7U2Oqqo4Qd6hYGRkZtLW1EQqF8Hg8LF++PHVuYGy0XBMd5wJhABbPcCClZE6mle94HNS1XaK/boP2QwQb6xjg1B2K8UVrN4Uz8qgNQG0ATicchFT7MLvjLT1kpltJWBwpuybVS1A3cPirNlYvnEmGWQUpufcG32BtkIAYlrvAaeaDljB63lxuK/0hDqsZ2b8gJCCH2CdnMrLOEx2jrlC/38/OnTt56qmnaG1txel0UllZmdoUjebz6aefsnbtWnbs2IGiKDQ1NWGz2ZBS0tTURHNzM88//zwNDQ1UVFSM8B/r6osrRu59uZpX77+dc71RilwWHqp8hx49jewhE01+jx9rsEogFZV7Xv47rz5wOw/+oIhoQifHbmLNaydoCPSl4vRKIz/b9TZ771vJhcsxTKogw2Lgxl3HONIR4qXDx/no/htpD8b48PQ5ihxZI0QYwHMHjvB++S3U+yP0BMN0XgqNLpgYPg+JSIk7EcSqVatGtYpEIoTDYXJzc/H7/ZhMJkwmE+3t7eTkDHYLOzo6yMpKPiN6enrweDx0d3cjhEDXdVwuFwCBQABN0zAajaNuisZ92xLuRXSeJdfrpqM7gOYuALsbtDiy6SSicFF/BXTkmU8Qs28cM5RsrkNk5oPNBbEIsr0BT7oNk9FAe3cPMm8OGC3IhmOIokUgFAj3IjvPkudxk9ASdPYEETMXAAIZaIWeDjCYeODH3+OGfCebD7VCNIzsOIPwLRjM7W8hU7tMQjXSG9Uhcgkx81pQjcjGT1K/QSLrjyKK+5s4iSjy/CnErOvGrlE/xJ133jmu7JqmoSgKYpJdE03TUrvhK8HEr88kJOJgMALfcAdHS4DUJ46diIEQ/UVPYt+GhbT09uGwGFgyI52fVBzgS+keP5cQoFx5jSaDCTtFipLsDk62k/PfG6nJYlI+qrF/m/oNd28UFVAnjj0g5BCum3ftZ35BNtFYjAcbmwlnFoJpnBgDQn6NGk0GBillG1P4YnsA07U577fl8n5bJLnqsucmb9FTN5cWg67rVUKIbVPFYADTVVCEChb74P8pnIdAVhk0Tdve/8zbCORNFZlpK+jVgVYQVdJ0+bH/AJyZ4rOySnpvAAAAAElFTkSuQmCC)](https://telegram.me/joinchat/BzwLaQcORqjkM1k9YbTNmg) [![Join the chat at Gitter](https://badges.gitter.im/x64dbg/x64dbg.svg)](https://gitter.im/x64dbg/x64dbg?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) [![Jenkins](http://jenkins.x64dbg.com/job/vs13/badge/icon)](http://jenkins.x64dbg.com) [![Stories in Ready](https://badge.waffle.io/x64dbg/x64dbg.png?label=ready&title=Ready)](http://waffle.io/x64dbg/x64dbg)



## Note

Please run `install.bat` before you start committing code, this ensures your code is auto-formatted to the *x64dbg* [standards](https://github.com/x64dbg/x64dbg/wiki/Coding-Guidelines).

## Compiling

For a complete guide on compiling *x64dbg* read [this](https://github.com/x64dbg/x64dbg/wiki/Compiling the whole project).

## Downloads

Releases of *x64dbg* can be found [here](http://releases.x64dbg.com).

Jenkins build server can be found [here](http://jenkins.x64dbg.com).

## Overview

*x64dbg* is an open-source x32/x64 debugger for Windows.

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
- Built-in assembler (XEDParse)
- Executable patching
- Yara Pattern Matching
- Decompiler (Snowman)
- Analysis

## License

*x64dbg* is licensed under GPLv3, which means you can freely distribute and/or modify the source of *x64dbg*, as long as you share your changes with us. The only exception is that plugins you write do not have to comply with the GPLv3 license. They do not have to be open-source and they can be commercial and/or private. The only exception to this is when your plugin uses code copied from *x64dbg*. In that case you would still have to share the changes to *x64dbg* with us.

## Credits

- Debugger core by [TitanEngine Community Edition](https://bitbucket.org/titanengineupdate/titanengine-update)
- Disassembly powered by [Capstone](http://capstone-engine.org)
- Assembly powered by [XEDParse](https://bitbucket.org/mrexodia/xedparse)
- Import reconstruction powered by [Scylla](https://github.com/NtQuery/Scylla)
- JSON powered by [Jansson](http://www.digip.org/jansson)
- Database compression powered by [lz4](https://bitbucket.org/mrexodia/lz4)
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
- _pusher_
- firelegend

## Developers

- [mrexodia](http://mrexodia.cf)
- Sigma
- [tr4ceflow](http://blog.tr4ceflow.com)
- [Dreg](http://www.fr33project.org)
- [Nukem](https://github.com/Nukem9)
- [Herz3h](https://github.com/Herz3h)

## Contributers

- [torusrxxx](https://github.com/torusrxxx)
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
- [fetzerms](https://github.com/RadicalRaccoon)
- [muratsu](https://github.com/RadicalRaccoon)
- [ForNeVeR](https://github.com/RadicalRaccoon)
