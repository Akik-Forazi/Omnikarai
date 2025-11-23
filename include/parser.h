#ifndef OMNIKARAI_PARSER_H
#define OMNIKARAI_PARSER_H

#include "lexer.h"
#include "ast.h"

// --- PRECEDENCE --- //
typedef enum {
    LOWEST,
    EQUALS,      // ==
    LESSGREATER, // > or <
    SUM,         // +
    PRODUCT,     // *
    PREFIX,      // -X or !X
    CALL,        // myFunction(X)
} Precedence;

// Parser structure holds the state of our parser
typedef struct {
    Lexer* lexer; // Pointer to the lexer instance
    Token currentToken;
    Token peekToken;
} Parser;

// --- Parser API ---
Parser* new_parser(Lexer* l);
void parser_next_token(Parser* p);
AST_Program* parse_program(Parser* p);
AST_Statement* parse_statement(Parser* p);
AST_Statement* parse_let_statement(Parser* p);
AST_Expression* parse_expression(Parser* p, Precedence precedence);
AST_Expression* parse_identifier(Parser* p);
AST_Expression* parse_integer_literal(Parser* p);
AST_Expression* parse_boolean(Parser* p);
AST_Expression* parse_string_literal(Parser* p);
AST_Expression* parse_function_literal(Parser* p);
AST_Expression* parse_call_expression(Parser* p, AST_Expression* function);

#endif //OMNIKARAI_PARSER_H
