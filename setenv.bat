@echo off

if "%OLDPATH%"=="" set OLDPATH=%PATH%

if "%QT32PATH%"=="" set QT32PATH=d:\Qt\Qt5.5.1_x86\5.5\msvc2013\bin
if "%QT64PATH%"=="" set QT64PATH=d:\Qt\Qt5.5.1_x64\5.5\msvc2013_64\bin
if "%QTCREATORPATH%"=="" set QTCREATORPATH=d:\Qt\Qt5.5.1_x64\Tools\QtCreator\bin
if "%VSVARSALLPATH%"=="" set VSVARSALLPATH=c:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat
if "%COVERITYPATH%"=="" set COVERITYPATH=c:\coverity\bin
if "%DOXYGENPATH%"=="" set DOXYGENPATH=C:\Program Files\doxygen\bin
if "%CHMPATH%"=="" set CHMPATH=d:\tools\winchm

if "%1"=="x32" (
    goto x32
) else if "%1"=="x64" (
    goto x64
) else if "%1"=="coverity" (
    goto coverity
) else if "%1"=="doxygen" (
    goto doxygen
) else if "%1"=="chm" (
    goto chm
) else (
    echo "Usage: setenv x32/x64/coverity/doxygen/chm"
    goto :eof
)

:x32
echo Setting Qt in PATH
set PATH=%PATH%;%QT32PATH%
set PATH=%PATH%;%QTCREATORPATH%
echo Setting VS in PATH
call "%VSVARSALLPATH%"
goto :eof

:x64
echo Setting Qt in PATH
set PATH=%PATH%;%QT64PATH%
set PATH=%PATH%;%QTCREATORPATH%
echo Setting VS in PATH
call "%VSVARSALLPATH%" amd64
goto :eof

:coverity
echo Setting Coverity in PATH
set PATH=%PATH%;%COVERITYPATH%
goto :eof

:doxygen
echo Setting Doxygen in PATH
set PATH=%PATH%;%DOXYGENPATH%
goto :eof

:chm
echo Setting CHM in PATH
set PATH=%PATH%;%CHMPATH%
goto :eof