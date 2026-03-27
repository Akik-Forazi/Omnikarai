cd C:\Users\akikf\programing\omnikarai\omniwin
bin\omnicc.exe dump tests\t_steps.ok 2>&1 | Out-File -Encoding ASCII dump_steps.txt
Write-Host "Dump written to dump_steps.txt"
