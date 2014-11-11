@echo off
set RELEASEDIR=.\release
rmdir /S /Q %RELEASEDIR%
mkdir %RELEASEDIR%

echo qt_base

mkdir %RELEASEDIR%\qt_base
mkdir %RELEASEDIR%\qt_base\x32
mkdir %RELEASEDIR%\qt_base\x64

copy bin\x32\QtCore4.dll %RELEASEDIR%\qt_base\x32\QtCore4.dll
copy bin\x32\QtGui4.dll %RELEASEDIR%\qt_base\x32\QtGui4.dll
copy bin\x32\QtNetwork4.dll %RELEASEDIR%\qt_base\x32\QtNetwork4.dll
copy bin\x64\QtCore4.dll %RELEASEDIR%\qt_base\x64\QtCore4.dll
copy bin\x64\QtGui4.dll %RELEASEDIR%\qt_base\x64\QtGui4.dll
copy bin\x64\QtNetwork4.dll %RELEASEDIR%\qt_base\x64\QtNetwork4.dll

echo bin_base

mkdir %RELEASEDIR%\bin_base
mkdir %RELEASEDIR%\bin_base\x32
mkdir %RELEASEDIR%\bin_base\x64

copy bin\x32\BeaEngine.dll %RELEASEDIR%\bin_base\x32\BeaEngine.dll
copy bin\x32\dbghelp.dll %RELEASEDIR%\bin_base\x32\dbghelp.dll
copy bin\x32\symsrv.dll %RELEASEDIR%\bin_base\x32\symsrv.dll
copy bin\x32\DeviceNameResolver.dll %RELEASEDIR%\bin_base\x32\DeviceNameResolver.dll
copy bin\x32\Scylla.dll %RELEASEDIR%\bin_base\x32\Scylla.dll
copy bin\x32\jansson.dll %RELEASEDIR%\bin_base\x32\jansson.dll
copy bin\x32\lz4.dll %RELEASEDIR%\bin_base\x32\lz4.dll
copy bin\x32\TitanEngine.dll %RELEASEDIR%\bin_base\x32\TitanEngine.dll
copy bin\x32\XEDParse.dll %RELEASEDIR%\bin_base\x32\XEDParse.dll
copy bin\x64\BeaEngine.dll %RELEASEDIR%\bin_base\x64\BeaEngine.dll
copy bin\x64\dbghelp.dll %RELEASEDIR%\bin_base\x64\dbghelp.dll
copy bin\x64\symsrv.dll %RELEASEDIR%\bin_base\x64\symsrv.dll
copy bin\x64\DeviceNameResolver.dll %RELEASEDIR%\bin_base\x64\DeviceNameResolver.dll
copy bin\x64\Scylla.dll %RELEASEDIR%\bin_base\x64\Scylla.dll
copy bin\x64\jansson.dll %RELEASEDIR%\bin_base\x64\jansson.dll
copy bin\x64\lz4.dll %RELEASEDIR%\bin_base\x64\lz4.dll
copy bin\x64\TitanEngine.dll %RELEASEDIR%\bin_base\x64\TitanEngine.dll
copy bin\x64\XEDParse.dll %RELEASEDIR%\bin_base\x64\XEDParse.dll

echo help

mkdir %RELEASEDIR%\help

copy help\x64_dbg.chm %RELEASEDIR%\help

echo pluginsdk

mkdir %RELEASEDIR%\pluginsdk
mkdir %RELEASEDIR%\pluginsdk\BeaEngine
mkdir %RELEASEDIR%\pluginsdk\dbghelp
mkdir %RELEASEDIR%\pluginsdk\DeviceNameResolver
mkdir %RELEASEDIR%\pluginsdk\jansson
mkdir %RELEASEDIR%\pluginsdk\lz4
mkdir %RELEASEDIR%\pluginsdk\TitanEngine
mkdir %RELEASEDIR%\pluginsdk\XEDParse

xcopy x64_dbg_dbg\BeaEngine %RELEASEDIR%\pluginsdk\BeaEngine /S /Y
xcopy x64_dbg_dbg\dbghelp %RELEASEDIR%\pluginsdk\dbghelp /S /Y
xcopy x64_dbg_dbg\DeviceNameResolver %RELEASEDIR%\pluginsdk\DeviceNameResolver /S /Y
xcopy x64_dbg_dbg\jansson %RELEASEDIR%\pluginsdk\jansson /S /Y
xcopy x64_dbg_dbg\lz4 %RELEASEDIR%\pluginsdk\lz4 /S /Y
xcopy x64_dbg_dbg\TitanEngine %RELEASEDIR%\pluginsdk\TitanEngine /S /Y
del %RELEASEDIR%\pluginsdk\TitanEngine\TitanEngine.txt /F /Q
xcopy x64_dbg_dbg\XEDParse %RELEASEDIR%\pluginsdk\XEDParse /S /Y
copy x64_dbg_dbg\_plugin_types.h %RELEASEDIR%\pluginsdk\_plugin_types.h
copy x64_dbg_dbg\_plugins.h %RELEASEDIR%\pluginsdk\_plugins.h
copy x64_dbg_dbg\_dbgfunctions.h %RELEASEDIR%\pluginsdk\_dbgfunctions.h
copy x64_dbg_bridge\bridgemain.h %RELEASEDIR%\pluginsdk\bridgemain.h

genlib bin\x32\x32_bridge.dll
copy x32_bridge.a %RELEASEDIR%\pluginsdk\libx32_bridge.a
del x32_bridge.def
del x32_bridge.a
copy bin\x32\x32_bridge.lib %RELEASEDIR%\pluginsdk\x32_bridge.lib

genlib bin\x32\x32_dbg.dll
copy x32_dbg.a %RELEASEDIR%\pluginsdk\libx32_dbg.a
del x32_dbg.def
del x32_dbg.a
copy bin\x32\x32_dbg.lib %RELEASEDIR%\pluginsdk\x32_dbg.lib

genlib bin\x64\x64_bridge.dll
copy x64_bridge.a %RELEASEDIR%\pluginsdk\libx64_bridge.a
del x64_bridge.def
del x64_bridge.a
copy bin\x64\x64_bridge.lib %RELEASEDIR%\pluginsdk\x64_bridge.lib

genlib bin\x64\x64_dbg.dll
copy x64_dbg.a %RELEASEDIR%\pluginsdk\libx64_dbg.a
del x64_dbg.def
del x64_dbg.a
copy bin\x64\x64_dbg.lib %RELEASEDIR%\pluginsdk\x64_dbg.lib

echo release

mkdir %RELEASEDIR%\release
mkdir %RELEASEDIR%\release\x32
mkdir %RELEASEDIR%\release\x64

copy bin\x96_dbg.exe %RELEASEDIR%\release\x96_dbg.exe
copy bin\x32\x32_bridge.dll %RELEASEDIR%\release\x32\x32_bridge.dll
copy bin\x32\x32_dbg.dll %RELEASEDIR%\release\x32\x32_dbg.dll
copy bin\x32\x32_dbg.exe %RELEASEDIR%\release\x32\x32_dbg.exe
copy bin\x32\x32_gui.dll %RELEASEDIR%\release\x32\x32_gui.dll
copy bin\x64\x64_bridge.dll %RELEASEDIR%\release\x64\x64_bridge.dll
copy bin\x64\x64_dbg.dll %RELEASEDIR%\release\x64\x64_dbg.dll
copy bin\x64\x64_dbg.exe %RELEASEDIR%\release\x64\x64_dbg.exe
copy bin\x64\x64_gui.dll %RELEASEDIR%\release\x64\x64_gui.dll

xcopy %RELEASEDIR%\qt_base %RELEASEDIR%\release /S /Y
xcopy %RELEASEDIR%\bin_base %RELEASEDIR%\release /S /Y
xcopy %RELEASEDIR%\help %RELEASEDIR%\release /S /Y

exit 0