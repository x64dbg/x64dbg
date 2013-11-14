@echo off
nasm -f win64 pferrie.asm
golink pferrie.obj /entry _start
del /Q *.obj
pause