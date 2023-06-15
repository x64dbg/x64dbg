# x64dbg

<img width="100" src="./src/bug_black.png"/>

一个开源的Windows二进制调试器，旨在进行恶意软件分析和你没有源代码的可执行文件的逆向工程。有许多可用的功能和一个全面的插件系统 来添加你自己的功能。你可以在博客上找到更多信息!

## 屏幕截图

<!-- TODO: recreate Chinese screenshots -->

![main interface (light)](.github/screenshots/cpu-light.png)

![main interface (dark)](.github/screenshots/cpu-dark.png)

| ![graph](.github/screenshots/graph-light.png) | ![memory map](.github/screenshots/memory-map-light.png) |
| :--: | :--: |

## 安装与使用

1. 下载快照，并将其解压缩到您的用户具有写权限的位置。
2. _可选择_ 使用 `x96dbg.exe` 来注册一个shell扩展，并在桌面上添加快捷方式。
3. 如果你想调试一个32位的可执行文件，你现在可以运行 `x32\x32dbg.exe` 或者 `x64\x64dbg.exe`来调试一个64位的可执行文件。如果你不确定，你可以随时运行 `x96dbg.exe` 并在那里选择你的架构。

你也可以通过几个简单的步骤 自己编译 x64dbg!

## 贡献

这是一个社区的努力，我们接受拉取请求! 更多信息请参见  CONTRIBUTING 文档。如果你有任何问题，你可以随时联系我们 或提交一个问题. 你可以看一下 容易解决的问题 以开始贡献。

## 制作人员

- 调试器核心由TitanEngine社区版提供技术支持
- 反汇编引擎由 Zydis提供技术支持
- 汇编由 XEDParse 和 asmjit提供技术支持
- 导入重建由 Scylla提供技术支持
- JSON由 Jansson提供技术支持
- 数据库优化由 lz4提供技术支持
- Bug 图标由 VisualPharm设计
- 界面图标由Fugue设计
- 网站由tr4ceflow负责

## 开发人员

- mrexodia
- Sigma
- tr4ceflow
- Dreg
- Nukem
- Herz3h
- torusrxxx

## 代码贡献

你可以在这里找到一份详尽的GitHub贡献者名单.

## 特别感谢

- Sigma开发了初始图形用户界面
- 所有的捐赠者!
- 每一个提交问题的人!
- 我忘记添加到这个名单的人
- 博客文章作者
- EXETools 社区
- Tuts4You 社区
- ReSharper
- Coverity
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
- kao
- sstrato
- kobalicek
- athre0z
- ZehMatt

如果没有许多人和其他开源项目的帮助，就不可能使x64dbg成为今天的样子，谢谢你们!
