// ============================================================
//  omni_regalloc.h — Linear Scan Register Allocator
//  Omnikarai v6.0 | Fraziym Tech & AI | 2026
//
//  Assigns callee-saved registers to hot variables inside
//  function bodies and the main program body, eliminating
//  stack load/store overhead for inner loop variables.
//
//  Available registers (callee-saved, Windows x64):
//    r12, r13, r14, r15, rbx, rsi, rdi  = 7 slots
//  (r14/r15 already used for for-loop counters — we extend this)
//
//  Algorithm: Linear scan with use-count priority
//    1. Walk AST, compute live ranges + use counts per variable
//    2. Sort by use_count DESC (hottest vars get registers first)
//    3. Assign registers greedily, spill lowest-priority on conflict
//    4. Codegen checks regalloc before emitting load/store
// ============================================================
#ifndef OMNI_REGALLOC_H
#define OMNI_REGALLOC_H

#include "ast.h"
#include <stdint.h>

#define RA_MAX_VARS   128   // max variables tracked per function
#define RA_NUM_REGS   5     // r12, r13, rbx, rsi, rdi (r14/r15 reserved for loops)

// Register IDs we allocate (indices into ra_reg_names[])
// These are callee-saved on Windows x64 and not used by existing codegen
#define RA_REG_RBX  0   // rbx
#define RA_REG_RSI  1   // rsi
#define RA_REG_RDI  2   // rdi
#define RA_REG_R12  3   // r12
#define RA_REG_R13  4   // r13
// r14 = loop counter 0 (existing)
// r15 = loop counter 1 (existing)

#define RA_SPILLED  -1  // variable stays on stack

typedef struct {
    char  name[64];     // variable name
    int   first_use;    // statement index of first use
    int   last_use;     // statement index of last use
    int   use_count;    // number of reads (higher = hotter)
    int   reg;          // RA_REG_* or RA_SPILLED
    int   stack_off;    // stack offset (always valid as fallback)
} RAVar;

typedef struct {
    RAVar vars[RA_MAX_VARS];
    int   var_count;
    int   reg_used[RA_NUM_REGS]; // 1 if register is allocated
    char  reg_owner[RA_NUM_REGS][64]; // which var owns this reg
    int   enabled;      // 1 if regalloc is active for current scope
} RegAllocState;

// Initialize an empty regalloc state
void ra_init(RegAllocState* ra);

// Analyze an AST block: compute live ranges and use counts
// stmt_offset: starting statement index (for nested blocks)
void ra_analyze_block(RegAllocState* ra, AST_Statement_Block* block, int stmt_offset);

// Run the allocation: assign registers to variables by priority
void ra_assign(RegAllocState* ra);

// Look up which register a variable is assigned to (or RA_SPILLED)
int ra_get_reg(RegAllocState* ra, const char* name);

// Emit x86-64 prologue saves for allocated registers
// Must be called after function prologue (push rbp; mov rbp,rsp; sub rsp,N)
void ra_emit_saves(RegAllocState* ra, void* codebuf);

// Emit x86-64 epilogue restores for allocated registers
// Must be called before ret
void ra_emit_restores(RegAllocState* ra, void* codebuf);

// x86-64 encoding helpers (register → MOV rax, reg  and  MOV reg, rax)
// These are called by codegen when a variable is register-allocated
void ra_emit_load_to_rax(void* codebuf, int reg_id);   // MOV RAX, reg
void ra_emit_store_from_rax(void* codebuf, int reg_id); // MOV reg, RAX

#endif // OMNI_REGALLOC_H
