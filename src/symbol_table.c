#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A simple hash function (djb2)
static unsigned long hash_string(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

SymbolTable* symbol_table_create(unsigned int capacity) {
    SymbolTable* table = malloc(sizeof(SymbolTable));
    if (!table) return NULL;

    table->capacity = capacity;
    table->buckets = calloc(capacity, sizeof(Symbol*));
    if (!table->buckets) {
        free(table);
        return NULL;
    }
    return table;
}

void symbol_table_destroy(SymbolTable* table) {
    if (!table) return;

    for (unsigned int i = 0; i < table->capacity; i++) {
        Symbol* current = table->buckets[i];
        while (current) {
            Symbol* next = current->next;
            free(current->name);
            free(current);
            current = next;
        }
    }
    free(table->buckets);
    free(table);
}

void symbol_table_set(SymbolTable* table, const char* name, LLVMValueRef value) {
    unsigned int index = hash_string(name) % table->capacity;

    // Check if the symbol already exists and update it
    Symbol* current = table->buckets[index];
    while (current) {
        if (strcmp(current->name, name) == 0) {
            current->value = value;
            return;
        }
        current = current->next;
    }

    // If it doesn't exist, create a new symbol
    Symbol* new_symbol = malloc(sizeof(Symbol));
    new_symbol->name = strdup(name);
    new_symbol->value = value;
    new_symbol->next = table->buckets[index];
    table->buckets[index] = new_symbol;
}

LLVMValueRef symbol_table_get(SymbolTable* table, const char* name) {
    unsigned int index = hash_string(name) % table->capacity;
    Symbol* current = table->buckets[index];
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}
