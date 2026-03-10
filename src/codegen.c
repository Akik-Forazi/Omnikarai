// ============================================================
//  OMNIKARAI x86-64 Native Code Generator
//  Windows x64 ABI — No LLVM — No dependencies
//  
//  Pipeline: AST -> type inference -> x86-64 bytes -> VirtualAlloc -> run
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "codegen.h"
#include "ast.h"

// ============================================================
//  CODE BUFFER
// ============================================================

static void buf_init(CodeBuf* b) {
    b->capacity = 4096;
    b->size     = 0;
    b->data     = malloc(b->capacity);
    if (!b->data) { fprintf(stderr, "Fatal: OOM in buf_init\n"); exit(1); }
}

static void buf_free(CodeBuf* b) {
    free(b->data);
    b->data = NULL;
    b->size = b->capacity = 0;
}

// Emit a single byte
static void emit_u8(CodeBuf* b, uint8_t v) {
    if (b->size >= b->capacity) {
        b->capacity *= 2;
        b->data = realloc(b->data, b->capacity);
        if (!b->data) { fprintf(stderr, "Fatal: OOM in emit_u8\n"); exit(1); }
    }
    b->data[b->size++] = v;
}

// Emit a 32-bit little-endian value
static void emit_u32(CodeBuf* b, uint32_t v) {
    emit_u8(b, (uint8_t)(v));
    emit_u8(b, (uint8_t)(v >> 8));
    emit_u8(b, (uint8_t)(v >> 16));
    emit_u8(b, (uint8_t)(v >> 24));
}

// Emit a 64-bit little-endian value
static void emit_u64(CodeBuf* b, uint64_t v) {
    emit_u32(b, (uint32_t)(v));
    emit_u32(b, (uint32_t)(v >> 32));
}

// Patch a 32-bit value at a given offset (for backpatching jumps)
static void patch_u32(CodeBuf* b, size_t offset, uint32_t v) {
    b->data[offset]     = (uint8_t)(v);
    b->data[offset + 1] = (uint8_t)(v >> 8);
    b->data[offset + 2] = (uint8_t)(v >> 16);
    b->data[offset + 3] = (uint8_t)(v >> 24);
}

// ============================================================
//  SYMBOL TABLE (FNV-1a hash, open chaining)
// ============================================================

static unsigned int sym_hash(const char* s) {
    unsigned int h = 2166136261u;
    while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; }
    return h % SYM_TABLE_SIZE;
}

static SymbolTable* scope_new(SymbolTable* parent) {
    SymbolTable* st = calloc(1, sizeof(SymbolTable));
    st->parent      = parent;
    st->next_offset = parent ? parent->next_offset : 8; // start at RBP-8
    return st;
}

static void scope_free(SymbolTable* st) {
    for (int i = 0; i < SYM_TABLE_SIZE; i++) {
        Symbol* s = st->buckets[i];
        while (s) { Symbol* n = s->next; free(s); s = n; }
    }
    free(st);
}

// Look up symbol — walks up scopes
static Symbol* scope_get(SymbolTable* st, const char* name) {
    while (st) {
        unsigned int idx = sym_hash(name);
        Symbol* s = st->buckets[idx];
        while (s) {
            if (strcmp(s->name, name) == 0) return s;
            s = s->next;
        }
        st = st->parent;
    }
    return NULL;
}

// Define a new symbol in current scope, returns stack offset
static Symbol* scope_define(SymbolTable* st, const char* name, OmniType type) {
    Symbol* s = calloc(1, sizeof(Symbol));
    strncpy_s(s->name, sizeof(s->name), name, _TRUNCATE);
    s->type         = type;
    s->stack_offset = st->next_offset;
    st->next_offset += 8; // all values are 8 bytes (64-bit)
    unsigned int idx = sym_hash(name);
    s->next          = st->buckets[idx];
    st->buckets[idx] = s;
    return s;
}

// ============================================================
//  x86-64 INSTRUCTION EMITTERS
//  Windows x64 ABI:
//    - args: RCX, RDX, R8, R9, then stack
//    - caller preserves: RAX, RCX, RDX, R8, R9, R10, R11
//    - callee preserves: RBX, RBP, RDI, RSI, R12-R15
//    - shadow space: 32 bytes before call
//    - stack aligned to 16 bytes before CALL
// ============================================================

// REX.W prefix for 64-bit operands
#define REX_W   0x48
#define REX_WR  0x4C  // REX.W + REX.R (for r8-r15 in reg field)
#define REX_WB  0x49  // REX.W + REX.B (for r8-r15 in rm field)

// push rbp
static void emit_push_rbp(CodeBuf* b) {
    emit_u8(b, 0x55);
}

// pop rbp
static void emit_pop_rbp(CodeBuf* b) {
    emit_u8(b, 0x5D);
}

// push rax
static void emit_push_rax(CodeBuf* b) {
    emit_u8(b, 0x50);
}

// pop rax
static void emit_pop_rax(CodeBuf* b) {
    emit_u8(b, 0x58);
}

// pop rcx
static void emit_pop_rcx(CodeBuf* b) {
    emit_u8(b, 0x59);
}

// mov rbp, rsp
static void emit_mov_rbp_rsp(CodeBuf* b) {
    emit_u8(b, REX_W); emit_u8(b, 0x89); emit_u8(b, 0xE5);
}

// mov rsp, rbp
static void emit_mov_rsp_rbp(CodeBuf* b) {
    emit_u8(b, REX_W); emit_u8(b, 0x89); emit_u8(b, 0xEC);
}

// sub rsp, imm8
static void emit_sub_rsp_imm8(CodeBuf* b, uint8_t imm) {
    emit_u8(b, REX_W); emit_u8(b, 0x83); emit_u8(b, 0xEC); emit_u8(b, imm);
}

// sub rsp, imm32
static void emit_sub_rsp_imm32(CodeBuf* b, uint32_t imm) {
    emit_u8(b, REX_W); emit_u8(b, 0x81); emit_u8(b, 0xEC); emit_u32(b, imm);
}

// add rsp, imm8
static void emit_add_rsp_imm8(CodeBuf* b, uint8_t imm) {
    emit_u8(b, REX_W); emit_u8(b, 0x83); emit_u8(b, 0xC4); emit_u8(b, imm);
}

// mov rax, imm64
static void emit_mov_rax_imm64(CodeBuf* b, int64_t imm) {
    emit_u8(b, REX_W); emit_u8(b, 0xB8); emit_u64(b, (uint64_t)imm);
}

// mov rcx, imm64
static void emit_mov_rcx_imm64(CodeBuf* b, int64_t imm) {
    emit_u8(b, REX_W); emit_u8(b, 0xB9); emit_u64(b, (uint64_t)imm);
}

// mov [rbp - offset], rax  (store rax to stack)
static void emit_mov_stack_rax(CodeBuf* b, int offset) {
    // REX.W + MOV [RBP-disp32], RAX
    emit_u8(b, REX_W); emit_u8(b, 0x89); emit_u8(b, 0x85);
    emit_u32(b, (uint32_t)(-offset));
}

// mov rax, [rbp - offset]  (load from stack to rax)
static void emit_mov_rax_stack(CodeBuf* b, int offset) {
    emit_u8(b, REX_W); emit_u8(b, 0x8B); emit_u8(b, 0x85);
    emit_u32(b, (uint32_t)(-offset));
}

// add rax, rcx  (rax = rax + rcx)
static void emit_add_rax_rcx(CodeBuf* b) {
    emit_u8(b, REX_W); emit_u8(b, 0x03); emit_u8(b, 0xC1);
}

// sub rax, rcx  (rax = rax - rcx)
static void emit_sub_rax_rcx(CodeBuf* b) {
    emit_u8(b, REX_W); emit_u8(b, 0x2B); emit_u8(b, 0xC1);
}

// imul rax, rcx  (rax = rax * rcx)
static void emit_imul_rax_rcx(CodeBuf* b) {
    emit_u8(b, REX_W); emit_u8(b, 0x0F); emit_u8(b, 0xAF); emit_u8(b, 0xC1);
}

// idiv rcx — divides rdx:rax by rcx, quotient in rax
static void emit_idiv_rcx(CodeBuf* b) {
    // cqo (sign-extend rax into rdx)
    emit_u8(b, REX_W); emit_u8(b, 0x99);
    // idiv rcx
    emit_u8(b, REX_W); emit_u8(b, 0xF7); emit_u8(b, 0xF9);
}

// cmp rax, rcx
static void emit_cmp_rax_rcx(CodeBuf* b) {
    emit_u8(b, REX_W); emit_u8(b, 0x3B); emit_u8(b, 0xC1);
}

// ret
static void emit_ret(CodeBuf* b) {
    emit_u8(b, 0xC3);
}

// xor rax, rax  (rax = 0, also clears flags efficiently)
static void emit_xor_rax_rax(CodeBuf* b) {
    emit_u8(b, REX_W); emit_u8(b, 0x31); emit_u8(b, 0xC0);
}

// setcc al  (set al based on condition code)
// then movzx rax, al
static void emit_setcc_rax(CodeBuf* b, uint8_t cc) {
    // xor eax, eax  (zero upper bits)
    emit_u8(b, 0x31); emit_u8(b, 0xC0);
    // setcc al
    emit_u8(b, 0x0F); emit_u8(b, cc); emit_u8(b, 0xC0);
}

// je rel32 — emit opcode + 4-byte placeholder, return offset for patching
static size_t emit_je_placeholder(CodeBuf* b) {
    emit_u8(b, 0x0F); emit_u8(b, 0x84);
    size_t patch_pos = b->size;
    emit_u32(b, 0); // placeholder
    return patch_pos;
}

// jmp rel32 — unconditional jump placeholder
static size_t emit_jmp_placeholder(CodeBuf* b) {
    emit_u8(b, 0xE9);
    size_t patch_pos = b->size;
    emit_u32(b, 0);
    return patch_pos;
}

// Resolve a forward jump: fill in displacement
static void resolve_jump(CodeBuf* b, size_t patch_pos) {
    int32_t disp = (int32_t)(b->size - (patch_pos + 4));
    patch_u32(b, patch_pos, (uint32_t)disp);
}

// ============================================================
//  FORWARD DECLARATIONS
// ============================================================

static void cg_stmt(CodeGen* cg, AST_Statement* stmt);
static void cg_expr(CodeGen* cg, AST_Expression* expr); // result in RAX

// ============================================================
//  EXPRESSION CODEGEN  (result always in RAX)
// ============================================================

static void cg_integer_literal(CodeGen* cg, AST_Expression_IntegerLiteral* node) {
    emit_mov_rax_imm64(&cg->code, node->value);
}

static void cg_boolean_literal(CodeGen* cg, AST_Expression_Boolean* node) {
    emit_mov_rax_imm64(&cg->code, node->value ? 1 : 0);
}

static void cg_identifier(CodeGen* cg, AST_Expression_Identifier* node) {
    Symbol* sym = scope_get(cg->scope, node->value);
    if (!sym) {
        fprintf(stderr, "CodeGen Error: undefined variable '%s'\n", node->value);
        exit(1);
    }
    emit_mov_rax_stack(&cg->code, sym->stack_offset);
}

static void cg_infix(CodeGen* cg, AST_Expression_Infix* node) {
    // Evaluate left → push RAX, evaluate right → RAX, pop RCX, operate
    cg_expr(cg, node->left);
    emit_push_rax(&cg->code);          // push left value
    cg_expr(cg, node->right);
    emit_pop_rcx(&cg->code);           // rcx = left, rax = right

    // Swap so rax = left, rcx = right (for non-commutative ops)
    // xchg rax, rcx
    emit_u8(&cg->code, REX_W); emit_u8(&cg->code, 0x87); emit_u8(&cg->code, 0xC1);

    TokenType op = node->base.token.type;
    switch (op) {
        case TOKEN_PLUS:   emit_add_rax_rcx(&cg->code); break;
        case TOKEN_MINUS:  emit_sub_rax_rcx(&cg->code); break;
        case TOKEN_STAR:   emit_imul_rax_rcx(&cg->code); break;
        case TOKEN_SLASH:  emit_idiv_rcx(&cg->code); break;
        case TOKEN_EQ:
            emit_cmp_rax_rcx(&cg->code);
            emit_setcc_rax(&cg->code, 0x94); // sete
            break;
        case TOKEN_NOT_EQ:
            emit_cmp_rax_rcx(&cg->code);
            emit_setcc_rax(&cg->code, 0x95); // setne
            break;
        case TOKEN_LT:
            emit_cmp_rax_rcx(&cg->code);
            emit_setcc_rax(&cg->code, 0x9C); // setl
            break;
        case TOKEN_GT:
            emit_cmp_rax_rcx(&cg->code);
            emit_setcc_rax(&cg->code, 0x9F); // setg
            break;
        case TOKEN_LTE:
            emit_cmp_rax_rcx(&cg->code);
            emit_setcc_rax(&cg->code, 0x9E); // setle
            break;
        case TOKEN_GTE:
            emit_cmp_rax_rcx(&cg->code);
            emit_setcc_rax(&cg->code, 0x9D); // setge
            break;
        default:
            fprintf(stderr, "CodeGen Error: unsupported infix operator token %d\n", op);
            exit(1);
    }
}

static void cg_expr(CodeGen* cg, AST_Expression* expr) {
    if (!expr) return;
    switch (expr->type) {
        case INTEGER_LITERAL:
            cg_integer_literal(cg, (AST_Expression_IntegerLiteral*)expr); break;
        case BOOLEAN_LITERAL:
            cg_boolean_literal(cg, (AST_Expression_Boolean*)expr); break;
        case IDENTIFIER:
            cg_identifier(cg, (AST_Expression_Identifier*)expr); break;
        case INFIX_EXPRESSION:
            cg_infix(cg, (AST_Expression_Infix*)expr); break;
        case PREFIX_EXPRESSION: {
            AST_Expression_Prefix* p = (AST_Expression_Prefix*)expr;
            cg_expr(cg, p->right);
            if (p->base.token.type == TOKEN_MINUS) {
                // neg rax
                emit_u8(&cg->code, REX_W); emit_u8(&cg->code, 0xF7); emit_u8(&cg->code, 0xD8);
            }
            break;
        }
        default:
            fprintf(stderr, "CodeGen Error: unsupported expression type %d\n", expr->type);
            exit(1);
    }
}

// ============================================================
//  STATEMENT CODEGEN
// ============================================================

static void cg_set_statement(CodeGen* cg, AST_Statement_Set* stmt) {
    // Evaluate value → RAX
    cg_expr(cg, stmt->value);

    // Define or update symbol
    Symbol* sym = scope_get(cg->scope, stmt->name->value);
    if (!sym) {
        sym = scope_define(cg->scope, stmt->name->value, OMNI_TYPE_INT);
        // Update total stack size needed
        if (sym->stack_offset > cg->stack_size)
            cg->stack_size = sym->stack_offset;
    }

    // Store RAX → [RBP - offset]
    emit_mov_stack_rax(&cg->code, sym->stack_offset);
}

static void cg_if_statement(CodeGen* cg, AST_Statement_If* stmt) {
    // Evaluate condition → RAX
    cg_expr(cg, stmt->condition);

    // test rax, rax
    emit_u8(&cg->code, REX_W); emit_u8(&cg->code, 0x85); emit_u8(&cg->code, 0xC0);

    // je else_label (if rax == 0, condition is false)
    size_t je_patch = emit_je_placeholder(&cg->code);

    // Consequence block
    for (int i = 0; i < stmt->consequence->statement_count; i++)
        cg_stmt(cg, stmt->consequence->statements[i]);

    if (stmt->alternative) {
        // jmp end_label (skip else)
        size_t jmp_patch = emit_jmp_placeholder(&cg->code);
        resolve_jump(&cg->code, je_patch); // else starts here
        cg_stmt(cg, stmt->alternative);
        resolve_jump(&cg->code, jmp_patch); // end
    } else {
        resolve_jump(&cg->code, je_patch); // end (no else)
    }
}

static void cg_while_statement(CodeGen* cg, AST_Statement_While* stmt) {
    size_t loop_start = cg->code.size;

    // Evaluate condition → RAX
    cg_expr(cg, stmt->condition);

    // test rax, rax
    emit_u8(&cg->code, REX_W); emit_u8(&cg->code, 0x85); emit_u8(&cg->code, 0xC0);

    // je exit (condition false → exit loop)
    size_t je_patch = emit_je_placeholder(&cg->code);

    // Body
    for (int i = 0; i < stmt->body->statement_count; i++)
        cg_stmt(cg, stmt->body->statements[i]);

    // jmp loop_start (unconditional back-jump)
    emit_u8(&cg->code, 0xE9);
    int32_t back_disp = (int32_t)(loop_start - (cg->code.size + 4));
    emit_u32(&cg->code, (uint32_t)back_disp);

    resolve_jump(&cg->code, je_patch); // exit label
}

static void cg_return_statement(CodeGen* cg, AST_Statement_Return* stmt) {
    if (stmt->return_value)
        cg_expr(cg, stmt->return_value);
    else
        emit_xor_rax_rax(&cg->code);

    // Epilogue
    emit_mov_rsp_rbp(&cg->code);
    emit_pop_rbp(&cg->code);
    emit_ret(&cg->code);
}

static void cg_block(CodeGen* cg, AST_Statement_Block* block) {
    for (int i = 0; i < block->statement_count; i++)
        cg_stmt(cg, block->statements[i]);
}

static void cg_stmt(CodeGen* cg, AST_Statement* stmt) {
    if (!stmt) return;
    switch (stmt->type) {
        case SET_STATEMENT:
            cg_set_statement(cg, (AST_Statement_Set*)stmt); break;
        case EXPRESSION_STATEMENT:
            cg_expr(cg, ((AST_Statement_Expression*)stmt)->expression); break;
        case IF_STATEMENT:
            cg_if_statement(cg, (AST_Statement_If*)stmt); break;
        case WHILE_STATEMENT:
            cg_while_statement(cg, (AST_Statement_While*)stmt); break;
        case RETURN_STATEMENT:
            cg_return_statement(cg, (AST_Statement_Return*)stmt); break;
        case BLOCK_STATEMENT:
            cg_block(cg, (AST_Statement_Block*)stmt); break;
        default:
            fprintf(stderr, "CodeGen Error: unsupported statement type %d\n", stmt->type);
            break;
    }
}

// ============================================================
//  PROGRAM PROLOGUE / EPILOGUE
// ============================================================

static void emit_prologue(CodeGen* cg) {
    emit_push_rbp(&cg->code);
    emit_mov_rbp_rsp(&cg->code);
    // Reserve stack space — we'll backpatch this after we know stack_size
    // emit sub rsp, imm32 placeholder (5 bytes: REX.W 0x81 0xEC + 4 bytes)
    emit_u8(&cg->code, REX_W);
    emit_u8(&cg->code, 0x81);
    emit_u8(&cg->code, 0xEC);
    cg->stack_size = 0; // will be resolved after codegen
    size_t patch_at = cg->code.size;
    emit_u32(&cg->code, 0); // placeholder — patched in emit_epilogue_patch
    (void)patch_at;
}

// Patch the sub rsp instruction after we know total stack usage
static void patch_prologue_stack(CodeGen* cg, size_t patch_offset, int total_size) {
    // Align to 16 bytes (Windows ABI requirement)
    total_size = (total_size + 15) & ~15;
    // Add shadow space (32 bytes) for Windows ABI
    total_size += 32;
    patch_u32(&cg->code, patch_offset, (uint32_t)total_size);
}

static void emit_epilogue(CodeGen* cg) {
    emit_xor_rax_rax(&cg->code); // default return 0
    emit_mov_rsp_rbp(&cg->code);
    emit_pop_rbp(&cg->code);
    emit_ret(&cg->code);
}

// ============================================================
//  PUBLIC API
// ============================================================

void codegen_init(CodeGen* cg) {
    buf_init(&cg->code);
    cg->scope       = scope_new(NULL);
    cg->label_count = 0;
    cg->patch_count = 0;
    cg->stack_size  = 0;
}

void codegen_free(CodeGen* cg) {
    buf_free(&cg->code);
    scope_free(cg->scope);
    cg->scope = NULL;
}

int codegen_compile(CodeGen* cg, AST_Program* program) {
    if (!program) return 0;

    // Prologue — note: stack size patched after body
    // Offset of the imm32 in sub rsp is at position 7 (after push rbp, mov rbp rsp, rex/opcode/modrm)
    emit_push_rbp(&cg->code);
    emit_mov_rbp_rsp(&cg->code);
    // sub rsp, placeholder (3 + 4 bytes)
    emit_u8(&cg->code, REX_W);
    emit_u8(&cg->code, 0x81);
    emit_u8(&cg->code, 0xEC);
    size_t stack_patch_offset = cg->code.size;
    emit_u32(&cg->code, 0); // patched later

    // Compile all statements
    for (int i = 0; i < program->statement_count; i++)
        cg_stmt(cg, program->statements[i]);

    // Default return 0
    emit_xor_rax_rax(&cg->code);
    emit_mov_rsp_rbp(&cg->code);
    emit_pop_rbp(&cg->code);
    emit_ret(&cg->code);

    // Now patch sub rsp with actual stack size (aligned + shadow space)
    patch_prologue_stack(cg, stack_patch_offset, cg->stack_size + 8);

    return 1;
}

int codegen_run(CodeGen* cg) {
    if (cg->code.size == 0) {
        fprintf(stderr, "CodeGen: nothing to run\n");
        return -1;
    }

    // Allocate executable memory using Windows VirtualAlloc
    void* mem = VirtualAlloc(
        NULL,
        cg->code.size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    if (!mem) {
        fprintf(stderr, "CodeGen: VirtualAlloc failed (error %lu)\n", GetLastError());
        return -1;
    }

    // Copy machine code into executable memory
    memcpy(mem, cg->code.data, cg->code.size);

    // Cast to function pointer and call
    typedef int (*OmniEntry)(void);
    OmniEntry entry = (OmniEntry)mem;
    int result = entry();

    // Free executable memory
    VirtualFree(mem, 0, MEM_RELEASE);

    return result;
}

void codegen_dump(CodeGen* cg) {
    printf("=== OMNIKARAI x86-64 CODE DUMP (%zu bytes) ===\n", cg->code.size);
    for (size_t i = 0; i < cg->code.size; i++) {
        printf("%02X ", cg->code.data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n=== END DUMP ===\n");
}
