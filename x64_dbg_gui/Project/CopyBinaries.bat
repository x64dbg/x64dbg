@echo off

:: Flag reset when a file has been found and copied 	
set flag=1

copy %1.a %2 /y

if %ERRORLEVEL%==0 (
set flag=0
)


copy %1.exe %2 /y

if %ERRORLEVEL%==0 (
set flag=0
)

copy %1.dll %2 /y

if %ERRORLEVEL%==0 (
set flag=0
)

exit /b %flag%
