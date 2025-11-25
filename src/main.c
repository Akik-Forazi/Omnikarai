#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // For unique temp file names

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "interpreter.h"
#include "object.h"
#include "compiler.h"
#include "jit_engine.h"

// Function to read the entire content of a file into a string
char *read_file(const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        perror("Could not open file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);

    if (length < 0) {
        perror("Could not determine file size");
        fclose(file);
        return NULL;
    }
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(length + 1);
    if (buffer == NULL) {
        perror("Could not allocate memory for file content");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);

    return buffer;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Fatal: No input files specified. Usage: omnicc [-jit] <file.ok>\n");
        return 1;
    }
    
    int use_jit = 0;
    char* source_file_path = NULL;

    if (argc > 2 && strcmp(argv[1], "-jit") == 0) {
        use_jit = 1;
        source_file_path = argv[2];
    } else {
        source_file_path = argv[1];
    }
    
    printf("Processing: %s\n", source_file_path);

    char *source_code = read_file(source_file_path);
    if (source_code == NULL) {
        return 1;
    }

    Lexer l;
    lexer_init(&l, source_code);
    Parser* p = new_parser(&l);

    AST_Program* program = parse_program(p);

    if (p->error_count > 0) {
        printf("Parser encountered %d errors:\n", p->error_count);
        for (int i = 0; i < p->error_count; i++) {
            printf("- %s\n", p->errors[i]);
        }
        printf("Processing failed.\n");
    } else {
        if (use_jit) {
            printf("Parsing complete. JIT Compiling...\n");
            
            jit_init();

            LLVMModuleRef module = compile_to_llvm_ir((AST_Node*)program);

            if (module) {
                LLVMExecutionEngineRef engine = jit_create_engine(module);
                if (engine) {
                    printf("JIT compilation complete. Running...\n");
                    int result = jit_run_main(engine);
                    printf("JIT Result: %d\n", result);
                    LLVMDisposeExecutionEngine(engine);
                }
            } else {
                printf("JIT compilation failed.\n");
            }
            
            jit_shutdown();

        } else {
            printf("Parsing complete. Interpreting...\n");
            Object* result = interpret(program);
            printf("Result: ");
            print_object(result);
            printf("\n");
        }
    }

    // TODO: Need a function to free the entire AST, parser errors, etc.
    // free_program(program);
    // free_parser(p);
    free(source_code);
    
    return 0;
}

