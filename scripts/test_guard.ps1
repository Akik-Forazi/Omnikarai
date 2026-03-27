cd C:\Users\akikf\programing\omnikarai\omniwin
Write-Host "--- guarded steps ---"
bin\omnicc.exe run --quiet tests\t_steps_guard.ok
Write-Host "---"
Write-Host "--- beta trace ---"
bin\omnicc.exe check --beta tests\t_steps_guard.ok 2>&1 | Where-Object { $_ -match "\[CG\]" }
