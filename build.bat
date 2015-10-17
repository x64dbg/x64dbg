@echo off

cd %~dp0

if /i "%1"=="x32"	call setenv.bat x32&set type="Release|Win32"&goto build
if /i "%1"=="x64"	call setenv.bat x64&set type="Release|x64"&goto build
if /i "%1"=="coverity"	goto coverity
if /i "%1"=="doxygen"	call setenv.bat doxygen&goto doxygen
if /i "%1"=="chm"	call setenv.bat chm&goto chm

goto usage


:build
echo Building DBG...
devenv /Rebuild %type% x64dbg.sln

echo Building GUI...
cd /src/gui
qmake x64dbg.pro CONFIG+=release
jom
cd ..
cd ..
goto :EOF


:coverity
    if "%2"=="" (
        echo "Usage: build.bat coverity x32/x64"
        goto usage
    )

call setenv.bat coverity
echo Building with Coverity
cov-configure --msvc
cov-build --dir cov-int --instrument build.bat %2%
goto :EOF


:doxygen
doxygen
goto :EOF


:chm
start /w "" winchm.exe help\x64_dbg.wcp /h
goto :EOF


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
goto :EOF