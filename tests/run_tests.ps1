# ================================================================
#  Omnikarai v6.02.24 -- Full Test Suite Runner
#  Run from omniwin directory:
#    .\tests\run_tests.ps1
# ================================================================

$ErrorActionPreference = "Continue"
$root      = Split-Path -Parent $PSScriptRoot
$omnicc    = "$root\bin\omnicc.exe"
$tests_dir = "$root\tests"

function Grn($m)  { Write-Host $m -ForegroundColor Green }
function Red($m)  { Write-Host $m -ForegroundColor Red }
function Cyn($m)  { Write-Host $m -ForegroundColor Cyan }
function Yel($m)  { Write-Host $m -ForegroundColor Yellow }
function Wht($m)  { Write-Host $m -ForegroundColor White }

Write-Host ""
Cyn  "======================================================"
Cyn  "       OMNIKARAI v6.02.24  --  FULL TEST SUITE         "
Cyn  "======================================================"
Write-Host ""

if (-not (Test-Path $omnicc)) {
    Red "ERROR: omnicc.exe not found at $omnicc"
    Red "Build: gcc -Iinclude -O2 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 -lm"
    exit 1
}

Yel "Compiler: $omnicc"
Write-Host ""

# ----------------------------------------------------------------
# Test definitions
# expect = comma-separated expected output lines (first N lines)
# Use "*" to just check it runs without crash
# ----------------------------------------------------------------
$tests = @(
    @{ file="t01_core_arithmetic.ok"; name="Core Arithmetic";        expect="10,3,13,7,30,3,1,true,true,true,true,true,true" },
    @{ file="t02_logic.ok";           name="Logic operators";         expect="true,false,true,false,false,true" },
    @{ file="t03_if_elif_else.ok";    name="if/elif/else";            expect="A,B,C,F,pass,fail" },
    @{ file="t04_loops.ok";           name="Loops+break+continue";    expect="0,1,2,3,4,1,2,4,5,6,0,1,2,3,4" },
    @{ file="t05_functions.ok";       name="Functions+recursion";     expect="15,20,120,25,55" },
    @{ file="t06_match.ok";           name="Match/case";              expect="one,two,three,other,small,big" },
    @{ file="t07_strings.ok";         name="Strings";                 expect="42,hello world,11,helloworld,5,100" },
    @{ file="t08_time.ok";            name="Time module";             expect="*" },
    @{ file="t09_math.ok";            name="Math module";             expect="7,10,3,4,5,1,10" },
    @{ file="t10_datetime.ok";        name="Datetime module";         expect="*" },
    @{ file="t11_os.ok";              name="OS module";               expect="windows,*,*,1" },
    @{ file="t12_io.ok";              name="IO module";               expect="1,1,Hello Omnikarai,1,1,0" },
    @{ file="t13_sys.ok";             name="Sys module";              expect="Omnikarai v6.02.24 (x86-64 Windows),windows-x64,6.02.24,64" },
    @{ file="t14_list.ok";            name="List module";             expect="0,3,10,20,30,30,2,1,0" },
    @{ file="t15_assert.ok";          name="Assert builtin";          expect="ok,done" },
    @{ file="t16_ai_alloc.ok";        name="AI alloc/set/get/free";   expect="1065353216,1090519040,0" },
    @{ file="t17_ai_relu.ok";         name="AI relu (AVX2)";          expect="0,0,0,1073741824" },
    @{ file="t18_ai_dot.ok";          name="AI dot product";          expect="1106771968" },
    @{ file="t19_ai_matmul.ok";       name="AI matmul";               expect="1077936128,1090519040" },
    @{ file="t20_ai_dot_i8.ok";       name="AI dot_i8 (INT8)";        expect="36" },
    @{ file="t21_fixes.ok";           name="Bug fixes (power/str/idx)"; expect="8,100,1,hello,world,done,10,99,15" }
)

# ----------------------------------------------------------------
$pass = 0; $fail = 0; $crash = 0
$results = @()

foreach ($t in $tests) {
    $filepath = Join-Path $tests_dir $t.file
    $name     = $t.name
    $expect   = $t.expect

    if (-not (Test-Path $filepath)) {
        $results += [PSCustomObject]@{ Status="SKIP"; Name=$name; Reason="file not found" }
        continue
    }

    $output   = & $omnicc run --quiet $filepath 2>&1
    $exitCode = $LASTEXITCODE

    if ($exitCode -ne 0 -and $null -ne $exitCode) {
        $crash++
        $results += [PSCustomObject]@{ Status="CRASH"; Name=$name; Reason="exit $exitCode"; Output=$output }
        continue
    }

    $actual_lines_raw = ($output -split "`n") | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" }
    if ($expect -eq "*") {
        $pass++
        $results += [PSCustomObject]@{ Status="PASS"; Name=$name; Reason="ran OK"; Output=$actual_lines_raw }
        continue
    }

    $expected_lines = $expect -split ","
    $actual_lines   = $actual_lines_raw

    $ok = $true; $fail_reason = ""
    for ($i = 0; $i -lt $expected_lines.Count; $i++) {
        $exp = $expected_lines[$i].Trim()
        if ($exp -eq "*") { continue }   # wildcard line -- skip check
        $act = if ($i -lt $actual_lines.Count) { $actual_lines[$i] } else { "(missing)" }
        if ($act -ne $exp) {
            $ok = $false
            $fail_reason = "line $($i+1): expected '$exp' got '$act'"
            break
        }
    }

    if ($ok) {
        $pass++
        $results += [PSCustomObject]@{ Status="PASS"; Name=$name; Reason=""; Output=$actual_lines }
    } else {
        $fail++
        $results += [PSCustomObject]@{ Status="FAIL"; Name=$name; Reason=$fail_reason; Output=$actual_lines }
    }
}

# ----------------------------------------------------------------
Write-Host ""
Wht ("-" * 62)
Write-Host ("  {0,-4}  {1,-32}  {2}" -f "No","Test","Result")
Wht ("-" * 62)

$i = 1
foreach ($r in $results) {
    $num = "{0:D2}" -f $i
    $n   = $r.Name
    switch ($r.Status) {
        "PASS"  {
            Grn ("  $num    {0,-32}  [PASS]" -f $n)
            if ($r.Output) {
                foreach ($line in $r.Output) { Write-Host ("          >> $line") -ForegroundColor DarkGreen }
            }
        }
        "FAIL"  {
            Red ("  $num    {0,-32}  [FAIL]  -- {1}" -f $n, $r.Reason)
            if ($r.Output) {
                foreach ($line in $r.Output) { Write-Host ("          >> $line") -ForegroundColor DarkRed }
            }
        }
        "CRASH" {
            Red ("  $num    {0,-32}  [CRASH] -- {1}" -f $n, $r.Reason)
            if ($r.Output) {
                foreach ($line in $r.Output) { Write-Host ("          >> $line") -ForegroundColor DarkRed }
            }
        }
        "SKIP"  { Yel ("  $num    {0,-32}  [SKIP]  -- {1}" -f $n, $r.Reason) }
    }
    $i++
}

Wht ("-" * 62)
Write-Host ""

$color = if ($fail -eq 0 -and $crash -eq 0) { "Green" } else { "Red" }
Write-Host ("  Results: {0}/{1} passed  |  {2} failed  |  {3} crashed" -f $pass,$tests.Count,$fail,$crash) -ForegroundColor $color
Write-Host ""

if ($fail -eq 0 -and $crash -eq 0) {
    Grn "  ALL TESTS PASSED -- Omnikarai v6.02.24 is working correctly!"
} else {
    Red "  SOME TESTS FAILED -- see above"
    Write-Host ""
    Yel "  Debug a failing test:"
    Yel "    .\bin\omnicc.exe run --beta tests\<file>.ok"
}
Write-Host ""
exit ($fail + $crash)
