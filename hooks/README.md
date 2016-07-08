# hooks

This contains two binaries `AStyleWhore.exe` and `AStyle.dll` to format any code before you commit. They are signed with GPG.

To verify:

```
gpg --verify AStyle.dll.sig AStyle.dll
gpg: Signature made 07/08/16 06:01:39 W. Europe Daylight Time using RSA key ID AA0073B4
gpg: Good signature from "Duncan Ogilvie <mr.exodia.tpodt@gmail.com>" [ultimate]

gpg --verify AStyleWhore.exe.sig AStyleWhore.exe
gpg: Signature made 07/08/16 06:01:42 W. Europe Daylight Time using RSA key ID AA0073B4
gpg: Good signature from "Duncan Ogilvie <mr.exodia.tpodt@gmail.com>" [ultimate]
```

The *key ID* should match the last 8 characters of the signature of [this commit](https://github.com/x64dbg/x64dbg/commit/c855c15fd79870312ea5b4a1fbf3cb0dd8ae6240).

Git hashes are:

```
git ls-files -s
100644 6ef20910c6ab4e94cc2270e289a5b73d712c9c50 0       AStyle.dll
100644 5ea8a5daf0580e030406cedb83fb73ca9c187138 0       AStyle.dll.sig
100644 10dd63522b059eb3a43c01b35b807a9d50b5034d 0       AStyleWhore.exe
100644 d910d7ff178703b1452bc717491e5c24b9db1945 0       AStyleWhore.exe.sig
100644 0706138f1ec594c0b5d41978900b9d45cd2d99d7 0       pre-commit
```

If you are unsure about the integrity of the files, don't hesitate to contact me (mrexodia). The source code is available at [BitBucket](https://bitbucket.org/mrexodia/astylewhore). The version of `AStyle.dll` is compiled from `AStyle_2.04_windows.zip` which came from [Sourceforge](https://sourceforge.net/projects/astyle/files/astyle/astyle%202.04/AStyle_2.04_windows.zip/download).