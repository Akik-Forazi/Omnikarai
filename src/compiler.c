#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "ast.h"
#include "object.h" // For object types if we need to generate runtime objects
#include "omni_runtime.h" // NEW INCLUDE

// --- Dynamic String Builder ---
typedef struct {
    char* buffer;
    size_t length;
    size_t capacity;
} StringBuilder;

void sb_init(StringBuilder* sb) {
    sb->capacity = 1024; // Initial capacity
    sb->buffer = malloc(sb->capacity);
    if (sb->buffer == NULL) {
        fprintf(stderr, "Fatal: Memory allocation failed for StringBuilder.\n");
        exit(1);
    }
    sb->length = 0;
    sb->buffer[0] = '\0';
}

void sb_append(StringBuilder* sb, const char* str) {
    size_t str_len = strlen(str);
    while (sb->length + str_len + 1 > sb->capacity) {
        sb->capacity *= 2;
        sb->buffer = realloc(sb->buffer, sb->capacity);
        if (sb->buffer == NULL) {
            fprintf(stderr, "Fatal: Memory re-allocation failed for StringBuilder.\n");
            exit(1);
        }
    }
    strcpy(sb->buffer + sb->length, str);
    sb->length += str_len;
}

void sb_free(StringBuilder* sb) {
    free(sb->buffer);
    sb->buffer = NULL;
    sb->length = 0;
    sb->capacity = 0;
}

// --- Forward Declarations ---
static void compile_node(StringBuilder* sb, AST_Node* node);
static void compile_program(StringBuilder* sb, AST_Program* program);
static void compile_integer_literal(StringBuilder* sb, AST_Expression_IntegerLiteral* integer);
static void compile_boolean_literal(StringBuilder* sb, AST_Expression_Boolean* boolean);
static void compile_string_literal(StringBuilder* sb, AST_Expression_StringLiteral* string);
static void compile_expression_statement(StringBuilder* sb, AST_Statement_Expression* stmt);
static void compile_call_expression(StringBuilder* sb, AST_Expression_Call* call_expr);

// --- Compilation Functions for AST Nodes ---
static void compile_program(StringBuilder* sb, AST_Program* program) {
    for (int i = 0; i < program->statement_count; i++) {
        compile_node(sb, (AST_Node*)program->statements[i]);
    }
}

static void compile_node(StringBuilder* sb, AST_Node* node) {
    if (node == NULL) {
        return;
    }
    switch (node->type) {
        case EXPRESSION_STATEMENT:
            compile_expression_statement(sb, (AST_Statement_Expression*)node);
            break;
        case INTEGER_LITERAL:
            compile_integer_literal(sb, (AST_Expression_IntegerLiteral*)node);
            break;
        case BOOLEAN_LITERAL:
            compile_boolean_literal(sb, (AST_Expression_Boolean*)node);
            break;
        case STRING_LITERAL:
            compile_string_literal(sb, (AST_Expression_StringLiteral*)node);
            break;
        // TODO: Add other AST node types here
        default:
            fprintf(stderr, "Fatal: Unsupported AST node type in compiler: %d\n", node->type);
            exit(1);
    }
}

// --- Public Compile Function ---
char* compile(AST_Program* program) {
    StringBuilder sb;
    sb_init(&sb);

    compile_program(&sb, program); // Directly call compile_program

    char* result = sb.buffer; // Take ownership of the buffer
    // Reset sb state so sb_free doesn't free it again later if called
    sb.buffer = NULL; 
    sb.length = 0;
    sb.capacity = 0;

    return result;
}

static void compile_expression_statement(StringBuilder* sb, AST_Statement_Expression* stmt) {
    // TODO: Implement this function
}

static void compile_integer_literal(StringBuilder* sb, AST_Expression_IntegerLiteral* integer) {
    // TODO: Implement this function
}

static void compile_boolean_literal(StringBuilder* sb, AST_Expression_Boolean* boolean) {
    // TODO: Implement this function
}

static void compile_string_literal(StringBuilder* sb, AST_Expression_StringLiteral* string) {
    // TODO: Implement this function
}

static void compile_call_expression(StringBuilder* sb, AST_Expression_Call* call_expr) {
    // TODO: Implement this function
}
