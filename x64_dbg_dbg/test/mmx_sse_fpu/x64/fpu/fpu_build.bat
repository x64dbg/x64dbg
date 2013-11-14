@echo off
nasm -f win64 fpu.asm
golink fpu.obj /entry _start
del /Q *.obj
move fpu.exe ..\fpu.exe > nul
pause