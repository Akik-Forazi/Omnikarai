@echo off
cd /d C:\Users\akikf\programing\omnikarai\omniwin
echo Building...
gcc -Iinclude -O2 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 -lm 2>&1
if %errorlevel% neq 0 ( echo [BUILD FAILED] & exit /b 1 )
echo [BUILD OK]
echo.
echo ===== augassign (expect: 105 42 42 25 2) =====
bin\omnicc.exe run test_augassign.ok
echo.
echo ===== forlist (expect: 150 1 2 3) =====
bin\omnicc.exe run test_forlist.ok
echo.
echo ===== indexwrite (expect: 99 2 77) =====
bin\omnicc.exe run test_indexwrite.ok
echo.
echo ===== mathext (expect: 1 3 0 0) =====
bin\omnicc.exe run test_mathext.ok
echo.
echo ===== tier1 (expect: 150 34 99 1 3) =====
bin\omnicc.exe run test_tier1.ok
echo.
echo ===== test2 regression =====
bin\omnicc.exe run test2.ok
