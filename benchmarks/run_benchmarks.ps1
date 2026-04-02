$root  = "C:\Users\akikf\programing\omnikarai\omniwin"
$bench = "$root\benchmarks"
Set-Location $root

# Build Omnikarai
$b = & gcc -Iinclude -O3 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 -lm 2>&1
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED" -ForegroundColor Red; $b; exit 1 }
Write-Host "BUILD OK" -ForegroundColor Green

# Build C and C++ benchmarks
Write-Host "Building C/C++ benchmarks..." -ForegroundColor DarkGray
foreach ($stem in @("bench_loop","bench_fib","bench_primes","bench_dotprod","bench_matmul","bench_neuron")) {
    if (Test-Path "$bench\$stem.c")   { & gcc  -O3 -march=native -mavx2            -o "$bench\${stem}_c.exe"   "$bench\$stem.c"   2>$null }
    if (Test-Path "$bench\$stem.cpp") { & g++  -O3 -march=native -mavx2 -std=c++17 -o "$bench\${stem}_cpp.exe" "$bench\$stem.cpp" 2>$null }
}

# Correctness check
$p=0;$f=0
Get-ChildItem "$root\tests\t[0-9]*.ok" | Sort-Object Name | ForEach-Object {
    & "$root\bin\omnicc.exe" run --quiet $_.FullName 2>$null | Out-Null
    if ($LASTEXITCODE -eq 0) { $p++ } else { $f++; Write-Host "FAIL $($_.Name)" -ForegroundColor Red }
}
$tpass=($f -eq 0)
Write-Host "Tests:  $(if($tpass){"$p/$($p+$f) PASS"}else{"$f FAILED!"})" -ForegroundColor $(if($tpass){"Green"}else{"Red"})
if (-not $tpass) { exit 1 }

# Runner: Omnikarai (reads timing from second line of stdout)
function RunOmni($file, $timeout_ms=180000) {
    $proc = Start-Process "$root\bin\omnicc.exe" -ArgumentList @("run","--quiet",$file) `
        -PassThru -NoNewWindow `
        -RedirectStandardOutput "$bench\_o.txt" -RedirectStandardError "$bench\_e.txt"
    if (-not $proc.WaitForExit($timeout_ms)) { $proc.Kill(); return "TIMEOUT","?" }
    $lines = @(Get-Content "$bench\_o.txt" -ErrorAction SilentlyContinue | Where-Object {$_ -ne ""})
    $res = if($lines.Count -gt 0){$lines[0]}else{"CRASH"}
    $t   = if($lines.Count -gt 1){$lines[1]}else{"?"}
    return $res,$t
}

# Runner: native exe (reads COMPUTE_MS= from stderr)
function RunExe($file, $timeout_ms=180000) {
    $proc = Start-Process $file -PassThru -NoNewWindow `
        -RedirectStandardOutput "$bench\_o.txt" -RedirectStandardError "$bench\_e.txt"
    if (-not $proc.WaitForExit($timeout_ms)) { $proc.Kill(); return "TIMEOUT","?" }
    $out = @(Get-Content "$bench\_o.txt" -ErrorAction SilentlyContinue | Where-Object {$_ -ne ""})
    $err = @(Get-Content "$bench\_e.txt" -ErrorAction SilentlyContinue)
    $res = if($out.Count -gt 0){$out[0]}else{"CRASH"}
    $t = "?"; foreach($l in $err){if($l -match "COMPUTE_MS=([0-9.]+)"){$t=$matches[1];break}}
    return $res,$t
}

# Runner: script (python / node)
function RunScript($cmd, $scriptArgs, $timeout_ms=300000) {
    $proc = Start-Process $cmd -ArgumentList $scriptArgs -PassThru -NoNewWindow `
        -RedirectStandardOutput "$bench\_o.txt" -RedirectStandardError "$bench\_e.txt"
    if (-not $proc.WaitForExit($timeout_ms)) { $proc.Kill(); return "TIMEOUT","?" }
    $out = @(Get-Content "$bench\_o.txt" -ErrorAction SilentlyContinue | Where-Object {$_ -ne ""})
    $err = @(Get-Content "$bench\_e.txt" -ErrorAction SilentlyContinue)
    $res = if($out.Count -gt 0){$out[0]}else{"CRASH"}
    $t = "?"; foreach($l in $err){if($l -match "COMPUTE_MS=([0-9.]+)"){$t=$matches[1];break}}
    return $res,$t
}

Write-Host ""
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  OMNIKARAI v6.0 -- vs C -O3, C++ -O3, Python, Node.js" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan

$bms = @(
    @{n="Loop  100M";    ok="bench_loop_timed.ok";    c="bench_loop_c.exe";    cpp="bench_loop_cpp.exe";    py="bench_loop.py";    js="bench_loop.js"},
    @{n="Fib(40)  x5";  ok="bench_fib_timed.ok";     c="bench_fib_c.exe";     cpp="bench_fib_cpp.exe";     py="bench_fib.py";     js="bench_fib.js"},
    @{n="Primes   x30"; ok="bench_primes_timed.ok";  c="bench_primes_c.exe";  cpp="bench_primes_cpp.exe";  py="bench_primes.py";  js="bench_primes.js"},
    @{n="Dotprod x10k"; ok="bench_dotprod_timed.ok"; c="bench_dotprod_c.exe"; cpp="bench_dotprod_cpp.exe"; py="bench_dotprod.py"; js="bench_dotprod.js"},
    @{n="Matmul   x5k"; ok="bench_matmul_timed.ok";  c="bench_matmul_c.exe";  cpp="bench_matmul_cpp.exe";  py="bench_matmul.py";  js="bench_matmul.js"}
)

$wins=0; $total=0

foreach ($bm in $bms) {
    Write-Host ""
    Write-Host "  --- $($bm.n) ---" -ForegroundColor White

    $r = RunOmni "$bench\$($bm.ok)"
    $omni_ms = if($r[1] -ne "?"){[double]$r[1]}else{0}
    Write-Host ("    {0,-10} {1,-22} {2,8}ms" -f "Omnikarai",$r[0],$r[1]) -ForegroundColor Yellow

    if (Test-Path "$bench\$($bm.c)") {
        $rc = RunExe "$bench\$($bm.c)"
        $c_ms = if($rc[1] -ne "?"){[double]$rc[1]}else{0}
        $ratio = if($c_ms -gt 0.001 -and $omni_ms -gt 0){"ratio=$([math]::Round($omni_ms/$c_ms,2))x"}else{"(C too fast)"}
        $col = if($c_ms -lt 0.001){"DarkGray"}elseif($omni_ms -le $c_ms*1.1){"Green"}elseif($omni_ms -le $c_ms*2){"Yellow"}else{"Red"}
        Write-Host ("    {0,-10} {1,-22} {2,8}ms   {3}" -f "C -O3",$rc[0],$rc[1],$ratio) -ForegroundColor $col
        $total++; if($omni_ms -le $c_ms*1.5){$wins++}
    }

    if (Test-Path "$bench\$($bm.cpp)") {
        $rcpp = RunExe "$bench\$($bm.cpp)"
        $cpp_ms = if($rcpp[1] -ne "?"){[double]$rcpp[1]}else{0}
        $ratio = if($cpp_ms -gt 0.001 -and $omni_ms -gt 0){"ratio=$([math]::Round($omni_ms/$cpp_ms,2))x"}else{"(C++ too fast)"}
        $col = if($cpp_ms -lt 0.001){"DarkGray"}elseif($omni_ms -le $cpp_ms*1.1){"Green"}elseif($omni_ms -le $cpp_ms*2){"Yellow"}else{"Red"}
        Write-Host ("    {0,-10} {1,-22} {2,8}ms   {3}" -f "C++ -O3",$rcpp[0],$rcpp[1],$ratio) -ForegroundColor $col
    }

    if (Test-Path "$bench\$($bm.py)") {
        $rpy = RunScript "python" @("$bench\$($bm.py)") 300000
        $py_ms = if($rpy[1] -ne "?"){[double]$rpy[1]}else{0}
        $ratio = if($py_ms -gt 0.001 -and $omni_ms -gt 0){"Omni $([math]::Round($py_ms/$omni_ms,1))x faster"}else{"?"}
        Write-Host ("    {0,-10} {1,-22} {2,8}ms   {3}" -f "Python",$rpy[0],$rpy[1],$ratio) -ForegroundColor DarkCyan
    }

    if (Test-Path "$bench\$($bm.js)") {
        $rjs = RunScript "node" @("$bench\$($bm.js)") 300000
        $js_ms = if($rjs[1] -ne "?"){[double]$rjs[1]}else{0}
        $ratio = if($js_ms -gt 0.001 -and $omni_ms -gt 0){"Omni $([math]::Round($js_ms/$omni_ms,1))x faster"}else{"?"}
        Write-Host ("    {0,-10} {1,-22} {2,8}ms   {3}" -f "Node.js",$rjs[0],$rjs[1],$ratio) -ForegroundColor DarkCyan
    }
}

Write-Host ""
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  $wins/$total benchmarks within 1.5x of C -O3" -ForegroundColor $(if($wins -eq $total){"Green"}else{"Yellow"})
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  NOTE: Python matmul=200 reps, dotprod=1000 reps (rest=full)" -ForegroundColor DarkGray
Remove-Item "$bench\_o.txt" -ErrorAction SilentlyContinue
Remove-Item "$bench\_e.txt" -ErrorAction SilentlyContinue
