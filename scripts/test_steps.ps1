cd C:\Users\akikf\programing\omnikarai\omniwin

Write-Host "--- steps(2) steps(4) steps(6) ---" -ForegroundColor Yellow
$proc = Start-Process -FilePath "bin\omnicc.exe" -ArgumentList "run","--quiet","tests\t_steps.ok" `
    -PassThru -NoNewWindow -RedirectStandardOutput "tmp.txt" -RedirectStandardError "tmp_e.txt"
if (-not $proc.WaitForExit(4000)) { $proc.Kill(); Write-Host "HUNG" -ForegroundColor Red }
else {
    Get-Content "tmp.txt" -ErrorAction SilentlyContinue
    Write-Host "Exit: $($proc.ExitCode)"
}

Write-Host ""
Write-Host "--- beta trace of t_steps.ok ---" -ForegroundColor Yellow
bin\omnicc.exe check --beta tests\t_steps.ok 2>&1 | Where-Object { $_ -match "\[CG\] (fn_body|set)" }

Remove-Item "tmp.txt","tmp_e.txt" -ErrorAction SilentlyContinue
