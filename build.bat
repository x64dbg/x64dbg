@echo off

echo Saving PATH
if "%OLDPATH%"=="" set OLDPATH=%PATH%

cd %~dp0

if /i "%1"=="x32"	call setenv.bat x32&set type=Configuration=Release;Platform=Win32&goto build
if /i "%1"=="x64"	call setenv.bat x64&set type=Configuration=Release;Platform=x64&goto build
if /i "%1"=="coverity"	goto coverity
if /i "%1"=="sonarqube"	goto sonarqube

goto usage


:build
echo Building DBG...
if "%MAXCORES%"=="" (
    msbuild.exe x64dbg.sln /m /verbosity:minimal /t:Rebuild /p:%type%
) else (
    set CL=/MP%MAXCORES%
    msbuild.exe x64dbg.sln /m:1 /verbosity:minimal /t:Rebuild /p:%type%
)
if not %ERRORLEVEL%==0 exit

echo Building GUI...
rmdir /S /Q src\gui_build
cd src\gui
qmake x64dbg.pro CONFIG+=release
if not %ERRORLEVEL%==0 exit
if "%MAXCORES%"=="" (
    jom
) else (
    jom /J %MAXCORES%
)
if not %ERRORLEVEL%==0 exit
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
cov-build --dir cov-int --instrument build.bat %2
goto :restorepath


:sonarqube
if "%2"=="" (
    echo "Usage: build.bat sonarqube x32/x64"
    goto usage
)

echo Building with SonarQube
build-wrapper --out-dir bw-output build.bat %2
if not defined APPVEYOR_PULL_REQUEST_NUMBER (
sonar-scanner -Dsonar.projectKey=x64dbg -Dsonar.sources=. -Dsonar.cfamily.build-wrapper-output=bw-output -Dsonar.host.url=https://sonarcloud.io -Dsonar.organization=mrexodia-github -Dsonar.login=%SONARQUBE_TOKEN% -Dsonar.exclusions=src/capstone_wrapper/**,src/dbg/btparser/**,src/gui_build/**,src/zydis_wrapper/zydis/**
) else (
sonar-scanner -Dsonar.projectKey=x64dbg -Dsonar.sources=. -Dsonar.cfamily.build-wrapper-output=bw-output -Dsonar.host.url=https://sonarcloud.io -Dsonar.organization=mrexodia-github -Dsonar.login=%SONARQUBE_TOKEN% -Dsonar.exclusions=src/capstone_wrapper/**,src/dbg/btparser/**,src/gui_build/**,src/zydis_wrapper/zydis/** -Dsonar.analysis.mode=preview -Dsonar.github.pullRequest=%APPVEYOR_PULL_REQUEST_NUMBER% -Dsonar.github.repository=x64dbg/x64dbg -Dsonar.github.oauth=%GITHUB_TOKEN%
)
goto :restorepath


:usage
echo "Usage: build.bat x32/x64/coverity"
echo.
echo Examples:
echo   build.bat x32               : builds 32-bit release build
echo   build.bat x64               : builds 64-bit release build
echo   build.bat coverity x32      : builds 32-bit coverity build
echo   build.bat coverity x64      : builds 64-bit coverity build
goto :restorepath

:restorepath
echo Resetting PATH
set PATH=%OLDPATH%
set OLDPATH=