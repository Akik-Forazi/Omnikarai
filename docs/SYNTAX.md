# Omnikarai Language Specification (v6.0)

> **Creator:** Akik Faraji — Fraziym Tech & AI  
> **Compiler:** `omnicc` (x86-64 native, Windows)  
> **Package Manager:** `omnip` (Omnikarai Package Index)  
> **File Extension:** `.ok`

---

## 1. Overview

Omnikarai is a **high-level, indentation-sensitive language** that compiles directly to native x86-64 machine code on Windows — no LLVM, no VM, no bytecode, no interpreter.

**Core principles:**

- **Readable:** Minimal punctuation, Python-like indentation blocks
- **Native:** Compiles straight to x86-64 bytes via `omnicc`, runs via `VirtualAlloc`
- **Zero dependencies:** The compiler is pure C with only `kernel32` linkage
- **Pythonic feel, C-level speed:** Familiar syntax, machine-code execution

**Compiler pipeline:**

```
source.ok → Lexer → Parser → AST → x86-64 Codegen → VirtualAlloc → Execute
```

---

## 2. Lexical Conventions

### 2.1 Identifiers

- Must start with a letter or `_`
- Can contain letters, digits, `_`
- Case-sensitive

```omnikarai
set name = "Alice"
set _counter123 = 0
set MY_CONST = 42
```

### 2.2 Keywords

```
set  fn  class  init  if  elif  else  match  case
while  for  in  return  use  as  true  false  nil
and  or  not  break  continue  self
```

### 2.3 Comments

```omnikarai
# single-line comment
#| multi-line
   comment |#
```

### 2.4 Literals

| Type    | Examples                     |
|---------|------------------------------|
| Integer | `10`, `0`, `-5`              |
| Float   | `3.14`, `1.0`                |
| Hex     | `0xFF`, `0x1A`               |
| Binary  | `0b1010`, `0b0001`           |
| String  | `"hello"`, `'world'`         |
| Boolean | `true`, `false`              |
| Nil     | `nil`                        |

---

## 3. Variables

```omnikarai
set x = 42          # declaration — always use 'set' for first assignment
x = x + 1           # re-assignment — no 'set' needed
```

- `set` declares a new variable in the current scope
- Re-assignment without `set` updates an existing variable
- All values are 64-bit internally (int64, bool as 0/1, str as char*)
- Type is inferred — no annotations needed

---

## 4. Expressions

### 4.1 Arithmetic

```omnikarai
set a = 10 + 5
set b = a * 2
set c = b / 3
set d = a - 1
```

Operators: `+  -  *  /  %  **`

> ⚠️ **Note (v6.0):** Operator precedence (`*` before `+`) is not yet implemented in the parser. Expressions are evaluated left-to-right. Use parentheses to enforce order: `set c = a + (b * 2)`

### 4.2 Comparison

```omnikarai
set eq  = 10 == 10   # true
set neq = 10 != 9    # true
set lt  = 5 < 10     # true
set gt  = 10 > 5     # true
set lte = 5 <= 5     # true
set gte = 6 >= 5     # true
```

### 4.3 Logical

```omnikarai
if a > 0 and a < 100:
    print("in range")

if not is_done:
    print("still working")
```

Operators: `and  or  not`

---

## 5. Control Flow

### 5.1 If / Elif / Else

```omnikarai
if score >= 90:
    print("A")
elif score >= 75:
    print("B")
elif score >= 60:
    print("C")
else:
    print("F")
```

### 5.2 While Loop

```omnikarai
set counter = 0
while counter < 5:
    print(counter)
    set counter = counter + 1
```

### 5.3 For Loop *(parsed, codegen coming)*

```omnikarai
for item in items:
    print(item)

for i in range(10):
    print(i)
```

> ⚠️ `for` is parsed and produces a valid AST, but codegen for iteration is not yet implemented. Use `while` for now.

### 5.4 Match / Case *(parsed, codegen coming)*

```omnikarai
match status_code:
    case 200:
        print("OK")
    case 404:
        print("Not Found")
    case _:
        print("Unknown")
```

> ⚠️ `match` is parsed but codegen not yet implemented.

---

## 6. Functions

### 6.1 Definition

```omnikarai
fn add(x, y):
    return x + y
```

### 6.2 Calling

```omnikarai
set result = add(5, 10)
print(result)
```

> ⚠️ **v6.0 status:** `fn` definitions are parsed and produce a valid AST. User-defined function *calls* are not yet compiled by codegen — this is the next major milestone. Currently only the built-in `print()` is callable.

### 6.3 Return

```omnikarai
fn classify(n):
    if n > 0:
        return "positive"
    elif n < 0:
        return "negative"
    else:
        return "zero"
```

- `return` exits the current function and passes the value back
- Dead code after `return` is silently ignored (same as Python)
- Top-level `return` sets the program exit code

---

## 7. Built-in Functions (currently implemented)

| Function | Description | Example |
|----------|-------------|---------|
| `print(x)` | Prints value + newline | `print("hello")` |
| `print(n)` | Prints integer + newline | `print(42)` |
| `print(b)` | Prints `true`/`false` + newline | `print(true)` |

All output goes through `WriteFile` (Win32 kernel) — no CRT dependency.

---

## 8. Classes

```omnikarai
class Person:
    fn init(self, name, age):
        self.name = name
        self.age = age

    fn greet(self):
        print("Hello, " + self.name)

set p = Person("Alice", 30)
p.greet()
```

- `init` is the constructor
- `self` refers to the current instance
- Methods must declare `self` as first parameter

> ⚠️ `class` is parsed and produces an AST. Codegen for class instantiation and method dispatch is planned for a future phase.

---

## 9. Collections *(parsed, codegen coming)*

### 9.1 Lists

```omnikarai
set fruits = ["apple", "banana", "cherry"]
fruits.append("orange")
print(len(fruits))
```

### 9.2 Dictionaries

```omnikarai
set person = {"name": "Alice", "age": 30}
person["city"] = "Dhaka"
```

### 9.3 Tuples

```omnikarai
set point = (10, 20)
```

> ⚠️ Collections are in the language design and will be implemented in the runtime phase.

---

## 10. Modules (`use` / `omnip`)

### 10.1 Importing

```omnikarai
use math
print(math.sqrt(16))

use collections.Array as MyList
set items = MyList.new()
```

### 10.2 Omnip package manager

```bash
omnip install requests      # install from OPi (Omnikarai Package Index)
omnip install .             # install local module
omnip uninstall requests    # remove module
omnip list                  # show installed modules
omnip publish .             # publish to OPi
```

### 10.3 Module packaging (`omnikarai.toml`)

```toml
[metadata]
name = "my_module"
version = "1.0.0"
author = "Akik Faraji"
description = "My Omnikarai module"
license = "MIT"

[dependencies]
math = ">=1.0"
```

Installed modules live at: `~/.omnikarai/modules/`  
Registry at: `~/.omnip/installed_modules.json`

> ⚠️ `use` is lexed and parsed. Module loading and `omnip` are planned for the next development phase after user-defined function calls are implemented.

---

## 11. Compiler CLI (`omnicc`)

```bash
omnicc run   file.ok    # compile and run immediately
omnicc build file.ok    # compile to .exe  (coming soon — PE writer)
omnicc dump  file.ok    # dump generated x86-64 machine code bytes
```

### 11.1 What `omnicc run` does internally

1. `read_file()` — loads `.ok` source
2. `lexer_init()` — tokenizes, emits INDENT/DEDENT tokens
3. `new_parser()` + `parse_program()` — builds AST
4. `codegen_compile()` — emits x86-64 bytes
5. `VirtualAlloc(PAGE_EXECUTE_READWRITE)` — allocates executable memory
6. Copies bytes → casts to function pointer → calls it
7. Returns `int64_t` exit code
8. `VirtualFree()` — releases memory

### 11.2 Stack frame layout

```
[RBP]       ← saved caller RBP
[RBP - 8]   ← first local variable
[RBP - 16]  ← second local variable
...
[RSP]       ← current stack top (16-byte aligned before calls)
```

Shadow space (32 bytes) is reserved before every `call` for Windows x64 ABI compliance.

---

## 12. Error Handling *(future)*

```omnikarai
try:
    risky_function()
except Exception as e:
    print("Error:", e)
```

> Planned after class system and module loading are complete.

---

## 13. Standard Library (planned)

| Module | Functions |
|--------|-----------|
| `math` | `sqrt`, `sin`, `cos`, `abs`, `floor`, `ceil` |
| `collections` | `Array`, `Map`, `Set` |
| `os` | file and system operations |
| `io` | `print`, `input`, `open`, `read`, `write` |
| `re` | regex matching |
| `random` | random numbers |
| `json` | JSON encode/decode |

---

## 14. What Works Right Now (v6.0)

| Feature | Status |
|---------|--------|
| Lexer (all tokens, INDENT/DEDENT) | ✅ Complete |
| Parser (all statements/expressions) | ✅ Complete |
| `set` variable declaration | ✅ Codegen working |
| Variable re-assignment | ✅ Codegen working |
| Integer arithmetic `+ - * /` | ✅ Codegen working |
| Comparison operators `== != < > <= >=` | ✅ Codegen working |
| `if / elif / else` | ✅ Codegen working |
| `while` loop | ✅ Codegen working |
| `print()` built-in (int, str, bool) | ✅ Codegen working |
| `return` statement | ✅ Codegen working |
| String literals | ✅ Codegen working |
| Boolean literals `true / false` | ✅ Codegen working |
| Negative numbers / prefix `-` | ✅ Codegen working |
| `fn` definition | ✅ Parsed → codegen next |
| `for` loop | ✅ Parsed → codegen next |
| `match / case` | ✅ Parsed → codegen next |
| `class` definition | ✅ Parsed → codegen next |
| User-defined function calls | 🔜 Next milestone |
| Collections (list, dict) | 🔜 Runtime phase |
| `use` / module loading | 🔜 Module phase |
| `omnip` package manager | 🔜 Module phase |
| PE writer (`omnicc build`) | 🔜 Future |
| Operator precedence (`*` before `+`) | 🔜 Parser fix needed |
| `try / except` | 🔜 Future |

---

## 15. Project File Structure

```
omniwin/
├── bin/
│   └── omnicc.exe
├── src/
│   ├── main.c          — CLI entry point (run/build/dump)
│   ├── lexer.c         — tokenizer, INDENT/DEDENT emission
│   ├── parser.c        — Pratt parser → AST
│   └── codegen.c       — x86-64 native code emitter
├── include/
│   ├── lexer.h
│   ├── parser.h
│   ├── ast.h
│   └── codegen.h
├── docs/
│   ├── SYNTAX.md           — this file
│   ├── DEVELOPMENT.md      — compiler internals & history
│   ├── development_plan.md — roadmap
│   ├── SimpleGuide.md      — beginner's guide
│   └── BuildYourOwnOmni.md — step-by-step language tutorial
├── test.ok
├── test_advanced.ok
├── test_comprehensive.ok
└── Makefile
```

---

## 16. Example Programs

### Hello World
```omnikarai
print("Hello, World!")
```

### Variables and Arithmetic
```omnikarai
set a = 100
set b = 37
set c = a - b
print(a)
print(b)
print(c)
return 0
```

### If / Elif / Else
```omnikarai
set score = 82
if score >= 90:
    print("A")
elif score >= 75:
    print("B")
else:
    print("C")
```

### While Loop
```omnikarai
set i = 0
while i < 5:
    print(i)
    set i = i + 1
```

### Booleans and Comparisons
```omnikarai
set x = 42
set is_big = x > 40
print(is_big)
print(x == 42)
print(x != 0)
```
