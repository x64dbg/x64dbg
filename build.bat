@echo off

if "%1%"=="x32" (
    echo Building x32
    set type="Win32"
) else if "%1%"=="x64" (
    echo Building x64
    set type="x64"
) else (
    echo error, invalid parameter
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