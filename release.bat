@echo off
set RELEASEDIR=.\release
rmdir /S /Q %RELEASEDIR%
mkdir %RELEASEDIR%

echo qt_base

mkdir %RELEASEDIR%\qt_base
mkdir %RELEASEDIR%\qt_base\x32
mkdir %RELEASEDIR%\qt_base\x32\platforms
mkdir %RELEASEDIR%\qt_base\x64
mkdir %RELEASEDIR%\qt_base\x64\platforms

copy bin\x32\Qt5Core.dll %RELEASEDIR%\qt_base\x32\
copy bin\x32\Qt5Gui.dll %RELEASEDIR%\qt_base\x32\
copy bin\x32\Qt5Widgets.dll %RELEASEDIR%\qt_base\x32\
copy bin\x32\Qt5Network.dll %RELEASEDIR%\qt_base\x32\
copy bin\x32\libeay32.dll %RELEASEDIR%\qt_base\x32\
copy bin\x32\ssleay32.dll %RELEASEDIR%\qt_base\x32\
copy bin\x32\platforms\qwindows.dll %RELEASEDIR%\qt_base\x32\platforms\
copy bin\x64\Qt5Core.dll %RELEASEDIR%\qt_base\x64\
copy bin\x64\Qt5Gui.dll %RELEASEDIR%\qt_base\x64\
copy bin\x64\Qt5Widgets.dll %RELEASEDIR%\qt_base\x64\
copy bin\x64\Qt5Network.dll %RELEASEDIR%\qt_base\x64\
copy bin\x64\libeay32.dll %RELEASEDIR%\qt_base\x64\
copy bin\x64\ssleay32.dll %RELEASEDIR%\qt_base\x64\
copy bin\x64\platforms\qwindows.dll %RELEASEDIR%\qt_base\x64\platforms\

echo bin_base

mkdir %RELEASEDIR%\bin_base
mkdir %RELEASEDIR%\bin_base\x32
mkdir %RELEASEDIR%\bin_base\x64

copy bin\x32\x32_bridge.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\x32_dbg.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\capstone.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\dbghelp.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\symsrv.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\DeviceNameResolver.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\Scylla.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\jansson.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\lz4.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\TitanEngine.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\XEDParse.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\asmjit.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\yara.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\snowman.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\keystone.dll %RELEASEDIR%\bin_base\x32\
copy bin\x32\ldconvert.dll %RELEASEDIR%\bin_base\x32\
copy bin\x64\x64_bridge.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\x64_dbg.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\capstone.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\dbghelp.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\symsrv.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\DeviceNameResolver.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\Scylla.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\jansson.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\lz4.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\TitanEngine.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\XEDParse.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\asmjit.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\yara.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\snowman.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\keystone.dll %RELEASEDIR%\bin_base\x64\
copy bin\x64\ldconvert.dll %RELEASEDIR%\bin_base\x64\

echo help

mkdir %RELEASEDIR%\help

copy help\x64dbg.chm %RELEASEDIR%\help

echo pluginsdk

mkdir %RELEASEDIR%\pluginsdk
mkdir %RELEASEDIR%\pluginsdk\capstone
mkdir %RELEASEDIR%\pluginsdk\dbghelp
mkdir %RELEASEDIR%\pluginsdk\DeviceNameResolver
mkdir %RELEASEDIR%\pluginsdk\jansson
mkdir %RELEASEDIR%\pluginsdk\lz4
mkdir %RELEASEDIR%\pluginsdk\TitanEngine
mkdir %RELEASEDIR%\pluginsdk\XEDParse
mkdir %RELEASEDIR%\pluginsdk\yara
mkdir %RELEASEDIR%\pluginsdk\yara\yara

xcopy src\capstone_wrapper\capstone %RELEASEDIR%\pluginsdk\capstone /S /Y
xcopy src\dbg\dbghelp %RELEASEDIR%\pluginsdk\dbghelp /S /Y
xcopy src\dbg\DeviceNameResolver %RELEASEDIR%\pluginsdk\DeviceNameResolver /S /Y
xcopy src\dbg\jansson %RELEASEDIR%\pluginsdk\jansson /S /Y
xcopy src\dbg\lz4 %RELEASEDIR%\pluginsdk\lz4 /S /Y
xcopy src\dbg\TitanEngine %RELEASEDIR%\pluginsdk\TitanEngine /S /Y
del %RELEASEDIR%\pluginsdk\TitanEngine\TitanEngine.txt /F /Q
xcopy src\dbg\XEDParse %RELEASEDIR%\pluginsdk\XEDParse /S /Y
xcopy src\dbg\yara %RELEASEDIR%\pluginsdk\yara /S /Y
copy src\dbg\_plugin_types.h %RELEASEDIR%\pluginsdk\_plugin_types.h
copy src\dbg\_plugins.h %RELEASEDIR%\pluginsdk\_plugins.h
copy src\dbg\_scriptapi*.h %RELEASEDIR%\pluginsdk\_scriptapi*.h
copy src\dbg\_dbgfunctions.h %RELEASEDIR%\pluginsdk\_dbgfunctions.h
copy src\bridge\bridge*.h %RELEASEDIR%\pluginsdk\bridge*.h

genlib bin\x32\x32bridge.dll
copy x32bridge.a %RELEASEDIR%\pluginsdk\libx32bridge.a
del x32bridge.def
del x32bridge.a
copy bin\x32\x32bridge.lib %RELEASEDIR%\pluginsdk\x32bridge.lib

genlib bin\x32\x32dbg.dll
copy x32dbg.a %RELEASEDIR%\pluginsdk\libx32dbg.a
del x32dbg.def
del x32dbg.a
copy bin\x32\x32dbg.lib %RELEASEDIR%\pluginsdk\x32dbg.lib

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
mkdir %RELEASEDIR%\release\x32
mkdir %RELEASEDIR%\release\x64

copy bin\x96dbg.exe %RELEASEDIR%\release\
copy bin\mnemdb.json %RELEASEDIR%\release\
copy bin\errordb.txt %RELEASEDIR%\release\
copy bin\exceptiondb.txt %RELEASEDIR%\release\
copy bin\ntstatusdb.txt %RELEASEDIR%\release\
copy bin\winconstants.txt %RELEASEDIR%\release\
xcopy src\gui\Translations\*.qm %RELEASEDIR%\release\translations /S /Y
copy bin\x32\x32bridge.dll %RELEASEDIR%\release\x32\
copy bin\x32\x32dbg.dll %RELEASEDIR%\release\x32\
copy bin\x32\x32dbg.exe %RELEASEDIR%\release\x32\
copy bin\x32\x32gui.dll %RELEASEDIR%\release\x32\
copy bin\x64\x64bridge.dll %RELEASEDIR%\release\x64\
copy bin\x64\x64dbg.dll %RELEASEDIR%\release\x64\
copy bin\x64\x64dbg.exe %RELEASEDIR%\release\x64\
copy bin\x64\x64gui.dll %RELEASEDIR%\release\x64\

xcopy %RELEASEDIR%\qt_base %RELEASEDIR%\release /S /Y
xcopy %RELEASEDIR%\bin_base %RELEASEDIR%\release /S /Y
xcopy %RELEASEDIR%\help %RELEASEDIR%\release /S /Y

exit 0