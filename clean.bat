@echo off

echo Cleaning base directory...
del /Q *.sdf
del /Q *.layout
del /Q /A H *.suo
rmdir /S /Q ipch
rmdir /S /Q release
rmdir /S /Q build
rmdir /S /Q cov-int

echo Cleaning BRIDGE...
cd src\bridge
call :delfiles

echo Cleaning DBG...
cd src\dbg
call :delfiles

echo Cleaning EXE...
cd src\exe
call :delfiles

echo Cleaning LAUNCHER...
cd src\launcher
call :delfiles

echo Cleaning GUI SRC...
rmdir /S /Q src\gui_build

echo Cleaning GUI...
cd src/gui
rmdir /S /Q build
del /Q Makefile*
del /Q *.pdb
cd ..\..

echo Cleaning bin\
del /Q bin\*.pdb
del /Q bin\*.exp
del /Q bin\*.a
del /Q bin\*.lib
del /Q bin\*.def
del /Q bin\x96dbg.exe

echo Cleaning bin\x32...
rmdir /S /Q bin\x32\db
del /Q bin\x32\*.pdb
del /Q bin\x32\*.exp
del /Q bin\x32\*.a
del /Q bin\x32\*.lib
del /Q bin\x32\*.def
del /Q bin\x32\x32dbg.exe
del /Q bin\x32\x32dbg.dll
del /Q bin\x32\x32gui.dll
del /Q bin\x32\x32bridge.dll

echo Cleaning bin\x64...
rmdir /S /Q bin\x64\db
del /Q bin\x64\*.pdb
del /Q bin\x64\*.exp
del /Q bin\x64\*.a
del /Q bin\x64\*.lib
del /Q bin\x64\*.def
del /Q bin\x64\x64dbg.exe
del /Q bin\x64\x64dbg.dll
del /Q bin\x64\x64gui.dll
del /Q bin\x64\x64bridge.dll

echo Cleaning help...
cd help
del /Q *.chm
rmdir /S /Q output

echo Done!
exit 0

:delfiles
rmdir /S /Q obj
rmdir /S /Q Win32
rmdir /S /Q x64
del /Q *.bmarks
del /Q *.layout
del /Q *.depend
del /Q *.pdb
cd ..\..
