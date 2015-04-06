@echo off
echo Setting Qt in PATH
set PATH=%PATH%;"c:\Qt\qt-4.8.6-x64-msvc2013\qt-4.8.6-x64-msvc2013\bin"
set PATH=%PATH%;"c:\Qt\qtcreator-3.1.1\bin"
echo Setting VS in PATH
call "c:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64