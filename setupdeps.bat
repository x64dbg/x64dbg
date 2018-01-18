@echo off
git submodule update --init --recursive
mkdir bin\x32
xcopy deps\x32 bin\x32 /S /Y
mkdir bin\x64
xcopy deps\x64 bin\x64 /S /Y