@echo off
nasm -fwin64 sse.asm
golink sse.obj /entry _start
move sse.exe ..\sse.exe > nul
del /Q *.obj
pause