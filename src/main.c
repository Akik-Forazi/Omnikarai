// ============================================================
//  OMNIKARAI Compiler — omnicc  v5.0
//  Windows x64 — No LLVM — No runtime dependency
//
//  Usage:
//    omnicc run   [--quiet] [--beta] <file.ok>
//    omnicc build [--quiet] [--beta] <file.ok>   (PE writer — coming)
//    omnicc dump  [--quiet] [--beta] <file.ok>
//    omnicc check [--quiet] [--beta] <file.ok>   (parse only, no run)
//
//  Flags:
//    --quiet  suppress [omnicc] diagnostic lines
//    --beta   enable verbose codegen + runtime debug traces
//    --ut     extension/unit-test mode: no [omnicc] stderr, clean stdout only
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"

// ── Global flags ─────────────────────────────────────────────
static int g_quiet = 0;
static int g_ut    = 0;     // --ut: suppresses ALL [omnicc] stderr for extension use
extern int g_beta;          // defined in codegen.c

#define OMNI_LOG(...) do { if (!g_quiet && !g_ut) fprintf(stderr, __VA_ARGS__); } while(0)

// ── Version banner ───────────────────────────────────────────
static void print_version(void) {
    fprintf(stderr,
        "Omnikarai Compiler (omnicc) v5.0\n"
        "  x86-64 native code | Windows | No LLVM | No dependencies\n"
        "  Modules: time, datetime, math, os, io, sys, list, str\n"
    );
}

// ── Usage ────────────────────────────────────────────────────
static void print_usage(void) {
    print_version();
    fprintf(stderr,
        "\nUsage:\n"
        "  omnicc run   [--quiet] [--beta] <file.ok>   compile and run\n"
        "  omnicc build [--quiet] [--beta] <file.ok>   compile to .exe  (coming soon)\n"
        "  omnicc dump  [--quiet] [--beta] <file.ok>   dump x86-64 machine code bytes\n"
        "  omnicc check [--quiet] [--beta] <file.ok>   parse + check only (no run)\n"
        "  omnicc version                               show version info\n"
        "\nFlags:\n"
        "  --quiet   suppress [omnicc] diagnostic output\n"
        "  --beta    enable verbose beta debug traces (codegen + runtime)\n"
    );
}

// ── File reader ──────────────────────────────────────────────
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

// ── Compile pipeline: source → AST ──────────────────────────
static AST_Program* compile_source(const char* source, const char* filename) {
    Lexer l;
    lexer_init(&l, source);

    Parser* p = new_parser(&l);
    AST_Program* program = parse_program(p);

    if (p->error_count > 0) {
        fprintf(stderr, "\n  File \"%s\"\n", filename);
        fprintf(stderr, "ParseError: %d error(s) found\n\n", p->error_count);
        for (int i = 0; i < p->error_count; i++)
            fprintf(stderr, "  [%d] %s\n", i + 1, p->errors[i]);
        fprintf(stderr, "\n");
        free_parser(p);
        return NULL;
    }

    OMNI_LOG("[omnicc] parsed %d statement(s) from '%s'\n",
             program->statement_count, filename);
    free_parser(p);
    return program;
}

// ── omnicc run ───────────────────────────────────────────────
static int cmd_run(const char* filepath) {
    char* source = read_file(filepath);
    if (!source) return 1;

    AST_Program* program = compile_source(source, filepath);
    if (!program) { free(source); return 1; }

    CodeGen cg;
    codegen_set_source(filepath, source);
    free(source);
    codegen_init(&cg);

    if (!codegen_compile(&cg, program)) {
        fprintf(stderr, "Compilation failed.\n");
        codegen_free(&cg);
        return 1;
    }

    OMNI_LOG("[omnicc] running %zu bytes of native x86-64 code...\n", cg.code.size);

    LARGE_INTEGER t0, t1, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);

    int64_t exit_code = codegen_run(&cg);

    QueryPerformanceCounter(&t1);
    double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)freq.QuadPart;
    OMNI_LOG("[omnicc] done in %.3f ms | exit_code=%lld\n", ms, (long long)exit_code);

    codegen_free(&cg);
    return (int)exit_code;
}

// ── omnicc dump ──────────────────────────────────────────────
static int cmd_dump(const char* filepath) {
    char* source = read_file(filepath);
    if (!source) return 1;

    AST_Program* program = compile_source(source, filepath);
    if (!program) { free(source); return 1; }

    CodeGen cg;
    codegen_set_source(filepath, source);
    free(source);
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

// ── omnicc check (parse + codegen, no run) ───────────────────
static int cmd_check(const char* filepath) {
    char* source = read_file(filepath);
    if (!source) return 1;

    AST_Program* program = compile_source(source, filepath);
    if (!program) { free(source); return 1; }

    CodeGen cg;
    codegen_set_source(filepath, source);
    free(source);
    codegen_init(&cg);

    int ok = codegen_compile(&cg, program);
    if (ok) {
        fprintf(stderr, "[omnicc] check OK — %zu bytes generated, no errors\n",
                cg.code.size);
    } else {
        fprintf(stderr, "[omnicc] check FAILED\n");
    }
    codegen_free(&cg);
    return ok ? 0 : 1;
}

// ── Entry point ──────────────────────────────────────────────
int main(int argc, char** argv) {
    if (argc < 2) { print_usage(); return 1; }

    const char* cmd = argv[1];

    // version command (no file needed)
    if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0) {
        print_version(); return 0;
    }

    // Parse flags + filepath
    const char* filepath = NULL;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--quiet") == 0) {
            g_quiet = 1;
        } else if (strcmp(argv[i], "--beta") == 0) {
            g_beta = 1;
            fprintf(stderr, "[omnicc] --beta mode enabled\n");
        } else if (strcmp(argv[i], "--ut") == 0) {
            g_ut    = 1;
            g_quiet = 1;
        } else {
            filepath = argv[i];
        }
    }

    if (strcmp(cmd, "run") == 0 || strcmp(cmd, "dump") == 0 || strcmp(cmd, "check") == 0) {
        if (!filepath) {
            fprintf(stderr, "Error: no source file specified\n");
            print_usage(); return 1;
        }
    }

    if (strcmp(cmd, "run")   == 0) return cmd_run(filepath);
    if (strcmp(cmd, "dump")  == 0) return cmd_dump(filepath);
    if (strcmp(cmd, "check") == 0) return cmd_check(filepath);
    if (strcmp(cmd, "build") == 0) {
        fprintf(stderr, "omnicc build: PE writer coming in v6.0.\n"
                        "  Use 'omnicc run' for now.\n");
        return 1;
    }

    fprintf(stderr, "Unknown command '%s'\n\n", cmd);
    print_usage();
    return 1;
}
