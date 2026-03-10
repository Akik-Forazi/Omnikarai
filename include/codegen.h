#ifndef OMNI_CODEGEN_H
#define OMNI_CODEGEN_H

#include "ast.h"
#include <stdint.h>
#include <stddef.h>

// ============================================================
//  OMNIKARAI x86-64 Native Code Generator
//  Target: Windows x64 (Microsoft ABI)
//  No LLVM. No dependencies. Pure machine code emission.
// ============================================================

// --- Register IDs (x86-64) ---
typedef enum {
    REG_RAX = 0,
    REG_RCX = 1,
    REG_RDX = 2,
    REG_RBX = 3,
    REG_RSP = 4,
    REG_RBP = 5,
    REG_RSI = 6,
    REG_RDI = 7,
    REG_R8  = 8,
    REG_R9  = 9,
    REG_R10 = 10,
    REG_R11 = 11,
} Register;

// --- Value Location ---
// Where does a value live at any point during codegen?
typedef enum {
    LOC_REGISTER,   // value is in a CPU register
    LOC_STACK,      // value is at [RBP - offset]
    LOC_IMMEDIATE,  // compile-time constant (folded)
    LOC_NONE,       // no value (void / statement)
} ValueLocKind;

typedef struct {
    ValueLocKind kind;
    union {
        Register reg;       // LOC_REGISTER
        int      offset;    // LOC_STACK: negative offset from RBP
        int64_t  imm;       // LOC_IMMEDIATE: constant folded value
    };
} ValueLoc;

// --- Symbol (variable in scope) ---
#define SYM_TABLE_SIZE 256

typedef enum {
    OMNI_TYPE_INT,    // 64-bit signed integer
    OMNI_TYPE_BOOL,   // 1-bit boolean (stored as 8-bit)
    OMNI_TYPE_STR,    // pointer to null-terminated string
    OMNI_TYPE_UNKNOWN // not yet inferred
} OmniType;

typedef struct Symbol {
    char     name[64];
    OmniType type;
    int      stack_offset; // [RBP - stack_offset]
    struct Symbol* next;   // chaining for hash collisions
} Symbol;

typedef struct SymbolTable {
    Symbol*           buckets[SYM_TABLE_SIZE];
    struct SymbolTable* parent; // enclosing scope
    int               next_offset; // next available stack slot (grows down)
} SymbolTable;

// --- Code Buffer ---
// Dynamically growing byte buffer for emitted machine code
typedef struct {
    uint8_t* data;
    size_t   size;
    size_t   capacity;
} CodeBuf;

// --- Patch Entry ---
// For backpatching forward jumps
typedef struct {
    size_t patch_offset; // byte offset in CodeBuf where the 32-bit displacement sits
    size_t target_label; // label index to resolve
} Patch;

// --- Label ---
typedef struct {
    size_t offset;    // resolved byte offset in CodeBuf, SIZE_MAX = unresolved
} Label;

// --- CodeGen State ---
#define MAX_LABELS  1024
#define MAX_PATCHES 1024

typedef struct {
    CodeBuf      code;
    SymbolTable* scope;       // current scope
    Label        labels[MAX_LABELS];
    int          label_count;
    Patch        patches[MAX_PATCHES];
    int          patch_count;
    int          stack_size;  // total bytes reserved on stack for current fn
} CodeGen;

// ============================================================
//  Public API
// ============================================================

// Initialize a CodeGen context
void codegen_init(CodeGen* cg);

// Free all CodeGen resources
void codegen_free(CodeGen* cg);

// Compile an entire AST program → fills cg->code with x86-64 bytes
// Returns 1 on success, 0 on failure
int codegen_compile(CodeGen* cg, AST_Program* program);

// Run the compiled code in-memory using VirtualAlloc (Windows)
// Returns the integer exit code of the program
int codegen_run(CodeGen* cg);

// Dump raw bytes to stdout (for debugging)
void codegen_dump(CodeGen* cg);

#endif // OMNI_CODEGEN_H
