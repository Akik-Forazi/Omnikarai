@echo off
cd /d C:\Users\akikf\programing\omnikarai\omniwin

echo Building omnicc...
gcc -Iinclude -O2 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 -lm
if %errorlevel% neq 0 ( echo [FAIL] omnicc & exit /b 1 )
echo [OK] omnicc

echo Building omnip...
gcc -Wall -O2 -o bin/omnip.exe omnip/src/omnip.c -lkernel32 -lwinhttp
if %errorlevel% neq 0 ( echo [FAIL] omnip & exit /b 1 )
echo [OK] omnip

echo.
echo === Both tools built ===
bin\omnicc.exe version
bin\omnip.exe version
