cd C:\Users\akikf\programing\omnikarai\omniwin

Write-Host "Building..." -ForegroundColor Cyan
$build = & gcc -Iinclude -O2 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 2>&1
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED:" -ForegroundColor Red; $build; exit 1 }
Write-Host "BUILD OK" -ForegroundColor Green
Write-Host ""

# Quick smoke test for the core bug
Write-Host "=== SMOKE TEST: if/else inside while ===" -ForegroundColor Cyan

$smoke = @"
fn collatz(n):
    set steps = 0
    while n != 1:
        if n % 2 == 0:
            set n = n / 2
        else:
            set n = n * 3 + 1
        set steps = steps + 1
    return steps

print(collatz(2))
print(collatz(6))
print(collatz(27))
"@
$smoke | Out-File -Encoding ASCII tests\scratch\smoke_collatz.ok

$proc = Start-Process "bin\omnicc.exe" -ArgumentList "run","--quiet","tests\scratch\smoke_collatz.ok" `
    -PassThru -NoNewWindow -RedirectStandardOutput "_s.txt" -RedirectStandardError "_e.txt"
if (-not $proc.WaitForExit(5000)) { $proc.Kill(); Write-Host "  HUNG" -ForegroundColor Red }
else {
    $out = Get-Content "_s.txt" -ErrorAction SilentlyContinue
    Write-Host "  collatz(2)=$($out[0])  (expect 1)"  -ForegroundColor (if($out[0] -eq "1"){"Green"}else{"Red"})
    Write-Host "  collatz(6)=$($out[1])  (expect 8)"  -ForegroundColor (if($out[1] -eq "8"){"Green"}else{"Red"})
    Write-Host "  collatz(27)=$($out[2]) (expect 111)" -ForegroundColor (if($out[2] -eq "111"){"Green"}else{"Red"})
}
Remove-Item "_s.txt","_e.txt" -ErrorAction SilentlyContinue
Write-Host ""

# Full test suite
Set-Location tests
& powershell -File run_tests.ps1
Write-Host ""
& powershell -File run_stress.ps1
