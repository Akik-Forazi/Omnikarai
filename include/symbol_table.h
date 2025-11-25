#ifndef OMNI_SYMBOL_TABLE_H
#define OMNI_SYMBOL_TABLE_H

#include <llvm-c/Core.h>

// A simple key-value pair for our symbol table
typedef struct Symbol {
    char* name;
    LLVMValueRef value; // This will be a pointer to the memory location (alloca)
    struct Symbol* next; // For handling hash collisions
} Symbol;

// A basic hash map implementation for the symbol table
typedef struct {
    Symbol** buckets;
    unsigned int capacity;
} SymbolTable;

SymbolTable* symbol_table_create(unsigned int capacity);
void symbol_table_destroy(SymbolTable* table);
void symbol_table_set(SymbolTable* table, const char* name, LLVMValueRef value);
LLVMValueRef symbol_table_get(SymbolTable* table, const char* name);

#endif // OMNI_SYMBOL_TABLE_H
