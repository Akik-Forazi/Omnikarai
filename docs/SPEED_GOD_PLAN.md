# OMNIKARAI — THE SPEED GOD PLAN
> How to make a compiler faster than C on CPU — and why that means faster than everything, everywhere.

**Author:** Akik Fuad | Fraziym Tech & AI | March 2026  
**Status:** Active Roadmap — Internal Engineering Document

---

## The One Problem We Own

> *"Why does AI run on Python when Python is slow? Because no one built a language that compiles AI workloads to native machine code — cleanly, without a GPU, without a runtime, without overhead."*

That is Omnikarai's problem. Specific, real, unsolved. This document is the plan to solve it completely.

---

## Why Beating C on CPU Means Beating Everything

If you beat C on CPU, you beat everything because:

- **GPU still loses on latency** — 5–50µs kernel launch overhead. CPU: 0.1µs. For batch=1, CPU starts and finishes before GPU even begins.
- **GPU loses on small models** — RTX 5090 costs $2,000, exists in <0.1% of machines. 2 billion laptops have CPUs. Omnikarai runs everywhere by default.
- **Mobile/edge runs on CPU** — Every phone, tablet, embedded device. No GPU. Omnikarai is the only native AI compiler for this market.
- **Beating C on CPU = beating PyTorch, ONNX, llama.cpp** — They all ultimately run C/C++ under the hood. Beat C, beat them all.

The claim: **"Fastest AI inference on the 2 billion devices that already exist in everyone's hands."**

---

## The Hardware Math (Already Proven)

| | i5-8350U | RTX 5090 | Frontier SC |
|---|---|---|---|
| FP32 GFLOPS | 460 | 209,400 | 2,412,000,000 |
| Memory BW | 37.5 GB/s | 1,792 GB/s | 7,680 GB/s |
| Kernel overhead | 0 µs | 5–50 µs | N/A |
| TDP | 15W | 575W | 21.1 MW |
| Cost | ~$0 | $2,000 | ~$600M |
| Devices | 2 billion | <0.1% | 1 on earth |

### The Roofline Reality

Inference (batch=1) arithmetic intensity = **0.5 FLOPS/byte** → memory-bound.

| | Peak FLOPS | Effective at inference |
|---|---|---|
| CPU | 460 GFLOPS | 18.8 GFLOPS |
| RTX 5090 | 209,400 GFLOPS | 896 GFLOPS |

**Real gap: 48×, not 454×.** And that 48× shrinks with cache, latency, and kernel overhead.

### Cache Bandwidths (Coffee Lake, measured)

| Level | Size | Bandwidth | Fits |
|---|---|---|---|
| L1 | 32 KB | 192 GB/s | 90×90 float matrix |
| L2 | 256 KB | 64 GB/s | 256×256 float matrix |
| L3 | 6 MB | 40 GB/s | 1,254×1,254 matrix = **1.57M params** |
| RAM | ∞ | 37.5 GB/s | Everything else |

**Key insight:** Any model under ~1.5M parameters fits entirely in L3. After warmup, RAM is never touched.

### Landauer's Principle (Physics Floor)

- Min energy per bit erased: 2.87×10⁻²¹ J at 300K
- We are 10⁸–10⁹× above the physics floor
- **The hardware is not the limit. The software is the limit.**
- That is where Omnikarai operates.

---

## The Five Pillars of Speed

### Pillar 1 — Ephemeral Computation *(Original — No Compiler Does This)*

**Definition:** A value V is ephemeral if:
1. All inputs that produced V are still live in registers
2. Recompute cost < memory load cost
3. V is not observed externally before inputs are evicted

**Theorem:** If fraction `f` of intermediate values are ephemeral:
```
Effective bandwidth = B × (1 - f)
Speedup (memory-bound) = 1 / (1 - f)
```

**Application — Neural Network Forward Pass:**

Your i5-8350U has **16 YMM registers × 8 floats = 128 float32 values** in the register pool at once.

For any network with hidden dimension ≤ 128:
- ALL intermediate activations fit in YMM registers
- Zero bytes written to memory for intermediates
- GPU cannot do this — CUDA mandates VRAM writes

```
Standard compiler (GCC/C):    4 layers × 128 floats × 4 bytes = 2,048 bytes written/inference
Omnikarai Ephemeral mode:     0 bytes written
```

For f ≈ 0.85: **speedup ≈ 6.7× over C** on memory-bound inference.

**This is original. No compiler currently implements this.**

---

### Pillar 2 — AVX2 Auto-Vectorization *(8× throughput)*

C with `-O2` sometimes vectorizes. Omnikarai **always** vectorizes numeric loops because:
- Variables are typed (INT or FLOAT) — no runtime type check needed
- No C aliasing rules to respect
- No undefined behavior to preserve

```
Scalar float add (C -O2):     1 float/cycle
AVX2 VADDPS (Omnikarai):      8 floats/cycle   ← 8×
AVX2 VFMADD231PS:            16 ops/cycle       ← 16× (mul + add fused)
```

**Target:** Every `for k in range(n): x = x + a * b` compiles to a single `VFMADD231PS` loop — not scalar iteration.

**Implementation:**
1. Detect inner loop with fixed bounds + linear numeric body → mark vectorizable
2. Emit `VBROADCASTSS` + `VFMADD231PS` + horizontal reduce
3. Scalar fallback for remainder (n % 8 != 0)

---

### Pillar 3 — Cache-Aware Memory Layout *(2–10× for real workloads)*

C allocates memory how you tell it. Omnikarai allocates memory the way the CPU cache wants it — automatically.

**What Omnikarai does:**
- Pack weight matrices in access-order layout (row-major for matmul)
- Group frequently-accessed weights into single 64-byte cache lines
- Align all allocations to 64-byte boundaries (`__declspec(align(64))`)
- Tile matrix multiplications to L2 cache size (256×256 blocks)

C can do this — but only if you know to do it. Omnikarai does it for you.

---

### Pillar 4 — Zero-Cost AI Primitives *(No function call overhead)*

C has no built-in matmul. You call BLAS — which has thread setup, dispatch overhead, and no knowledge of your specific matrix size.

Omnikarai's `ai.*` primitives are **compiler builtins**, not function calls. When you write:

```
set y = ai.matmul(A, x)
```

The compiler does not call a function. It emits the exact AVX2 bytecode for that specific matrix shape — unrolled, FMA-fused, cache-tiled — at compile time, zero overhead.

**AI primitive roadmap:**

| Primitive | Hardware instruction | Speedup over C |
|---|---|---|
| `ai.matmul(A, x)` | VFMADD231PS loop | 8× (AVX2) + tiling |
| `ai.relu(x)` | VMAXPS | 1 instruction total |
| `ai.softmax(x)` | Fused exp+sum+div | No intermediate writes |
| `ai.layernorm(x)` | Fused mean+var+scale | No extra memory pass |
| `ai.dot(a, b)` | VFMADD + hadd | Register-only |
| `ai.quantize(x, 8)` | VPMADDUBSW | 4× throughput |

---

### Pillar 5 — INT8 Quantized Compute *(4× throughput over FP32)*

AVX2 fits 8 float32 values per YMM register. INT8 fits **32 values** — 4× as many.

```
FP32 VFMADD231PS:    8  multiply-adds per cycle
INT8 VPMADDUBSW:    32  multiply-adds per cycle   ← 4×
```

**Effective throughput on i5-8350U:**
```
FP32 effective (memory-bound):   18 GFLOPS
INT8 effective (4× denser):      72 GFLOPS
```

C requires manual intrinsics for this. Omnikarai emits VPMADDUBSW automatically when input types are INT8.

---

## Where Omnikarai Beats C — Precise and Honest

**We win:**

| Workload | Why |
|---|---|
| AI inference, batch=1 | Ephemeral: zero intermediate writes. C writes everything. |
| Small matrix ops (≤128×128) | Cache-resident + register-resident. C misses layout. |
| Repeated inference (same weights) | L3-resident after warmup. C doesn't know. |
| Fused AI loops (matmul + relu + norm) | Cross-operation FMA fusion. C can't fuse across calls. |
| INT8 inference | VPMADDUBSW auto-emitted. C needs manual intrinsics. |

**C still wins:**

| Workload | Why |
|---|---|
| Large training (>1.5M params) | Both RAM-bound. BLAS is mature. |
| Systems code (pointers, OS, hardware) | C was built for this. Omni doesn't do raw pointers. |
| General-purpose programs | C's 50-year ecosystem is unbeatable here. |

We don't fight C everywhere. We fight C at one thing — **AI inference on CPU** — and we win.

---

## The Benchmark Target (Precise)

```
Hardware:    Intel i5-8350U | 4 cores | 15W TDP
Model:       4-layer MLP | hidden=128 | input=128 | output=10 | FP32
Batch:       1 (single inference, no batching)

Target:
  Python (no libs):         ~0.500 ms
  PyTorch CPU:              ~0.800 ms  (framework overhead)
  C -O3 -march=native:      ~0.050 ms
  Omnikarai target:         < 0.020 ms  ← beat C by 2.5×

Why 2.5×?
  C stores all intermediates: 4 layers × 128 floats × 4 bytes = 2,048 bytes/pass × 3 passes = ~6,144 bytes
  At 37.5 GB/s:             6,144 bytes = 0.164 µs saved
  FMA fusion:               ~30% fewer instructions vs scalar C
  Total:                    ~2.5× wall-clock speedup on this workload
```

---

## Implementation Roadmap — In Order

### ✅ Done
- x86-64 native codegen (JIT via VirtualAlloc)
- Full lexer + Pratt parser
- Variables, arithmetic, comparisons, logical ops
- if/elif/else, while, for, match/case
- Functions, recursion, nested calls
- Modules: time, datetime, math, os, io, sys, list, str
- AVX2 Speed God engine (VFMADD231PS emitter stub)
- assert builtin, boolean/string/float types

### 🔧 Step 1 — Fix All Bugs (CURRENT — do this first, nothing else)

All 15 tests must pass before any optimization work. Bugs are a credibility killer.

**Known failing:**
- T04: while+continue loop body partially missing (print not emitted)
- T04: for loop after while not running
- T05: recursive fib crashes (stack frame / cg->returned issue)
- t05_simple.ok: ParseError (empty file or token issue)

**Exit condition: 15/15 tests pass.**

### 🔧 Step 2 — Complete `ai` Module Dispatch

The `ai` module hook is in `cg_module_call` but the dispatch is incomplete.

Fill in: `ai.matmul`, `ai.relu`, `ai.dot`, `ai.softmax` — as scalar stubs first, then replace with AVX2 emitters.

Add `"ai"` to `g_known_modules[]`.

### 🔧 Step 3 — FP32 AVX2 Matmul Emitter

Emit `VFMADD231PS` loops with L2 cache tiling for float matmul.

Target: match or beat OpenBLAS single-threaded on square matrices ≤ 256×256.

### 🔧 Step 4 — INT8 VPMADDUBSW Matmul

32 INT8 ops per cycle vs 8 FP32 ops. Auto-emit when input types are INT8.

```c
void emit_int8_dot_avx2(CodeGen* cg, int a_off, int b_off, int len);
```

### 🔧 Step 5 — Ephemeral Computation Pass

Analysis pass after codegen that identifies and eliminates redundant stores.

**Algorithm:**
1. Walk SSA-like value graph for each function body
2. For each `emit_store_rax`: check if value is read before any call/branch that would evict registers
3. If yes and value fits in a YMM/GPR → replace store+load pair with register alias
4. Track register liveness across instructions

This is the core original contribution. No other compiler does this for AI workloads.

### 🔧 Step 6 — Benchmark Suite (`omnicc bench`)

Standard reproducible benchmarks reported in GFLOPS and ms/inference:

- 128×128 FP32 matmul — vs C `-O3 -mavx2`, vs numpy
- 128×128 INT8 matmul — vs C `-O3 -mavx2`
- fib(35) recursion — vs C `-O2`
- 4-layer MLP forward pass, hidden=128 — vs PyTorch CPU, ONNX Runtime, llama.cpp

**Milestone: beat C on the MLP benchmark → claim is proven.**

### 🔧 Step 7 — Load Pretrained Weights

To run real models:
- `io.load_gguf(path)` → weight map
- `io.load_safetensors(path)` → weight map
- Memory-mapped reading (no full-file malloc)

### 🔧 Step 8 — IGRIS_1M Inference in Native Omnikarai

Write IGRIS_1M forward pass (from ZARX) directly in `.ok` using `ai.*` primitives.

Benchmark against PyTorch CPU, llama.cpp, ONNX Runtime CPU.

**If Omnikarai is faster: the claim is proven. Publish the benchmark.**

---

## The Exact Timeline

```
TODAY         Fix all 15 tests → 15/15 passing
WEEK 1        Complete ai module + FP32 vectorized matmul
WEEK 2        INT8 quantized matmul via VPMADDUBSW
WEEK 3        Ephemeral computation elimination pass
WEEK 4        MLP benchmark vs C, Python, PyTorch
WEEK 5        Beat C on MLP benchmark → claim proven
WEEK 6+       Load GGUF weights → run IGRIS_1M → beat llama.cpp
```

---

## The Founding Principle

> *"Every other compiler asks: what is the fastest way to execute this program?*
> *Omnikarai asks a different question: which parts of this program need to exist at all?*
>
> *Values that can be reconstructed from registers are never written to memory.*
> *We call this Ephemeral Computation.*
> *It is the first time a compiler has treated memory writes as optional rather than mandatory.*
>
> *Result: The fastest AI inference runtime on the 2 billion devices that already exist —*
> *with no GPU, no Python overhead, and no kernel launch latency."*

---

*Fraziym Tech & AI | Omnikarai Compiler | March 2026*  
*"The software is the limit. We are the software."*
