@echo off
rem Working directory: '%{sourceDir}'
rem argument1: architecture 'x32' or 'x64'
rem argument2: built binary directory '%{buildDir}\release' for example
echo Copying %2\%1gui.dll to ..\..\bin\%1\
copy "%2\%1gui.dll" "..\..\bin\%1\"
echo Copying %2\%1gui.pdb to ..\..\bin\%1\
copy "%2\%1gui.pdb" "..\..\bin\%1\"
