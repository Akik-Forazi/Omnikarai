# OMNIKARAI BENCHMARK RESULTS
# Date: 2026-03-23
# Build: gcc -O2 (omnicc), gcc/g++ -O3 -march=native -mavx2 (C/C++)
# Python 3.13.11 | Node.js v24.11.1 | g++ 15.2.0
# Machine: Windows x64 (Akik's dev machine)
# Tests: 20/20 PASS

## Full Results

### Loop 100M
| Lang       | Time     | vs Omnikarai |
|------------|----------|--------------|
| Omnikarai  | 363ms    | baseline     |
| C -O3      | ~0ms*    | C too fast*  |
| C++ -O3    | ~0ms*    | C++ too fast*|
| Python     | 21363ms  | 58.9x SLOWER |
| Node.js    | 7232ms   | 19.9x SLOWER |

*C/C++ optimize the loop away at -O3 (dead store elimination). Not a fair comparison.
 Omnikarai correctly computes and prints the result — genuine 100M iterations.

### Fib(40) x5
| Lang       | Time     | vs Omnikarai |
|------------|----------|--------------|
| Omnikarai  | 7992ms   | baseline     |
| C -O3      | 2100ms   | 3.81x faster |
| C++ -O3    | 1822ms   | 4.39x faster |
| Python     | 182396ms | 22.8x SLOWER |
| Node.js    | 17358ms  | 2.2x SLOWER  |

Gap vs C: 3.81x. Root cause: no function-level register allocation.
Each recursive call spills all vars to stack. C keeps them in registers.
Node.js JIT is impressive here — only 2.2x slower than Omnikarai.

### Primes x30
| Lang       | Time     | vs Omnikarai |
|------------|----------|--------------|
| Omnikarai  | 1899ms   | baseline     |
| C -O3      | 799ms    | 2.38x faster |
| C++ -O3    | 494ms    | 3.85x faster |
| Python     | 21520ms  | 11.3x SLOWER |
| Node.js    | 616ms    | 0.3x SLOWER* |

*Node.js is FASTER than Omnikarai on primes (616ms vs 1899ms).
 V8 JIT inlines is_prime aggressively. This is a known weakness in our
 function call overhead — is_prime cannot be inlined (has a while loop).
 Fix: function-level register allocation will reduce call overhead.

### Dotprod x10k (Python=1000 reps, rest=10000)
| Lang       | Time     | vs Omnikarai |
|------------|----------|--------------|
| Omnikarai  | 55ms     | baseline     |
| C -O3      | ~0ms*    | C too fast*  |
| C++ -O3    | ~0ms*    | C++ too fast*|
| Python     | 158ms    | 2.9x SLOWER  |
| Node.js    | 28ms     | 0.5x SLOWER* |

*C/C++ and Node.js both eliminate/optimize the inner loop at -O3/JIT.
 Node.js is faster than Omnikarai here (28ms vs 55ms) — V8 JIT wins.

### Matmul x5k (Python=200 reps scaled)
| Lang       | Time     | vs Omnikarai |
|------------|----------|--------------|
| Omnikarai  | 7299ms   | baseline     |
| C -O3      | 201ms    | 36.35x faster|
| C++ -O3    | 217ms    | 33.64x faster|
| Python     | 2515ms*  | 0.3x SLOWER* |
| Node.js    | 2207ms   | 0.3x SLOWER  |

*Python ran 200 reps (not 5000). Extrapolated full run: ~62,875ms (86x slower).
 Node.js ran full 5000 reps: 2207ms — Omnikarai is SLOWER than Node.js here.
 Root cause: triple-nested while loop spills ALL variables to stack every iteration.
 C keeps acc/a/b/i/j/k all in registers. Omnikarai does 3 loads + 1 store per iteration.

## Summary

| Benchmark    | vs C -O3 | vs C++ -O3 | vs Python | vs Node.js |
|--------------|----------|------------|-----------|------------|
| Loop 100M    | N/A*     | N/A*       | 58.9x     | 19.9x      |
| Fib(40) x5   | 3.81x    | 4.39x      | 22.8x     | 2.2x       |
| Primes x30   | 2.38x    | 3.85x      | 11.3x     | -0.3x**    |
| Dotprod x10k | N/A*     | N/A*       | 2.9x      | -0.5x**    |
| Matmul x5k   | 36.35x   | 33.64x     | >8x       | -0.3x**    |

*C/C++ optimize away the computation entirely — not comparable.
**Negative = Node.js V8 JIT beats Omnikarai. This is the honest reality.

## What this means

### Wins
- Omnikarai beats Python by 3x to 58x across all benchmarks
- Omnikarai beats Node.js on loop-heavy integer work (20x on loop sum)
- Omnikarai runs correctly (no dead code elimination cheating)
- 20/20 tests pass — compiler is correct

### Gaps
- C/C++ beat Omnikarai 2.4x to 36x on compute benchmarks
- Node.js V8 JIT beats Omnikarai on primes, dotprod, matmul
  (V8 has years of JIT optimization; Omnikarai has none yet)
- Root cause for ALL gaps: no register allocation
  Every variable spills to stack. C keeps hot vars in registers.

### What fixes it
ONE change closes most of the gap: function-level register allocator
- Identify 3 hottest local vars per function (e.g. acc, a, b in matmul)
- Pin them to rbx/r12/r13 (callee-saved, Windows x64)
- Save/restore in function prologue/epilogue
- Expected: matmul 7299ms → ~300ms (matching C/C++ range)
- Expected: primes 1899ms → ~500ms (beating Node.js again)
- Expected: fib    7992ms → ~3000ms (closing gap to C)

## Next Target

| Benchmark    | Current  | Target (after regalloc) | C -O3  |
|--------------|----------|------------------------|--------|
| Fib(40) x5   | 7992ms   | ~2500ms                | 2100ms |
| Primes x30   | 1899ms   | ~400ms                 | 799ms  |
| Matmul x5k   | 7299ms   | ~300ms                 | 201ms  |
