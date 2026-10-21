@echo off
setlocal
cd /d "%~dp0"
set MODE=%~1
set ARCH=%~2
set BACKENDS=%~3
if "%MODE%"=="" set MODE=release
if "%ARCH%"=="" set ARCH=x64
if "%BACKENDS%"=="" set BACKENDS=dx9,dx10,dx11,dx12,opengl,vulkan
set FOLDER=Debug
if /i "%MODE%"=="release" set FOLDER=Release
set SAVED=
for /f "usebackq delims=" %%L in (`powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\SavedConfig.ps1" -Arch %ARCH%`) do set SAVED=%%L
call "%~dp0Source\Third-Party\OmegaWareProjects\OmegaWare-GUI\Build.bat" %MODE% %ARCH% || exit /b 1
xmake config -P . -p windows -a %ARCH% -m %MODE% --proxy=version --engine=none -y || exit /b 1
xmake build -P . OmegaWareProxy || exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\OverlayTest\HostMatrix.ps1" -Configs "%ARCH% %FOLDER%" -Backends "%BACKENDS%" %4
set RESULT=%ERRORLEVEL%
if defined SAVED (
    echo Restoring the previous build options: %SAVED%
    xmake config -P . -p windows %SAVED% -y
    xmake build -P . OmegaWareProxy
)
exit /b %RESULT%
