#ifndef OMNIKARAI_PARSER_H
#define OMNIKARAI_PARSER_H

#include "lexer.h"
#include "ast.h"

// Forward declare Parser for use in function pointer types
typedef struct Parser Parser;

// --- Pratt Parser Function Types ---
typedef AST_Expression* (*prefix_parse_fn)(Parser* p);
typedef AST_Expression* (*infix_parse_fn)(Parser* p, AST_Expression* left);

// Parser structure holds the state of our parser
struct Parser {
    Lexer* lexer; // Pointer to the lexer instance
    Token currentToken;
    Token peekToken;

    // For error handling
    char** errors;
    int error_count;

    // Pratt parser function tables
    prefix_parse_fn prefix_parse_fns[256]; // Assuming max 256 token types
    infix_parse_fn infix_parse_fns[256];
};

// --- Parser Public API ---
Parser* new_parser(Lexer* l);
void free_parser(Parser* p); // Good practice to have a way to free memory
AST_Program* parse_program(Parser* p);

#endif //OMNIKARAI_PARSER_H

