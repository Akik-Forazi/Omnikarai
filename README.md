# Omnikarai

**A compiled language that produces native x86-64 machine code directly.**  
No LLVM. No runtime. No VM. One binary. Ships as `omnicc.exe`.

```
print("Hello, World!")
```

```
omnicc run hello.ok
→ Hello, World!
```

---

## What it is

Omnikarai is a programming language with Python-like syntax that compiles straight to native Windows x86-64 machine code. The compiler handles everything — lexing, parsing, and code generation — and produces real machine instructions that run at CPU speed with zero overhead layers.

The compiler is a single C file under 4,000 lines. The entire toolchain is one executable.

---

## Quick start

```powershell
# Run a program
omnicc run myprogram.ok

# Build a standalone .exe
omnicc build myprogram.ok

# Inspect the generated machine code
omnicc dump myprogram.ok

# Check for syntax errors without running
omnicc check myprogram.ok
```

---

## The language

Omnikarai looks like Python. It runs like C.

```python
fn fib(n):
    if n <= 1:
        return n
    return fib(n - 1) + fib(n - 2)

print(fib(10))
```

```python
use time

set t0 = time.now()
set i = 0
set total = 0
while i < 1000000:
    set total = total + i
    set i = i + 1
set ms = time.ms(t0)
print(total)
print(ms)
```

**Core features:**
- Indentation-based blocks (no braces, no semicolons)
- Type inference — no annotations needed
- Functions, recursion, nested calls
- `if / elif / else`, `while`, `for i in range(n)`, `match / case`
- Lists, strings, booleans, integers, floats
- Modules: `math`, `io`, `os`, `sys`, `time`, `datetime`, `list`, `str`, `ai`
- AVX2/FMA matrix operations in the `ai` module
- `use` imports from the OPI package registry

**Standard library modules:**

| Module | What it does |
|--------|-------------|
| `time` | High-precision timers via QueryPerformanceCounter |
| `datetime` | System clock, date formatting, timezone |
| `math` | sqrt, sin, cos, log, floor, ceil, min, max, gcd |
| `os` | exit, cwd, getenv, mkdir, getpid |
| `io` | read, write, append, exists, delete, size |
| `sys` | version, platform, arch |
| `list` | new, push, pop, get, set, len, free |
| `str` | eq, concat, len, slice, toint, fromint |
| `ai` | alloc, free, matmul, relu, dot, softmax, layernorm |

---

## Performance

Current benchmark results on an Intel i5-8350U (4-core, 15W), Windows x64:

| Benchmark | Omnikarai | C -O3 | Python | Node.js |
|-----------|-----------|-------|--------|---------|
| Loop 100M | 230ms | 11ms | 29,970ms | 8,000ms |
| Fib(40) ×5 | 7,323ms | 1,954ms | 202,673ms | 11,144ms |
| Primes ×30 | 1,355ms | 360ms | 14,927ms | 503ms |
| Dotprod ×10k | 280ms | 109ms | 16,521ms | 128ms |
| Matmul ×5k | 979ms | 133ms | 1,747ms | 1,337ms |

**vs Python:** 3× to 130× faster across all benchmarks.  
**vs Node.js:** competitive — wins on loops and large computation, competitive on math.  
**vs C -O3:** 2.5×–7× slower currently. The gap is the register allocator in progress — see `docs/OPTIMIZATION_PLAN.md`.

Honest note: GCC has 35 years of optimization engineering. Omnikarai's register allocator is actively being built. The architecture is correct and the ceiling is high. The matmul gap went from 36× to 7× in one sprint.

---

## How it works

The compilation pipeline is five stages:

```
source.ok
    ↓  Lexer       tokenize, emit INDENT/DEDENT
    ↓  Parser      Pratt parser → AST
    ↓  Codegen     walk AST, emit x86-64 bytes directly
    ↓  VirtualAlloc  allocate executable memory (JIT mode)
    ↓  PE writer   write .exe (build mode)
       ↓
       runs
```

No intermediate representation. No optimization passes yet (they're coming). One-pass direct emission.

**`omnicc run`** — compiles source to machine bytes in memory, calls them as a function via `VirtualAlloc`, returns the exit code. Zero disk writes. Compilation + execution in one step.

**`omnicc build`** — compiles source to machine bytes, then writes a valid PE32+ Windows `.exe` with a `.text` section (trampoline + JIT code), `.idata` section (KERNEL32.DLL imports), and `.reloc` section (base relocation table for ASLR). The output file runs standalone.

---

## Build from source

Requires `gcc` (MinGW-w64 or MSYS2):

```powershell
gcc -Iinclude -O2 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32 -lm
```

Or use the Makefile:

```powershell
make
```

---

## Run the test suite

```powershell
.\tests\run_tests.ps1      # 15 tests: arithmetic through modules
.\tests\run_stress.ps1     # 9 stress tests: algorithms, recursion
.\benchmarks\run_benchmarks.ps1   # vs C, C++, Python, Node.js
```

Current status: **15/15 tests pass. 9/9 stress tests pass.**

---

## Project structure

```
src/
  main.c       CLI entry point, omnicc build PE writer
  lexer.c      tokenizer, INDENT/DEDENT emission
  parser.c     Pratt parser → AST
  codegen.c    x86-64 native code emitter, all runtime helpers

include/
  lexer.h      token types, Lexer struct
  parser.h     Parser struct
  ast.h        all AST node types
  codegen.h    CodeGen struct, public API

tests/
  t01_core_arithmetic.ok  through  t15_assert.ok
  t16_ai_alloc.ok  through  t20_ai_dot_i8.ok
  stress01 through stress09
  run_tests.ps1
  run_stress.ps1

benchmarks/
  bench_loop_timed.ok      bench_loop.py    bench_loop.c
  bench_fib_timed.ok       bench_fib.py     bench_fib.c
  bench_primes_timed.ok    bench_primes.py  bench_primes.c
  bench_dotprod_timed.ok   bench_dotprod.py bench_dotprod.c
  bench_matmul_timed.ok    bench_matmul.py  bench_matmul.c
  run_benchmarks.ps1

docs/
  OMNIKARAI_BOOK.md        full compiler internals reference
  KNOWN_ISSUES.md          active bugs and limitations
  OPTIMIZATION_PLAN.md     register allocator and SIMD roadmap
  SPEED_GOD_PLAN.md        beating C on AI workloads — the thesis
  BENCHMARK_RESULTS.md     historical benchmark tracking

opi/
  package registry (Omnikarai Package Index) — web interface

bin/
  omnicc.exe               compiled compiler (gitignored)
```

---

## What's being built right now

**Register allocator** — pins the hottest variables in each function into CPU registers (RBX, R12, R13, RSI, RDI) instead of stack slots. This is the single change that closes most of the C gap. Currently implemented for `while` loops; function-level RA is in progress.

**Ephemeral computation** — eliminates store/load pairs for temporary variables that are used exactly once. `set a = x + 1; set b = a * y` currently spills `a` to the stack and immediately reloads it. The ephemeral pass detects this and keeps `a` in a register.

**LICM (loop-invariant code motion)** — hoists expressions whose inputs don't change inside a loop to before the loop. `(i + k) % 17` inside a `while k < n` loop can hoist `i % 17` to before the loop and run a cheap running counter for `k`.

**`ai.*` AVX2 primitives** — AVX2 FP32 matrix multiply, ReLU, softmax, layernorm already implemented in the runtime. Beating Python + NumPy on CPU-resident AI inference is the target.

---

## Part of FRAZIYM

Omnikarai is the language layer of the FRAZIYM tech stack:

- **Omnikarai** — language and native compiler
- **OPI** — Omnikarai Package Index (registry + CLI)
- **AXONIX** — offline agentic AI runtime
- **XorZen** — custom transformer architecture

Built by one person. No team. No funding. No excuses.

---

## Status

**v6.1** — active development. Core language stable and fully tested. Register allocator in progress. `omnicc build` (standalone .exe) functional — see `docs/KNOWN_ISSUES.md` for current limitations.

*Omnikarai by Akik Faraji — FRAZIYM Tech & AI*
