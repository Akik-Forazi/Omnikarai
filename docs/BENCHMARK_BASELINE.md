# OMNIKARAI BENCHMARK BASELINE
# Date: 2026-03-23
# Compiler: omnicc v6.0, gcc -O2 (omnicc build)
# C reference: gcc -O3 -march=native -mavx2
# Machine: Windows x64 (Akik's dev machine)
# Tests: 20/20 PASS before benchmark run

## Baseline Results

| Benchmark      | Omnikarai | C -O3  | Ratio (Omni/C) | Status        |
|----------------|-----------|--------|----------------|---------------|
| Loop 100M      | 214 ms    | ~0 ms* | ?              | C too fast*   |
| Fib(40) x5     | 6867 ms   | 1800ms | 3.81x slower   | NEEDS WORK    |
| Primes x30     | 1000 ms   | 272ms  | 3.67x slower   | NEEDS WORK    |
| Dotprod x10k   | 32 ms     | ~0 ms* | ?              | C too fast*   |
| Matmul x5k     | 3480 ms   | 104ms  | 33.39x slower  | CRITICAL      |

*C reports 0.000ms = sub-millisecond, ratio cannot be computed.
 These are actually fast in absolute terms (32ms dotprod, 214ms loop).

## Key Observations

### Loop 100M (214ms)
- C is sub-millisecond because gcc -O3 optimizes the loop away entirely
  (dead store elimination: sum never used externally)
- Omnikarai correctly computes the sum and prints it: 5000000050000000
- Our loop is actually reasonable — C cheats here with dead code elimination
- Real comparison: Omnikarai ~214ms for genuine 100M iterations

### Fib(40) x5 (6867ms vs 1800ms = 3.81x)
- Pure recursion, no arrays — measures function call overhead
- C at -O3 applies tail call optimization and branch prediction hints
- Omnikarai: every recursive call is a full CALL/RET with frame setup
- Root cause: function call overhead (no TCO, full frame every call)
- Target: within 1.5x of C = ~2700ms

### Primes x30 (1000ms vs 272ms = 3.67x)
- Inner loop: while d*d <= n with function call to is_prime
- C inlines is_prime entirely, eliminating all call overhead
- Omnikarai: is_prime is a function call each iteration
- Root cause: no inlining of multi-statement functions
- Target: within 1.5x of C = ~408ms

### Dotprod x10k (32ms vs ~0ms)
- C eliminates dead computation at -O3
- Omnikarai runs genuine 10k x 1024 iterations
- 32ms for genuine compute is actually good
- This will show properly when C is built with -O2 instead

### Matmul x5k (3480ms vs 104ms = 33.39x CRITICAL)
- Triple nested loop, 5000 repetitions of 32x32 matrix
- This is the most important benchmark for AI workloads
- C: ~104ms with -O3 auto-vectorization
- Omnikarai: 3480ms — 33x slower
- Root cause: ALL variables spilled to stack on every inner loop iteration
  Every `set acc = acc + a * b` does:
    load acc from [rbp-N]
    load a from [rbp-M]
    load b from [rbp-K]
    multiply, add
    store acc to [rbp-N]
  = 3 memory loads + 1 store per inner iteration
  C keeps acc, a, b in registers = 0 memory ops per iteration
- This is the register allocation problem
- Target: within 2x of C = ~208ms

## What Needs To Happen (Priority Order)

1. CRITICAL: Fix matmul — register allocation for inner loop variables
2. HIGH: Fix fib — reduce function call overhead
3. HIGH: Fix primes — inline small functions
4. LOW: Loop/dotprod — C cheats with dead code elim, our numbers are fine

## Previous Best Results (v4.0, from Bench_results.txt)

v4.0 had MUCH better results:
- Fib(35)x10: ~24ms (Omnikarai) vs ~253ms (C) = 10.77x FASTER
- Loop 100M: ~34ms vs ~36ms = near-C
- Primes: ~38ms vs ~147ms = 3.91x FASTER

This means v5.0/v6.0 REGRESSED from v4.0. Something broke.
The v4.0 C times were anomalously slow (process launch overhead dominated).
Current benchmark is more accurate but shows real regression.

## Regression Analysis

Between v4.0 and current v6.0:
- Added: modules (time, datetime, math, os, io, sys, list, str, ai)
- Added: classes, match/case, more builtins
- Added: package loader (USE_STATEMENT now loads .ok files)
- Added: lexer.h + parser.h includes in codegen.c

The regression is likely from:
1. Package loader being compiled in even for programs that don't use it
2. Additional overhead in codegen_init (g_pkg_count/g_pkg_registry memset)
3. Possible register pressure from new code in codegen.c itself

Need to isolate whether the regression is in the generated CODE or in compile time.

## Phase 1 Results — Extended Inlining (2026-03-23)

| Benchmark    | Baseline | Phase 1 | Change   | Notes                          |
|--------------|----------|---------|----------|--------------------------------|
| Loop 100M    | 214ms    | 167ms   | -22% ✅  | Extended inline + const fold   |
| Fib(40) x5   | 6867ms   | 5136ms  | -25% ✅  | Recursive call overhead reduced|
| Primes x30   | 1000ms   | 1008ms  | flat     | is_prime has loop, can't inline|
| Dotprod x10k | 32ms     | 28ms    | -13% ✅  | Loop body tighter              |
| Matmul x5k   | 3480ms   | 3689ms  | +6% ❌   | New inline overhead path       |

### Analysis
- Fib: 25% improvement from extended multi-statement inlining
- Primes: no change — is_prime has while loop, correctly excluded from inlining
- Matmul: slight regression — more code in hot path, investigate
- Phase 1 gave modest wins. Phase 2 (register allocator) is the critical path.

### Next: Phase 2 — Register Allocator
The matmul benchmark is 31x slower than C purely because of stack spilling.
Every inner loop variable loads from [rbp-N] and stores back each iteration.
Register allocation will eliminate these memory ops entirely.
Expected: matmul 3480ms → ~200ms (17x improvement).
