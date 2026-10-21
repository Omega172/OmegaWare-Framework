@echo off
setlocal
cd /d "%~dp0"
python -I Source\OmegaWareProxy\GenerateExports.py %* || exit /b 1
