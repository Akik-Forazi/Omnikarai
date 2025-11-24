#ifndef OMNIKARAI_INTERPRETER_H
#define OMNIKARAI_INTERPRETER_H

#include "ast.h"
#include "object.h"

// --- Forward Declarations ---
typedef struct Environment Environment;

// --- Public API ---
Object* interpret(AST_Program* program);
void print_object(Object* obj);

Environment* new_environment();
Object* get_environment(Environment* env, char* name);
void set_environment(Environment* env, char* name, Object* val);

#endif //OMNIKARAI_INTERPRETER_H
