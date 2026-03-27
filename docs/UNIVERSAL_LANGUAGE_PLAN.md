# OMNIKARAI — UNIVERSAL LANGUAGE PLAN
## Making Omnikarai Fast, Complete, and Memory-Efficient

**Author:** Akik Fuad | Fraziym Tech & AI | March 2026  
**Goal:** A language anyone can use for anything — systems, web, AI, CLI, scripts — faster than C, less memory than Rust, cleaner than Python.

---

## What "Universal" Means

| Domain | Example use | Target |
|--------|-------------|--------|
| Systems | OS tools, drivers, CLI apps | Beat C -O2 |
| Web backend | HTTP server, API, JSON | Beat Go |
| AI/ML | Inference, training loops | Beat PyTorch CPU |
| Scripts | File processing, automation | Beat Python 3 |
| Embedded | Microcontrollers, edge | Beat MicroPython |
| Games | Game loops, physics | Beat C# / Unity scripts |

One language. One compiler. No runtime.

---

## The Three Laws of Omnikarai Design

1. **Speed Law** — If two designs produce the same result, choose the one that emits fewer instructions.
2. **Memory Law** — Never allocate memory you don't need. Every heap allocation must have a known lifetime.
3. **Clarity Law** — If a feature makes code harder to read without making it faster, reject it.

---

## Current State (March 2026)

✅ **Fully working:**
Variables, arithmetic, comparisons, logical ops, if/elif/else, while, for range(), match/case, user-defined functions, recursion, nested calls, modules (time, datetime, math, os, io, sys, list, str), print(), input(), len(), int(), str(), assert(), AVX2 speed engine, VS Code extension.

🔴 **Broken (fix before anything new):**
- T04: while+continue loop body partially not emitting (print missing)
- T05: recursive fib crashes (stack frame issue)
- T04: for loop after while not running
- t05_simple.ok: ParseError on edge case

📊 **Test suite: 13/15 passing. Must reach 15/15 first.**

---

## Part 1 — Language Features

### 1.1 Print Formatting — `print(f"...")`

**Does Omnikarai have f-strings? NO. This is the #1 missing QoL feature.**

Currently the only way to print mixed content:
```
# Ugly — current state
print(str.concat("x = ", str(x)))
print(str.concat(str.concat("a=", str(a)), str.concat(" b=", str(b))))
```

**What we need — f-string syntax:**
```
# Clean — planned
print(f"x = {x}")
print(f"a={a} b={b} sum={a+b}")
print(f"Hello {name}, you are {age} years old!")
print(f"Pi is {math.pi} and e is {math.e}")
```

**How f-strings compile (zero overhead):**

At compile time, the parser sees `f"..."` and splits it into segments:
- Literal parts → string constants in the pool
- `{expr}` parts → evaluate expr, call `str()` on result, concat

So `print(f"x = {x}")` compiles to exactly the same bytecode as:
```
print(str.concat("x = ", str(x)))
```

Zero runtime overhead. No format string parsing at runtime. Pure compile-time transformation.

**Implementation plan:**
1. Lexer: detect `f"..."` token, emit `FSTRING_LITERAL` token type
2. Parser: parse `{expr}` segments inside the f-string body
3. Codegen: emit sequence of str.concat calls, same as manual concat
4. Result: `print(f"n={n}")` → 2 instructions: str(n) + concat + print

**Priority: HIGH. This affects every program ever written in Omnikarai.**

---

### 1.2 Type System

**Current:** Everything is int64 internally. Strings are char*. Floats are double.

**Design (static, inferred, zero-cost):**
```
set x = 42          # compiler infers: int
set y = 3.14        # compiler infers: float
set name = "Akik"   # compiler infers: str
set ok = true       # compiler infers: bool

# Optional explicit annotation (for clarity)
set count: int = 0
set ratio: float = 0.0
```

| Category | Types | Internal |
|----------|-------|----------|
| Integer | `int`, `int8`, `int16`, `int32`, `int64` | register (64-bit) |
| Unsigned | `uint`, `uint8`, `uint16`, `uint32`, `uint64` | register (64-bit) |
| Float | `float` (64-bit), `float32` (32-bit) | XMM register |
| Boolean | `bool` | 0 or 1 in register |
| String | `str` | pointer to null-terminated heap string |
| Bytes | `bytes` | pointer + length struct |
| Void | `void` | no return value |

**Memory cost: Zero.** Types are compiler-only — no tag bits, no boxing, no vtable unless class is used.

---

### 1.3 Collections — Lists, Dicts, Sets, Tuples

**List:**
```
set nums = [1, 2, 3, 4, 5]
set first = nums[0]
nums[0] = 99
nums.push(6)
nums.pop()
set length = len(nums)
for item in nums:
    print(item)
```

Internal: 24-byte header on stack (data pointer + length + capacity). Data on heap. Same layout as C++ `std::vector`. No GC.

**Dict:**
```
set person = {"name": "Akik", "age": 22}
set name = person["name"]
person["city"] = "Dhaka"
for key in person.keys():
    print(f"{key} = {person[key]}")
```

Internal: open-addressing hash map. No pointer chasing per lookup.

**Tuple (immutable, stack-allocated for ≤4 elements):**
```
set point = (10, 20)
set x, y = point        # destructuring — zero copy
```

**Set:**
```
set seen = {1, 2, 3}
seen.add(4)
set has = seen.contains(2)   # true
```

---

### 1.4 Strings — Full Power

```
set s = "Hello, " + name           # concat
set upper = str.upper(s)           # NEW
set lower = str.lower(s)           # NEW
set trimmed = str.trim(s)          # NEW
set parts = str.split(s, ",")      # NEW — returns list
set joined = str.join(", ", parts) # NEW
set found = str.contains(s, "ell") # NEW — bool
set idx = str.find(s, "ell")       # NEW — int or -1
set sub = str.slice(s, 2, 5)       # partially done
set replaced = str.replace(s, "o", "0")  # NEW
print(f"Length: {len(s)}")         # f-string
```

All operations return new strings. Original is never mutated. String literals are interned — same literal = same pointer = comparison is pointer equality.

---

### 1.5 Error Handling — `try / except / raise`

```
fn divide(a, b):
    if b == 0:
        raise ValueError("division by zero")
    return a / b

try:
    set result = divide(10, 0)
except ValueError as e:
    print(f"Error: {e.message}")
except:
    print("unknown error")
```

**Internal:** Error state lives in a thread-local slot — not stack unwinding. No SEH, no `.pdata` section. Checking cost: 1 memory read per call.

---

### 1.6 Classes and Objects

```
class Vec2:
    fn init(self, x, y):
        self.x = x
        self.y = y

    fn length(self):
        return math.sqrt(self.x * self.x + self.y * self.y)

    fn add(self, other):
        return Vec2(self.x + other.x, self.y + other.y)

set v1 = Vec2(3.0, 4.0)
print(f"Length: {v1.length()}")   # 5.0
```

Internal: struct + plain functions. `v1.length()` → `Vec2_length(&v1)`. No vtable unless `extends` is used.

**Inheritance (single only):**
```
class Animal:
    fn init(self, name):
        self.name = name
    fn speak(self):
        print("...")

class Dog extends Animal:
    fn speak(self):
        print(f"Woof! I am {self.name}")

set d = Dog("Rex")
d.speak()
```

---

### 1.7 Closures and First-Class Functions

```
fn make_adder(n):
    return fn(x):
        return x + n

set add5 = make_adder(5)
print(add5(3))   # 8

set nums = [1, 2, 3, 4, 5]
set doubled = list.map(nums, fn(x): return x * 2)
set evens   = list.filter(nums, fn(x): return x % 2 == 0)
set total   = list.reduce(nums, fn(acc, x): return acc + x, 0)
```

---

### 1.8 Pattern Matching — Full Power

```
match value:
    case 0:
        print("zero")
    case 1..10:
        print("small")
    case n if n > 1000:
        print(f"huge: {n}")
    case _:
        print("other")
```

Compiles to `cmp + je` chains. Zero overhead.

---

### 1.9 Compile-Time Features

```
const PI = 3.14159265358979    # zero runtime cost
const MAX = 1024 * 1024        # evaluated at compile time

@inline
fn square(x):
    return x * x               # call disappears, body inlined

@simd
for i in range(n):
    result[i] = a[i] * b[i]   # emits VMULPS, 8 floats/cycle
```

---

### 1.10 Concurrency

```
use thread

fn worker(id):
    print(f"worker {id} running")

thread.run(worker, 0)
thread.run(worker, 1)
thread.join_all()
```

Real OS threads. No GIL. No GC pauses.

---

### 1.11 Unsafe / Systems Programming

```
unsafe:
    set ptr = malloc(1024)
    store(ptr, 42)
    set val = deref(ptr)
    free(ptr)

set buf: int[256]   # 256 ints on the stack, no heap
```

Outside `unsafe`: all access bounds-checked. Inside `unsafe`: raw MOV instructions. Clean separation.

---

## Part 2 — Standard Library

### 2.1 Core (always available)

| Function | Description |
|----------|-------------|
| `print(x)` | Print any value + newline |
| `print(f"...")` | **Formatted print — f-string** |
| `input(prompt)` | Read line from stdin |
| `len(x)` | Length of string, list, dict |
| `int(x)` | Convert to integer |
| `float(x)` | Convert to float |
| `str(x)` | Convert to string |
| `bool(x)` | Convert to bool |
| `type(x)` | Return type name as string |
| `assert(cond, msg)` | Assert with message |
| `range(n)` | Integer range |
| `range(start, stop, step)` | Integer range with step |
| `min(a, b)` | Minimum |
| `max(a, b)` | Maximum |
| `abs(x)` | Absolute value |
| `copy(x)` | Deep copy of any value |

### 2.2 `use math`

`sqrt`, `pow`, `sin`, `cos`, `tan`, `log`, `log2`, `log10`, `floor`, `ceil`, `round`, `clamp`, `gcd`, `pi`, `e`, `inf`, `abs`, `sign`, `mod`

### 2.3 `use str`

`upper`, `lower`, `trim`, `split`, `join`, `contains`, `find`, `replace`, `starts_with`, `ends_with`, `slice`, `len`, `eq`, `format`, `repeat`, `pad_left`, `pad_right`, `to_int`, `from_int`, `encode`, `decode`

### 2.4 `use list`

`new`, `push`, `pop`, `get`, `set`, `len`, `sort`, `reverse`, `contains`, `find`, `slice`, `map`, `filter`, `reduce`, `flat`, `zip`, `print`, `clear`, `copy`

### 2.5 `use dict`

`new`, `get`, `set`, `has`, `delete`, `keys`, `values`, `items`, `len`, `merge`, `clear`, `copy`

### 2.6 `use io`

`read`, `write`, `append`, `exists`, `delete`, `size`, `lines`, `copy`, `move`, `mkdir`, `open`, `close`

### 2.7 `use os`

`platform`, `cwd`, `getenv`, `setenv`, `getpid`, `exit`, `mkdir`, `exists`, `listdir`, `run`, `args`

### 2.8 `use time`

`now`, `sleep`, `since`, `format`, `parse`, `clock`, `bench`

### 2.9 `use math` (extended for AI)

`dot`, `matmul`, `norm`, `clamp`, `sigmoid`, `relu`, `softmax`

### 2.10 `use ai`

`matmul`, `relu`, `softmax`, `dot`, `layernorm`, `quantize`, `load_gguf`, `load_safetensors`

### 2.11 `use net` (future)

`get`, `post`, `listen`, `connect`, `send`, `recv`

### 2.12 `use json` (future)

`parse`, `stringify`

### 2.13 `use thread` (future)

`run`, `join`, `join_all`, `lock`, `unlock`, `channel`

---

## Part 3 — Memory Model

### How Other Languages Handle Memory

| Language | Model | Problem |
|----------|-------|---------|
| Python | GC + boxing | Every int = 28-byte heap object |
| Java | GC + boxing | GC pauses, 4× memory overhead |
| Rust | Ownership + borrow checker | Correct but complex, compiler fights you |
| C | Manual | Fast, but leaks and use-after-free |
| Go | GC | GC pauses unpredictable |
| **Omnikarai** | **Stack-first + scope lifetime** | Fast, safe, predictable |

### Omnikarai Memory Rules

**Rule 1 — Stack-first:** Small values (int, float, bool, small structs ≤64 bytes) live on the stack. Freed automatically on function return.

**Rule 2 — Heap only for dynamic size:** Lists, dicts, strings. Their header lives on the stack. Data on heap. Header freed automatically. Data freed at end of scope.

**Rule 3 — Scope lifetime, no GC:** Compiler emits `free()` at end of each scope where heap was allocated. Deterministic, no pauses.

**Rule 4 — Pass by reference by default:** Large objects are passed as pointers. No copy unless you say `copy(x)`.

**Rule 5 — Strings are immutable:** All string ops return new strings. No mutation, no hidden bugs.

### Memory Cost Comparison (4-layer MLP, hidden=128)

| Language | Memory | Why |
|----------|--------|-----|
| Python | ~45 MB | Interpreter + boxing |
| PyTorch CPU | ~120 MB | Framework overhead |
| C naive | ~0.1 MB | Manual, no overhead |
| Rust | ~0.08 MB | Stack-heavy |
| **Omnikarai** | **~0.05 MB** | Ephemeral computation — intermediates never written |

---

## Part 4 — Speed Architecture

### Compilation Pipeline

```
Source → Lexer → Parser → AST → [IR → Optimizations] → x86-64 → Run
  ~1µs     ~5µs            ~10µs      ~20µs               ~50µs   ~1µs
```
Total for 1000-line program: **< 100µs**. Fast enough to use as a scripting language.

### Optimization Passes (planned)

| Pass | Effect |
|------|--------|
| Constant folding | `2+3` → `5` at compile time |
| Dead code elimination | Remove code after `return` |
| Inline expansion | `@inline` fn calls replaced with body |
| Ephemeral store elimination | Skip writes for register-resident values (up to 6.7×) |
| Loop unrolling | Unroll fixed-count loops |
| SIMD vectorization | `@simd` loops → AVX2 (8× throughput) |
| Common subexpression elimination | `a*b + a*b` → `t=a*b; t+t` |
| Strength reduction | `x*2` → `x+x` |

### Code Quality Targets

| Operation | GCC -O3 | Omnikarai target |
|-----------|---------|-----------------|
| Integer add | 1 cycle | 1 cycle |
| Float add | 1 cycle | 1 cycle |
| 128×128 matmul | ~0.050ms | <0.020ms (via Ephemeral) |
| f-string print | ~200ns | ~80ns (compile-time concat) |
| Function call | ~2ns | ~1ns (@inline removes it) |
| For loop body | 1 op/cycle | 8 ops/cycle (SIMD) |

---

## Part 5 — Compiler Output Modes

```bash
omnicc run   file.ok    # compile + run immediately
omnicc build file.ok    # compile to standalone .exe (no runtime)
omnicc dump  file.ok    # dump x86-64 bytes
omnicc check file.ok    # type check + lint only
omnicc bench file.ok    # run with timing + GFLOPS report
```

### PE Writer (`omnicc build`)

```
file.ok → omnicc build → file.exe   (~10–50 KB, no runtime needed)
```

Structure: DOS header + PE header + `.text` (code) + `.rdata` (strings + imports) + import table for `kernel32.dll` only. No CRT. No MSVCRT. Smaller than a C "Hello World" from MSVC.

### Cross-Platform Roadmap

| Platform | Status |
|----------|--------|
| Windows x64 | ✅ Done |
| Linux x64 | 🔜 Phase 9 |
| macOS ARM64 | 🔜 Phase 10 |
| Linux ARM64 | 🔜 Phase 10 |
| WASM | 🔜 Future |

---

## Part 6 — Build Order (Exact Sequence)

### Phase 0 — Fix Bugs (NOW — nothing else until done)
- [ ] Fix while+continue (print not emitting inside loop body)
- [ ] Fix recursive function crash (fib deep recursion)
- [ ] Fix for loop after while not running
- [ ] Fix t05_simple.ok ParseError
- **Exit: 15/15 tests passing**

### Phase 1 — f-strings (highest QoL impact)
- [ ] Lexer: `f"..."` → `FSTRING_LITERAL` token
- [ ] Parser: parse `{expr}` segments inside f-string
- [ ] Codegen: emit str() + str.concat chain at compile time
- [ ] Test: `print(f"x={x} y={y}")` works for int, float, str, bool
- **Exit: f-strings work in all contexts**

### Phase 2 — Language Completeness
- [ ] `const` keyword (compile-time constants)
- [ ] Multiple return values: `return a, b` → `set x, y = fn()`
- [ ] Default parameters: `fn greet(name, greeting="Hello"):`
- [ ] Proper operator precedence verified (`*` before `+`)
- [ ] Float full support (already partially done)
- [ ] `@inline` annotation
- [ ] `@simd` annotation

### Phase 3 — Collections Runtime
- [ ] List literal `[a, b, c]` codegen
- [ ] List index `lst[i]` read + write
- [ ] `for item in list` iteration
- [ ] Dict literal `{"key": val}` codegen
- [ ] Dict index `d["key"]` read + write
- [ ] Tuple `(a, b)` + destructuring
- [ ] `in` operator: `if x in lst:`

### Phase 4 — Full String Library
- [ ] `str.upper`, `str.lower`, `str.trim`
- [ ] `str.split`, `str.join`
- [ ] `str.contains`, `str.find`, `str.replace`
- [ ] `str.starts_with`, `str.ends_with`
- [ ] String interning for compile-time literals

### Phase 5 — Error Handling
- [ ] `raise ErrorType(message)`
- [ ] `try / except / except Type as e`
- [ ] Thread-local error slot (no SEH)
- [ ] Stack trace on uncaught error

### Phase 6 — Class System
- [ ] `class` definition codegen (struct layout)
- [ ] `init` constructor
- [ ] `self` as implicit first arg (pointer to struct)
- [ ] Method dispatch (`v.length()` → `Vec2_length(&v)`)
- [ ] Instance field access (`self.x`)
- [ ] `extends` + vtable generation (single inheritance only)

### Phase 7 — AI Module
- [ ] `ai.matmul` → AVX2 VFMADD231PS emitter
- [ ] `ai.relu` → VMAXPS emitter
- [ ] `ai.dot` → register-only dot product
- [ ] `ai.softmax`, `ai.layernorm`
- [ ] INT8 quantized matmul via VPMADDUBSW
- [ ] Ephemeral computation elimination pass
- [ ] Benchmark vs C -O3, PyTorch CPU

### Phase 8 — PE Writer (`omnicc build`)
- [ ] DOS + PE header emission
- [ ] `.text` section with generated code
- [ ] `.rdata` section with string pool
- [ ] Import table for kernel32.dll
- [ ] Relocation table
- [ ] Test: generated .exe runs standalone

### Phase 9 — Closures + First-Class Functions
- [ ] `fn(x): return x * 2` as expression
- [ ] Closure capture (value copy into heap struct)
- [ ] `list.map`, `list.filter`, `list.reduce` with fn arg

### Phase 10 — Concurrency
- [ ] `use thread` module
- [ ] `thread.run(fn, arg)`
- [ ] `thread.join_all()`
- [ ] Channel (lock-free ring buffer)
- [ ] `spawn` / `await` sugar

### Phase 11 — Cross-Platform
- [ ] Linux x64: syscall instead of Win32, ELF output
- [ ] macOS/Linux ARM64: ARM64 codegen backend

---

## The Benchmark Goal

```
Target: beat C -O3 -march=native -mavx2
On:     4-layer MLP, hidden=128, batch=1, FP32
By:     2.5× wall-clock speedup
Via:    Ephemeral Computation + AVX2 FMA fusion
```

That benchmark is the proof. Every phase above builds toward it.

---

*Fraziym Tech & AI | Omnikarai Compiler | March 2026*
*"The software is the limit. We are the software."*
