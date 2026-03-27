$root = "C:\Users\akikf\programing\omnikarai\omniwin"
Set-Location $root

# ── Build ─────────────────────────────────────────────────────────────────────
Write-Host "Building..." -ForegroundColor Cyan
$build = & gcc -Iinclude -O2 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "BUILD FAILED:" -ForegroundColor Red
    $build | ForEach-Object { Write-Host $_ }
    exit 1
}
Write-Host "BUILD OK" -ForegroundColor Green
Write-Host ""

# ── Full test suite ───────────────────────────────────────────────────────────
Set-Location "$root\tests"
& powershell -File run_tests.ps1
Write-Host ""

# ── Stress tests (with 10s timeout per test) ─────────────────────────────────
& powershell -File run_stress.ps1
Write-Host ""

# ── Benchmarks ───────────────────────────────────────────────────────────────
Set-Location $root
Write-Host "======================================================"  -ForegroundColor Cyan
Write-Host "  BENCHMARKS" -ForegroundColor Cyan
Write-Host "======================================================"  -ForegroundColor Cyan

$benches = @(
    @{name="Loop sum 1..100M";     file="benchmarks\bench_loop_timed.ok"},
    @{name="Fibonacci fib(40)x5";  file="benchmarks\bench_fib_timed.ok"},
    @{name="Primes to 100k x30";   file="benchmarks\bench_primes_timed.ok"}
)

foreach ($b in $benches) {
    Write-Host "  $($b.name)" -ForegroundColor Yellow
    $proc = Start-Process -FilePath "bin\omnicc.exe" -ArgumentList "run","--quiet",$b.file `
        -PassThru -NoNewWindow `
        -RedirectStandardOutput "_bench_out.txt" -RedirectStandardError "_bench_err.txt"
    if (-not $proc.WaitForExit(30000)) {
        $proc.Kill()
        Write-Host "    TIMEOUT" -ForegroundColor Red
    } else {
        $out = Get-Content "_bench_out.txt" -ErrorAction SilentlyContinue
        if ($proc.ExitCode -ne 0) {
            $err = Get-Content "_bench_err.txt" -ErrorAction SilentlyContinue
            Write-Host "    CRASH: $err" -ForegroundColor Red
        } else {
            $lines = $out | Where-Object { $_ -ne "" }
            $result = $lines[0]; $ms = $lines[1]
            Write-Host "    result=$result  time=${ms}ms" -ForegroundColor Green
        }
    }
    Remove-Item "_bench_out.txt","_bench_err.txt" -ErrorAction SilentlyContinue
}
Write-Host ""
