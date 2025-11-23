# Omnikarai Language Syntax Specification (v4)

## Guiding Principles

Omnikarai is a dynamically-typed, high-level programming language. Its design philosophy emphasizes code readability and simplicity, creating a unique syntax that feels familiar and productive.

1.  **Readability:** A clean, intuitive syntax that prioritizes clarity.
2.  **Expressiveness:** Provide powerful, modern language features in an approachable way.
3.  **Performance:** While the syntax is high-level, the underlying implementation aims for a runtime that is significantly faster than standard Python.

---

## 1. The Basics

### 1.1. Comments

-   Single-line comments start with `#`.
-   Multi-line (block) comments are enclosed in `#|` and `|#`.

```omnikarai
# This is a single-line comment.

#|
This is a multi-line, or block, comment.
It can span multiple lines.
|#
```

### 1.2. Variable Declaration and Assignment

Omnikarai uses the `set` keyword for the initial declaration of a variable. Subsequent re-assignment uses the standard `=` operator.

```omnikarai
set score = 100          # Variable declaration
score = 110              # Re-assignment

set name = "Omnikarai"
set is_active = true
```

### 1.3. Data Types

As a dynamically typed language, types are inferred at runtime. The primitive types are:

-   **Numbers:** Integers (`10`, `42`) and Floats (`3.14`).
-   **Strings:** Defined with single (`'...'`) or double (`"..."`) quotes.
-   **Booleans:** `true` and `false`.
-   **Nil:** `nil` (represents the absence of a value).

### 1.4. Code Blocks

Code blocks (for functions, classes, and control flow) are defined by **indentation**. A colon (`:`) is used to start an indented block.

---

## 2. Control Flow

### 2.1. If-Elif-Else Statements

Conditional logic uses standard keywords.

```omnikarai
set temperature = 25

if temperature > 30:
  print("It's hot.")
elif temperature < 10:
  print("It's cold.")
else:
  print("It's pleasant.")
```

### 2.2. Match Expressions

For more complex conditional logic, `match` provides powerful and readable pattern matching.

```omnikarai
set status_code = 200

match status_code:
    case 200:
        print("OK")
    case 404:
        print("Not Found")
    case 500..599:
        print("Server Error")
    case _:
        print("Unknown Status") # `_` is the default wildcard case
```

### 2.3. Loops

```omnikarai
# While loop
set counter = 0
while counter < 5:
  print(counter)
  counter = counter + 1

# For loop
set fruits = ["apple", "banana", "cherry"]
for fruit in fruits:
    print(fruit)
```

---

## 3. Data Structures

Collections are syntactically familiar.

-   **Lists (Arrays):** `set my_list = [1, "a", true]`
-   **Dictionaries (Maps):** `set my_dict = {"name": "Alice", "age": 30}`
-   **Tuples:** `set my_tuple = (10, 20, "c")`

---

## 4. Functions

Functions are defined with the `fn` keyword.

```omnikarai
fn say_hello():
    print("Hello, World!")

fn add(x, y):
    return x + y

# Calling functions
say_hello()
set result = add(5, 10)
```

---

## 5. Classes

Classes are defined with the `class` keyword, but methods use `fn`. The constructor method is named `init`.

```omnikarai
class Person:
    fn init(self, name, age):
        self.name = name
        self.age = age

    fn greet(self):
        print("Hello, my name is " + self.name)

# Creating an instance
set p = Person("Alice", 30)
p.greet() 
# Output: Hello, my name is Alice
```

---

## 6. Modules

Omnikarai uses the `use` keyword for its module system.

```omnikarai
# Import an entire module, members accessed with dot notation.
# Corresponds to Python's `import math`
use math
print(math.sqrt(16))

# Import a specific item from a module into the current namespace.
# Corresponds to Python's `from os import path`
use os.path
print(path.join("/home", "user"))

# Import an item and rename it with `as`.
use collections.Array as MyList
set items = MyList.new()
```