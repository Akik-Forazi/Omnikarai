# build_v6.ps1 — Omnikarai v6.02.24 full build + test
# Run: cd C:\Users\akikf\programing\omnikarai\omniwin; .\build_v6.ps1

Set-Location "C:\Users\akikf\programing\omnikarai\omniwin"
$ErrorActionPreference = "Stop"

function OK($m)  { Write-Host "  [OK] $m" -ForegroundColor Green }
function ERR($m) { Write-Host "  [!!] $m" -ForegroundColor Red; exit 1 }

Write-Host ""
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "   OMNIKARAI v6.02.24  --  BUILD" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host ""

# ── Build omnicc ──────────────────────────────────────────────
Write-Host "Building omnicc..." -ForegroundColor White
$r = & gcc -Iinclude -O2 -march=native -mavx2 -mfma `
    -o bin/omnicc.exe `
    src/main.c src/lexer.c src/parser.c src/codegen.c `
    -lkernel32 -lm 2>&1
if ($LASTEXITCODE -ne 0) { $r | ForEach-Object { Write-Host $_ -ForegroundColor Red }; ERR "omnicc build failed" }
OK "omnicc.exe built"

# ── Build omnip ───────────────────────────────────────────────
Write-Host "Building omnip..." -ForegroundColor White
$r2 = & gcc -O2 -o bin/omnip.exe omnip/src/omnip.c -lwinhttp -lkernel32 2>&1
if ($LASTEXITCODE -ne 0) { $r2 | ForEach-Object { Write-Host $_ -ForegroundColor Red }; ERR "omnip build failed" }
OK "omnip.exe built"

# ── Version check ─────────────────────────────────────────────
Write-Host ""
Write-Host "Version check:" -ForegroundColor White
& bin\omnicc.exe version 2>&1
Write-Host ""
& bin\omnip.exe version

# ── Test suite ────────────────────────────────────────────────
Write-Host ""
Write-Host "================================================================" -ForegroundColor Cyan
.\tests\run_tests.ps1
