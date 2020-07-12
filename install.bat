@echo off
echo Run this to install the auto-format hook.
git config core.autocrlf false
copy hooks\pre-commit .git\hooks\pre-commit