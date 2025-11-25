#include "jit_engine.h"
#include <stdio.h>
#include <stdlib.h>

void jit_init() {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
}

void jit_shutdown() {
    // No explicit shutdown in LLVM-C API for the JIT.
}

LLVMExecutionEngineRef jit_create_engine(LLVMModuleRef module) {
    char *error = NULL;
    LLVMExecutionEngineRef engine;

    if (LLVMCreateJITCompilerForModule(&engine, module, 2, &error)) {
        fprintf(stderr, "Failed to create JIT compiler: %s\n", error);
        LLVMDisposeMessage(error);
        return NULL;
    }

    return engine;
}

int jit_run_main(LLVMExecutionEngineRef engine) {
    uint64_t res = 0;
    LLVMValueRef main_func = NULL;

    if (LLVMFindFunction(engine, "main", &main_func) || main_func == NULL) {
        fprintf(stderr, "JIT Error: 'main' function not found in module.\n");
        return -1;
    }
    
    // We assume main returns an int, so we execute it and get the result.
    LLVMGenericValueRef exec_res = LLVMRunFunction(engine, main_func, 0, NULL);
    res = LLVMGenericValueToInt(exec_res, 0); // Second arg is is_signed
    LLVMDisposeGenericValue(exec_res);

    return (int)res;
}

