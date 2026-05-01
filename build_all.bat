@echo off
cd C:\Users\akikf\programing\omnikarai\omniwin

echo ================================================================
echo   OMNIKARAI v6.02.24  --  BUILD + TEST
echo ================================================================
echo.

echo [1/3] Building omnicc...
gcc -Iinclude -O2 -march=native -mavx2 -mfma -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 -lm
if %ERRORLEVEL% NEQ 0 (
    echo BUILD FAILED - omnicc
    exit /b 1
)
echo   omnicc OK

echo.
echo [2/3] Building omnip...
gcc -O2 -o bin/omnip.exe omnip/src/omnip.c -lwinhttp -lkernel32
if %ERRORLEVEL% NEQ 0 (
    echo BUILD FAILED - omnip
    exit /b 1
)
echo   omnip OK

echo.
echo [3/3] Running tests...
powershell -ExecutionPolicy Bypass -File tests\run_tests.ps1

echo.
echo ================================================================
echo   bin\omnicc.exe  --  omnicc version:
bin\omnicc.exe version 2>&1
echo   bin\omnip.exe   --  omnip version:
bin\omnip.exe version
echo ================================================================
