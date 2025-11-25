#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// LLVM includes
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Analysis.h>

// Represents the state of our compiler
typedef struct {
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    // We will add a symbol table here later
} Compiler;

// Forward declare our recursive compile function
LLVMValueRef compile_node(Compiler* compiler, AST_Node* node);

LLVMModuleRef compile_to_llvm_ir(AST_Node* ast) {
    if (ast == NULL) {
        fprintf(stderr, "Cannot compile a NULL AST.\n");
        return NULL;
    }

    Compiler compiler;
    compiler.module = LLVMModuleCreateWithName("omni_module");
    compiler.builder = LLVMCreateBuilder();

    // Create a main function
    LLVMTypeRef main_func_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(compiler.module, "main", main_func_type);
    
    // Create a basic block to start inserting code into
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");
    LLVMPositionBuilderAtEnd(compiler.builder, entry);

    // --- Start compiling the AST ---
    AST_Program* program = (AST_Program*)ast;
    LLVMValueRef last_value = NULL;
    for (int i = 0; i < program->statement_count; i++) {
        // Cast the specific statement pointer to the generic AST_Node pointer
        last_value = compile_node(&compiler, (AST_Node*)program->statements[i]);
    }

    // Use the last evaluated expression as the return value
    if (last_value) {
        LLVMBuildRet(compiler.builder, last_value);
    } else {
        // If there was no expression, return 0
        LLVMBuildRet(compiler.builder, LLVMConstInt(LLVMInt32Type(), 0, 0));
    }
    // --- End compiling ---
    
    // Verify the generated code, checking for consistency
    char *error = NULL;
    LLVMVerifyModule(compiler.module, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);

    // Clean up the builder
    LLVMDisposeBuilder(compiler.builder);

    printf("Compiler: Successfully generated LLVM IR.\n");
    LLVMDumpModule(compiler.module); // Print the IR to console for debugging

    return compiler.module;
}

LLVMValueRef compile_node(Compiler* compiler, AST_Node* node) {
    if (node == NULL) return NULL;

    switch (node->type) {
        case EXPRESSION_STATEMENT: {
            AST_Statement_Expression* stmt = (AST_Statement_Expression*)node;
            return compile_node(compiler, (AST_Node*)stmt->expression);
        }

        case INTEGER_LITERAL: {
            AST_Expression_IntegerLiteral* literal = (AST_Expression_IntegerLiteral*)node;
            return LLVMConstInt(LLVMInt32Type(), literal->value, 0);
        }

        case INFIX_EXPRESSION: {
            AST_Expression_Infix* expr = (AST_Expression_Infix*)node;
            LLVMValueRef left = compile_node(compiler, (AST_Node*)expr->left);
            LLVMValueRef right = compile_node(compiler, (AST_Node*)expr->right);

            if (strcmp(expr->operator, "+") == 0) {
                return LLVMBuildAdd(compiler->builder, left, right, "addtmp");
            } else if (strcmp(expr->operator, "-") == 0) {
                return LLVMBuildSub(compiler->builder, left, right, "subtmp");
            } else if (strcmp(expr->operator, "*") == 0) {
                return LLVMBuildMul(compiler->builder, left, right, "multmp");
            } else if (strcmp(expr->operator, "/") == 0) {
                return LLVMBuildSDiv(compiler->builder, left, right, "divtmp"); // Signed division
            } else {
                fprintf(stderr, "Compiler Error: Unknown infix operator: %s\n", expr->operator);
                return NULL;
            }
        }

        default:
            fprintf(stderr, "Compiler Error: Unrecognized AST node type: %d\n", node->type);
            return NULL;
    }
}