@echo off
nasm -f win64 highlighting.asm
golink highlighting.obj /entry _start
del /Q *.obj
pause