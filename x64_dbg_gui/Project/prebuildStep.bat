@echo off
rem Working directory: '%{sourceDir}'
rem argument1: architecture 'x32' or 'x64'
copy ..\..\bin\%1\lib%1_bridge.a Src\Bridge\
if %ERRORLEVEL%==0 exit 0
copy ..\..\bin\%1\%1_bridge.lib Src\Bridge\