#ifndef OMNIKARAI_COMPILER_H
#define OMNIKARAI_COMPILER_H

#include "ast.h"

// The main function to compile an AST into C code.
// Returns a string containing the generated C code.
char* compile(AST_Program* program);

#endif //OMNIKARAI_COMPILER_H
