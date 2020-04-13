@echo off
nasm -fwin32 mmx.asm
golink mmx.obj /entry _start
del /Q *.obj
move mmx.exe ..\mmx.exe > nul
pause