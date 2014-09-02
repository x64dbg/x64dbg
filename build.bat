@echo off
coverity_setenv.bat

echo Building DBG...
devenv /Rebuild "Release|x64" x64_dbg.sln

echo Building GUI...
mkdir build
cd build
qmake ..\x64_dbg_gui\Project\x64_dbg.pro CONFIG+=release
jom