cd C:\Users\akikf\programing\omnikarai\omniwin
Write-Host "=== 6%2 and 3%2 via function ==="
bin\omnicc.exe run --quiet tests\t_mod_test.ok

Write-Host ""
Write-Host "=== 6%2 and 3%2 at top level ==="
bin\omnicc.exe run --quiet tests\t_modonly.ok

Write-Host ""
Write-Host "=== inline if n%2==0 test ==="
bin\omnicc.exe run --quiet tests\t_mod_iftest.ok
