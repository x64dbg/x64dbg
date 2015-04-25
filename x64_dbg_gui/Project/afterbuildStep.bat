@echo off
rem Working directory: '%{sourceDir}'
rem argument1: architecture 'x32' or 'x64'
rem argument2: built binary directory '%{buildDir}\release' for example
copy %2\%1gui.dll ..\..\bin\%1\
copy %2\%1gui.pdb ..\..\bin\%1\