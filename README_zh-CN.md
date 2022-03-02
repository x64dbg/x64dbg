# x64dbg

<img width="100" src="https://github.com/x64dbg/x64dbg/raw/development/src/bug_black.png"/>

[![赏金来源](https://www.bountysource.com/badge/team?team_id=18188&style=raised)](https://github.com/sponsors/mrexodia) [![构建状态](https://ci.appveyor.com/api/projects/status/h1j489qa1mx67e0h?svg=true)](https://ci.appveyor.com/project/mrexodia/x64dbg) [![开放源码](https://www.codetriage.com/x64dbg/x64dbg/badges/users.svg)](https://www.codetriage.com/x64dbg/x64dbg) [![Crowdin](https://d322cqt584bo4o.cloudfront.net/x64dbg/localized.svg)](http://translate.x64dbg.com) [![下载x64dbg](https://img.shields.io/sourceforge/dm/x64dbg.svg)](https://sourceforge.net/projects/x64dbg/files/latest/download)

[![Telegram](https://img.shields.io/badge/chat-%20on%20Telegram-blue.svg)](https://telegram.me/x64dbg) [![Discord](https://img.shields.io/badge/chat-on%20Discord-green.svg)](https://invite.gg/x64dbg) [![Slack](https://img.shields.io/badge/chat-on%20Slack-red.svg)](https://x64dbg-slack.herokuapp.com) [![Gitter](https://img.shields.io/badge/chat-on%20Gitter-lightseagreen.svg)](https://gitter.im/x64dbg/x64dbg) [![Freenode](https://img.shields.io/badge/chat-%20on%20freenode-brightgreen.svg)](https://webchat.freenode.net/?channels=x64dbg) [![Matrix](https://img.shields.io/badge/chat-on%20Matrix-yellowgreen.svg)](https://riot.im/app/#/room/#x64dbg:matrix.org) [![XMPP](https://img.shields.io/badge/chat-%20on%20XMPP-orange.svg)](https://inverse.chat/#converse/room?jid=x64dbg@conference.jwchat.org)

一个开源的Windows二进制调试器，旨在进行恶意软件分析和你没有源代码的可执行文件的逆向工程。有许多可用的功能和一个全面的[插件系统](http://plugins.x64dbg.com) 来添加你自己的功能。你可以在[博客](https://x64dbg.com/blog)!上找到更多信息!

## 屏幕截图

![主界面](https://i.imgur.com/V2f5AP9.png)

![函数图像](https://i.imgur.com/gVjzntJ.png) ![内存布局](https://i.imgur.com/cLJwTjY.png)

## 安装与使用

1. 从[GitHub](https://github.com/x64dbg/x64dbg/releases)、[SourceForge](https://sourceforge.net/projects/x64dbg/files/snapshots) 或 [OSDN](https://osdn.net/projects/x64dbg) 下载快照，并将其解压缩到您的用户具有写权限的位置。
2. _可选择_ 使用 `x96dbg.exe` 来注册一个shell扩展，并在桌面上添加快捷方式。
3. 如果你想调试一个32位的可执行文件，你现在可以运行 `x32\x32dbg.exe` 或者 `x64\x64dbg.exe`来调试一个64位的可执行文件。如果你不确定，你可以随时运行 `x96dbg.exe` 并在那里选择你的架构。

你也可以通过几个简单的步骤 [自己编译](https://github.com/x64dbg/x64dbg/wiki/Compiling-the-whole-project) x64dbg!

## 贡献

这是一个社区的努力，我们接受拉取请求! 更多信息请参见  [CONTRIBUTING](https://github.com/x64dbg/x64dbg/blob/development/CONTRIBUTING.md) 文档。如果你有任何问题，你可以随时[联系我们](https://x64dbg.com/#contact) 或提交一个[问题](https://github.com/x64dbg/x64dbg/issues). 你可以看一下 [容易解决的问题](https://github.com/x64dbg/x64dbg/issues?q=is%3Aissue+is%3Aopen+label%3Aeasy) 以开始贡献。

## 制作人员

- 调试器核心由[TitanEngine社区版](https://github.com/x64dbg/TitanEngine)提供技术支持
- 反汇编引擎由 [Zydis](https://zydis.re)提供技术支持
- 组件由 [XEDParse](https://github.com/x64dbg/XEDParse) 和 [asmjit](https://github.com/asmjit)提供技术支持
- 导入重建由 [Scylla](https://github.com/NtQuery/Scylla)提供技术支持
- JSON由 [Jansson](https://www.digip.org/jansson)提供技术支持
- 数据库优化由 [lz4](https://bitbucket.org/mrexodia/lz4)提供技术支持
- Bug 图标由 [VisualPharm](https://www.visualpharm.com)设计
- 界面图标由[Fugue](https://p.yusukekamiyamane.com)设计
- 网站由[tr4ceflow](https://tr4ceflow.com)负责

## 开发人员

- [mrexodia](https://mrexodia.github.io)
- Sigma
- [tr4ceflow](https://blog.tr4ceflow.com)
- [Dreg](https://www.fr33project.org)
- [Nukem](https://github.com/Nukem9)
- [Herz3h](https://github.com/Herz3h)
- [torusrxxx](https://github.com/torusrxxx)

## 代码贡献

你可以在[这里](https://github.com/x64dbg/x64dbg/graphs/contributors)找到一份详尽的GitHub贡献者名单.

## 特别感谢

- Sigma开发了初始图形用户界面
- 所有的[捐赠者](https://www.bountysource.com/teams/x64dbg/backers)!
- 每一个提交问题的人!
- 我忘记添加到这个名单的人
- [博客文章作者](https://x64dbg.com/blog/2016/07/09/Looking-for-writers.html)!
- [EXETools 社区](https://forum.exetools.com)
- [Tuts4You 社区](https://forum.tuts4you.com)
- [ReSharper](https://www.jetbrains.com/resharper)
- [Coverity](https://www.coverity.com)
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
- [kao](https://lifeinhex.com)
- sstrato
- [kobalicek](https://github.com/kobalicek)
- [athre0z](https://github.com/athre0z)
- [ZehMatt](https://github.com/ZehMatt)

如果没有许多人和其他开源项目的帮助，就不可能使x64dbg成为今天的样子，谢谢你们!
