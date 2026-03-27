# Build Your Own Omnikarai: A Step-by-Step Guide

This guide walks through building a language compiler from scratch — the same way Omnikarai was actually built. By the end you'll understand every layer: lexer, parser, AST, and native code generation.

---

## Part 1: A Program That Listens (REPL)

Before anything else, we need a loop that reads code and does something with it.

```c
// main.c
#include <stdio.h>
#include <stdlib.h>

char input[1024];

int main(void) {
    printf("Omnikarai REPL\nCtrl+C to exit.\n");
    while (1) {
        printf(">> ");
        if (!fgets(input, sizeof(input), stdin)) break;
        printf("You typed: %s", input);
    }
    return 0;
}
```

```bash
gcc main.c -o repl
./repl
```

**Common bugs:**
- Missing `;` → compiler says `error: expected ';' before 'X'` — look at the line *before* the error line
- Misspelled function → `undefined reference to 'printff'` — check your spelling

---

## Part 2: The Lexer (Word Sorter)

The lexer converts a string of characters into a list of tokens.

### Token types

```c
// lexer.h
typedef enum {
    TOKEN_EOF,
    TOKEN_IDENT,    // variable names, keywords
    TOKEN_INT,      // 42, 100
    TOKEN_STRING,   // "hello"
    TOKEN_SET,      // set
    TOKEN_IF,       // if
    TOKEN_WHILE,    // while
    TOKEN_RETURN,   // return
    TOKEN_ASSIGN,   // =
    TOKEN_PLUS,     // +
    TOKEN_MINUS,    // -
    TOKEN_STAR,     // *
    TOKEN_SLASH,    // /
    TOKEN_EQ,       // ==
    TOKEN_LT,       // <
    TOKEN_GT,       // >
    TOKEN_LPAREN,   // (
    TOKEN_RPAREN,   // )
    TOKEN_COLON,    // :
    TOKEN_NL,       // newline
    TOKEN_INDENT,   // indentation increased
    TOKEN_DEDENT,   // indentation decreased
    TOKEN_TRUE,     // true
    TOKEN_FALSE,    // false
} TokenType;

typedef struct {
    TokenType type;
    char*     literal;
    int       line;
} Token;
```

### Lexer struct

```c
typedef struct {
    const char* input;
    size_t      input_len;   // cached — NEVER call strlen() in a loop
    size_t      pos;
    size_t      read_pos;
    char        ch;
    int         line;
    int         at_bol;      // at beginning of line
    int         indent_stack[64];
    int         indent_top;
} Lexer;
```

**Critical:** cache `input_len` upfront. Calling `strlen()` inside `read_char()` every character is O(n²) — for a 1000-line file that's a million string scans.

### INDENT / DEDENT — the hard part

Omnikarai uses indentation for blocks. At the start of every line, compare the new indentation to the stack top:

```c
void handle_indentation(Lexer* l, int spaces) {
    int current = l->indent_stack[l->indent_top];
    if (spaces > current) {
        l->indent_stack[++l->indent_top] = spaces;
        emit_pending(l, TOKEN_INDENT);
    } else {
        while (spaces < l->indent_stack[l->indent_top]) {
            l->indent_top--;
            emit_pending(l, TOKEN_DEDENT);
        }
    }
}
```

These INDENT/DEDENT tokens replace `{` and `}` in the parser.

---

## Part 3: The Parser (Rule Checker + Tree Builder)

The parser turns the token stream into an **Abstract Syntax Tree (AST)**.

### AST node types

```c
// ast.h
typedef enum {
    SET_STATEMENT,
    IF_STATEMENT,
    WHILE_STATEMENT,
    RETURN_STATEMENT,
    EXPRESSION_STATEMENT,
    BLOCK_STATEMENT,
    FN_DEFINITION,
    INTEGER_LITERAL,
    STRING_LITERAL,
    BOOLEAN_LITERAL,
    IDENTIFIER,
    INFIX_EXPRESSION,
    PREFIX_EXPRESSION,
    CALL_EXPRESSION,
} NodeType;
```

Each node is a C struct that embeds a base type:

```c
typedef struct { NodeType type; Token token; } AST_Statement;
typedef struct { NodeType type; Token token; } AST_Expression;

typedef struct {
    AST_Statement        base;
    AST_Expression_Identifier* name;
    AST_Expression*      value;
} AST_Statement_Set;

typedef struct {
    AST_Expression       base;
    AST_Expression*      left;
    char*                operator;
    AST_Expression*      right;
} AST_Expression_Infix;
```

### Pratt parser for expressions

A Pratt parser handles operator precedence without complex rules. Every token type gets two functions:

```c
typedef AST_Expression* (*prefix_fn)(Parser* p);         // called when token starts an expr
typedef AST_Expression* (*infix_fn)(Parser* p, AST_Expression* left); // called when between exprs

// Register them:
p->prefix_fns[TOKEN_INT]   = parse_integer_literal;
p->prefix_fns[TOKEN_IDENT] = parse_identifier;
p->prefix_fns[TOKEN_MINUS] = parse_prefix_expression;
p->infix_fns[TOKEN_PLUS]   = parse_infix_expression;
p->infix_fns[TOKEN_STAR]   = parse_infix_expression;
p->infix_fns[TOKEN_LPAREN] = parse_call_expression;
```

Precedence table:
```c
typedef enum {
    PREC_LOWEST,
    PREC_EQUALS,      // == !=
    PREC_COMPARE,     // < > <= >=
    PREC_SUM,         // + -
    PREC_PRODUCT,     // * /
    PREC_PREFIX,      // -x
    PREC_CALL,        // fn()
} Precedence;
```

The main expression loop:

```c
AST_Expression* parse_expression(Parser* p, Precedence prec) {
    prefix_fn prefix = p->prefix_fns[p->cur.type];
    if (!prefix) { /* error */ return NULL; }
    AST_Expression* left = prefix(p);

    while (prec < get_precedence(p->peek.type)) {
        infix_fn infix = p->infix_fns[p->peek.type];
        if (!infix) return left;
        advance(p);
        left = infix(p, left);
    }
    return left;
}
```

### Block parsing

```c
AST_Statement_Block* parse_block(Parser* p) {
    // consume newlines after ':'
    while (peek_is(p, TOKEN_NL)) advance(p);
    expect(p, TOKEN_INDENT);
    advance(p);  // past INDENT

    // collect statements until DEDENT
    while (!cur_is(p, TOKEN_DEDENT) && !cur_is(p, TOKEN_EOF)) {
        AST_Statement* stmt = parse_statement(p);
        if (stmt) append_statement(block, stmt);
        if (!cur_is(p, TOKEN_DEDENT)) advance(p);
    }
    // currentToken is now TOKEN_DEDENT
    return block;
}
```

---

## Part 4: Native x86-64 Code Generation

This is where Omnikarai differs from most tutorials. Instead of a tree-walking interpreter or bytecode VM, we emit real x86-64 machine code directly.

### Code buffer

```c
typedef struct {
    uint8_t* data;
    size_t   size;
    size_t   capacity;
} CodeBuf;

void emit_u8(CodeBuf* b, uint8_t v) {
    if (b->size >= b->capacity) {
        b->capacity *= 2;
        b->data = realloc(b->data, b->capacity);
    }
    b->data[b->size++] = v;
}

void emit_u32(CodeBuf* b, uint32_t v) {
    emit_u8(b, v);
    emit_u8(b, v >> 8);
    emit_u8(b, v >> 16);
    emit_u8(b, v >> 24);
}
```

### Stack frame (Windows x64)

```c
// Prologue
emit_u8(b, 0x55);              // push rbp
emit_u8(b, 0x48); emit_u8(b, 0x89); emit_u8(b, 0xE5); // mov rbp, rsp
// sub rsp, N  — patched later
size_t patch = b->size;
emit_u8(b, 0x48); emit_u8(b, 0x81); emit_u8(b, 0xEC);
emit_u32(b, 0);  // placeholder

// ... emit body ...

// Epilogue
emit_u8(b, 0x48); emit_u8(b, 0x89); emit_u8(b, 0xEC); // mov rsp, rbp
emit_u8(b, 0x5D);              // pop rbp
emit_u8(b, 0xC3);              // ret
```

**Stack alignment rule (Windows x64):**
After `push rbp`, RSP is `16n - 8`. After `sub rsp, N`, RSP is `16n - 8 - N`. For RSP to be 16-aligned before any `call`, N must be `≡ 8 (mod 16)`.

```c
int align_stack(int locals) {
    int N = locals + 32;      // +32 for Windows shadow space
    N = (N + 15) & ~15;       // round up to multiple of 16
    N += 8;                    // now N % 16 == 8
    return N;
}
```

### Compile a variable: `set x = 42`

```c
// eval 42 → RAX
emit_u8(b, 0x48); emit_u8(b, 0xB8);  // mov rax, imm64
emit_u64(b, 42);

// store RAX → [rbp - 8]
emit_u8(b, 0x48); emit_u8(b, 0x89); emit_u8(b, 0x85);
emit_u32(b, (uint32_t)(-8));
```

### Compile infix: `x + y`

```asm
; eval x → rax
; mov r10, rax        ; save left
; eval y → rax
; mov rcx, r10        ; rcx = left
; xchg rax, rcx      ; rax = left, rcx = right
; add rax, rcx        ; result in rax
```

We use R10 as scratch instead of push/pop — push would change RSP and break alignment.

### Compile if statement

```c
// eval condition → rax
// test rax, rax
emit_u8(b, 0x48); emit_u8(b, 0x85); emit_u8(b, 0xC0);
// je <else_label>  — emit placeholder
emit_u8(b, 0x0F); emit_u8(b, 0x84);
size_t patch = b->size;
emit_u32(b, 0);

// ... consequence block ...

// jmp <end_label>
emit_u8(b, 0xE9);
size_t patch2 = b->size;
emit_u32(b, 0);

// patch je → here (else label)
int32_t disp = (int32_t)(b->size - (patch + 4));
memcpy(b->data + patch, &disp, 4);

// ... alternative block (else) ...

// patch jmp → here (end label)
int32_t disp2 = (int32_t)(b->size - (patch2 + 4));
memcpy(b->data + patch2, &disp2, 4);
```

### Running the code — VirtualAlloc

```c
void* mem = VirtualAlloc(NULL, code_size, MEM_COMMIT | MEM_RESERVE,
                         PAGE_EXECUTE_READWRITE);
memcpy(mem, code_bytes, code_size);
typedef int64_t (*Fn)(void);
int64_t result = ((Fn)mem)();
VirtualFree(mem, 0, MEM_RELEASE);
```

### Output — WriteFile, not printf

```c
void omni_print_int(int64_t v) {
    char buf[32];
    // manual itoa into buf
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
    WriteFile(h, buf, len, &written, NULL);
    WriteFile(h, "\n", 1, &written, NULL);
}
```

Never use `printf` from JIT code on Windows. MinGW's printf uses SEH (structured exception handling) internally. The exception dispatcher walks the call stack looking for unwind records — our JIT frame has none, causing a crash.

---

## Part 5: Putting It All Together

```c
// main.c
int main(int argc, char** argv) {
    char* source = read_file(argv[2]);

    // 1. Lex
    Lexer l;
    lexer_init(&l, source);

    // 2. Parse
    Parser* p = new_parser(&l);
    AST_Program* program = parse_program(p);

    // 3. Codegen
    CodeGen cg;
    codegen_init(&cg);
    codegen_compile(&cg, program);

    // 4. Run
    int64_t exit_code = codegen_run(&cg);
    printf("exit: %lld\n", exit_code);

    codegen_free(&cg);
    return (int)exit_code;
}
```

```bash
gcc -Iinclude -O2 -o bin/omnicc.exe \
    src/main.c src/lexer.c src/parser.c src/codegen.c \
    -lkernel32
./bin/omnicc run hello.ok
```

---

## Part 6: Adding a New Feature

Every new feature follows the same three steps:

**Example: adding `%` (modulo) operator**

**Step 1 — Lexer:** Add `TOKEN_PERCENT` to the enum. In `get_next_token()`, add `case '%': return new_token(TOKEN_PERCENT, "%");`

**Step 2 — Parser:** Register `parse_infix_expression` for `TOKEN_PERCENT`. Add `[TOKEN_PERCENT] = PREC_PRODUCT` to the precedence table.

**Step 3 — Codegen:** In `cg_infix()`, add:
```c
else if (strcmp(op, "%") == 0) {
    // idiv rcx gives quotient in rax, remainder in rdx
    emit_idiv_rcx(b);
    // mov rax, rdx  (we want the remainder)
    emit_u8(b, 0x48); emit_u8(b, 0x89); emit_u8(b, 0xD0);
}
```

That's it. Three files, three additions.

---

## Part 7: What's Next for Omnikarai

After the basics work, the natural progression is:

1. **User-defined function calls** — record `fn` offsets, call them with Windows x64 ABI
2. **For loops** — compile as `while` with an index variable
3. **Collections** — heap-allocated tagged structs for lists and dicts
4. **Classes** — vtable dispatch, `self` as first arg
5. **Module loading** — `use math` finds `math.ok` and compiles it
6. **PE writer** — emit a real `.exe` without any runtime

Each phase builds on the previous one. The compiler you have today already handles the hardest part — generating correct x86-64 that runs natively.
