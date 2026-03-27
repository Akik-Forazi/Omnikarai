cd C:\Users\akikf\programing\omnikarai\omniwin

Write-Host "=== collatz smoke test ===" -ForegroundColor Cyan
$proc = Start-Process "bin\omnicc.exe" -ArgumentList "run","--quiet","tests\scratch\smoke_collatz.ok" `
    -PassThru -NoNewWindow -RedirectStandardOutput "_s.txt" -RedirectStandardError "_e.txt"
if (-not $proc.WaitForExit(8000)) {
    $proc.Kill()
    Write-Host "HUNG" -ForegroundColor Red
} else {
    Write-Host "Exit: $($proc.ExitCode)"
    Write-Host "STDOUT:"
    Get-Content "_s.txt" -ErrorAction SilentlyContinue
    Write-Host "STDERR:"
    Get-Content "_e.txt" -ErrorAction SilentlyContinue
}
Remove-Item "_s.txt","_e.txt" -ErrorAction SilentlyContinue
