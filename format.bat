@echo off
call %~dp0.github\format\install-git-hook.bat
echo Formatting code...
%~dp0.github\format\AStyleHelper.exe Silent
