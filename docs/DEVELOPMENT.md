# Omnikarai Compiler Development Log

This document details the development journey of the Omnikarai compiler, covering key design decisions, implementation challenges, and ongoing maintenance considerations. It's written from the perspective of a human developer navigating the complexities of building a language toolchain.

## 1. Project Conception & Setup

The Omnikarai project began with the goal of creating a simple, indentation-sensitive language with a focus on clear syntax. Initial setup involved:
- **Tooling:** Standard C development environment (GCC, Makefiles for build automation, a text editor/IDE like VS Code).
- **Core Components:** Recognizing the need for a Lexer (tokenization) and a Parser (syntax tree construction) as fundamental steps.
- **Language Design:** Early drafts of the language specification, focusing on Python-like indentation and basic control flow.

## 2. Lexer Development (`lexer.c`, `lexer.h`)

The lexer's primary role is to convert the source code into a stream of tokens. This phase presented several interesting challenges:

### 2.1. Basic Tokenization
Implementing functions like `read_char`, `peek_char`, `read_identifier`, `read_number`, and `read_string` was straightforward. The `new_token` helper simplified token creation. Keyword lookup (`lookup_ident`) was handled efficiently with a series of `strcmp` calls.

### 2.2. Whitespace and Comments
- **Inline Whitespace:** `skip_inline_whitespace` handles spaces and tabs within a line.
- **Single-line Comments:** Comments starting with `#` were designed to be skipped until the end of the line.
- **Multiline Comments:** A decision was made to use `#| ... |#` for multiline comments. This required special handling to ensure correct skipping, including nested comment awareness (though the current implementation is simple and might not support arbitrary nesting).

### 2.3. Indentation-Sensitive Parsing (The Pythonic Challenge)
This was arguably the most complex part of the lexer. To support indentation-based blocks, the lexer needed to:
- **Track Indentation Levels:** A stack (`indent_stack`) was implemented to keep track of the current and previous indentation levels.
- **Emit `INDENT` and `DEDENT` Tokens:** The `handle_indentation` function was crucial. It runs at the beginning of each line (`at_bol`) to calculate the new indentation, compare it to the current level, and emit `TOKEN_INDENT` or `TOKEN_DEDENT` tokens as necessary.
- **Error Handling for Inconsistent Indent:** Detecting and reporting `IndentationError` for inconsistent dedents was critical for a robust lexer.
- **Pending Tokens:** The `pending_tokens` array was introduced to allow `handle_indentation` to push `INDENT`/`DEDENT` tokens *before* the actual code tokens on a line, ensuring they are processed in the correct order by the parser.

### 2.4. Debugging the Lexer
Extensive `printf` debugging statements (which were later removed for cleaner production code) were used throughout the lexer functions to trace `ch` (current character), `position`, `readPosition`, `line_num`, `at_bol`, and the emitted tokens. The `handle_indentation` function, in particular, required meticulous tracing of `indent_stack` and `pending_count` to ensure correct `INDENT`/`DEDENT` emission.

## 3. Parser Development (`parser.c`, `parser.h`)

The parser takes the token stream from the lexer and builds an Abstract Syntax Tree (AST).

### 3.1. Pratt Parser Implementation
A Pratt parser (also known as a Top-Down Operator Precedence parser) was chosen for its flexibility and ability to handle operator precedence and associativity efficiently.
- **`prefix_parse_fn` and `infix_parse_fn`:** Two main function types were defined to handle expressions based on whether an operator is a prefix (unary) or infix (binary) operator.
- **Precedence Table:** The `precedences` array maps token types to their respective precedence levels, which is central to the Pratt parser's operation.
- **`parse_expression`:** This core function recursively parses expressions, respecting operator precedence.

### 3.2. Statement Parsing
Various `parse_*_statement` functions were implemented for language constructs like `set`, `if`/`elif`/`else`, `fn` (function definitions), `while`, `for`, `class`, `match`, and `return`.
- **Block Statements:** `parse_block_statement` handles code blocks, typically following a colon and an `INDENT` token, continuing until a `DEDENT`.

### 3.3. Expression Parsing
Functions like `parse_identifier`, `parse_integer_literal`, `parse_boolean`, `parse_string_literal`, `parse_grouped_expression`, `parse_call_expression`, and `parse_prefix_expression`/`parse_infix_expression` cover the various types of expressions.

### 3.4. The Semicolon Operator (An Interesting Edge Case)
Initially, `TOKEN_SEMICOLON` was handled as an infix operator, primarily to consume it in the token stream without complex statement-ending logic. The `parse_semicolon_operator` function was a simple pass-through: it accepted the `left` expression and returned it, effectively ignoring the semicolon's presence in the AST for now. This was a pragmatic choice for rapid development.
- **Type Compatibility:** A minor hurdle was ensuring `parse_semicolon_operator`'s signature matched the `infix_parse_fn` type. Initially, an attempt was made to remove the `Parser* p` parameter (as it was unused in `parse_semicolon_operator` itself), which led to a compiler warning about incompatible function types. The decision was made to reintroduce the `Parser* p` parameter and explicitly cast it to `void` within the function (`(void)p;`) to suppress the "unused parameter" warning, prioritizing type compatibility and avoiding the more intrusive changes that would be required to redefine the `infix_parse_fn` contract.

### 3.5. Debugging the Parser
Debugging the parser often involved:
- **Token Stream Inspection:** Verifying that the lexer was emitting the correct sequence of tokens.
- **AST Node Inspection:** After parsing, examining the structure of the generated AST to ensure it accurately represented the source code. This typically involved custom `print_ast` functions (not included in the provided code) or using a debugger to step through the parsing logic.
- **Error Tracing:** Following the `parser_add_error` calls to understand where parsing failures occurred.

## 4. Error Handling & Memory Management

- **Error Reporting:** A simple error reporting mechanism (`parser_add_error`) stores error messages in a dynamically allocated array.
- **Memory Allocation:** Extensive use of `malloc`, `realloc`, and `free` for AST nodes, token literals, and parser/lexer structures. Careful attention to freeing memory is crucial to prevent leaks, especially for complex AST structures which require recursive freeing.

## 5. Build Process (`Makefile`)

A simple `Makefile` automates the compilation process, linking `lexer.c`, `parser.c`, and `main.c` into the `bin/omnicc` executable. It includes standard GCC flags for warnings (`-Wall -Wextra`) and C99 standard compliance (`-std=c99`).

## 6. Testing

The `test.ok` files (e.g., `test.ok`, `test_v4.ok`, `test_advanced.ok`, `test_kitchen_sink.ok`) serve as integration tests for the lexer and parser. They contain Omnikarai source code snippets that the compiler should be able to process without syntax errors, and eventually, interpret or compile correctly. Running the `omnicc` executable with these files helps validate the parsing logic.

## 7. Future Work / Known Limitations

- **Full Semantic Analysis:** The current compiler focuses on lexical analysis and syntactic parsing. A future phase would involve semantic analysis (type checking, variable resolution, etc.).
- **Code Generation/Interpretation:** After semantic analysis, the next step would be to either interpret the AST directly or generate bytecode/machine code.
- **Improved Error Recovery:** The parser's error recovery is currently basic. More sophisticated error recovery mechanisms would improve the user experience for developers writing Omnikarai code.
- **AST Node Cleanup:** While some `free` calls are present, a comprehensive AST freeing mechanism is essential to prevent memory leaks in a long-running compiler or interpreter.
- **Comprehensive Test Suite:** Expanding the test suite with more unit tests for individual parser/lexer functions and a wider range of language features.
- **Enhanced Multiline Comment Handling:** Ensure `#| ... |#` fully supports arbitrary nesting if that's a language requirement.

This document serves as a living record of the Omnikarai compiler's development, highlighting the engineering decisions and practical considerations involved.