cd C:\Users\akikf\programing\omnikarai\omniwin

function Test($label, $code, $expect, $ms=5000) {
    $code | Out-File -Encoding ASCII "_t.ok"
    $proc = Start-Process "bin\omnicc.exe" -ArgumentList "run","--quiet","_t.ok" `
        -PassThru -NoNewWindow -RedirectStandardOutput "_o.txt" -RedirectStandardError "_e.txt"
    if (-not $proc.WaitForExit($ms)) {
        $proc.Kill()
        Write-Host "  $label  -> HUNG" -ForegroundColor Red
    } else {
        $out = (Get-Content "_o.txt" -ErrorAction SilentlyContinue) -join ","
        $ok = if ($out -eq $expect) { "Green" } else { "Red" }
        Write-Host "  $label  -> '$out'  (expect '$expect')" -ForegroundColor $ok
    }
    Remove-Item "_t.ok","_o.txt","_e.txt" -ErrorAction SilentlyContinue
}

Write-Host "=== ISOLATING HANGING FUNCTION ===" -ForegroundColor Cyan

Test "is_prime(17)" @"
fn is_prime(n):
    if n < 2:
        return 0
    if n == 2:
        return 1
    if n % 2 == 0:
        return 0
    set i = 3
    while i * i <= n:
        if n % i == 0:
            return 0
        set i = i + 2
    return 1
print(is_prime(17))
print(is_prime(18))
"@ "1,0"

Test "gcd(48,18)" @"
fn gcd(a, b):
    while b != 0:
        set t = b
        set b = a % b
        set a = t
    return a
print(gcd(48, 18))
print(gcd(100, 75))
"@ "6,25"

Test "pow_mod(2,10,1000)" @"
fn pow_mod(base, exp, mod):
    set result = 1
    while exp > 0:
        if exp % 2 == 1:
            set result = result * base % mod
        set exp = exp / 2
        set base = base * base % mod
    return result
print(pow_mod(2, 10, 1000))
print(pow_mod(3, 5, 100))
"@ "24,43" 8000

Test "digit_sum(12345)" @"
fn digit_sum(n):
    if n < 0:
        set n = 0 - n
    set total = 0
    while n > 0:
        set total = total + n % 10
        set n = n / 10
    return total
print(digit_sum(12345))
print(digit_sum(999))
"@ "15,27"

Test "collatz(6)" @"
fn collatz(n):
    set steps = 0
    while n != 1:
        if n % 2 == 0:
            set n = n / 2
        else:
            set n = n * 3 + 1
        set steps = steps + 1
    return steps
print(collatz(6))
print(collatz(27))
"@ "8,111" 8000

Test "count_primes(20)" @"
fn is_prime(n):
    if n < 2:
        return 0
    if n == 2:
        return 1
    if n % 2 == 0:
        return 0
    set i = 3
    while i * i <= n:
        if n % i == 0:
            return 0
        set i = i + 2
    return 1
fn count_primes(limit):
    set count = 0
    set n = 2
    while n <= limit:
        if is_prime(n) == 1:
            set count = count + 1
        set n = n + 1
    return count
print(count_primes(20))
"@ "8" 8000
