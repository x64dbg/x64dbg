@echo off
echo cleaning base directory...
del /Q *.sdf
del /Q *.layout
del /Q /A H *.suo
rmdir /S /Q ipch
rmdir /S /Q release
echo cleaning x64_dbg_bridge...
cd x64_dbg_bridge
rmdir /S /Q obj
rmdir /S /Q Win32
rmdir /S /Q x64
del /Q *.bmarks
del /Q *.layout
del /Q *.depend
cd ..
echo cleaning x64_dbg_dbg...
cd x64_dbg_dbg
rmdir /S /Q obj
rmdir /S /Q Win32
rmdir /S /Q x64
del /Q *.bmarks
del /Q *.layout
del /Q *.depend
cd ..
echo cleaning x64_dbg_exe...
cd x64_dbg_exe
rmdir /S /Q obj
rmdir /S /Q Win32
rmdir /S /Q x64
del /Q *.bmarks
del /Q *.layout
del /Q *.depend
cd ..
echo cleaning x64_dbg_gui...
cd x64_dbg_gui
rmdir /S /Q bin
rmdir /S /Q Project\GeneratedFiles
rmdir /S /Q Project\release
rmdir /S /Q Project\debug
rmdir /S /Q Project\Win32
rmdir /S /Q Project\x64
del /Q Project\Src\Bridge\libx32_bridge.a
del /Q Project\Src\Bridge\libx64_bridge.a
del /Q Project\Src\Bridge\x32_bridge.lib
del /Q Project\Src\Bridge\x64_bridge.lib
cd ..
echo cleaning bin\
del /Q bin\*.pdb
del /Q bin\*.exp
del /Q bin\*.a
del /Q bin\*.lib
del /Q bin\*.def
del /Q bin\x96_dbg.exe
echo cleaning bin\x32...
rmdir /S /Q bin\x32\db
del /Q bin\x32\*.pdb
del /Q bin\x32\*.exp
del /Q bin\x32\*.a
del /Q bin\x32\*.lib
del /Q bin\x32\*.def
del /Q bin\x32\x32_dbg.exe
del /Q bin\x32\x32_dbg.dll
del /Q bin\x32\x32_gui.dll
del /Q bin\x32\x32_bridge.dll
echo cleaning bin\x64...
rmdir /S /Q bin\x64\db
del /Q bin\x64\*.pdb
del /Q bin\x64\*.exp
del /Q bin\x64\*.a
del /Q bin\x64\*.lib
del /Q bin\x64\*.def
del /Q bin\x64\x64_dbg.exe
del /Q bin\x64\x64_dbg.dll
del /Q bin\x64\x64_gui.dll
del /Q bin\x64\x64_bridge.dll
echo cleaning help...
cd help
del /Q *.chm
rmdir /S /Q output
exit 0