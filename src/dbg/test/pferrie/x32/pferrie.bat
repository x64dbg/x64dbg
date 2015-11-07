@echo off
nasm -f win32 pferrie.asm
golink pferrie.obj /entry _start
del /Q *.obj
pause