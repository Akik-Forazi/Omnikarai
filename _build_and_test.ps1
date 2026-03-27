cd C:\Users\akikf\programing\omnikarai\omniwin
gcc -Iinclude -O2 -march=native -mavx2 -mfma -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 -lm
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED" -ForegroundColor Red; exit 1 }
Write-Host "BUILD OK" -ForegroundColor Green
.\tests\run_tests.ps1
