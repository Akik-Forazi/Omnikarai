$root = "C:\Users\akikf\programing\omnikarai\omniwin"
Set-Location $root

# ── 1. Move all root-level .ok scratch/debug files to tests\scratch ──────────
$rootOk = @(
    "test.ok","test_advanced.ok","test_comprehensive.ok","test_v2.ok","test_v3.ok",
    "test_time.ok","test_time_format.ok","test_time_simple.ok","test_nested_fn.ok",
    "test_tail.ok","test_ne.ok",
    "debug_step1.ok","debug_step2.ok","debug_step3.ok",
    "_break_test.ok","_continue_test.ok",
    "_d1.ok","_d2.ok","_d3.ok","_debug.ok","_debug_fact.ok","_df.ok",
    "_fn_test.ok","_for_test.ok","_if_last.ok","_jtest.ok","_loop_test.ok",
    "_min.ok","_r2.ok","_r3.ok","_r4.ok","_range_test.ok","_simple_while.ok",
    "_tmp.ok","_while2.ok","_while_break.ok","_while_if.ok"
)
foreach ($f in $rootOk) {
    if (Test-Path "$root\$f") {
        Move-Item "$root\$f" "$root\tests\scratch\$f" -Force
    }
}

# ── 2. Move debug/dump .ps1 scripts to scripts\ ───────────────────────────────
$debugScripts = @(
    "cmp_collatz.ps1","debug_run.ps1",
    "do_dump.ps1","do_dump2.ps1","do_dump3.ps1",
    "dump_both.ps1","organize.ps1",
    "test_collatz.ps1","test_debug.ps1","test_guard.ps1",
    "test_mod.ps1","test_mod2.ps1","test_steps.ps1"
)
foreach ($f in $debugScripts) {
    if (Test-Path "$root\$f") {
        Move-Item "$root\$f" "$root\scripts\$f" -Force
    }
}

# ── 3. Move t_* scratch tests + extras from tests\ to tests\scratch\ ──────────
$scratchTests = @(
    "t_collatz.ok","t_collatz2.ok","t_dot_debug.ok",
    "t_dt_min.ok","t_dt_min2.ok","t_dt_min3.ok",
    "t_fac_minimal.ok","t_fn_simple.ok",
    "t_modonly.ok","t_mod_iftest.ok","t_mod_test.ok",
    "t_parse_debug.ok","t_rec_debug.ok",
    "t_steps.ok","t_steps_debug.ok","t_steps_guard.ok",
    "t_while_for.ok","t05_fact.ok","t05_simple.ok",
    "run_tests_tmp.ps1"
)
foreach ($f in $scratchTests) {
    if (Test-Path "$root\tests\$f") {
        Move-Item "$root\tests\$f" "$root\tests\scratch\$f" -Force
    }
}

# ── 4. Delete stale dump/temp files ───────────────────────────────────────────
$toDelete = @(
    "$root\dump_guard.txt","$root\dump_mod.txt",
    "$root\out.txt","$root\err.txt","$root\err_t04.txt",
    "$root\test_io_tmp.txt","$root\tests\test_io_tmp.txt",
    "$root\test_dt.c","$root\test_dt.exe",
    "$root\test_comprehensive.expected"
)
foreach ($f in $toDelete) {
    if (Test-Path $f) { Remove-Item $f -Force }
}

Write-Host "Organized." -ForegroundColor Green
