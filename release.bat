@echo off
set RELEASEDIR=.\release
rmdir /S /Q %RELEASEDIR%
mkdir %RELEASEDIR%

echo pluginsdk

mkdir %RELEASEDIR%\pluginsdk
mkdir %RELEASEDIR%\pluginsdk\dbghelp
mkdir %RELEASEDIR%\pluginsdk\DeviceNameResolver
mkdir %RELEASEDIR%\pluginsdk\jansson
mkdir %RELEASEDIR%\pluginsdk\lz4
mkdir %RELEASEDIR%\pluginsdk\TitanEngine
mkdir %RELEASEDIR%\pluginsdk\XEDParse

xcopy src\dbg\dbghelp %RELEASEDIR%\pluginsdk\dbghelp /S /Y
xcopy src\dbg\DeviceNameResolver %RELEASEDIR%\pluginsdk\DeviceNameResolver /S /Y
xcopy src\dbg\jansson %RELEASEDIR%\pluginsdk\jansson /S /Y
xcopy src\dbg\lz4 %RELEASEDIR%\pluginsdk\lz4 /S /Y
xcopy src\dbg\TitanEngine %RELEASEDIR%\pluginsdk\TitanEngine /S /Y
del %RELEASEDIR%\pluginsdk\TitanEngine\TitanEngine.txt /F /Q
xcopy src\dbg\XEDParse %RELEASEDIR%\pluginsdk\XEDParse /S /Y
copy src\dbg\_plugin_types.h %RELEASEDIR%\pluginsdk\_plugin_types.h
copy src\dbg\_plugins.h %RELEASEDIR%\pluginsdk\_plugins.h
copy src\dbg\_scriptapi*.h %RELEASEDIR%\pluginsdk\_scriptapi*.h
copy src\dbg\_dbgfunctions.h %RELEASEDIR%\pluginsdk\_dbgfunctions.h
copy src\bridge\bridge*.h %RELEASEDIR%\pluginsdk\bridge*.h

genlib bin\x86\x86bridge.dll
copy x86bridge.a %RELEASEDIR%\pluginsdk\libx86bridge.a
del x86bridge.def
del x86bridge.a
copy bin\x86\x86bridge.lib %RELEASEDIR%\pluginsdk\x86bridge.lib

genlib bin\x86\x86dbg.dll
copy x86dbg.a %RELEASEDIR%\pluginsdk\libx86dbg.a
del x86dbg.def
del x86dbg.a
copy bin\x86\x86dbg.lib %RELEASEDIR%\pluginsdk\x86dbg.lib

genlib bin\x64\x64bridge.dll
copy x64bridge.a %RELEASEDIR%\pluginsdk\libx64bridge.a
del x64bridge.def
del x64bridge.a
copy bin\x64\x64bridge.lib %RELEASEDIR%\pluginsdk\x64bridge.lib

genlib bin\x64\x64dbg.dll
copy x64dbg.a %RELEASEDIR%\pluginsdk\libx64dbg.a
del x64dbg.def
del x64dbg.a
copy bin\x64\x64dbg.lib %RELEASEDIR%\pluginsdk\x64dbg.lib

echo release

mkdir %RELEASEDIR%\release
mkdir %RELEASEDIR%\release\translations
mkdir %RELEASEDIR%\release\x86
mkdir %RELEASEDIR%\release\x64

xcopy deps\x86 %RELEASEDIR%\release\x86 /S /Y
xcopy deps\x64 %RELEASEDIR%\release\x64 /S /Y

copy help\x64dbg.chm %RELEASEDIR%\release\
copy bin\x96dbg.exe %RELEASEDIR%\release\
copy bin\mnemdb.json %RELEASEDIR%\release\
copy bin\errordb.txt %RELEASEDIR%\release\
copy bin\exceptiondb.txt %RELEASEDIR%\release\
copy bin\ntstatusdb.txt %RELEASEDIR%\release\
copy bin\winconstants.txt %RELEASEDIR%\release\
xcopy src\gui\Translations\*.qm %RELEASEDIR%\release\translations /S /Y
copy bin\x86\x86bridge.dll %RELEASEDIR%\release\x86\
copy bin\x86\x86dbg.dll %RELEASEDIR%\release\x86\
copy bin\x86\x86dbg.exe %RELEASEDIR%\release\x86\
copy bin\x86\x86gui.dll %RELEASEDIR%\release\x86\
copy bin\x64\x64bridge.dll %RELEASEDIR%\release\x64\
copy bin\x64\x64dbg.dll %RELEASEDIR%\release\x64\
copy bin\x64\x64dbg.exe %RELEASEDIR%\release\x64\
copy bin\x64\x64gui.dll %RELEASEDIR%\release\x64\

echo "creating commithash.txt"
git rev-parse HEAD > %RELEASEDIR%\commithash.txt

exit 0