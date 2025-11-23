#ifndef OMNIKARAI_AST_H
#define OMNIKARAI_AST_H

#include "lexer.h"

// --- FORWARD DECLARATIONS ---
// None needed now

// --- NODE TYPES ---
typedef enum {
    AST_LET_STATEMENT,
    // ... other statement types like RETURN, etc.
} AST_StatementType;

typedef enum {
    AST_IDENTIFIER,
    AST_INTEGER_LITERAL,
    AST_PREFIX_EXPRESSION,
    AST_BOOLEAN_LITERAL,
    AST_STRING_LITERAL,
    AST_FUNCTION_LITERAL,
    AST_CALL_EXPRESSION,
    // ... other expression types like INFIX_EXPRESSION, BOOLEAN, STRING_LITERAL, etc.
} AST_ExpressionType;


// --- BASE NODES ---
// Every node in our AST will have a `type` field.
typedef struct {
    AST_StatementType type;
} AST_Statement;

typedef struct {
    AST_ExpressionType type;
} AST_Expression;


// --- STATEMENTS ---
// `let <name> = <value>;`
typedef struct {
    AST_StatementType type; // Always AST_LET_STATEMENT
    Token token; // The `let` token
    AST_Expression* name;
    AST_Expression* value;
} AST_Statement_Let;


// --- EXPRESSIONS ---
// An identifier, e.g., `x`, `my_var`
typedef struct {
    AST_ExpressionType type; // Always AST_IDENTIFIER
    Token token; // The `TOKEN_IDENT` token
    char* value;
} AST_Expression_Identifier;

// An integer literal, e.g., `5`, `100`
typedef struct {
    AST_ExpressionType type; // Always AST_INTEGER_LITERAL
    Token token; // The `TOKEN_INT` token
    long long value;
} AST_Expression_IntegerLiteral;

// A boolean literal, e.g., `true`, `false`
typedef struct {
    AST_ExpressionType type; // Always AST_BOOLEAN_LITERAL
    Token token; // The `TOKEN_TRUE` or `TOKEN_FALSE` token
    int value;   // 1 for true, 0 for false
} AST_Expression_Boolean;

// A string literal, e.g., `"hello"`
typedef struct {
    AST_ExpressionType type; // Always AST_STRING_LITERAL
    Token token; // The `TOKEN_STRING` token
    char* value;
} AST_Expression_StringLiteral;

// A prefix expression, e.g., `!true`, `-5`
typedef struct {
    AST_ExpressionType type; // Always AST_PREFIX_EXPRESSION
    Token token; // The operator token, e.g., `!` or `-`
    char* operator; // The operator as a string
    AST_Expression* right; // The expression to which the operator applies
} AST_Expression_Prefix;

// A function literal, e.g., `fn(x, y) { x + y; }`
typedef struct {
    AST_ExpressionType type; // Always AST_FUNCTION_LITERAL
    Token token; // The `fn` token
    AST_Expression_Identifier** parameters;
    int parameter_count;
    // TODO: BlockStatement* body;
} AST_Expression_FunctionLiteral;

// A function call expression, e.g., `add(5, 10)`
typedef struct {
    AST_ExpressionType type; // Always AST_CALL_EXPRESSION
    Token token; // The `(` token
    AST_Expression* function; // The function being called (Identifier or FunctionLiteral)
    AST_Expression** arguments;
    int argument_count;
} AST_Expression_Call;


// --- PROGRAM ---
// The root of every AST our parser produces.
// A program is just a sequence of statements.
typedef struct {
    AST_Statement** statements;
    int statement_count;
} AST_Program;


#endif //OMNIKARAI_AST_H
