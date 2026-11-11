@echo off
setlocal
cd /d "%~dp0"
where gcc >nul 2>nul
if errorlevel 1 (
  echo Para recompilar, instale GCC/MinGW-w64 e adicione gcc ao PATH.
  echo Para apenas usar o simulador, abra iniciar.bat.
  pause
  exit /b 1
)
if not exist bin mkdir bin
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -I simulator/src simulator/src/main.c simulator/src/mic1.c simulator/src/isa.c simulator/src/microcode.c simulator/src/assembler.c simulator/src/cache.c simulator/src/protocol.c simulator/src/server.c simulator/src/json.c -static -lws2_32 -lshell32 -o bin/mic1-windows-x64.exe
if errorlevel 1 (
  pause
  exit /b 1
)
pause
