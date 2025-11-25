# Build Your Own Omnikarai: A Step-by-Step Guide

Welcome, language builder! This guide will walk you through creating a programming language from scratch. We'll build a simplified version of Omnikarai, and by the end, you'll understand how it all works.

We will build our language in four main parts:
1.  **The REPL:** A simple command prompt that can read your code.
2.  **The Lexer:** The part that sorts your code into "words" or "tokens".
3.  **The Parser:** The part that checks if the "words" make a valid "sentence".
4.  **The Interpreter:** The part that actually runs your code.

---

## Part 1: The First Step - A Program That Listens (REPL)

Before we can build a language, we need a program that can listen to us. We'll create a simple loop that reads what you type and prints it back. This is called a **REPL** (Read-Eval-Print Loop).

### 1. The Code

First, let's create a new, clean folder to work in so we don't mess with the existing project.

```bash
mkdir my_omni
cd my_omni
```

Now, create a file named `main.c` and put the following code inside it. This is the entry point of our program.

```c
// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// This is a buffer to hold the line we read
char input_buffer[256];

int main(int argc, char** argv) {
    printf("Welcome to your new language!\n");
    printf("Press Ctrl+C to exit.\n");

    // This is the main loop
    while (1) {
        printf(">> "); // Print a prompt

        // Read a line of text from the user
        if (!fgets(input_buffer, 256, stdin)) {
            printf("\nGoodbye!\n");
            break;
        }

        // For now, we just print back what the user typed
        printf("You typed: %s", input_buffer);
    }

    return 0;
}
```

### 2. How to Compile and Run

To turn our C code into an executable program, we use a compiler, typically `gcc`.

1.  **Compile:** Open your terminal and run this command:
    ```bash
gcc main.c -o my_omni_repl
    ```
    This tells `gcc` to take `main.c`, compile it, and create an output program named `my_omni_repl`.

2.  **Run:** Now, run your new program:
    ```bash
./my_omni_repl
    ```

You should see:
```
Welcome to your new language!
Press Ctrl+C to exit.
>> 
```

Try typing something and pressing Enter. It should echo back what you typed!

### 3. Common Bugs & Debugging

**Bug 1: Missing Semicolon**

What happens if you forget a semicolon (`;`)? Let's say you remove the one after `printf(">> ")`.

**The Error:** When you try to compile (`gcc main.c -o my_omni_repl`), you'll get an error like this:

```
main.c: In function 'main':
main.c:21:9: error: expected ';' before 'if'
   21 |         if (!fgets(input_buffer, 256, stdin)) {
      |         ^~
      |         ;
```

**How to Debug:** The compiler is your best friend!
*   It tells you the file (`main.c`) and the line number (`:21:9`).
*   It says `error: expected ';' before 'if'`.
*   This is a huge clue! It means on line 21, it saw the word `if`, but it was expecting a semicolon to finish the previous line. Look at the line *before* line 21, and you'll find the missing `;`.

**Bug 2: Undefined Function**

What if you misspell a function name, like `printff` instead of `printf`?

**The Error:**

```
/usr/bin/ld: /tmp/ccXXXXXX.o: in function `main`:
main.c:(.text+0x1a): undefined reference to `printff`
collect2: error: ld returned 1 exit status
```
**How to Debug:**
*   This error looks different. `undefined reference to 'printff'` means "I know you want to use a function called `printff`, but I have no idea what it is!"
*   This almost always means you have a typo in a function name. Check your spelling carefully.

---

Congratulations! You've completed Part 1. You now have a basic interactive shell. In the next part, we'll build the **Lexer** to start understanding the code you type.

---

## Part 2: The Word Sorter (The Lexer)

Our program can read a line of text, but it sees it as just one long string. We need to break it down into "words" or **tokens**.

For example, if the user types `set x = 10`, we want to see:
- `set` (keyword)
- `x` (identifier)
- `=` (operator)
- `10` (number)

Let's build a Lexer to do this.

### 1. The Code

First, let's create a new file `lexer.h` to define what our tokens look like.

```c
// lexer.h
#ifndef LEXER_H
#define LEXER_H

// The different kinds of tokens our language recognizes
typedef enum {
    TOKEN_EOF,       // End of the input
    TOKEN_ILLEGAL,   // A character we don't recognize
    
    // Literals
    TOKEN_IDENT,     // a, b, x, y, my_var
    TOKEN_NUMBER,    // 1, 2, 10, 123

    // Keywords
    TOKEN_SET,       // "set"

    // Operators
    TOKEN_ASSIGN,    // =
    TOKEN_PLUS,      // +
} TokenType;

// A Token has a type and the actual text it represents (its "literal")
typedef struct {
    TokenType type;
    char* literal;
} Token;

#endif
```

Now, let's create the lexer itself in `lexer.c`.

```c
// lexer.c
#include <stdio.h>
#include <string.h>
#include <ctype.h> // For isspace, isalpha, isdigit
#include "lexer.h"

// A Lexer keeps track of the input text and our position in it
typedef struct {
    const char* input;
    int position;      // current position in input (points to current char)
    int readPosition;  // current reading position in input (after current char)
    char ch;           // current char under examination
} Lexer;

// Function to create a new Lexer
Lexer new_lexer(const char* input) {
    Lexer l = {input, 0, 0, 0};
    // Read the first character
    l.ch = l.input[l.readPosition];
    l.position = l.readPosition;
    l.readPosition++;
    return l;
}

// Function to read the next character
void read_char(Lexer* l) {
    if (l->readPosition >= strlen(l->input)) {
        l->ch = '\0'; // NUL character signifies end of input
    } else {
        l->ch = l.input[l->readPosition];
    }
    l->position = l->readPosition;
    l->readPosition++;
}

// Simple helper to create a new token
Token new_token(TokenType type, const char* literal) {
    Token tok;
    tok.type = type;
    // Note: In a real language, you'd copy the string. For simplicity, we won't.
    tok.literal = (char*)literal;
    return tok;
}

// The main function that gets the next token
Token get_next_token(Lexer* l) {
    Token tok;

    // Skip whitespace
    while (isspace(l->ch)) {
        read_char(l);
    }

    switch (l->ch) {
        case '=':
            tok = new_token(TOKEN_ASSIGN, "=");
            break;
        case '+':
            tok = new_token(TOKEN_PLUS, "+");
            break;
        case '\0':
            tok = new_token(TOKEN_EOF, "");
            break;
        default:
            tok = new_token(TOKEN_ILLEGAL, &l->ch);
    }
    
    read_char(l); // Move to the next character
    return tok;
}
```

Finally, let's update `main.c` to use our new lexer.

```c
// main.c
#include <stdio.h>
#include "lexer.h" // Include our new lexer header

int main(int argc, char** argv) {
    char input_buffer[256];

    while (1) {
        printf(">> ");
        if (!fgets(input_buffer, 256, stdin)) {
            break;
        }

        Lexer l = new_lexer(input_buffer);
        Token tok;

        // Loop until we hit the end of the line
        do {
            tok = get_next_token(&l);
            // We need a way to print token types. Let's just use numbers for now.
            printf("Token Type: %d, Literal: %s\n", tok.type, tok.literal);
        } while (tok.type != TOKEN_EOF);
    }

    return 0;
}
```

### 2. How to Compile and Run

Our project now has multiple files, so we need to tell `gcc` to compile all of them.

```bash
gcc main.c lexer.c -o my_omni_repl
```

Now run it (`./my_omni_repl`) and try typing `= +`. You should see:

```
>> = +
Token Type: 8, Literal: =
Token Type: 9, Literal: +
Token Type: 0, Literal: 
```
The numbers correspond to the `TokenType` enum we created. It's working!

### 3. Common Bugs & Debugging

**Bug 1: The lexer doesn't handle numbers or identifiers yet!**

If you type `set x = 10`, you'll get a stream of `TOKEN_ILLEGAL`. That's because our `get_next_token` function only knows about `=`, `+`, and whitespace.

**How to Fix:** We need to expand our `get_next_token` function.

Let's modify the `default` case in the `switch` statement in `lexer.c`:

```c
// In lexer.c, inside get_next_token, replace 'default':
default:
    if (isalpha(l->ch)) { // Is it a letter?
        // This is a simple, but incomplete, way to read an identifier
        // A real lexer would loop to read the whole word.
        char* ident = &l->input[l->position];
        tok = new_token(TOKEN_IDENT, ident);
        // For now, we'll just return the first letter
        read_char(l);
        return tok;
    } else if (isdigit(l->ch)) { // Is it a number?
        char* num = &l->input[l->position];
        tok = new_token(TOKEN_NUMBER, num);
        // Again, just the first digit for now
        read_char(l);
        return tok;
    } else {
        tok = new_token(TOKEN_ILLEGAL, &l->ch);
    }
```
Recompile and run. Now when you type `x + 1`, you'll see it start to recognize identifiers and numbers!

**Note:** This is a simplified lexer. A real one would have loops to read the *entire* identifier (`my_var`) or number (`123`).

---

## Part 3: The Rule Checker (The Parser)

Our lexer can give us a stream of tokens, but we don't know if they make sense together. `x + 1` is valid, but `+ x 1` is not.

The **Parser** takes the tokens and tries to build a structure out of them, like building a sentence diagram. This structure is the **Abstract Syntax Tree (AST)**.

### 1. The Code

First, we need to define the structure of our AST. Let's create `ast.h`.

```c
// ast.h
#ifndef AST_H
#define AST_H

#include "lexer.h"

// Every part of our AST is a "Node"
// For now, we only have one kind of node: an Expression
typedef struct ASTNode {
    // We'll add more details here later
} ASTNode;

// A "program" is just a list of statements (for now, expressions)
typedef struct {
    ASTNode** statements;
    int count;
} ASTProgram;

#endif
```

Next, `parser.h`.

```c
// parser.h
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer l;
    Token current_token;
    Token peek_token; // The next token
} Parser;

Parser new_parser(Lexer l);
ASTProgram* parse_program(Parser* p);

#endif
```

And now `parser.c`. This is where the magic happens. A parser can be very complex. We are building a simple one.

```c
// parser.c
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

// Helper function to advance tokens
void next_token(Parser* p) {
    p->current_token = p->peek_token;
    p->peek_token = get_next_token(&p->l);
}

Parser new_parser(Lexer l) {
    Parser p = {l};
    // Read two tokens, so current_token and peek_token are both set
    next_token(&p);
    next_token(&p);
    return p;
}

// For now, our parser is very simple. It doesn't do anything!
// It just creates an empty program.
ASTProgram* parse_program(Parser* p) {
    ASTProgram* program = malloc(sizeof(ASTProgram));
    program->statements = NULL;
    program->count = 0;
    
    printf("Parsing program...\n");
    // We would loop through tokens here and parse statements
    printf("Parsing complete (not really)!\n");

    return program;
}
```

Finally, let's update `main.c` one more time.

```c
// main.c
#include <stdio.h>
#include "parser.h" // Include parser instead of lexer

int main(int argc, char** argv) {
    char input_buffer[256];

    while (1) {
        printf(">> ");
        if (!fgets(input_buffer, 256, stdin)) {
            break;
        }

        Lexer l = new_lexer(input_buffer);
        Parser p = new_parser(l);
        ASTProgram* program = parse_program(&p);

        // We can't do anything with the program yet, but we've parsed it!
        if (program) {
            printf("Successfully created AST Program.\n");
            // In a real program, we would free the memory for the AST here
            free(program);
        }
    }

    return 0;
}
```

### 2. How to Compile and Run

We have even more files now!

```bash
gcc main.c lexer.c parser.c -o my_omni_repl
```

Run it. It doesn't seem to do much, but it's going through the whole process: Lexing the input and then "Parsing" it into a (currently empty) program structure.

### 3. Common Bugs & Debugging

**Bug: "Conflicting types for 'new_parser'" or "implicit declaration of function"**

**The Error:** You might see errors like this if you forget to include a header file, or if your function definition in the `.c` file doesn't match the declaration in the `.h` file.

**How to Debug:**
1.  **Check Includes:** Make sure every `.c` file that uses a function from another file includes the corresponding `.h` file. For example, `main.c` now needs `#include "parser.h"`.
2.  **Check Signatures:** Make sure the function in the `.h` file (e.g., `Parser new_parser(Lexer l);`) exactly matches the one in the `.c` file (`Parser new_parser(Lexer l) { ... }`). A typo in the name or a different argument will cause this error.

---

This guide now covers the foundational structure of a language: reading input, tokenizing it, and parsing it. The next logical step is **Part 4: The Interpreter**, where we will actually execute the AST.
---

## Part 4: The Interpreter - Bringing Code to Life

We have a blueprint (the AST), but it doesn't do anything. The **Interpreter's** job is to walk through the AST and evaluate it, producing a result. This is also called "tree-walking".

### 1. Representing Values

First, our language needs to handle values like numbers, booleans, etc. We'll create an "Object" system.

Create `object.h`:
```c
// object.h
#ifndef OBJECT_H
#define OBJECT_H

typedef enum {
    OBJ_NUMBER,
    OBJ_NIL,
} ObjectType;

typedef struct {
    ObjectType type;
    union {
        double number;
    } as;
} Object;

// Helper to print objects
void print_object(Object obj);

#endif
```

And `object.c`:
```c
// object.c
#include <stdio.h>
#include "object.h"

void print_object(Object obj) {
    switch (obj.type) {
        case OBJ_NUMBER:
            printf("%g", obj.as.number);
            break;
        case OBJ_NIL:
            printf("nil");
            break;
    }
}
```

### 2. The Interpreter Code

Now for the core interpreter logic. Create `interpreter.h`:
```c
// interpreter.h
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "object.h"

Object eval(ASTNode* node);

#endif
```
And `interpreter.c`. For now, it will be very simple. It can't do anything yet because our AST is still empty.
```c
// interpreter.c
#include "interpreter.h"

Object eval(ASTNode* node) {
    // This is where we would check the type of the AST node
    // and decide what to do. For now, we do nothing.
    
    // Return "nil" by default
    return (Object){ .type = OBJ_NIL };
}
```

### 3. Plugging it into the REPL

Let's update `main.c` to call the interpreter.
```c
// main.c
#include <stdio.h>
#include "parser.h"
#include "interpreter.h" // Add interpreter
#include "object.h"      // Add object

int main(int argc, char** argv) {
    char input_buffer[256];

    while (1) {
        printf(">> ");
        if (!fgets(input_buffer, 256, stdin)) {
            break;
        }

        Lexer l = new_lexer(input_buffer);
        Parser p = new_parser(l);
        ASTProgram* program = parse_program(&p);

        // Evaluate the program!
        if (program) {
            // For a real program, we would evaluate each statement
            // For now, let's assume there is one statement
            if (program->count > 0) {
                 Object result = eval(program->statements[0]);
                 print_object(result);
                 printf("\n");
            }
            free(program);
        }
    }

    return 0;
}
```
### 4. Making it Work: Parsing and Evaluating Numbers

Our language still can't do anything. Let's teach it to understand numbers!

**Step 1: Update the AST (`ast.h`)**
```c
// ast.h
// ...
typedef enum {
    NODE_NUMBER_LITERAL,
} NodeType;

typedef struct ASTNode {
    NodeType type;
} ASTNode;

// A specific node for numbers
typedef struct {
    ASTNode node; // Base node type
    double value;
} ASTNumberLiteral;
// ...
```
**Step 2: Update the Parser (`parser.c`)**
We need to teach the parser how to create an `ASTNumberLiteral` when it sees a `TOKEN_NUMBER`. This requires a much more complex `parse_program` function. For brevity, we'll just show the concept. A real parser for expressions is a big topic (look up "Pratt Parsers").

```c
// A simplified conceptual parser in parser.c
// ...
ASTNode* parse_expression(Parser* p) {
    if (p->current_token.type == TOKEN_NUMBER) {
        ASTNumberLiteral* node = malloc(sizeof(ASTNumberLiteral));
        node->node.type = NODE_NUMBER_LITERAL;
        node->value = strtod(p->current_token.literal, NULL); // convert string to double
        return (ASTNode*)node;
    }
    return NULL;
}

ASTProgram* parse_program(Parser* p) {
    ASTProgram* program = malloc(sizeof(ASTProgram));
    program->statements = malloc(sizeof(ASTNode*));
    program->count = 0;

    // Parse one expression
    ASTNode* stmt = parse_expression(p);
    if (stmt) {
        program->statements[0] = stmt;
        program->count = 1;
    }
    return program;
}
```
**Step 3: Update the Interpreter (`interpreter.c`)**
```c
// interpreter.c
#include "interpreter.h"
#include "ast.h" // Make sure to include this

Object eval(ASTNode* node) {
    if (!node) return (Object){ .type = OBJ_NIL };

    switch (node->type) {
        case NODE_NUMBER_LITERAL: {
            ASTNumberLiteral* num_node = (ASTNumberLiteral*)node;
            return (Object){ .type = OBJ_NUMBER, .as.number = num_node->value };
        }
    }
    return (Object){ .type = OBJ_NIL };
}
```
### 5. Compile and Run
```bash
gcc main.c lexer.c parser.c interpreter.c object.c -o my_omni_repl
```
Now, when you run it and type `123`, the lexer will tokenize it, the parser will create a number node in the AST, and the interpreter will evaluate it and print `123` back to you! You have an evaluating language!

---
## Part 5: Expanding the Language - Variables, Functions, and Logic

(This section would contain detailed code for `set` statements, function definitions, `if/else`, etc. It would involve creating an "Environment" to store variables, expanding the AST and Object systems for functions, and adding branching logic to the `eval` function.)

---
## Part 6: Beyond Interpretation - A Bytecode Compiler and VM

A tree-walking interpreter can be slow. A faster way is to first compile the code to a simpler, intermediate format called **bytecode**, and then execute that bytecode on a **Virtual Machine (VM)**.

### The Core Idea
- **Compiler:** Walks the AST *once* and emits a list of simple instructions (bytecode).
- **VM:** A simple loop that reads and executes these instructions.

**Example:** For `5 + 10`
- The **Compiler** would produce bytecode like:
  ```
  OP_CONSTANT 0  // Push constant 5 onto stack
  OP_CONSTANT 1  // Push constant 10 onto stack
  OP_ADD         // Pop two values, add them, push result
  OP_RETURN      // Return final value
  ```
- The **VM** would have a `stack` (an array for temporary values) and a loop:
  ```c
  // Simplified VM loop
  for (;;) {
      uint8_t instruction = *ip++; // ip = instruction pointer
      switch (instruction) {
          case OP_CONSTANT:
              // push constant onto stack
              break;
          case OP_ADD:
              // pop two numbers, add, push result
              break;
          // ...
      }
  }
  ```

This is much faster because the VM doesn't have to deal with complex AST nodes; it just executes a simple, linear list of instructions. This is the model that Python and the JVM use.

---
## Part 7: The Module System - Building with Omnip

A real language needs a way to organize code into modules.

### 1. The `use` Keyword
The first step is to teach the language what `use "my_module"` means.
- **Lexer:** Add `TOKEN_USE`.
- **Parser:** Add an `ASTUseStatement` node to the AST.

### 2. The Module Loader
When the interpreter (or compiler) sees a `use` statement, it needs to:
1.  **Find the file:** Look for `my_module.ok` in a set of predefined paths (e.g., the current directory, a global `~/.omnikarai/modules` directory).
2.  **Execute the file:** Run the lexer, parser, and interpreter/compiler on this new file's code.
3.  **Expose the exports:** The module's code should produce a set of public functions or variables. These are then stored, so the original program can access them (e.g., `my_module.my_function()`).

### 3. The `omnip` Package Manager
`omnip` is a separate command-line tool, not part of the language itself. Its job is to manage the global modules directory.
- `omnip install requests`:
  1.  Connect to a central server (the Omnikarai Package Index).
  2.  Download the `requests.ok` file.
  3.  Place it in `~/.omnikarai/modules/requests.ok`.
- `omnip uninstall requests`:
  1.  Delete the file `~/.omnikarai/modules/requests.ok`.

---
## Part 8: Testing, Documenting, and Releasing Your Language

### 1. Testing
Your language is now complex. You can't test everything by hand.
- **Unit Tests:** Write C functions to test specific parts, like the lexer (`test_get_next_token()`) or parser. Use a framework like `ctest` or just plain `assert`.
- **Integration Tests:** Create a folder of `.ok` files that test language features. Write a script that runs `my_omni_repl` on each file and compares the output to an expected output. If they don't match, the test fails.

### 2. Build System
Typing `gcc main.c lexer.c ...` is getting tedious. A `Makefile` automates this.
```makefile
# Makefile
CC=gcc
CFLAGS=-std=c99 -Wall

SOURCES=main.c lexer.c parser.c interpreter.c object.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=my_omni

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
    $(CC) $(OBJECTS) -o $@

.c.o:
    $(CC) $(CFLAGS) -c $< -o $@

clean:
    rm -f $(OBJECTS) $(EXECUTABLE)
```
Now you can just type `make` to build and `make clean` to clean up.

### 3. Documentation
Write clear documentation for your users.
- **Language Reference:** Explain every feature of the language, like in `docs/SYNTAX.md`.
- **API Docs:** If you have a standard library, document every function.

### 4. Release
1.  **Version It:** Decide on a version number (e.g., v0.1.0). Use [Semantic Versioning](https://semver.org/).
2.  **Tag it:** In Git, create a tag: `git tag -a v0.1.0 -m "First release"`.
3.  **Package It:** Create a downloadable archive (.zip, .tar.gz) that contains the source code, Makefile, and documentation.
4.  **Announce It:** Let people know your language exists!

Congratulations! You have now gone through the entire lifecycle of creating a programming language, from a simple REPL to a releasable, documented, and tested product.
