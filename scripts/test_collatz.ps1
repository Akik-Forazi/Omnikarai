cd C:\Users\akikf\programing\omnikarai\omniwin

$tests = @(
    @{file="tests\t_collatz.ok";  label="collatz (fn name=collatz)"},
    @{file="tests\t_collatz2.ok"; label="collatz_len (fn name=collatz_len)"}
)

foreach ($t in $tests) {
    Write-Host "=== $($t.label) ===" -ForegroundColor Yellow
    $proc = Start-Process -FilePath "bin\omnicc.exe" `
        -ArgumentList "run","--beta",$t.file `
        -PassThru -NoNewWindow `
        -RedirectStandardOutput "tmp_out.txt" -RedirectStandardError "tmp_err.txt"
    $hung = -not $proc.WaitForExit(3000)
    if ($hung) { $proc.Kill() }
    
    $err = Get-Content "tmp_err.txt" -ErrorAction SilentlyContinue
    $out = Get-Content "tmp_out.txt" -ErrorAction SilentlyContinue
    
    $err | Where-Object { $_ -match "\[CG\]|\[omnicc\]" } | ForEach-Object { Write-Host $_ }
    Write-Host "STDOUT: $out"
    if ($hung) { Write-Host "STATUS: HUNG (killed after 3s)" -ForegroundColor Red }
    else { Write-Host "STATUS: exited $($proc.ExitCode)" -ForegroundColor Green }
    Write-Host ""
    Remove-Item -ErrorAction SilentlyContinue "tmp_out.txt","tmp_err.txt"
}
