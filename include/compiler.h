#ifndef OMNI_COMPILER_H
#define OMNI_COMPILER_H

#include "ast.h"

// Forward declare LLVM types to avoid including llvm-c headers in our public header.
typedef struct LLVMOpaqueModule* LLVMModuleRef;

LLVMModuleRef compile_to_llvm_ir(AST_Node* ast);

#endif // OMNI_COMPILER_H