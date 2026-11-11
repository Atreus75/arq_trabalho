@echo off
setlocal
cd /d "%~dp0"
if not exist "bin\mic1-windows-x64.exe" (
  echo Extraia o ZIP inteiro antes de iniciar.
  echo O executavel esperado e bin\mic1-windows-x64.exe.
  pause
  exit /b 1
)
"bin\mic1-windows-x64.exe" %*
if errorlevel 1 pause
