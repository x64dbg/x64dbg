@echo off

echo Saving PATH
if "%OLDPATH%"=="" set OLDPATH=%PATH%

cd %~dp0

if /i "%1"=="x32"	call setenv.bat x32&set type=Configuration=Release;Platform=Win32&goto build
if /i "%1"=="x64"	call setenv.bat x64&set type=Configuration=Release;Platform=x64&goto build
if /i "%1"=="coverity"	goto coverity
if /i "%1"=="doxygen"	call setenv.bat doxygen&goto doxygen

goto usage


:build
echo Building DBG...
if "%MAXCORES%"=="" (
    msbuild.exe x64dbg.sln /m /verbosity:minimal /t:Rebuild /p:%type%
) else (
    set CL=/MP%MAXCORES%
    msbuild.exe x64dbg.sln /m:1 /verbosity:minimal /t:Rebuild /p:%type%
)

echo Building GUI...
rmdir /S /Q src\gui_build
cd src\gui
qmake x64dbg.pro CONFIG+=release
if "%MAXCORES%"=="" (
    jom
) else (
    jom /J %MAXCORES%
)
cd ..\..
goto :restorepath


:coverity
if "%2"=="" (
    echo "Usage: build.bat coverity x32/x64"
    goto usage
)

call setenv.bat coverity
echo Building with Coverity
cov-configure --msvc
cov-build --dir cov-int --instrument build.bat %2%
goto :restorepath


:doxygen
doxygen
goto :restorepath


:usage
echo "Usage: build.bat x32/x64/coverity (x32/x64)/doxygen/chm"
echo.
echo Examples:
echo   build.bat x32               : builds 32-bit release build
echo   build.bat x64               : builds 64-bit release build
echo   build.bat coverity x32      : builds 32-bit coverity build
echo   build.bat coverity x64      : builds 64-bit coverity build
echo   build.bat doxygen           : generate doxygen documentation
echo   build.bat chm               : generate windows help format documentation
goto :restorepath

:restorepath
echo Resetting PATH
set PATH=%OLDPATH%
set OLDPATH=