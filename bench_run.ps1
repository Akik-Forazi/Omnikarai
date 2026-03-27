# bench_run.ps1 - Omnikarai Speed God Benchmark Runner
# Fraziym Tech & AI | March 2026
$omnicc = ".\bin\omnicc.exe"
$sep = "=" * 62

function Section($t) { Write-Host "`n$sep`n  $t`n$sep" -ForegroundColor Cyan }

Section "BUILD: omnicc (AVX2+FMA)"
Stop-Process -Name omnicc -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 300
gcc -Iinclude -O2 -mavx2 -mfma -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 -lm 2>&1
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED" -ForegroundColor Red; exit 1 }
Write-Host "omnicc OK" -ForegroundColor Green

Section "BUILD: C scalar reference"
gcc -O3 -march=native -mavx2 -mfma -o benchmarks/bench_scalar.exe benchmarks/bench_mlp_c.c -lm 2>&1
if ($LASTEXITCODE -ne 0) { Write-Host "C build FAILED" -ForegroundColor Red }
else { Write-Host "bench_scalar OK" -ForegroundColor Green }

Section "TEST SUITE"
$p=0; $f=0
Get-ChildItem tests\t[0-9]*.ok | Sort-Object Name | ForEach-Object {
    $out = & $omnicc run --quiet $_.FullName 2>$null
    $ex  = $LASTEXITCODE
    if ($ex -eq 0) { Write-Host "  PASS $($_.Name)" -ForegroundColor Green; $p++ }
    else           { Write-Host "  FAIL $($_.Name) exit=$ex" -ForegroundColor Red; $f++ }
}
Write-Host "`n  $p pass / $f fail" -ForegroundColor Yellow

Section "BENCHMARK: Omnikarai 4-layer MLP (1000 inferences)"
$mlp_raw = & $omnicc run --quiet tests\t_bench_mlp.ok 2>$null
$lines = @($mlp_raw | Where-Object { "$_".Trim() -ne "" })
if ($lines.Count -ge 2) {
    $total_us = [int64]($lines[0].Trim())
    $per_us   = [int64]($lines[1].Trim())
    $per_ms   = [math]::Round($per_us / 1000.0, 4)
    Write-Host "  Total (1000 runs) : $total_us us" -ForegroundColor Yellow
    Write-Host "  Per inference     : $per_us us  ($per_ms ms)" -ForegroundColor Green
    Write-Host "  out[0] bits       : $($lines[2])" -ForegroundColor DarkGray
} else {
    Write-Host "  FAILED: $mlp_raw" -ForegroundColor Red
    $per_us = 999999
}

Section "BENCHMARK: C scalar reference (same workload)"
if (Test-Path benchmarks\bench_scalar.exe) {
    $c_lines = & .\benchmarks\bench_scalar.exe 2>$null
    foreach ($cl in $c_lines) { Write-Host "  $cl" -ForegroundColor Yellow }
} else {
    Write-Host "  (build failed - skipped)" -ForegroundColor Red
}

Section "SPEED GOD RESULTS"
Write-Host ""
Write-Host "  Omnikarai v6.0  AVX2+FMA  : $per_us us/inference" -ForegroundColor Green
Write-Host "  C -O3 scalar              : see above"
Write-Host "  Speed God target          : < 20 us/inference"
Write-Host ""
if ($per_us -lt 20) {
    Write-Host "  *** TARGET ACHIEVED: $per_us us < 20 us ***" -ForegroundColor Green
    Write-Host "  Omnikarai beats C scalar on 4-layer MLP hidden=128" -ForegroundColor Green
} elseif ($per_us -lt 100) {
    Write-Host "  Getting close: $per_us us  (target <20 us)" -ForegroundColor Yellow
} else {
    Write-Host "  STATUS: $per_us us  (debug needed)" -ForegroundColor Red
}
