# debug_run.ps1 - full build + trace + dump + test in one run
$omnicc = ".\bin\omnicc.exe"
$sep = "=" * 60

function Section($title) { Write-Host "`n$sep`n  $title`n$sep" -ForegroundColor Cyan }
function Run($label, $args_) {
    Write-Host "`n--- $label ---" -ForegroundColor Yellow
    & $omnicc @args_ 2>&1
    Write-Host "  [exit: $LASTEXITCODE]"
}

Section "BUILD"
Stop-Process -Name omnicc -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 400
gcc -Iinclude -O2 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 -lm 2>&1
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED - aborting" -ForegroundColor Red; exit 1 }
Write-Host "BUILD OK" -ForegroundColor Green

Section "BETA: t04_loops"
Run "t04 --beta" @("run","--beta","tests\t04_loops.ok")

Section "BETA: t_while_for (minimal)"
Run "t_while_for --beta" @("run","--beta","tests\t_while_for.ok")

Section "BETA: t_rec_debug (factorial)"
Run "t_rec_debug --beta" @("run","--beta","tests\t_rec_debug.ok")

Section "DUMP: t05_simple"
Run "t05_simple dump" @("dump","tests\t05_simple.ok")

Section "BETA: t05_simple"
Run "t05_simple --beta" @("run","--beta","tests\t05_simple.ok")

Section "BETA: t05_functions"
Run "t05 --beta" @("run","--beta","tests\t05_functions.ok")

Section "DUMP: t_rec_debug"
Run "t_rec_debug dump" @("dump","tests\t_rec_debug.ok")

Section "BETA: t08_time"
Run "t08 --beta" @("run","--beta","tests\t08_time.ok")

Section "BETA: t10_datetime"
Run "t10 --beta" @("run","--beta","tests\t10_datetime.ok")

Section "BETA: t12_io"
Run "t12 --beta" @("run","--beta","tests\t12_io.ok")

Section "FULL TEST SUITE"
$tests = Get-ChildItem tests\t[0-9]*.ok | Sort-Object Name
$pass = 0; $fail = 0
foreach ($f in $tests) {
    $out = & $omnicc run --quiet $f.FullName 2>$null
    $ex = $LASTEXITCODE
    $outStr = ($out -join " | ")
    if ($ex -eq 0) {
        Write-Host "  PASS  $($f.Name)  | $outStr" -ForegroundColor Green
        $pass++
    } else {
        Write-Host "  FAIL  $($f.Name)  exit=$ex | $outStr" -ForegroundColor Red
        $fail++
    }
}
Section "RESULTS: $pass pass / $fail fail"
