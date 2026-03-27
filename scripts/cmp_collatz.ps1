cd C:\Users\akikf\programing\omnikarai\omniwin

# Run both and compare CG traces
"bin\omnicc.exe","run","--beta","tests\t_collatz.ok" | Out-Null
$a = & bin\omnicc.exe run --beta tests\t_collatz.ok 2>&1
$b = & bin\omnicc.exe run --beta tests\t_collatz2.ok 2>&1

Write-Host "=== t_collatz.ok CG trace ===" -ForegroundColor Cyan
$a | Where-Object { $_ -match "\[CG\]" }
Write-Host ""
Write-Host "=== t_collatz2.ok CG trace ===" -ForegroundColor Cyan
$b | Where-Object { $_ -match "\[CG\]" }
