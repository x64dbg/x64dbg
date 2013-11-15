@echo off
rem Working directory: '%{sourceDir}'
rem argument1: architecture 'x32' or 'x64'
set result=1
copy ..\..\bin\%1\lib%1_bridge.a Src\Bridge\
if %ERRORLEVEL%==0 set result=0
copy ..\..\bin\%1\%1_bridge.lib Src\Bridge\
if %ERRORLEVEL%==0 set result=0
exit %result%