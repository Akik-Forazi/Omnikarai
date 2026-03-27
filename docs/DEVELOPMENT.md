# Omnikarai Compiler — Development Log & Internals

> This document covers the real engineering history of `omnicc` — every major decision, every architecture change, and what the code actually does today.

---

## 1. Project Origin

**Creator:** Akik Faraji, Fraziym Tech & AI, Dhaka, Bangladesh  
**Goal:** Build a programming language from scratch — readable like Python, compiled to native x86-64, zero external dependencies.

The language is called **Omnikarai** (`.ok` files). The compiler is `omnicc`.

---

## 2. Architecture Evolution

### Phase 1 — Tree-walk Interpreter
The original codebase had:
- `src/interpreter.c` — tree-walking interpreter using `Object*` boxing
- `src/compiler.c` — stub with LLVM dependency stubs
- `src/jit.c` — LLVM MCJIT stubs
- `src/omni_runtime.c` — `OmniValue` boxing runtime

**Problems found and removed:**
- LLVM dependency made the compiler unusable without a full LLVM install
- `OmniValue` boxing added unnecessary overhead for a statically-dispatchable type system
- Tree-walking is slow and complex
- All LLVM/interpreter/JIT/runtime files were deleted

### Phase 2 — Native x86-64 Codegen (current)
Replaced everything with a single `src/codegen.c` that:
- Emits raw x86-64 bytes directly into a `CodeBuf`
- Uses `VirtualAlloc(PAGE_EXECUTE_READWRITE)` for in-memory execution
- Follows Windows x64 ABI exactly (shadow space, 16-byte alignment, RBP frame)
- Uses `WriteFile` (Win32 kernel) for output — no CRT, no SEH issues

**New pipeline:**
```
.ok source → Lexer → Parser → AST → codegen_compile() → VirtualAlloc → run
```

---

## 3. Lexer (`src/lexer.c`)

### 3.1 Key design decisions

**Cached `input_len`:** Original code called `strlen()` inside `read_char()` and `peek_char()` on every character — O(n²) for the entire file. Fixed by caching length in `l->input_len` during `lexer_init()`.

**INDENT / DEDENT tokens:** The lexer emits synthetic `TOKEN_INDENT` and `TOKEN_DEDENT` tokens to signal block entry/exit, exactly like Python's tokenizer. This is handled by `handle_indentation()` which runs at the start of every new line (`at_bol = 1`).

**`pending_tokens` queue:** Because `handle_indentation` may need to emit multiple DEDENT tokens before the actual line token, a small pending queue holds them and drains before the next real token is read.

**Block comment `#| ... |#`:** Single-pass scanning, no nesting support yet.

### 3.2 Token types (complete list)

```
TOKEN_EOF, TOKEN_ILLEGAL, TOKEN_IDENT, TOKEN_INT, TOKEN_FLOAT,
TOKEN_STRING, TOKEN_NL, TOKEN_INDENT, TOKEN_DEDENT,
TOKEN_ASSIGN, TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH,
TOKEN_PERCENT, TOKEN_POWER, TOKEN_EQ, TOKEN_NOT_EQ,
TOKEN_LT, TOKEN_GT, TOKEN_LTE, TOKEN_GTE,
TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE,
TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_COMMA, TOKEN_COLON,
TOKEN_DOT, TOKEN_SEMICOLON, TOKEN_ARROW,
TOKEN_SET, TOKEN_FN, TOKEN_CLASS, TOKEN_IF, TOKEN_ELIF, TOKEN_ELSE,
TOKEN_WHILE, TOKEN_FOR, TOKEN_IN, TOKEN_RETURN, TOKEN_USE, TOKEN_AS,
TOKEN_TRUE, TOKEN_FALSE, TOKEN_NIL, TOKEN_AND, TOKEN_OR, TOKEN_NOT,
TOKEN_MATCH, TOKEN_CASE, TOKEN_BREAK, TOKEN_CONTINUE, TOKEN_SELF
```

---

## 4. Parser (`src/parser.c`)

### 4.1 Pratt parser

A **Top-Down Operator Precedence (Pratt) parser** is used for expressions. Two function tables:
- `prefix_parse_fns[]` — one function per token type, called when that token appears in prefix position
- `infix_parse_fns[]` — one function per token type, called when that token appears in infix (binary) position

Precedence levels:
```c
PREC_LOWEST
PREC_EQUALS       // ==  !=
PREC_LESSGREATER  // >  <  >=  <=
PREC_SUM          // +  -
PREC_PRODUCT      // *  /
PREC_PREFIX       // -x  !x
PREC_CALL         // fn()
PREC_INDEX        // arr[i]
```

> ⚠️ **Known limitation:** `PREC_PRODUCT > PREC_SUM` is defined in the table, but the parser currently evaluates left-to-right for some compound expressions. This is being tracked as a bug. Use parentheses to guarantee order.

### 4.2 Block parsing (`parse_block_statement`)

```
<keyword> <condition>:
    INDENT
    statement...
    DEDENT
```

The parser:
1. Consumes any trailing `TOKEN_NL` after `:`
2. Expects `TOKEN_INDENT`
3. Loops parsing statements until `TOKEN_DEDENT` or `TOKEN_EOF`
4. Leaves `currentToken` on `TOKEN_DEDENT` (consumed by caller)

### 4.3 `parse_if_statement` and elif chaining

`elif` is implemented by recursively calling `parse_if_statement()` and storing the result as `stmt->alternative`. This naturally builds a chain:

```
IF_STATEMENT
  condition: score >= 90
  consequence: [block]
  alternative: IF_STATEMENT          ← elif
    condition: score >= 75
    consequence: [block]
    alternative: BLOCK_STATEMENT     ← else
```

### 4.4 Memory: pre-allocated capacity

`parse_program()` pre-allocates capacity=16, grows 2× — fixed the original realloc-per-statement O(n²) heap thrashing.

### 4.5 Supported AST node types

**Statements:** `SET_STATEMENT`, `RETURN_STATEMENT`, `EXPRESSION_STATEMENT`, `BLOCK_STATEMENT`, `FN_DEFINITION`, `CLASS_DEFINITION`, `IF_STATEMENT`, `WHILE_STATEMENT`, `FOR_STATEMENT`, `MATCH_STATEMENT`, `MATCH_CASE_STATEMENT`

**Expressions:** `IDENTIFIER`, `INTEGER_LITERAL`, `STRING_LITERAL`, `BOOLEAN_LITERAL`, `NIL_LITERAL`, `ARRAY_LITERAL`, `MAP_LITERAL`, `INFIX_EXPRESSION`, `PREFIX_EXPRESSION`, `CALL_EXPRESSION`, `FN_LITERAL`, `MEMBER_ACCESS_EXPRESSION`, `EMPTY_EXPRESSION`

---

## 5. Code Generator (`src/codegen.c`)

### 5.1 Overview

`codegen.c` emits raw x86-64 bytes into a `CodeBuf` (dynamic byte array). After compilation, the bytes are copied into `VirtualAlloc`'d executable memory and called as a C function pointer.

**Windows x64 ABI rules followed:**
- Args passed in: RCX, RDX, R8, R9, then stack
- Caller-saved: RAX, RCX, RDX, R8-R11
- Callee-saved: RBX, RBP, RDI, RSI, R12-R15
- 32-byte shadow space before every `call`
- RSP must be 16-byte aligned before `call`
- After `push rbp`: RSP is 16n-8, so `sub rsp, N` where `N % 16 == 8`

### 5.2 Stack frame

```asm
push rbp
mov rbp, rsp
sub rsp, N        ; N = (locals + 32) rounded to N%16==8
                  ; patched after all statements are compiled

; locals at [rbp-8], [rbp-16], [rbp-24], ...

; epilogue:
mov rsp, rbp
pop rbp
ret
```

`N` is backpatched: a 4-byte placeholder is emitted at prologue, then `patch_u32()` fills it after `cg->stack_size` is known.

### 5.3 Symbol table

FNV-1a hash table (256 buckets, open chaining). Each symbol stores:
- `name[64]` — variable name
- `OmniType` — `INT`, `BOOL`, `STR`, `UNKNOWN`
- `stack_offset` — `[rbp - offset]`

Scopes chain via `parent` pointer. `scope_get()` walks up the chain.

### 5.4 Expression codegen — result always in RAX

All expressions leave their result in `RAX`. Infix expressions use `R10` as scratch:

```asm
; set r10 = left
; eval right → rax
; mov rcx, r10    (rcx = left)
; xchg rax, rcx  (rax = left, rcx = right)
; <operation>    (result in rax)
```

This avoids `push/pop` which would disturb RSP alignment.

### 5.5 Runtime output — WriteFile, not printf

`omni_print_int`, `omni_print_str`, `omni_print_bool` use `WriteFile` (Win32 kernel directly). This is critical — MinGW's `printf` uses SEH-based unwinding internally. When Windows' exception dispatcher walks our JIT stack frame it finds no `.pdata` unwind record and crashes. `WriteFile → kernel32` has none of that.

Functions are marked `__attribute__((noinline))` and stored in a `volatile` table so `-O2` cannot eliminate them.

### 5.6 `return` statement

```asm
; eval return_value → rax
mov rsp, rbp   ; unwind locals
pop rbp
ret
```

Sets `cg->returned = 1`. The default epilogue at the end of `codegen_compile()` is skipped if `returned == 1`.

### 5.7 Jump backpatching

Forward jumps (`if`, `while`) emit a placeholder `je`/`jmp` with a 4-byte displacement of 0. After the target is known, `resolve_jump()` fills the correct `int32_t` relative displacement.

### 5.8 String literals

String literals are heap-allocated as stable copies tracked in `cg->string_pool[]`. The address is emitted as `mov rax, imm64`. The code buffer gets `realloc`'d, so embedding string bytes there would be unsafe.

---

## 6. Main Entry Point (`src/main.c`)

```
omnicc run   <file>  → compile + codegen_run()  → returns int64_t exit code
omnicc dump  <file>  → compile + codegen_dump() → hex dump to stderr
omnicc build <file>  → stub (PE writer coming)
```

`cmd_run()` calls `codegen_run()` which returns `int64_t` — the full 64-bit RAX value. This was changed from `int` to avoid truncation on Windows x64 where upper 32 bits of RAX may differ.

---

## 7. Build

```makefile
CC      = gcc
CFLAGS  = -Iinclude -Wall -Wextra -std=c99 -O2
LDFLAGS = -lkernel32
TARGET  = bin/omnicc.exe
SOURCES = src/main.c src/lexer.c src/parser.c src/codegen.c
```

```bash
gcc -Iinclude -O2 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32
```

---

## 8. Bug History

| Bug | Root cause | Fix |
|-----|-----------|-----|
| Lexer O(n²) | `strlen()` called per character in `read_char` | Cached `l->input_len` |
| Parser heap thrash | `realloc` every statement in `parse_program` | Pre-alloc 16, grow 2× |
| Wrong exit code (`63` instead of `67`) | `printf("%d\n", exit_code)` printed the value AND it was also the return value — confused output | Removed the `printf` line |
| `int` vs `int64_t` return | `codegen_run` returned `int`, truncating 64-bit RAX | Changed return type to `int64_t` |
| `conflicting types for codegen_run` | Definition in `.c` still said `int` after `.h` was updated | Fixed definition to match `int64_t` |
| `return` doesn't stop execution | Default epilogue always emitted after statements | Added `cg->returned` flag, skip default epilogue |

---

## 9. Next Milestones

### Milestone 1 — User-defined function calls (next)
- Codegen for `FN_DEFINITION`: emit the function body, record its code offset
- Codegen for `CALL_EXPRESSION`: look up function address, set up frame, call it
- Pass arguments in RCX/RDX/R8/R9 per Windows x64 ABI

### Milestone 2 — Operator precedence fix
- Parser already has the precedence table correct
- Investigate why compound expressions evaluate left-to-right in some cases
- Add test cases to verify `2 + 3 * 4 == 14`

### Milestone 3 — For loop codegen
- `for i in range(n)` — compile as `while` with an index variable
- `for item in list` — requires runtime list support first

### Milestone 4 — Match/case codegen
- Compile as a chain of `cmp + je` instructions
- Range patterns (`500..599`) need a between-check

### Milestone 5 — Runtime type system
- Lists, dicts, tuples as heap-allocated tagged structs
- `len()`, `append()`, indexing `[]`

### Milestone 6 — Class system
- `class` → vtable-based dispatch
- `self` → pointer passed as first arg
- `init` → called on instantiation

### Milestone 7 — Module loader
- `use math` → find `math.ok` in `~/.omnikarai/modules/`
- Compile and expose its namespace
- `omnip install` to download from OPi

### Milestone 8 — PE writer (`omnicc build`)
- Emit a valid Windows PE32+ `.exe` with the generated x86-64 bytes
- No runtime dependency
