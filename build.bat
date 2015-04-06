@echo off

echo Saving PATH
if "%OLDPATH%"=="" set OLDPATH=%PATH%

if "%1%"=="x32" (
    call setenv.bat x32
    set type="Win32"
) else if "%1%"=="x64" (
    call setenv.bat x64
    set type="x64"
) else if "%1%"=="coverity" (
    if "%2"=="" (
        echo "usage: build.bat coverity x32/x64"
        goto :eof
    )
    call setenv.bat coverity
    echo Building with Coverity
    cov-configure --msvc
    cov-build --dir cov-int --instrument build.bat %2%
    goto :eof
) else (
    echo "usage: build.bat coverity/x32/x64"
    goto :eof
)

echo Building DBG...
devenv /Rebuild "Release|%type%" x64_dbg.sln

echo GUI prebuildStep
cd x64_dbg_gui\Project
cmd /k "prebuildStep.bat %1%"
cd ..
cd ..

echo Building GUI...
rmdir /S /Q build
mkdir build
cd build
qmake ..\x64_dbg_gui\Project\x64_dbg.pro CONFIG+=release
jom
cd ..

echo GUI afterbuildStep
cd x64_dbg_gui\Project
call afterbuildStep.bat %1% ..\..\build\release
cd ..
cd ..

echo Resetting PATH
set PATH=%OLDPATH%
set OLDPATH=