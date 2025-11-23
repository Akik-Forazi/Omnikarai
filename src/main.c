#include <stdio.h>
#include <stdlib.h>
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
    return buffer;
}

int main(int argc, char **argv) {
    printf("omnicc v0.0.1 - The Omnikarai Compiler\n");
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

    Lexer l;
    lexer_init(&l, source_code);
    Parser* p = new_parser(&l);

    AST_Program* program = parse_program(p);

    // --- Verification ---
    printf("Parser found %d statements.\n", program->statement_count);
    for (int i = 0; i < program->statement_count; i++) {
        AST_Statement* stmt = program->statements[i];
        if (stmt->type == AST_LET_STATEMENT) {
            AST_Statement_Let* let_stmt = (AST_Statement_Let*)stmt;
            AST_Expression_Identifier* ident = (AST_Expression_Identifier*)let_stmt->name;
            printf("Statement %d is a LET statement with identifier: %s", i + 1, ident->value);

            if (let_stmt->value != NULL) {
                if (let_stmt->value->type == AST_INTEGER_LITERAL) {
                    AST_Expression_IntegerLiteral* int_lit = (AST_Expression_IntegerLiteral*)let_stmt->value;
                    printf(", value: %lld\n", int_lit->value);
                } else {
                    printf(", value type: %d (not an integer literal)\n", let_stmt->value->type);
                }
            } else {
                printf(", no value parsed.\n");
            }

        } else {
            printf("Statement %d is of an unknown type.\n", i + 1);
        }
    }

    // TODO: Need a function to free the entire AST
    // free(program->statements); // This is more complex now
    free(program);
    free(p);
    free(source_code);
    
    printf("Compilation complete.\n");
    return 0;
}

