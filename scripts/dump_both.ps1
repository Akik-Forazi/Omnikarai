cd C:\Users\akikf\programing\omnikarai\omniwin

Write-Host "=== DUMP t_collatz.ok ===" -ForegroundColor Cyan
bin\omnicc.exe dump tests\t_collatz.ok 2>&1 | Select-Object -First 40

Write-Host ""
Write-Host "=== DUMP t_collatz2.ok ===" -ForegroundColor Cyan  
bin\omnicc.exe dump tests\t_collatz2.ok 2>&1 | Select-Object -First 40
