@echo off
cov-configure --msvc
cov-build --dir cov-int --instrument build_x64.bat

exit