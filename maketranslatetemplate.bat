@echo off
echo Run this to update translation templates after the source is modified. Be sure to set Qt path in setenv.bat

call setenv.bat x64
lupdate src/gui/x64dbg.pro
lrelease src/gui/x64dbg.pro
lupdate src/dbg/x64dbg_dbg_translations.pro
lrelease src/dbg/x64dbg_dbg_translations.pro
del src\gui\Translations\x64dbg.qm
del src\gui\Translations\x64dbg_dbg.qm