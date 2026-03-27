cd C:\Users\akikf\programing\omnikarai\omniwin
$proc = Start-Process -FilePath "bin\omnicc.exe" -ArgumentList "run","--quiet","tests\t_steps_debug.ok" `
    -PassThru -NoNewWindow -RedirectStandardOutput "tmp_out.txt" -RedirectStandardError "tmp_err.txt"
if (-not $proc.WaitForExit(3000)) { $proc.Kill(); Write-Host "HUNG after 3s" }
Write-Host "=== n values printed inside loop ==="
Get-Content "tmp_out.txt" -ErrorAction SilentlyContinue | Select-Object -First 30
Remove-Item "tmp_out.txt","tmp_err.txt" -ErrorAction SilentlyContinue
