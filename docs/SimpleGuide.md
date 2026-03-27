# A Simple Guide to How Omnikarai Works

Welcome! This guide explains how Omnikarai's compiler works — written simply so anyone can understand it, even if you've never built a language before.

---

## The Big Picture

When you write `.ok` code and run `omnicc run myprogram.ok`, here is exactly what happens:

```
Your text file (.ok)
      ↓
  1. Lexer       — reads characters, produces tokens
      ↓
  2. Parser      — reads tokens, builds a tree (AST)
      ↓
  3. Codegen     — walks the tree, emits x86-64 bytes
      ↓
  4. VirtualAlloc — Windows gives us executable memory
      ↓
  5. Execute     — CPU runs the bytes directly
      ↓
  Output on your terminal
```

There is no VM. No bytecode. No Python runtime. The bytes that come out of step 3 are the same kind of bytes your CPU runs when you open any `.exe` on Windows.

---

## Step 1: The Lexer (Word Sorter)

The source file is just a long string of characters. The lexer reads it character by character and groups them into **tokens** — the "words" of the language.

Example:

```omnikarai
set x = 42
```

Becomes this stream of tokens:

```
TOKEN_SET    "set"
TOKEN_IDENT  "x"
TOKEN_ASSIGN "="
TOKEN_INT    "42"
TOKEN_NL
```

**The hard part: INDENT and DEDENT**

Omnikarai uses indentation for code blocks (like Python). The lexer tracks the indentation level using a stack. When indentation increases, it emits a `TOKEN_INDENT`. When it decreases, it emits `TOKEN_DEDENT`. These synthetic tokens let the parser know where blocks start and end without needing `{` and `}`.

```omnikarai
if x > 0:
    print("positive")   ← INDENT emitted before this line
print("done")           ← DEDENT emitted before this line
```

---

## Step 2: The Parser (Rule Checker + Blueprint Builder)

The parser reads the token stream and checks that everything follows the grammar rules. If it does, it builds an **Abstract Syntax Tree (AST)** — a tree structure that represents the meaning of your code.

Example:

```omnikarai
set result = 10 + 5
```

Becomes this tree:

```
SET_STATEMENT
  name: "result"
  value: INFIX_EXPRESSION
           left:  INTEGER_LITERAL (10)
           op:    "+"
           right: INTEGER_LITERAL (5)
```

**The Pratt parser** is used for expressions. It handles operator precedence — knowing that `*` should bind tighter than `+`. Each token type has a "prefix function" (what to do when this token starts an expression) and an "infix function" (what to do when this token appears between two expressions).

---

## Step 3: The Code Generator (Byte Emitter)

The codegen walks the AST and emits real x86-64 CPU instructions as bytes.

Example — compiling `set x = 10 + 5`:

```asm
mov rax, 10          ; load left value (10) into RAX
mov r10, rax         ; save left into R10 (scratch register)
mov rax, 5           ; load right value (5) into RAX
mov rcx, r10         ; move left to RCX
xchg rax, rcx        ; now RAX=left(10), RCX=right(5)
add rax, rcx         ; RAX = 10 + 5 = 15
mov [rbp-8], rax     ; store result at variable slot on stack
```

Every variable lives at a fixed offset below `RBP` (the frame base pointer). The codegen builds a symbol table mapping variable names to their stack offsets.

**The stack frame looks like this:**

```
[RBP + 0] = saved old RBP
[RBP - 8] = first variable (x)
[RBP - 16] = second variable (y)
...
[RSP]      = current top of stack
```

---

## Step 4: VirtualAlloc — Making Bytes Executable

You can't just run bytes from a regular `malloc` buffer — Windows marks that memory as non-executable by default.

`VirtualAlloc(PAGE_EXECUTE_READWRITE)` gives us a special memory region that the CPU is allowed to execute. We copy our bytes there, cast the address to a function pointer, and call it.

```c
void* mem = VirtualAlloc(NULL, code_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
memcpy(mem, code_bytes, code_size);
typedef int64_t (*Fn)(void);
int64_t result = ((Fn)mem)();
VirtualFree(mem, 0, MEM_RELEASE);
```

---

## Step 5: Output — WriteFile, Not printf

For printing, Omnikarai calls `WriteFile` directly (a Windows kernel function) instead of `printf`. Why? Because `printf` on MinGW/Windows uses exception-handling machinery (SEH) internally. When Windows' exception handler tries to walk our JIT-generated stack frame, it crashes because our code has no unwind tables.

`WriteFile → kernel32.dll` has none of that overhead. It's also faster.

---

## The Pieces That Are Working Right Now

| What you write | What the compiler does |
|----------------|----------------------|
| `set x = 42` | emits `mov rax, 42` + stores to stack slot |
| `set z = x + y` | loads both from stack, adds in registers |
| `print("hello")` | loads string address into RCX, calls `omni_print_str` |
| `print(42)` | loads integer into RCX, calls `omni_print_int` |
| `if x > 0:` | emits `cmp + je` with backpatched jump |
| `while i < 5:` | emits compare + conditional jump + back-jump |
| `return x` | loads `x` into RAX + epilogue + `ret` |

---

## What's Coming Next

1. **User-defined function calls** — `fn add(x, y): return x + y` / `add(5, 10)`
2. **Operator precedence fix** — `2 + 3 * 4` should give `14`, not `20`
3. **For loops** — `for i in range(5):`
4. **Match/case** — `match status: case 200:`
5. **Lists and dictionaries** — `[1, 2, 3]` / `{"key": value}`
6. **Classes** — `class Person:` with methods
7. **Module loading** — `use math`
8. **PE writer** — `omnicc build` to produce a real `.exe` file

---

## How to Add a New Feature

Every new language feature follows the same three-step pattern:

### 1. Teach the Lexer (`src/lexer.c`)
Add a new token type in `include/lexer.h`. Teach `get_next_token()` to recognize the new keyword or symbol.

### 2. Teach the Parser (`src/parser.c`)
Register a prefix or infix parse function. Write a `parse_*_statement()` or `parse_*_expression()` function. Add a new AST node type in `include/ast.h` if needed.

### 3. Teach the Codegen (`src/codegen.c`)
Add a case to `cg_stmt()` or `cg_expr()`. Emit the x86-64 bytes that implement the feature.

That's it. Every feature in the language was built exactly this way.

---

## Build and Test

```bash
# Build
gcc -Iinclude -O2 -o bin/omnicc.exe src/main.c src/lexer.c src/parser.c src/codegen.c -lkernel32

# Run a program
./bin/omnicc run test.ok

# See the generated machine code bytes
./bin/omnicc dump test.ok
```
