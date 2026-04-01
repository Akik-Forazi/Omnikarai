# Omnikarai v7.0 — MKL Beater Progress Log
**Date:** 2026-03-31
**Goal:** Make compiler more powerful/effective than Intel MKL for math

---

## COMPLETED

### 1. Makefile — AVX2/FMA Enabled
- Changed `CFLAGS` from `-O2` to `-O3 -march=native -mavx2 -mfma -ffast-math -DNDEBUG`
- `Makefile` line 2: now enables AVX2+FMA code paths in runtime kernels
- Without this flag, all `#if defined(__AVX2__)` blocks were dead code

### 2. Register Allocator — Aggressive Pinning
**File:** `src/codegen.c`

**Function-level regalloc (line ~3460):**
- Removed `ra_has_calls` check — callee-saved registers (rbx/r12/r13/rsi/rdi) are preserved across calls by Windows x64 ABI, so pinning is safe even with function calls
- Lowered threshold from `count<2` to `count<1` — pin ALL hot variables, not just frequently-read ones
- Kept max 5 slots (rbx/r12/r13/rsi/rdi)

**While-loop pinning (line ~2984):**
- Increased from 3 slots to 5 slots (rbx/r12/r13/rsi/rdi)
- Lowered threshold from 2 to 1
- Updated `already pinned` check from `k<5` to `k<7`
- Added RSI/RDI (slots 5-6) to load/store switch statements

**For-loop body pinning (line ~3150):**
- Increased from 3 to 5 slots (`fbody_nsaved<5`)
- Updated `already` check from `k<5` to `k<7`
- Updated slot limit from `fslot>4` to `fslot>6`
- Added RSI/RDI cases to register load switch

### 3. Matrix-Matrix Multiply Kernel — AVX2 FMA + Cache Tiling
**File:** `src/codegen.c` (inserted after `omni_ai_matmul`, ~line 824)

**New runtime functions:**
- `omni_ai_matmul_nn(C, A, B, M, K, N)` — full matrix-matrix multiply
  - 4×8 micro-kernel: 4 rows × 8 AVX2 lanes = 32 FP32 ops per K iteration
  - K-loop unrolled ×4 for ILP
  - Cache tiling: MC=64, NC=64, KC=256 for L1/L2 efficiency
  - Scalar fallback for non-AVX2 builds
- `omni_matmul_nn_call()` — int64 ABI wrapper
- `omni_ai_gemm(C, A, B, M, K, N, alpha, beta)` — general matrix multiply C = αAB + βC
- `omni_gemm_call()` — int64 ABI wrapper
- Helper: `micro_kernel_4x8()` — inline 4×8 AVX2 FMA block
- Helper: `dot_row_col()` — scalar remainder dot product

**Function pointers added (line ~1197):**
- `g_fn_matmul_nn = omni_matmul_nn_call`
- `g_fn_gemm = omni_gemm_call`

**Codegen dispatch added (ai module, ~line 2580):**
- `ai.matmul_nn(C, A, B, M, K, N)` — 6 args, registers RCX/RDX/R8/R9 + stack [rsp+32],[rsp+40]
- `ai.gemm(C, A, B, M, K, N, alpha, beta)` — 8 args, registers + stack for args 5-8

### 4. Math Module Expansion
**New runtime functions added (after `omni_math_clamp`):**
- `omni_math_exp(v)` — e^v
- `omni_math_exp2(v)` — 2^v
- `omni_math_tanh(v)` — hyperbolic tangent
- `omni_math_asin(v)` — arcsine
- `omni_math_acos(v)` — arccosine
- `omni_math_atan(v)` — arctangent
- `omni_math_atan2(y,x)` — two-argument arctangent
- `omni_math_sinh(v)` — hyperbolic sine
- `omni_math_cosh(v)` — hyperbolic cosine
- `omni_math_cbrt(v)` — cube root
- `omni_math_hypot(a,b)` — hypotenuse
- `omni_math_sign(v)` — sign (-1, 0, 1)
- `omni_math_isnan(v)` — NaN check
- `omni_math_isinf(v)` — infinity check

---

## REMAINING (TODO)

### 5. Wire Math Functions into Codegen
- Add `g_fn_math_exp`, `g_fn_math_tanh`, etc. function pointers (after `g_fn_math_clamp` at line ~653)
- Add dispatch cases in `cg_module_call` math section (~line 2300): `math.exp()`, `math.tanh()`, `math.atan2()`, etc.
- Add `math.atan2` two-arg handler (similar to `math.pow` pattern)
- Update `infer_type()` to return `OMNI_TYPE_FLOAT` for new math functions

### 6. Vectorized Activation Functions (ai module)
Add to runtime (after `omni_ai_layernorm`):
- `omni_ai_exp_f32(arr, n)` — element-wise exp using AVX2 approx
- `omni_ai_tanh_f32(arr, n)` — element-wise tanh
- `omni_ai_sigmoid_f32(arr, n)` — element-wise sigmoid
- Wire into `ai` codegen dispatch as `ai.exp(arr,n)`, `ai.tanh(arr,n)`, `ai.sigmoid(arr,n)`

### 7. Improve Dotprod Kernel
- Current: 4-way unroll with 4 accumulators
- Upgrade to 8-way unroll with 8 accumulators for better throughput

### 8. BatchNorm and Softmax Fast
- `omni_ai_batchnorm(x, mean, var, gamma, beta, n, eps)` — fused AVX2 batch normalization
- `omni_ai_softmax_fast(arr, n)` — improved softmax with AVX2 exp approximation

### 9. Build and Test
- Run `make clean && make` to compile
- Run existing tests: `powershell -File tests/run_tests.ps1`
- Run benchmarks: `powershell -File benchmarks/run_benchmarks.ps1`
- Verify correctness of new matmul_nn and gemm with a test file

---

## KEY FILES
- `src/codegen.c` — main codegen (3800+ lines, all changes here)
- `include/codegen.h` — CodeGen struct definition
- `include/ast.h` — AST node types
- `src/parser.c` — Pratt parser
- `src/lexer.c` — lexer with indent/dedent
- `src/main.c` — CLI entry point
- `Makefile` — build config
- `Bench_results.txt` — historical benchmark results

## BUILD COMMAND
```
cd C:\Users\akikf\programing\omnikarai\omniwin
make clean && make
```

## TEST COMMAND
```
bin\omnicc.exe run tests\t16_ai_alloc.ok
bin\omnicc.exe run benchmarks\bench_matmul.ok
```
