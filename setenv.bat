@echo off

if "%QT32PATH%"=="" set QT32PATH=c:\Qt\qt-4.8.6-x86-msvc2013\qt-4.8.6-x86-msvc2013\bin
if "%QT64PATH%"=="" set QT64PATH=c:\Qt\qt-4.8.6-x64-msvc2013\qt-4.8.6-x64-msvc2013\bin
if "%QTCREATORPATH%"=="" set QTCREATORPATH=c:\Qt\qtcreator-3.1.1\bin
if "%COVERITYPATH%"=="" set COVERITYPATH=c:\coverity\bin
if "%DOXYGENPATH%"=="" set DOXYGENPATH=C:\Program Files\doxygen\bin
if "%CHMPATH%"=="" set CHMPATH=c:\Program Files (x86)\Softany\WinCHM

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
call :vc-set-2015
goto :eof

:x64
echo Setting Qt in PATH
set PATH=%PATH%;%QT64PATH%
set PATH=%PATH%;%QTCREATORPATH%
echo Setting VS in PATH
call :vc-set-2015 amd64
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




@rem Look for Visual Studio 2015
:vc-set-2015
if not defined VS140COMNTOOLS goto vc-set-2013
if not exist "%VS140COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2013
if "%VCVARS_VER%" NEQ "140" (
  call "%VS140COMNTOOLS%\..\..\vc\vcvarsall.bat" %1%
  SET VCVARS_VER=140
)
if not defined VCINSTALLDIR goto msbuild-not-found
set GYP_MSVS_VERSION=2015
goto msbuild-found

@rem Look for Visual Studio 2013
:vc-set-2013
if not defined VS120COMNTOOLS goto goto msbuild-not-found
if not exist "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" goto msbuild-not-found
if "%VCVARS_VER%" NEQ "120" (
  call "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" %1%
  SET VCVARS_VER=120
)
if not defined VCINSTALLDIR goto msbuild-not-found
set GYP_MSVS_VERSION=2013
goto msbuild-found

:msbuild-found
goto :EOF

:msbuild-not-found
echo ERROR: Visual Studio 2013/2015 wasn't found!
exit 1