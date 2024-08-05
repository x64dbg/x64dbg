@echo off
nasm -f win32 highlighting.asm
golink highlighting.obj /entry _start
del /Q *.obj
pause