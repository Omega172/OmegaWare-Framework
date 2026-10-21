@echo off
setlocal
cd /d "%~dp0"
set MODE=%~1
set ARCH=%~2
if "%MODE%"=="" set MODE=release
if "%ARCH%"=="" set ARCH=x64
set SAVED=
for /f "usebackq delims=" %%L in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\SavedConfig.ps1" -Arch %ARCH% -Skip arch,mode`) do set SAVED=%%L
xmake config -p windows -a %ARCH% -m %MODE% %SAVED% -y || exit /b 1
xmake build || exit /b 1
echo Built %MODE% %ARCH% into Build\%MODE%\%ARCH%
