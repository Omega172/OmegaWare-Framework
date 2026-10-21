@echo off
setlocal
cd /d "%~dp0"
set MODE=%~1
set ARCH=%~2
set PROXY=%~3
if "%MODE%"=="" set MODE=debug
if "%ARCH%"=="" set ARCH=x64
if "%PROXY%"=="" set PROXY=version
set FOLDER=Debug
if /i "%MODE%"=="release" set FOLDER=Release
set SAVED=
for /f "usebackq delims=" %%L in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\SavedConfig.ps1" -Arch %ARCH% -Skip arch,mode,proxy`) do set SAVED=%%L
xmake config -p windows -a %ARCH% -m %MODE% %SAVED% --proxy=%PROXY% -y || exit /b 1
xmake build OmegaWareProxy || exit /b 1
xmake build OmegaWareTests || exit /b 1
"Build\%FOLDER%\%ARCH%\OmegaWareTests\OmegaWareTests.exe"
