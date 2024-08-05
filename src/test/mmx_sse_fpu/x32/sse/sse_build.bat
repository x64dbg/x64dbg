@echo off
nasm -fwin32 sse.asm
golink sse.obj /entry _start
del /Q *.obj
move sse.exe ..\sse.exe > nul
pause