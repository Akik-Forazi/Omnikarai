# Omnikarai Language Development Plan (v0.1)

## 1. Overview

Omnikarai is a **high-level, dynamically typed programming language**. Its core principles are:

1. **Readability:** Simple syntax with indentation-based code blocks.
2. **Expressiveness:** Powerful language features with minimal boilerplate.
3. **Performance:** Uses **JIT compilation** for runtime efficiency, faster than interpreted languages.

Omnikarai programs have the `.ok` file extension. Users execute programs via the **Omnikarai compiler (`omnicc`)**, which handles **compile-and-run** in a single step.

---

## 2. Grammar

### 2.1 Comments

```omnikarai
# Single-line comment
#| 
Multi-line comment 
spanning multiple lines 
|#
```

### 2.2 Variables

```omnikarai
set name = "Omnikarai"  # Declaration
age = 20                # Re-assignment
```

### 2.3 Data Types

* Number (integer, float)
* String
* Boolean (`true`/`false`)
* Nil (`nil`)

### 2.4 Control Flow

```omnikarai
if condition:
    # code block
elif condition2:
    # code block
else:
    # code block

while counter < 10:
    counter = counter + 1

for item in list:
    print(item)
```

### 2.5 Functions

```omnikarai
fn add(x, y):
    return x + y

set result = add(5, 10)
```

### 2.6 Classes

```omnikarai
class Person:
    fn init(self, name):
        self.name = name

    fn greet(self):
        print("Hello, " + self.name)

set p = Person("Alice")
p.greet()
```

---

## 3. Module Management (Omnip)

Omnikarai has a **package manager called `omnip`** (like Python’s pip).

### 3.1 Installing a module

```bash
omnip install requests
```

### 3.2 Listing installed modules

```bash
omnip list
```

### 3.3 Removing a module

```bash
omnip uninstall requests
```

### 3.4 Importing modules in code

```omnikarai
use math
print(math.sqrt(25))

use collections.Array as MyList
set items = MyList.new()
```

---

## 4. Compiler and Execution (Omnicc)

Omnikarai programs are **compiled and run via `omnicc`**, which acts like a JIT **compiler-and-runner**.

### 4.1 Basic usage

```bash
omnicc hello.ok
```

### 4.2 Terminal Output

* Works like Python: prints directly to terminal.
* Supports runtime debugging.
* Handles JIT compilation on-the-fly; no separate compilation step required.

---

## 5. Runtime Architecture

1. **Lexer:** Tokenizes source code.
2. **Parser:** Converts tokens into AST (Abstract Syntax Tree).
3. **JIT Compiler:** Converts AST to optimized bytecode or native code.
4. **Runtime Execution:** Runs compiled code immediately in the terminal.

---

## 6. File Structure (for developers)

```
Omnikarai/
├─ bin/omnicc       # Compiler/executor
├─ src/
│  ├─ main.c
│  ├─ lexer.c
│  ├─ parser.c
│  ├─ compiler.c
│  ├─ runtime.c
├─ include/         # Header files
├─ modules/         # Installed Omnik modules
└─ tests/
   ├─ hello.ok
   └─ test.ok
```

---

## 7. Future Enhancements

* **Optimized JIT:** Improved runtime speed.
* **Error Handling:** Clear, user-friendly error messages.
* **Standard Library Expansion:** More built-in modules.
* **Interactive Shell:** REPL mode like Python.
* **Cross-platform support:** Linux, Windows, macOS.

