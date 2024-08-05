@echo off
echo Run this to install the auto-format hook.
git config core.autocrlf false
copy %~dp0pre-commit %~dp0..\..\.git\hooks\pre-commit