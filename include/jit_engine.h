#ifndef OMNI_JIT_ENGINE_H
#define OMNI_JIT_ENGINE_H

#include <llvm-c/ExecutionEngine.h>

void jit_init();
void jit_shutdown();
LLVMExecutionEngineRef jit_create_engine(LLVMModuleRef module);
int jit_run_main(LLVMExecutionEngineRef engine);

#endif // OMNI_JIT_ENGINE_H
