@echo off
setlocal
cd /d "%~dp0"
set MODE=%~1
set ARCH=%~2
if "%MODE%"=="" set MODE=release
if "%ARCH%"=="" set ARCH=x64
set FOLDER=Debug
if /i "%MODE%"=="release" set FOLDER=Release
xmake config -P . -p windows -a %ARCH% -m %MODE% -y || exit /b 1
xmake build -P . -g Proxies || exit /b 1
echo Built version.dll, winmm.dll and dinput8.dll into Build\%FOLDER%\%ARCH%\OmegaWareProxies
