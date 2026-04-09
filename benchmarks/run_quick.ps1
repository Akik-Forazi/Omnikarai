$root  = "C:\Users\akikf\programing\omnikarai\omniwin"
$bench = "$root\benchmarks"
Set-Location $root

function Omni($f, $ms=60000) {
    $proc = Start-Process "$root\bin\omnicc.exe" -ArgumentList @("run","--quiet",$f) `
        -PassThru -NoNewWindow `
        -RedirectStandardOutput "$bench\_o.txt" -RedirectStandardError "$bench\_e.txt"
    if (-not $proc.WaitForExit($ms)) { $proc.Kill(); return "TIMEOUT","?" }
    $lines = @(Get-Content "$bench\_o.txt" -ErrorAction SilentlyContinue | Where-Object { $_ -ne "" })
    $res = if ($lines.Count -gt 0) { $lines[0] } else { "CRASH" }
    $t   = if ($lines.Count -gt 1) { $lines[1] } else { "?" }
    return $res,$t
}
function Native($f, $ms=60000) {
    $proc = Start-Process $f -PassThru -NoNewWindow `
        -RedirectStandardOutput "$bench\_o.txt" -RedirectStandardError "$bench\_e.txt"
    if (-not $proc.WaitForExit($ms)) { $proc.Kill(); return "TIMEOUT","?" }
    $out = @(Get-Content "$bench\_o.txt" -ErrorAction SilentlyContinue | Where-Object { $_ -ne "" })
    $err = @(Get-Content "$bench\_e.txt" -ErrorAction SilentlyContinue)
    $res = if ($out.Count -gt 0) { $out[0] } else { "CRASH" }
    $ms2 = "?"; foreach ($line in $err) { if ($line -match "COMPUTE_MS=([0-9.]+)") { $ms2=$matches[1]; break } }
    return $res,$ms2
}

Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  OMNIKARAI -- BENCHMARKS vs C" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

$bms = @(
    @{n="Loop  100M";    ok="bench_loop_timed.ok";    c="bench_loop_c.exe"},
    @{n="Fib(40) x5";   ok="bench_fib_timed.ok";     c="bench_fib_c.exe"},
    @{n="Primes x30";   ok="bench_primes_timed.ok";  c="bench_primes_c.exe"},
    @{n="Dotprod x10k"; ok="bench_dotprod_timed.ok"; c="bench_dotprod_c.exe"},
    @{n="Matmul x5k";   ok="bench_matmul_timed.ok";  c="bench_matmul_c.exe"}
)

foreach ($bm in $bms) {
    Write-Host ""
    Write-Host "  $($bm.n)" -ForegroundColor White
    $r = Omni "$bench\$($bm.ok)"
    Write-Host ("    Omnikarai  {0,-20} {1,7}ms" -f $r[0],$r[1]) -ForegroundColor Yellow
    if (Test-Path "$bench\$($bm.c)") {
        $r2 = Native "$bench\$($bm.c)"
        if ($r[1] -ne "?" -and $r2[1] -ne "?") {
            $ratio = [math]::Round([double]$r[1] / [double]$r2[1], 2)
            Write-Host ("    C (-O2)    {0,-20} {1,7}ms  ratio={2}x" -f $r2[0],$r2[1],$ratio) -ForegroundColor Green
        } else {
            Write-Host ("    C (-O2)    {0,-20} {1,7}ms" -f $r2[0],$r2[1]) -ForegroundColor Green
        }
    }
}

Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
Remove-Item "$bench\_o.txt","$bench\_e.txt" -ErrorAction SilentlyContinue
