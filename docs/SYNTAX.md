# Omnikarai Language Syntax Specification

This document outlines the proposed syntax for the Omnikarai programming language. The design goals are to create a language that is easy to read and write, like Python, but with distinct features and a clear, explicit structure.

## 1. Comments

Single-line comments begin with the `#` symbol.

```omnikarai
# This is a single-line comment.
let x = 5; # This is an inline comment.
```

## 2. Variable Declaration

Variables are declared using the `let` keyword. Statements are terminated by a semicolon `;`. Omnikarai uses type inference by default but supports optional static type hints for clarity and potential performance optimizations.

```omnikarai
let score = 100;                 // Type inferred as integer
let pi: float = 3.14;            // Explicitly typed as float
let name: string = "Omnikarai";  // Explicitly typed as string
let is_active: bool = true;      // Explicitly typed as boolean
```

## 3. Modules (Imports)

Modules are brought into scope using the `use` keyword, which is a simple substitute for Python's `import`.

```omnikarai
use os;
use network.http;
```

## 4. Function Definition

Functions are defined with the `fn` keyword. Arguments can have optional type hints. The function body is enclosed in curly braces `{}`. The `return` keyword exits a function with an optional return value.

```omnikarai
// Function with no arguments
fn say_hello() {
  print("Hello, World!");
}

// Function with typed arguments and a return value
fn add(x: int, y: int) {
  return x + y;
}

// Usage
let sum = add(10, 20);
```

## 5. Control Flow

### If-Else Statements

Conditional logic uses `if`, `else if`, and `else`. Parentheses around the condition are not required.

```omnikarai
let temperature = 25;

if temperature > 30 {
  print("It's hot.");
} else if temperature < 10 {
  print("It's cold.");
} else {
  print("It's pleasant.");
}
```

### While Loops

The `while` loop executes a block of code as long as a condition is true.

```omnikarai
let counter = 0;
while counter < 5 {
  print(counter);
  counter = counter + 1;
}
```

### For Loops

Omnikarai uses a `for-in` construct for iteration over sequences, such as ranges or arrays.

```omnikarai
// Loop over a range (exclusive of the end value)
for i in 0..5 { // Will print 0, 1, 2, 3, 4
  print(i);
}

// Loop over an array
let fruits = ["apple", "banana", "cherry"];
for fruit in fruits {
  print(fruit);
}
```

## 6. Data Types & Literals

- **Numbers:** `10`, `99.9`
- **Strings:** `"Hello"`, `'World'` (both single and double quotes are valid)
- **Booleans:** `true`, `false`
- **Nil:** `nil` (represents the absence of a value, similar to Python's `None`)
- **Arrays (Lists):** A collection of values.
  `let my_array = [1, "a", true, 3.14];`
- **Maps (Dictionaries):** A collection of key-value pairs.
  `let my_map = {"name": "Alice", "age": 30};`
