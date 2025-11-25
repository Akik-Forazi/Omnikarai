#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // For unique temp file names

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "compiler.h" // NEW INCLUDE

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
        perror("Fatal: No input files specified. Usage: omnicc <file.ok>");
        return 1;
    }

    char *source_file_path = argv[1];
    printf("Compiling: %s\n", source_file_path);

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
        printf("Compilation failed.\n");
    } else {
        printf("Parsing complete. Generating C code...\n");

        char* c_code = compile(program); // CALL OUR COMPILER

        // Generate unique temporary filenames
        char temp_c_filepath[256];
        char temp_exe_filepath[256];
        srand(time(NULL)); // Seed for randomness
        sprintf(temp_c_filepath, "/tmp/%d_omni_temp.c", rand());
        sprintf(temp_exe_filepath, "/tmp/%d_omni_temp", rand());

        // Write generated C code to file
        FILE* temp_c_file = fopen(temp_c_filepath, "w");
        if (temp_c_file == NULL) {
            perror("Fatal: Could not open temporary C file for writing");
            free(c_code);
            // TODO: Free AST, parser, source_code more robustly
            return 1;
        }
        fputs(c_code, temp_c_file);
        fclose(temp_c_file);
        free(c_code); // Free the generated C code string

        printf("Generated C code written to: %s\n", temp_c_filepath);

        // Compile the C code using gcc
        char compile_command[1024];
        sprintf(compile_command, "gcc %s -o %s", temp_c_filepath, temp_exe_filepath);
        printf("Executing compile command: %s\n", compile_command);

        int compile_result = system(compile_command); // Execute gcc
        if (compile_result != 0) {
            perror("Fatal: C compilation failed");
            remove(temp_c_filepath); // Attempt to cleanup
            // TODO: Free AST, parser, source_code more robustly
            return 1;
        }
        printf("Compilation successful. Executable: %s\n", temp_exe_filepath);

        // Run the compiled executable
        printf("Executing compiled program:\n");
        int run_result = system(temp_exe_filepath); // Execute the compiled program
        if (run_result != 0) {
            perror("Fatal: Compiled program exited with error");
        }

        // Cleanup temporary files
        remove(temp_c_filepath);
        remove(temp_exe_filepath);
        printf("Cleaned up temporary files.\n");
    }

    // TODO: Need a function to free the entire AST, parser errors, etc.
    // free_program(program);
    // free_parser(p);
    free(source_code);
    
    return 0;
}

