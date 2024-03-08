@echo off
echo Run this to update translation templates after the source is modified. Be sure to set Qt path in setenv.bat

call setenv.bat x64
if not exist bin\translations mkdir bin\translations
git ls-files *.java *.jui *.ui *.c *.c++ *.cc *.cpp *.cxx *.ch *.h *.h++ *.hh *.hpp *.hxx *.js *.qs *.qml *.qrc > bin\translations\files.lst
lupdate @bin\translations\files.lst -locations absolute -ts x64dbg.ts
move /Y x64dbg.ts bin\translations\