@echo off

if "%OLDPATH%"=="" set OLDPATH=%PATH%

if "%1%"=="x32" (
    goto x32
) else if "%1%"=="x64" (
    goto x64
) else if "%1%"=="coverity" (
    goto coverity
) else if "%1%"=="doxygen" (
    goto doxygen
) else (
    echo "usage: setenv x32/x64/coverity/doxygen"
    goto :eof
)

:x32
echo Setting Qt in PATH
set PATH=%PATH%;"c:\Qt\qt-4.8.6-x86-msvc2013\qt-4.8.6-x86-msvc2013\bin"
set PATH=%PATH%;"c:\Qt\qtcreator-3.1.1\bin"
echo Setting VS in PATH
call "c:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"
goto :eof

:x64
echo Setting Qt in PATH
set PATH=%PATH%;"c:\Qt\qt-4.8.6-x64-msvc2013\qt-4.8.6-x64-msvc2013\bin"
set PATH=%PATH%;"c:\Qt\qtcreator-3.1.1\bin"
echo Setting VS in PATH
call "c:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64
goto :eof

:coverity
echo Setting Coverity in PATH
set PATH=%PATH%;"c:\coverity\bin"
goto :eof

:doxygen
echo Setting Doxygen in PATH
set PATH=%PATH%;"C:\Program Files\doxygen\bin"
goto :eof