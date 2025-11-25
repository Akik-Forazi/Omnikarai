#ifndef OMNIKARAI_OBJECT_H
#define OMNIKARAI_OBJECT_H

#include "ast.h" // Include ast.h for AST_Expression_Identifier and AST_Statement_Block definitions

// Forward declarations for types defined in other headers to break circular dependencies
typedef struct Environment Environment;

// --- Object System ---
typedef enum {
    OBJ_INTEGER,
    OBJ_BOOLEAN,
    OBJ_NIL,
    OBJ_STRING,
    OBJ_FUNCTION,
} ObjectType;

typedef struct ObjectFunction {
    AST_Expression_Identifier** parameters;
    int parameter_count;
    AST_Statement_Block* body;
    Environment* env;
} ObjectFunction;

typedef struct Object {
    ObjectType type;
    union {
        long long integer;
        int boolean;
        char* string;
        ObjectFunction* function;
    } value;
} Object;

#endif //OMNIKARAI_OBJECT_H
