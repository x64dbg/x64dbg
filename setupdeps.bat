@echo off
git submodule update --init --recursive
mkdir bin\x86
xcopy deps\x86 bin\x86 /S /Y
mkdir bin\x64
xcopy deps\x64 bin\x64 /S /Y

mkdir bin\x86d
xcopy deps\x86 bin\x86d /S /Y
xcopy deps\x86d bin\x86d /S /Y
mkdir bin\x64d
xcopy deps\x64 bin\x64d /S /Y
xcopy deps\x64d bin\x64d /S /Y