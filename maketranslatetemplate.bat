@echo off
echo Run this to update translation templates after the source is modified. Be sure to set Qt path in setenv.bat

call setenv.bat x64
lupdate src/x64dbg_translations.pro