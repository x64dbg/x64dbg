@echo off
echo Run this to update translation templates after the source is modified. Be sure to set Qt path in setenv.bat

call setenv.bat x64
lupdate src/gui/x64dbg.pro
lrelease src/gui/x64dbg.pro
