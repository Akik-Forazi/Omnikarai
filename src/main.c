// ============================================================
//  OMNIKARAI Compiler — omnicc
//  Windows x64 — No LLVM — No runtime
//
//  Usage:
//    omnicc run   <file.ok>   — compile and run immediately
//    omnicc build <file.ok>   — compile to .exe (TODO: pe_writer)
//    omnicc dump  <file.ok>   — show generated machine code bytes
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"

// ============================================================
//  FILE READER
// ============================================================

static char* read_file(const char* path) {
    FILE* f = NULL;
    if (fopen_s(&f, path, "rb") != 0 || !f) {
        fprintf(stderr, "Error: cannot open '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len < 0) { fclose(f); return NULL; }

    char* buf = malloc((size_t)len + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

// ============================================================
//  COMPILE PIPELINE
//  source → Lexer → Parser → AST → CodeGen → machine code
// ============================================================

static AST_Program* compile_source(const char* source, const char* filename) {
    Lexer l;
    lexer_init(&l, source);

    Parser* p = new_parser(&l);
    AST_Program* program = parse_program(p);

    if (p->error_count > 0) {
        fprintf(stderr, "Parse errors in '%s':\n", filename);
        for (int i = 0; i < p->error_count; i++)
            fprintf(stderr, "  [%d] %s\n", i + 1, p->errors[i]);
        free_parser(p);
        return NULL;
    }

    free_parser(p);
    return program;
}

// ============================================================
//  COMMANDS
// ============================================================

static int cmd_run(const char* filepath) {
    char* source = read_file(filepath);
    if (!source) return 1;

    AST_Program* program = compile_source(source, filepath);
    free(source);
    if (!program) return 1;

    CodeGen cg;
    codegen_init(&cg);

    if (!codegen_compile(&cg, program)) {
        fprintf(stderr, "Compilation failed.\n");
        codegen_free(&cg);
        return 1;
    }

    int exit_code = codegen_run(&cg);
    codegen_free(&cg);
    return exit_code;
}

static int cmd_dump(const char* filepath) {
    char* source = read_file(filepath);
    if (!source) return 1;

    AST_Program* program = compile_source(source, filepath);
    free(source);
    if (!program) return 1;

    CodeGen cg;
    codegen_init(&cg);

    if (!codegen_compile(&cg, program)) {
        fprintf(stderr, "Compilation failed.\n");
        codegen_free(&cg);
        return 1;
    }

    codegen_dump(&cg);
    codegen_free(&cg);
    return 0;
}

// ============================================================
//  ENTRY POINT
// ============================================================

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr,
            "Omnikarai Compiler (omnicc)\n"
            "Usage:\n"
            "  omnicc run   <file.ok>   compile and run\n"
            "  omnicc build <file.ok>   compile to .exe  (coming soon)\n"
            "  omnicc dump  <file.ok>   dump machine code bytes\n"
        );
        return 1;
    }

    const char* cmd      = argv[1];
    const char* filepath = argv[2];

    if (strcmp(cmd, "run") == 0)   return cmd_run(filepath);
    if (strcmp(cmd, "dump") == 0)  return cmd_dump(filepath);
    if (strcmp(cmd, "build") == 0) {
        fprintf(stderr, "omnicc build: PE writer coming in next phase.\n");
        return 1;
    }

    fprintf(stderr, "Unknown command '%s'\n", cmd);
    return 1;
}
