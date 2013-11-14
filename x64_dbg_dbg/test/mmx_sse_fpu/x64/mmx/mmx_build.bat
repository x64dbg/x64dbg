@echo off
nasm -fwin64 mmx.asm
golink mmx.obj /entry _start
del /Q *.obj
move mmx.exe ..\mmx.exe > nul
pause