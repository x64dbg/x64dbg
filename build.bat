@echo off

echo Saving PATH
if "%OLDPATH%"=="" set OLDPATH=%PATH%

cd %~dp0

if /i "%1"=="coverity"	goto coverity
if /i "%1"=="doxygen"	call setenv.bat doxygen&goto doxygen
if /i "%1"=="chm"	call setenv.bat chm&goto chm

set CONFIGURATION=release
if /i "%2"=="debug" set CONFIGURATION=debug
if /i "%1"=="x32"	call setenv.bat x32&set type=Configuration=%CONFIGURATION%;Platform=Win32&goto build
if /i "%1"=="x64"	call setenv.bat x64&set type=Configuration=%CONFIGURATION%;Platform=x64&goto build


goto usage


:build
echo Building DBG...
msbuild.exe x64dbg.sln /m /verbosity:minimal /t:Rebuild /p:%type%
if %ERRORLEVEL% neq 0 goto restorepath

echo Building GUI...
rmdir /S /Q src\gui_build
cd src\gui
qmake x64dbg.pro CONFIG+=%CONFIGURATION%
jom
cd ..\..
goto :restorepath


:coverity
if /i "%2"=="x32" goto coverity_ok
if /i "%2"=="x64" goto coverity_ok
goto usage

:coverity_ok
call setenv.bat coverity
echo Building with Coverity
cov-configure --msvc
cov-build --dir cov-int --instrument build.bat %2%
goto :restorepath


:doxygen
doxygen
goto :restorepath


:chm
start /w "" winchm.exe help\x64_dbg.wcp /h
goto :restorepath


:usage
echo "Usage: build.bat x32/x64 [debug|release] | coverity (x32/x64) | doxygen | chm"
echo.
echo Examples:
echo   build.bat x32               : builds 32-bit release build
echo   build.bat x64 debug         : builds 64-bit debug build
echo   build.bat coverity x32      : builds 32-bit coverity build
echo   build.bat coverity x64      : builds 64-bit coverity build
echo   build.bat doxygen           : generate doxygen documentation
echo   build.bat chm               : generate windows help format documentation
goto :restorepath

:restorepath
echo Resetting PATH
set PATH=%OLDPATH%
set OLDPATH=