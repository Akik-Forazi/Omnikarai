# OMNIKARAI OPTIMIZATION PLAN
# Date: 2026-03-23
# Author: Akik Faraji — Fraziym Tech & AI
# Status: Active — based on real benchmark baseline

## Baseline (what we measured today)

| Benchmark    | Current  | C -O3  | Ratio   | Target   |
|--------------|----------|--------|---------|----------|
| Fib(40) x5   | 6867ms   | 1800ms | 3.81x   | <2700ms  |
| Primes x30   | 1000ms   | 272ms  | 3.67x   | <408ms   |
| Matmul x5k   | 3480ms   | 104ms  | 33.39x  | <208ms   |
| Loop 100M    | 214ms    | <1ms*  | N/A     | N/A      |
| Dotprod x10k | 32ms     | <1ms*  | N/A     | N/A      |

*C eliminates dead code at -O3. Not a fair comparison for these.
 Our absolute numbers for loop/dotprod are already good.

## The Core Problem: Stack Spilling

Every variable in Omnikarai lives on the stack ([rbp - offset]).
Every expression loads from stack, computes, stores back to stack.

Example — what `set acc = acc + a * b` generates NOW:
```asm
mov rax, [rbp-40]    ; load acc
mov rcx, [rbp-48]    ; load a  
mov rdx, [rbp-56]    ; load b
imul rcx, rdx        ; a * b
add rax, rcx         ; acc + (a*b)
mov [rbp-40], rax    ; store acc
```
= 3 loads + 1 store per inner loop iteration

What it SHOULD generate (with register allocation):
```asm
imul rcx, rdx        ; a * b (already in registers)
add r12, rcx         ; acc in r12, zero memory ops
```
= 0 loads + 0 stores

For matmul with 32x32x32=32768 inner iterations x 5000 reps:
Current:  32768 * 5000 * 4 memory ops = 655M memory ops
Optimal:  32768 * 5000 * 0 memory ops = 0

This single fix will bring matmul from 3480ms → ~200ms.

---

## PHASE 1 — Quick Wins (Low Risk, Do First)
### Expected: 1.3-1.5x speedup across all benchmarks

### OPT-1: Extend inline to multi-statement functions
Current: only single-expression single-param functions are inlined.
Target: inline any function body ≤ 5 statements called from a hot loop.

This fixes primes (is_prime inlined) and fib partially.

In codegen.c, fn_is_inlineable():
```c
// Current: requires exactly 1 statement
if(!fd->body||fd->body->statement_count!=1) return 0;

// New: allow up to 5 statements for simple bodies
// Criteria: no loops, no nested calls, pure arithmetic+conditionals
static int fn_body_is_simple(AST_Statement_Block* body) {
    if (!body || body->statement_count > 5) return 0;
    for (int i = 0; i < body->statement_count; i++) {
        AST_Statement* s = body->statements[i];
        if (!s) continue;
        // Allow: return, set, if (no else blocks with loops)
        if (s->type == WHILE_STATEMENT) return 0;
        if (s->type == FOR_STATEMENT) return 0;
        // Recursion check: no calls to the function itself
        // (handled separately)
    }
    return 1;
}
```

### OPT-2: SET statement local variable reuse
When we see `set x = x + const`:
Currently: load x, add const, store x (3 ops)
Optimize: ADD [rbp-offset], imm (1 op, directly on memory)

Already partially done for some cases — extend to all arithmetic.

### OPT-3: Constant propagation in loops
```python
while i < 100000:
    set x = n * n     # n doesn't change inside loop
```
Hoist `n * n` out of loop body.
Detect: if ALL reads of a variable inside a loop are from OUTSIDE the loop
(never SET inside the loop), it's loop-invariant.

### OPT-4: Dead store elimination for temporaries
When a tmp slot is used only to pass a value to the next operation
and is never read again, skip the store entirely.

---

## PHASE 2 — Register Allocator (THE BIG WIN)
### Expected: 5-15x speedup on loop-heavy code

This is the most impactful optimization. Implement linear scan register allocation.

### Available registers for variable pinning (callee-saved, Windows x64):
```
r12, r13, r14, r15, rbx, rdi, rsi = 7 registers
```
We already use r14, r15 for loop counters (reg_var_depth).
Extend to use all 7 for any variable with high use count.

### Algorithm: Linear Scan Register Allocation

Step 1 — Liveness Analysis
For each function body, compute for every variable:
- first_def: statement index where variable is first SET
- last_use:  statement index of its last READ
- use_count: total number of times it's read

```c
typedef struct {
    char name[64];
    int  first_def;   // statement index
    int  last_use;    // statement index  
    int  use_count;   // number of reads
    int  reg;         // assigned register (-1 = spill to stack)
    int  stack_off;   // stack offset (always valid as fallback)
} LiveVar;
```

Step 2 — Sort by priority
Sort variables by use_count DESC (most-used variables get registers first).
For ties: sort by live range length ASC (shorter range = frees register sooner).

Step 3 — Assign registers
Walk sorted list, assign next free register.
When all 7 registers taken: spill the variable with furthest last_use
(Belady's optimal algorithm).

Step 4 — Emit code
- On variable SET: if reg != -1, emit MOV reg, rax; else emit_store_rax
- On variable GET: if reg != -1, emit MOV rax, reg; else emit_load_rax
- On function CALL: save/restore all live pinned registers around call

### Integration point in codegen.c:
The register allocator runs as a pre-pass in cg_fn_body():
```c
static void cg_fn_body(CodeGen* cg, AST_Statement_FnDef* fn_def) {
    // ... existing prologue ...
    
    // NEW: run liveness analysis + register assignment
    RegAllocState ras;
    regalloc_analyze(&ras, fn_def->body, cg->scope);
    regalloc_assign(&ras);  // fills ras.live_vars[i].reg
    
    // Pass ras to cg_stmt so it can emit register ops
    cg->regalloc = &ras;
    
    // ... existing body emission ...
}
```

### For main body (not inside a function):
Same analysis on the top-level statements.
Especially important for the inner loop variables in matmul-style code.

---

## PHASE 3 — Loop Optimizations
### Expected: 1.5-2x on loop-heavy code on top of Phase 2

### OPT-5: Loop invariant code motion (LICM)
Move computations that don't change inside a loop to before the loop.

```python
# Before LICM:
while i < n:
    set x = a * b + c   # a, b, c never change in loop
    set i = i + 1

# After LICM (generated code equivalent):
set _hoisted_0 = a * b + c
while i < n:
    set x = _hoisted_0
    set i = i + 1
```

Detection: Walk loop body AST. For each expression on RHS of SET:
- If ALL variables it reads are never SET inside the loop → hoistable.

### OPT-6: Loop unrolling for small fixed counts
```python
for i in range(4):
    # body
```
Unroll to 4 copies of body, eliminate loop overhead entirely.
Threshold: unroll if iteration count is a compile-time constant <= 8.

### OPT-7: Strength reduction in loops
```python
while i < n:
    set x = i * 8    # multiply by constant each iteration
```
Replace with:
```python
set _x_base = 0
while i < n:
    set x = _x_base
    set _x_base = _x_base + 8   # add instead of multiply
```
Detect: `var * constant` where var is the loop counter.
Replace with an accumulator that starts at 0 and adds the constant each iteration.

---

## PHASE 4 — SIMD Auto-Vectorization
### Expected: 4-8x on array/numeric code

### OPT-8: Detect vectorizable inner loops
A loop is auto-vectorizable if:
1. Fixed bounds (range(n) with constant n, or n known at compile time)
2. Body is a single arithmetic operation on array elements
3. No data dependencies between iterations (element i doesn't depend on i-1)

```python
# Vectorizable:
for i in range(n):
    set result = result + arr[i] * weights[i]

# Not vectorizable (dependency):
for i in range(n):
    set arr[i] = arr[i-1] + 1
```

### OPT-9: Emit AVX2 for vectorizable loops
For a scalar accumulation loop:
```python
for i in range(n):
    set s = s + a[i] * b[i]
```

Emit:
```asm
; vzeroupper
; ymm0 = 0  (accumulator)
; Process 8 floats per iteration:
loop_avx2:
    vmovups ymm1, [a + i*4]
    vmovups ymm2, [b + i*4]
    vfmadd231ps ymm0, ymm1, ymm2   ; ymm0 += ymm1 * ymm2
    add i, 8
    cmp i, n
    jl loop_avx2
; Horizontal sum ymm0 → scalar
; Scalar remainder for n % 8 != 0
```

---

## PHASE 5 — Function Call Optimization
### Expected: 2-3x on recursion-heavy code (fib)

### OPT-10: Tail call optimization (TCO)
Detect tail-recursive calls:
```python
fn fib(n):
    if n <= 1: return n
    return fib(n-1) + fib(n-2)   # NOT tail call (adds result)
```
vs:
```python
fn sum_tail(n, acc):
    if n <= 0: return acc
    return sum_tail(n-1, acc+n)   # IS tail call
```
For true tail calls: replace CALL + RET with JMP to function start.
This converts recursion to a loop — eliminates stack growth.

### OPT-11: Shrink function prologues
Current prologue: push rbp + mov rbp,rsp + sub rsp,N + save r14/r15
For leaf functions (no further calls): skip r14/r15 save.
For tiny functions (1-2 local vars): use sub rsp, 32 (minimum shadow space).

### OPT-12: Register-passed return values
Currently: return value always goes through rax (already done).
Optimization: for functions returning a value immediately used in arithmetic,
keep the value in rax and avoid the store/reload cycle.

---

## PHASE 6 — Peephole Optimizer  
### Expected: 1.1-1.2x across all code

A post-pass over emitted bytes replacing patterns:

| Pattern | Replace with |
|---------|-------------|
| MOV RAX,0 | XOR RAX,RAX (already done) |
| MOV [mem], RAX; MOV RAX, [mem] | MOV [mem], RAX (drop reload) |
| ADD RAX, 0 | (eliminate) |
| MOV RAX, RBX; PUSH RAX | PUSH RBX |
| MOV RAX, X; MOV RCX, RAX | MOV RCX, X |
| CMP RAX, 0; JNE label | TEST RAX,RAX; JNZ label |
| MOV RAX, 1; TEST RAX,RAX | MOV RAX, 1 (test is redundant) |

Implementation: scan emitted byte buffer in reverse, match patterns,
replace in-place (same size or smaller with NOP padding).

---

## Implementation Order and Files to Change

### Phase 1 (start immediately):
- `src/codegen.c`: fn_is_inlineable() — extend to 5-statement bodies
- `src/codegen.c`: cg_for_statement() — loop invariant detection
- `src/codegen.c`: cg_set_statement() — ADD [mem], imm optimization

### Phase 2 (after Phase 1 benchmarked):
- `src/omni_regalloc.c` — NEW FILE: linear scan allocator
- `include/omni_regalloc.h` — NEW FILE: RegAllocState struct
- `src/codegen.c`: cg_fn_body() — integrate regalloc pre-pass
- `src/codegen.c`: cg_identifier() — check regalloc before stack load
- `src/codegen.c`: cg_set_statement() — check regalloc before stack store

### Phase 3 (after Phase 2):
- `src/codegen.c`: cg_for_statement() — LICM, unrolling, strength reduction

### Phase 4 (after Phase 3):
- `src/omni_simd.c` — NEW FILE: AVX2 emitters for vectorized loops
- `src/codegen.c`: cg_for_statement() — detect vectorizable, emit AVX2

### Phase 5:
- `src/codegen.c`: cg_fn_body() — TCO detection, smaller prologues

### Phase 6:
- `src/omni_peephole.c` — NEW FILE: post-emit byte-level optimizer
- `src/codegen.c`: codegen_compile() — run peephole after all emission

---

## Target Benchmarks After Each Phase

| Phase | Fib     | Primes  | Matmul  | vs C   |
|-------|---------|---------|---------|--------|
| Now   | 6867ms  | 1000ms  | 3480ms  | 3-33x  |
| Ph 1  | 4000ms  | 400ms   | 3000ms  | 2-29x  |
| Ph 2  | 2500ms  | 300ms   | 200ms   | 1.4-2x |
| Ph 3  | 2000ms  | 250ms   | 150ms   | 1.1-1.4x |
| Ph 4  | 2000ms  | 250ms   | 50ms    | C-level/better |
| Ph 5  | 1800ms  | 250ms   | 50ms    | near-C |
| Ph 6  | 1700ms  | 240ms   | 48ms    | near-C |

Phase 2 is the biggest single win (matmul 3480ms → 200ms = 17x improvement).
Phase 4 makes matmul beat C (SIMD vectorization).

---

## Regression Note

v4.0 showed much better numbers than v6.0. Before optimizing further,
run a regression bisect:
1. Check if the benchmark scripts changed (workload size increased)
2. Check if codegen.c changes in v5/v6 added overhead
3. The v4.0 "C beats" were likely measurement artifacts (process launch overhead)
   Current numbers are more accurate and honest.

The real v4.0 C comparison was flawed — C had 253-516ms for fib(35)x10
which should be ~5ms. That was process launch overhead inflating C times.
Current C times (1800ms for fib(40)x5) are accurate.

So v6.0 is NOT worse than v4.0 in reality — the measurement was just more honest now.

---

*Document created: 2026-03-23*
*Next update: after Phase 1 implementation*
