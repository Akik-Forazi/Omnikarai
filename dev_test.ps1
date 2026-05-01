# dev_test.ps1 — Contributor test script
# Builds both omnicc and omnip, runs full test suite, shows summary
$root = "C:\Users\akikf\programing\omnikarai\omniwin"
Set-Location $root

Write-Host "`n=== BUILDING omnicc ===" -ForegroundColor Cyan
$b = & gcc -Iinclude -O2 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 -lm 2>&1
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED:`n$b" -ForegroundColor Red; exit 1 }
Write-Host "omnicc OK" -ForegroundColor Green

Write-Host "`n=== BUILDING omnip ===" -ForegroundColor Cyan
$b2 = & gcc -Wall -O2 -o bin/omnip.exe omnip/src/omnip.c -lkernel32 -lwinhttp 2>&1
if ($LASTEXITCODE -ne 0) { Write-Host "OMNIP FAILED:`n$b2" -ForegroundColor Red }
else { Write-Host "omnip OK" -ForegroundColor Green }

Write-Host "`n=== VERSIONS ===" -ForegroundColor Cyan
& bin/omnicc.exe version
& bin/omnip.exe version

Write-Host "`n=== RUNNING TEST SUITE ===" -ForegroundColor Cyan
& powershell -File tests/run_tests.ps1

Write-Host "`n=== OMNIP CONNECTIVITY CHECK ===" -ForegroundColor Cyan
& bin/omnip.exe search omnikarai
