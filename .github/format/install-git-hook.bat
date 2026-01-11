@echo off
git config core.autocrlf false

REM Get the git directory path (works for worktrees too)
for /f "delims=" %%i in ('git rev-parse --git-dir') do set GIT_DIR=%%i

REM Replace forward slashes with backslashes
set GIT_DIR=%GIT_DIR:/=\%

set PRE_COMMIT_HOOK=%GIT_DIR%\hooks\pre-commit

REM Create the hooks directory if it doesn't exist
if not exist "%GIT_DIR%\hooks" (
    mkdir "%GIT_DIR%\hooks"
)

if not exist "%PRE_COMMIT_HOOK%" (
    echo Installing pre-commit hook...
    copy "%~dp0pre-commit" "%PRE_COMMIT_HOOK%"
)
