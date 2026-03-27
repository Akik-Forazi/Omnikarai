cd C:\Users\akikf\programing\omnikarai\omniwin
bin\omnicc.exe dump tests\t_steps_guard.ok 2>&1 | Out-File -Encoding ASCII dump_guard.txt
Write-Host "File written"
Get-Item dump_guard.txt | Select-Object Length
