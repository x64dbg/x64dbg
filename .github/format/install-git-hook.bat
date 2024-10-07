@echo off
git config core.autocrlf false
set PRE_COMMIT_HOOK=%~dp0..\..\.git\hooks\pre-commit
if not exist "%PRE_COMMIT_HOOK%" (
    echo Installing pre-commit hook...
    copy %~dp0pre-commit %PRE_COMMIT_HOOK%
)