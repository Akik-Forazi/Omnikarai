#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

// Function to read the entire content of a file into a string
char *read_file(const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file: %s\n", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    printf("[DEBUG] read_file: ftell returned length %ld\n", length);
    if (length < 0) {
        fprintf(stderr, "Could not determine file size for %s.\n", filepath);
        fclose(file);
        return NULL;
    }
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(length + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Could not allocate memory for file content.\n");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);
    printf("[DEBUG] read_file: '%s' loaded, length %ld\n", filepath, length);
    return buffer;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Fatal: No input files specified. Usage: omnicc <file.ok>\n");
        return 1;
    }

    char *source_file_path = argv[1];
    printf("Compiling: %s\n", source_file_path);

    char *source_code = read_file(source_file_path);
    if (source_code == NULL) {
        return 1;
    }
    printf("[DEBUG] main: source_code address %p, length %zu\n", (void*)source_code, strlen(source_code));

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
        printf("Parsing complete. Found %d statements.\n", program->statement_count);
        printf("Compilation successful (for now... no code generation yet).\n");
    }

    // TODO: Need a function to free the entire AST, parser errors, etc.
    // free_program(program);
    // free_parser(p);
    free(source_code);
    
    return 0;
}

