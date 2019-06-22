@echo off
git submodule update --init --recursive
mkdir bin\x32
xcopy deps\x32 bin\x32 /S /Y
mkdir bin\x64
xcopy deps\x64 bin\x64 /S /Y

mkdir bin\x32d
xcopy deps\x32 bin\x32d /S /Y
xcopy deps\x32d bin\x32d /S /Y
mkdir bin\x64d
xcopy deps\x64 bin\x64d /S /Y
xcopy deps\x64d bin\x64d /S /Y