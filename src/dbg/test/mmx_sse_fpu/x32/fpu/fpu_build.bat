@echo off
nasm -f win32 fpu.asm
golink fpu.obj /entry _start
del /Q *.obj
move fpu.exe ..\fpu.exe > nul
pause