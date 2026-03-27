$root = "C:\Users\akikf\programing\omnikarai\omniwin"
Set-Location $root

function RunTest($label, $file, $timeoutMs=5000) {
    Write-Host "  $label" -ForegroundColor Yellow
    $proc = Start-Process "bin\omnicc.exe" -ArgumentList "run","--quiet",$file `
        -PassThru -NoNewWindow `
        -RedirectStandardOutput "_out.txt" -RedirectStandardError "_err.txt"
    if (-not $proc.WaitForExit($timeoutMs)) {
        $proc.Kill()
        Write-Host "    HUNG" -ForegroundColor Red
        return
    }
    $out = Get-Content "_out.txt" -ErrorAction SilentlyContinue
    $err = Get-Content "_err.txt" -ErrorAction SilentlyContinue | Where-Object { $_ -notmatch "^\[omnicc\]" }
    if ($proc.ExitCode -ne 0) {
        Write-Host "    CRASH exit=$($proc.ExitCode)" -ForegroundColor Red
        $err | Select-Object -First 3 | ForEach-Object { Write-Host "    $_" -ForegroundColor DarkRed }
    } else {
        $out | ForEach-Object { Write-Host "    >> $_" -ForegroundColor Green }
    }
    Remove-Item "_out.txt","_err.txt" -ErrorAction SilentlyContinue
}

# ── Minimal mod test ──────────────────────────────────────────────────────────
Write-Host "=== MOD / IF/ELSE DEBUG ===" -ForegroundColor Cyan

# Test 1: top-level modulo
$f = "tests\scratch\t_modonly.ok"
if (-not (Test-Path $f)) {
    "set a = 6`nset b = a % 2`nprint(b)`nset c = 3`nset d = c % 2`nprint(d)" | Out-File $f -Encoding ASCII
}
RunTest "top-level 6%2 and 3%2 (expect 0, 1)" $f

# Test 2: mod inside function
$f = "tests\scratch\t_mod_fn.ok"
"fn mod2(n):`n    return n % 2`nprint(mod2(6))`nprint(mod2(3))" | Out-File $f -Encoding ASCII
RunTest "mod inside function (expect 0, 1)" $f

# Test 3: if/else mod test
$f = "tests\scratch\t_ifmod.ok"
@"
fn test(n):
    if n % 2 == 0:
        print(1)
    else:
        print(0)

test(6)
test(3)
test(4)
test(7)
"@ | Out-File $f -Encoding ASCII
RunTest "if n%2==0 (expect 1,0,1,0)" $f

# Test 4: collatz minimal
$f = "tests\scratch\t_coll.ok"
@"
fn collatz(n):
    set steps = 0
    while n != 1:
        if n % 2 == 0:
            set n = n / 2
        else:
            set n = n * 3 + 1
        set steps = steps + 1
    return steps

print(collatz(2))
print(collatz(6))
"@ | Out-File $f -Encoding ASCII
RunTest "collatz(2)=1, collatz(6)=8" $f 8000

Write-Host ""
Write-Host "=== STRESS 08 (algorithms) ===" -ForegroundColor Cyan
RunTest "stress08_algorithms" "tests\stress08_algorithms.ok" 15000

Remove-Item "tests\scratch\t_mod_fn.ok","tests\scratch\t_ifmod.ok","tests\scratch\t_coll.ok" -ErrorAction SilentlyContinue
