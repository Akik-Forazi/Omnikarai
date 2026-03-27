# Omnikarai Development Roadmap
> **Creator:** Akik Fuad — Fraziym Tech & AI
> **Current version:** v5.0.2 (native x86-64, Windows)
> **Last updated:** March 18, 2026

---

## Vision

Omnikarai is a **universal, native-compiled language** — Python's readability, C-level speed, less memory than Rust, zero runtime dependencies. One compiler. Every domain.

```
source.ok  →  omnicc  →  x86-64 machine code  →  runs natively
```

---

## What Works Right Now (v5.0.2)

| Feature | Status |
|---------|--------|
| Full lexer (all tokens, INDENT/DEDENT) | ✅ Done |
| Full Pratt parser (all statements) | ✅ Done |
| Variables (`set`, reassignment) | ✅ Done |
| Integer arithmetic (`+ - * / %`) | ✅ Done |
| Float arithmetic | ✅ Done |
| Comparison operators | ✅ Done |
| Logical operators (`and`, `or`, `not`) | ✅ Done |
| `if / elif / else` (nested + chained) | ✅ Done |
| `while` + `break` / `continue` | ✅ Done |
| `for i in range(n)` | ✅ Done |
| `match / case` + wildcard | ✅ Done |
| User-defined functions | ✅ Done |
| Recursive functions | ✅ Done |
| Nested function calls | ✅ Done |
| `return` + dead code suppression | ✅ Done |
| `print()` (int, str, bool, float) | ✅ Done |
| `input()`, `len()`, `int()`, `str()` | ✅ Done |
| `assert(cond, msg)` | ✅ Done |
| String literals + concatenation | ✅ Done |
| Boolean literals (`true`/`false`) | ✅ Done |
| `if/else` inside `while` loops | ✅ Fixed v5.0.2 |
| Modulo with constant divisors | ✅ Fixed v5.0.2 |
| Module: `time` | ✅ Done |
| Module: `datetime` | ✅ Done |
| Module: `math` | ✅ Done |
| Module: `os` | ✅ Done |
| Module: `io` | ✅ Done |
| Module: `sys` | ✅ Done |
| Module: `list` | ✅ Done |
| Module: `str` | ✅ Done |
| VS Code extension + 3 themes | ✅ Done |

**Test suite: 15/15 passing. Stress suite: 9/9 passing.**

---

## Bug History (Fixed in v5.0.2)

| Bug | Root Cause | Fix |
|-----|-----------|-----|
| `if/else` inside `while` always took else | Barrett reduction (`MUL` instruction) produced wrong results in JIT memory — `MUL RCX` with magic number gave RDX=0 at runtime despite correct simulation | Replaced Barrett with `CQO + IDIV` for all modulo ops |
| `n % 2` returning `n` instead of `0`/`1` | Same Barrett/MUL bug | Same fix |
| `pow_mod` returning wrong results | Same root cause via `% mod` in while loop | Same fix |
| `collatz` hanging forever | Same root cause — `if n%2==0` always took else branch | Same fix |
| Shadow space overwriting locals | Function frame too small — `stack_size=0` meant `aligned_frame()` gave 48 bytes, putting shadow space `[rsp+8..rsp+32]` at `[rbp-40..rbp-16]`, overlapping locals | Set `stack_size=32` minimum so frame is always at least 64 bytes |
| t12_io failing | `io.delete` and `io.exists` lines were commented out in test file | Uncommented them |

---

## Codegen Optimizations (v5.0.2)

| Optimization | Impact |
|-------------|--------|
| LHS identifier fast path in `cg_infix` | Eliminates tmp slot + 2 memory ops when left side is a variable — affects every `a + b`, `a * b`, `a < b` expression |
| `% 2^k` → `AND RAX, mask` | 1 cycle vs 40–90 cycles for IDIV |
| `/ 2^k` → `SAR RAX, k` | 1 cycle vs 40–90 cycles for IDIV |
| `/ d` (const, non-power-of-2) → `MOV RCX,d; CQO; IDIV` | Avoids temp slot allocation |

---

## Benchmark Results (v5.0.2, i5-8350U, 15W)

| Benchmark | Omnikarai | C (-O2) | Ratio |
|-----------|-----------|---------|-------|
| Loop sum 100M | 400ms | ~0ms* | — |
| Fibonacci fib(40)×5 | 8091ms | 2188ms | 3.7× slower |
| Primes to 100k ×30 | 1284ms | 359ms | 3.6× slower |
| Dot product ×10000 | 54ms | ~0ms* | — |
| Matmul 32×32 ×5000 | 4274ms | 600ms | 7.1× slower |

*C optimizer eliminates the loop entirely with constant folding.

**Gap vs C:** ~3.5–7× on compute-heavy benchmarks. No register allocator, no vectorization, no optimization passes — this is the baseline for a single-pass JIT.

---

## Known Remaining Gaps vs C

1. **No register allocator** — every variable hits the stack on every access. Hot loop vars like `k`, `acc` bounce through `[rbp-N]` on every iteration. A linear-scan allocator would cut the matmul gap to ~2×.
2. **No loop-invariant code motion** — `(i + k) % 17` recomputes on every inner iteration even though `17` is constant.
3. **No vectorization** — loop bodies process one element at a time. AVX2 would do 4 int64 per cycle.
4. **No inlining of user functions in hot paths** — `is_prime(n)` call overhead adds up over 30 reps × 100k iterations.
5. **Division** — even with SAR for powers of 2, general IDIV is still 40–90 cycles.

---

## Next Priority: Close the Gap

### Immediate (highest ROI, low complexity)
- [ ] **Register allocator (linear scan)** — assign hot while-loop vars to `rbx`, `rsi`, `rdi` (callee-saved). Save/restore around calls. Biggest single speedup.
- [ ] **Leaf function inline expansion** — if a called function has no calls itself, inline it at the call site. Eliminates `is_prime` call overhead in primes benchmark.
- [ ] **Strength reduction: `x * c` where c is power-of-2** → `SHL RAX, k`

### Near-term (language features)
- [ ] **f-strings** — `f"Hello {name}!"` — highest QoL impact
- [ ] **`omnicc build`** — emit standalone PE `.exe` without needing the compiler to run
- [ ] **List indexing** — `lst[i]` read/write (parser done, codegen missing)
- [ ] **`for item in list`** iteration

### Medium-term
- [ ] **Self-hosting** — rewrite the compiler in Omnikarai itself
- [ ] **`const` keyword** — compile-time constant folding
- [ ] **Multiple return values** — `return a, b` + `set x, y = fn()`
- [ ] **Class system** — `class Name:` with `self.field` and method calls

### Long-term
- [ ] **AI module** — AVX2 matmul, relu, dot, softmax (beat C -O3 on 4-layer MLP)
- [ ] **Cross-platform** — Linux x64 (syscalls + ELF), macOS ARM64
- [ ] **Concurrency** — `thread.run`, lock-free channels, no GIL

---

## Phase 1 — f-strings

`print(f"Hello {name}, you are {age} years old!")`

- [ ] Lexer: detect `f"..."` → emit `TOKEN_FSTRING`
- [ ] Parser: parse `{expr}` segments → build concat AST
- [ ] Codegen: emit `str() + str.concat` chain — zero overhead vs manual
- [ ] Test: int, float, str, bool, expressions all work inside `{}`

---

## Phase 2 — PE Writer (`omnicc build`)

Emit a valid standalone Windows PE32+ `.exe`:
- [ ] DOS + PE + optional header
- [ ] `.text` section with JIT machine code
- [ ] `.rdata` section with string pool
- [ ] Import table for `kernel32.dll` only
- [ ] No CRT, no MSVCRT — self-contained < 50 KB for Hello World

---

## Phase 3 — Register Allocator

Linear-scan allocation for function-scope variables:
- [ ] Liveness analysis: track which vars are live at each instruction
- [ ] Assign `rbx`, `rsi`, `rdi`, `r12`, `r13` to hottest vars
- [ ] Save/restore callee-saved registers in function prologue/epilogue
- [ ] Spill to stack when registers exhausted
- [ ] Expected speedup: 2–3× on matmul, primes, fib

---

## Phase 4 — Collections Runtime

- [ ] List literal `[a, b, c]` codegen (parser done)
- [ ] List index `lst[i]` read and write
- [ ] `for item in list` iteration
- [ ] Dict literal `{"key": val}` codegen (parser done)
- [ ] Dict index `d["key"]` read/write
- [ ] Tuple + destructuring

---

## Phase 5 — AI Module (Speed God)

```
Hardware:    i5-8350U, 15W, no GPU
Model:       4-layer MLP, hidden=128, batch=1, FP32
Target:      < 0.020ms/inference
Beat:        C -O3 -march=native -mavx2 (~0.050ms)
```

- [ ] `ai.matmul(A, x)` → inline AVX2 VFMADD231PS
- [ ] `ai.relu(x)` → inline VMAXPS
- [ ] `ai.dot(a, b)` → register-only dot product
- [ ] `ai.softmax(x)`, `ai.layernorm(x)`
- [ ] INT8 quantized matmul via VPMADDUBSW (4× throughput)
- [ ] Load GGUF weights: `ai.load_gguf(path)`

---

## Phase 6 — Self-Hosting

Rewrite the compiler in Omnikarai itself:
1. Port lexer → `lexer.ok`
2. Port parser → `parser.ok`
3. Port codegen → `codegen.ok`
4. `omnicc run compiler.ok source.ok` → produces same output as C version
5. Bootstrap: compiler compiles itself

---

## Build Command

```bash
gcc -Iinclude -O2 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32
```

## Run Tests

```powershell
.\tests\run_tests.ps1
.\tests\run_stress.ps1
.\benchmarks\run_benchmarks.ps1
```

## Project Structure

```
omniwin/
  src/           parser.c, codegen.c, lexer.c, main.c
  include/       lexer.h, ast.h, parser.h, codegen.h
  bin/           omnicc.exe
  tests/         t01-t15, stress01-09, run_tests.ps1, run_stress.ps1
  tests/scratch/ debug/experiment .ok files
  benchmarks/    bench_*.ok, bench_*_c.exe, bench_*_go.exe, run_benchmarks.ps1
  docs/          development_plan.md
  build_and_test.ps1
  run_all.ps1
  Makefile, README.md
```
