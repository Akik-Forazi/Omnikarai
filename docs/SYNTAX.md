# Omnikarai Language Specification (v5.0)

## 1. Overview

Omnikarai is a **high-level, dynamically typed language** with the following characteristics:

* **Readable & Expressive:** Minimal punctuation, indentation-based code blocks.
* **Dynamic Typing:** Variables infer their type at runtime.
* **JIT Compilation:** `omnicc` compiles and runs `.ok` files on-the-fly.
* **Module System:** Managed by `omnip`, analogous to Python's pip.

---

## 2. Lexical Conventions

### 2.1 Identifiers

* Must start with a letter or `_`.
* Can contain letters, digits, `_`.

```omnikarai
set name = "Alice"
set _counter123 = 0
```

### 2.2 Keywords

```
set, fn, class, init, if, elif, else, match, case, while, for, in, return, use, true, false, nil
```

### 2.3 Comments

```omnikarai
# single-line comment
#| multi-line comment |#
```

### 2.4 Literals

* **Numbers:** Integers (10), Floats (3.14)
* **Strings:** `'text'` or `"text"`
* **Booleans:** `true`, `false`
* **Nil:** `nil`

---

## 3. Variables and Assignment

```omnikarai
set x = 42       # declaration
x = x + 1        # reassignment
```

* `set` is only used for initial declaration.
* No type annotation needed; type is dynamic.

---

## 4. Expressions

* Arithmetic: `+ - * / % **`
* Comparison: `== != > < >= <=`
* Logical: `and or not`

```omnikarai
set a = 10 + 5
if a > 12 and a < 20:
    print("Valid")
```

---

## 5. Control Flow

### 5.1 If-elif-else

```omnikarai
if condition:
    # code
elif condition2:
    # code
else:
    # code
```

### 5.2 Match-case (pattern matching)

```omnikarai
match code:
    case 200: print("OK")
    case 404: print("Not Found")
    case 500..599: print("Server Error")
    case _: print("Unknown")
```

### 5.3 Loops

```omnikarai
# While loop
while counter < 5:
    counter = counter + 1

# For loop
for fruit in ["apple", "banana"]:
    print(fruit)
```

---

## 6. Functions

```omnikarai
fn add(x, y):
    return x + y

set result = add(5, 10)
```

* Functions are first-class objects.
* Supports default arguments, variable-length arguments:

```omnikarai
fn greet(name="World", *args):
    print("Hello " + name)
```

---

## 7. Classes

```omnikarai
class Person:
    fn init(self, name, age):
        self.name = name
        self.age = age

    fn greet(self):
        print("Hello, " + self.name)
```

* Methods must include `self` for instance reference.
* `init` is the constructor.

---

## 8. Collections

### 8.1 Lists

```omnikarai
set fruits = ["apple", "banana", "cherry"]
fruits.append("orange")
```

### 8.2 Dictionaries

```omnikarai
set person = {"name": "Alice", "age": 30}
person["city"] = "Dhaka"
```

### 8.3 Tuples (immutable)

```omnikarai
set point = (10, 20)
```

---

## 9. Modules (Omnip)

* `omnip` is the module manager.
* **Install a module:** `omnip install requests`
* **Remove module:** `omnip uninstall requests`
* **List modules:** `omnip list`

### 9.1 Importing

```omnikarai
use math
print(math.sqrt(16))

use collections.Array as MyList
set items = MyList.new()
```

---

## 10. Compiler & Execution (`omnicc`)

* **Compile-and-run command:**

```bash
omnicc hello.ok
```

* Acts as **JIT compiler**:

  * Lexer → Parser → AST → JIT Compilation → Execute
* Runtime output appears directly in terminal.

### 10.1 Execution flow

1. Parse `.ok` file.
2. Build AST.
3. Compile AST to optimized bytecode.
4. Run bytecode in runtime VM.

---

## 11. Standard Library

* `math` → `sqrt`, `sin`, `cos`
* `collections` → `Array`, `Map`
* `os` → file and system operations
* `io` → print, read input

---

## 12. Error Handling

```omnikarai
try:
    risky_function()
except Exception as e:
    print("Error:", e)
```

* Future enhancement: structured exception classes.

---

## 13. File Structure for Development

```
Omnikarai/
├─ bin/omnicc
├─ src/
│  ├─ main.c
│  ├─ lexer.c
│  ├─ parser.c
│  ├─ compiler.c
│  ├─ runtime.c
├─ include/
├─ modules/
└─ tests/
```

---

## 14. JIT Compilation Notes

* Each `.ok` file is **parsed and compiled in-memory** before execution.
* Intermediate bytecode allows **optimizations**, such as:

  * Constant folding
  * Function inlining
  * Loop unrolling (future optimization)

---

## 15. Example Program

```omnikarai
# hello.ok
set name = "Omnikarai"

fn greet(user):
    print("Hello, " + user)

greet(name)

# Run
# omnicc hello.ok
```

**Terminal Output:**

```
Hello, Omnikarai
```


## A demo transformer

---

### `transformer_demo.ok`

```omnikarai
# Omnikarai Transformer Demo
# A minimal transformer to demonstrate ML concepts

use math
use random

# ------------------------------
# Utilities
# ------------------------------
fn softmax(x):
    set exps = []
    set sum_exp = 0
    for i in x:
        set e = math.exp(i)
        exps.append(e)
        sum_exp = sum_exp + e
    set out = []
    for e in exps:
        out.append(e / sum_exp)
    return out

fn dot(a, b):
    # dot product of two vectors
    set sum = 0
    for i in range(len(a)):
        sum = sum + a[i] * b[i]
    return sum

fn matmul(a, b):
    # simple matrix multiplication a[m][n] * b[n][p]
    set result = []
    for row in a:
        set new_row = []
        for j in range(len(b[0])):
            set sum = 0
            for k in range(len(row)):
                sum = sum + row[k] * b[k][j]
            new_row.append(sum)
        result.append(new_row)
    return result

# ------------------------------
# Attention Mechanism
# ------------------------------
fn scaled_dot_product_attention(Q, K, V):
    # Q,K,V are matrices
    set scores = matmul(Q, transpose(K))
    # scale
    set d_k = len(K[0])
    for i in range(len(scores)):
        for j in range(len(scores[0])):
            scores[i][j] = scores[i][j] / math.sqrt(d_k)
    # apply softmax row-wise
    set attention = []
    for row in scores:
        attention.append(softmax(row))
    # weighted sum
    return matmul(attention, V)

fn transpose(matrix):
    set transposed = []
    for i in range(len(matrix[0])):
        set new_row = []
        for row in matrix:
            new_row.append(row[i])
        transposed.append(new_row)
    return transposed

# ------------------------------
# Toy Transformer Forward
# ------------------------------
fn transformer_forward(x, W_Q, W_K, W_V):
    set Q = matmul(x, W_Q)
    set K = matmul(x, W_K)
    set V = matmul(x, W_V)
    return scaled_dot_product_attention(Q, K, V)

# ------------------------------
# Demo
# ------------------------------
# Input: 3 tokens, embedding dim 4
set x = [
    [1, 0, 1, 0],
    [0, 1, 0, 1],
    [1, 1, 0, 0]
]

# Random weight matrices for Q, K, V
set W_Q = [
    [0.1, 0.2, 0.3, 0.4],
    [0.2, 0.1, 0.0, 0.3],
    [0.0, 0.3, 0.1, 0.2],
    [0.1, 0.0, 0.2, 0.1]
]

set W_K = [
    [0.2, 0.1, 0.0, 0.3],
    [0.1, 0.3, 0.2, 0.0],
    [0.0, 0.2, 0.1, 0.3],
    [0.1, 0.0, 0.3, 0.2]
]

set W_V = [
    [0.1, 0.0, 0.2, 0.1],
    [0.0, 0.1, 0.0, 0.2],
    [0.1, 0.2, 0.1, 0.0],
    [0.0, 0.1, 0.2, 0.1]
]

set out = transformer_forward(x, W_Q, W_K, W_V)

print("Input embeddings:")
print(x)
print("Transformer output:")
print(out)
```

---

### ✅ How this works

1. **Utilities:** `dot`, `matmul`, `softmax`, `transpose` implement basic linear algebra.
2. **Attention:** `scaled_dot_product_attention` computes attention weights and weighted sum.
3. **Transformer forward:** Simple single-head attention demo.
4. **Execution:** Run in terminal with:

```bash
omnicc transformer_demo.ok
```

You’ll see input embeddings and the output matrix after applying attention.

This is a **toy demonstration**, but you can extend it with:

* Multi-head attention
* Feed-forward layers
* Tokenization / positional encoding
* Mini-batching

---

### `omnikarai_master_demo.ok`

```omnikarai
# ==================================================
# Omnikarai Master Demo: EVERYTHING in one file
# ==================================================

# ------------------------------
# Variables and Data Types
# ------------------------------
set name = "Omnikarai"
set age = 5
set height = 1.75
set is_active = true
set languages = ["Omnikarai", "Python", "C++"]
set person = {"name": "Alice", "age": 30, "active": true}
set scores = [85, 92, 78, 64]

print("Name:", name)
print("Age:", age)
print("Height:", height)
print("Languages:", languages)
print("Person:", person)
print("Scores:", scores)

# ------------------------------
# Functions
# ------------------------------
fn greet(user_name):
    print("Hello, " + user_name + "!")

fn add(x, y):
    return x + y

greet("Student")
set sum_result = add(10, 20)
print("Sum Result:", sum_result)

# Lambda style function
set multiply = fn(a, b): return a * b
print("5 * 6 =", multiply(5, 6))

# ------------------------------
# Control Flow
# ------------------------------
set score = 85

if score >= 90:
    print("Grade: A")
elif score >= 75:
    print("Grade: B")
else:
    print("Grade: C")

# Loops
print("Looping over numbers 0-4")
for i in range(5):
    print(i)

print("Looping over languages")
for lang in languages:
    print(lang)

# While loop
set counter = 0
while counter < 3:
    print("Counter:", counter)
    counter = counter + 1

# Match / pattern matching
set status_code = 404
match status_code:
    case 200:
        print("OK")
    case 404:
        print("Not Found")
    case 500..599:
        print("Server Error")
    case _:
        print("Unknown Status")

# ------------------------------
# Classes and Objects
# ------------------------------
class Animal:
    fn init(self, name, species):
        self.name = name
        self.species = species
    
    fn speak(self):
        print(self.name + " says hello!")

set dog = Animal("Buddy", "Dog")
dog.speak()

# Class inheritance
class Dog(Animal):
    fn speak(self):
        print(self.name + " barks loudly!")

set rex = Dog("Rex", "Dog")
rex.speak()

# ------------------------------
# Collections & Comprehensions
# ------------------------------
set squared = [x * x for x in range(5)]
print("Squared List:", squared)

set even_numbers = [x for x in range(10) if x % 2 == 0]
print("Even numbers:", even_numbers)

# Dict iteration
for key, value in person.items():
    print(key, "->", value)

# Nested structures
set nested = [{"name": "Alice"}, {"name": "Bob"}, {"name": "Charlie"}]
for item in nested:
    print(item["name"])

# ------------------------------
# File I/O
# ------------------------------
set filename = "omnikarai_test.txt"

# Writing to a file
try:
    set file = open(filename, "w")
    file.write("Omnikarai file I/O test.\nLine 2.")
    file.close()
except Exception as e:
    print("Error writing file:", e)

# Reading from a file
try:
    set file = open(filename, "r")
    for line in file.readlines():
        print("Read line:", line)
    file.close()
except Exception as e:
    print("Error reading file:", e)

# ------------------------------
# Regex
# ------------------------------
use re

set text = "My phone number is 123-456-7890."
set pattern = r"\d{3}-\d{3}-\d{4}"
set match = re.search(pattern, text)
if match != nil:
    print("Found phone number:", match.group(0))

# ------------------------------
# Exception Handling
# ------------------------------
try:
    set result = 10 / 0
except ZeroDivisionError as e:
    print("Caught exception:", e)

# ------------------------------
# Mini ML: Linear Regression Example (JIT-style)
# ------------------------------
set X = [0, 1, 2, 3, 4]
set Y = []

# Generate Y = 2*X + 1
for x in X:
    Y.append(2 * x + 1)

fn predict(x):
    return 2 * x + 1

print("Predictions:")
for x in X:
    print("x:", x, "y_pred:", predict(x))

# ------------------------------
# Module Manager Demo (Omnip)
# ------------------------------
# Assume we installed a module called math_utils using Omnip
# use math_utils
# print(math_utils.factorial(5))

# ------------------------------
# Demonstrating runtime code execution (JIT)
# ------------------------------
fn jit_demo(code):
    # Simulate JIT by evaluating a string expression
    print("Evaluating code:", code)
    return eval(code)

set result = jit_demo("5 * 7 + 2")
print("JIT Result:", result)

# ------------------------------
# Final Statement
# ------------------------------
print("Omnikarai Master Demo complete! All major Python-equivalent features demonstrated.")
```

---

### ✅ Features Demonstrated

1. Variables: numbers, strings, bool, list, dict
2. Functions: normal & lambda (`fn`)
3. Control Flow: `if-elif-else`, `for`, `while`, `match`
4. Classes & inheritance
5. Collections & comprehensions
6. File I/O (`open`, `read`, `write`)
7. Regex (`re.search`)
8. Exceptions (`try-except`)
9. Mini ML (linear regression prediction)
10. Module manager usage (`omnip`)
11. JIT-style evaluation (`eval` simulation)

---

**Run it:**

```bash
omnicc omnikarai_master_demo.ok
```

This is basically **Python 3 condensed into one Omnikarai file** with everything covered.

---


## 1. Module Packaging

Suppose you have a module you want to install locally:

```
my_module/
├── my_module.ok          # The actual module
├── omnikarai.toml        # Metadata (like setup.py or pyproject.toml)
└── README.md
```

**Example `omnikarai.toml`:**

```toml
[metadata]
name = "my_module"
version = "1.0.0"
author = "Akik Forazi"
description = "A demonstration module for Omnikarai"
license = "MIT"

[dependencies]
# other Omnikarai modules your module needs
```

* `omnikarai.toml` is **mandatory** for Omnip to register the module.
* You can include dependencies just like Python’s `install_requires`.

---

## 2. Local Installation (like `pip install .`)

From the module root:

```bash
omnip install .
```

**What happens internally:**

1. Omnip reads `omnikarai.toml`.
2. Copies your `.ok` file to **the global module directory**, e.g.:

```
~/.omnikarai/modules/my_module.ok
```

3. Registers the module in **Omnip registry** (like a local database):

```
~/.omnip/installed_modules.json
```

4. After that, any Omnikarai program can do:

```omnikarai
use my_module
```

---

## 3. Remote Installation (like PyPI)

If you have a remote repository (Omnikarai Package Index, OPi):

```bash
omnip install my_module
```

* Omnip looks in the OPi index (like PyPI).
* Downloads the module `.ok` and its metadata.
* Registers it globally.

Directory layout stays the same:

```
~/.omnikarai/modules/my_module.ok
```

---

## 4. Uninstalling

```bash
omnip uninstall my_module
```

* Removes the `.ok` file.
* Cleans the registry.

---

## 5. Publishing a Module

To publish to OPi:

```bash
omnip publish .
```

* Omnip reads `omnikarai.toml`.
* Packages the module (`.ok` + metadata).
* Uploads it to OPi.

---

## 6. Using Modules After Installation

```omnikarai
use my_module      # Access everything in the module
use my_module as mm
print(mm.some_function())
```

---

### TL;DR Comparison to Python

| Python                        | Omnikarai                     |
| ----------------------------- | ----------------------------- |
| `setup.py` / `pyproject.toml` | `omnikarai.toml`              |
| `pip install .`               | `omnip install .`             |
| `pip uninstall`               | `omnip uninstall`             |
| PyPI                          | OPi (Omnikarai Package Index) |

---

