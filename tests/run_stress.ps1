# ================================================================
#  Omnikarai v5.0 -- Stress Test Suite
#  Run from omniwin directory:
#    .\tests\run_stress.ps1
# ================================================================

$ErrorActionPreference = "Continue"
$root   = Split-Path -Parent $PSScriptRoot
$omnicc = "$root\bin\omnicc.exe"

function Grn($m) { Write-Host $m -ForegroundColor Green }
function Red($m) { Write-Host $m -ForegroundColor Red }
function Cyn($m) { Write-Host $m -ForegroundColor Cyan }
function Yel($m) { Write-Host $m -ForegroundColor Yellow }
function Wht($m) { Write-Host $m -ForegroundColor White }

Write-Host ""
Cyn  "======================================================"
Cyn  "     OMNIKARAI v5.0  --  STRESS TEST SUITE           "
Cyn  "======================================================"
Write-Host ""

if (-not (Test-Path $omnicc)) {
    Red "ERROR: omnicc.exe not found at $omnicc"
    Red "Build: gcc -Iinclude -O2 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 -lm"
    exit 1
}

Yel "Compiler: $omnicc"
Write-Host ""

$tests = @(
    @{ file="stress01_arithmetic.ok";   name="Arithmetic Extremes"   },
    @{ file="stress02_control_flow.ok"; name="Control Flow Extremes" },
    @{ file="stress03_functions.ok";    name="Functions Extremes"    },
    @{ file="stress04_strings.ok";      name="Strings Extremes"      },
    @{ file="stress05_lists.ok";        name="Lists Extremes"        },
    @{ file="stress06_modules.ok";      name="Modules Extremes"      },
    @{ file="stress07_logic.ok";        name="Logic Extremes"        },
    @{ file="stress08_algorithms.ok";   name="Algorithms"            },
    @{ file="stress09_combined.ok";     name="Combined Stress"       }
)

$pass = 0; $crash = 0

Wht ("-" * 62)
Write-Host ("  {0,-4}  {1,-30}  {2}" -f "No","Test","Result")
Wht ("-" * 62)

$i = 1
foreach ($t in $tests) {
    $filepath = Join-Path $root "tests\$($t.file)"
    if (-not (Test-Path $filepath)) {
        Yel ("  {0:D2}    {1,-30}  [SKIP] not found" -f $i, $t.name)
        $i++; continue
    }

    $proc = Start-Process -FilePath $omnicc -ArgumentList "run","--quiet",$filepath `
        -PassThru -NoNewWindow `
        -RedirectStandardOutput "$root\_stress_out.txt" `
        -RedirectStandardError  "$root\_stress_err.txt"
    $timedOut = $false
    if (-not $proc.WaitForExit(10000)) {
        $proc.Kill()
        $timedOut = $true
    }
    $exitCode = if ($timedOut) { -1 } else { $proc.ExitCode }
    $output = @()
    if (Test-Path "$root\_stress_out.txt") { $output += Get-Content "$root\_stress_out.txt" }
    if (Test-Path "$root\_stress_err.txt") { $output += Get-Content "$root\_stress_err.txt" }
    Remove-Item -ErrorAction SilentlyContinue "$root\_stress_out.txt","$root\_stress_err.txt"

    if ($timedOut) {
        $crash++
        Red ("  {0:D2}    {1,-30}  [TIMEOUT] >10s" -f $i, $t.name)
    } elseif ($exitCode -ne 0 -and $null -ne $exitCode) {
        $crash++
        Red ("  {0:D2}    {1,-30}  [CRASH] exit={2}" -f $i, $t.name, $exitCode)
        $lines = ($output) | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" }
        foreach ($line in ($lines | Select-Object -First 5)) {
            Write-Host "          >> $line" -ForegroundColor DarkRed
        }
    } else {
        $pass++
        $lines = ($output) | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" }
        $total = $lines.Count
        Grn ("  {0:D2}    {1,-30}  [PASS]  ({2} lines)" -f $i, $t.name, $total)
        foreach ($line in ($lines | Select-Object -First 4)) {
            Write-Host "          >> $line" -ForegroundColor DarkGreen
        }
        if ($total -gt 4) {
            Write-Host "          >> ... ($($total - 4) more)" -ForegroundColor DarkGreen
        }
    }
    $i++
}

Wht ("-" * 62)
Write-Host ""
$color = if ($crash -eq 0) { "Green" } else { "Red" }
Write-Host ("  Results: {0}/{1} passed  |  {2} crashed" -f $pass,$tests.Count,$crash) -ForegroundColor $color
Write-Host ""
if ($crash -eq 0) {
    Grn "  All stress tests passed!"
} else {
    Yel "  Debug a failing test:"
    Yel "    .\bin\omnicc.exe run --beta tests\<file>.ok"
}
Write-Host ""
exit $crash
