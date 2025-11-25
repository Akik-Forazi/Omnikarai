#ifndef OMNIKARAI_AST_H
#define OMNIKARAI_AST_H

#include "lexer.h"

// --- FORWARD DECLARATIONS ---
struct AST_Statement;
typedef struct AST_Statement_Block AST_Statement_Block; // Forward declaration

// --- NODE TYPES ---
typedef enum {
    // Statements
    SET_STATEMENT,
    RETURN_STATEMENT,
    EXPRESSION_STATEMENT,
    BLOCK_STATEMENT,
    FN_DEFINITION,
    CLASS_DEFINITION,
    IF_STATEMENT,
    WHILE_STATEMENT,
    FOR_STATEMENT,
    MATCH_STATEMENT,
    MATCH_CASE_STATEMENT,
    
    // Expressions
    IDENTIFIER,
    INTEGER_LITERAL,
    STRING_LITERAL,
    BOOLEAN_LITERAL,
    NIL_LITERAL,
    ARRAY_LITERAL,
    MAP_LITERAL,
    INFIX_EXPRESSION,
    PREFIX_EXPRESSION,
    CALL_EXPRESSION,
    MEMBER_ACCESS_EXPRESSION, // For obj.property
    FN_LITERAL, // New: Function literal as an expression
    EMPTY_EXPRESSION // For temporary empty block expressions like {}
} AST_NodeType;


// --- BASE NODES ---
// A generic node type
typedef struct AST_Node {
    AST_NodeType type;
} AST_Node;

typedef struct AST_Statement {
    AST_NodeType type;
    Token token; // The primary token of the statement (e.g., TOKEN_SET)
} AST_Statement;

typedef struct AST_Expression {
    AST_NodeType type;
    Token token; // The primary token of the expression
} AST_Expression;


// --- EXPRESSIONS ---

typedef struct {
    AST_Expression base;
    char* value;
} AST_Expression_Identifier;

typedef struct {
    AST_Expression base;
    long long value;
} AST_Expression_IntegerLiteral;

typedef struct {
    AST_Expression base;
    char* value;
} AST_Expression_StringLiteral;

typedef struct {
    AST_Expression base;
    int value; // 1 for true, 0 for false
} AST_Expression_Boolean;

typedef struct {
    AST_Expression base;
    // No value needed for nil
} AST_Expression_NilLiteral;

typedef struct {
    AST_Expression base;
    AST_Expression** elements;
    int element_count;
} AST_Expression_ArrayLiteral;

typedef struct {
    AST_Expression* key;
    AST_Expression* value;
} AST_MapEntry;

typedef struct {
    AST_Expression base;
    AST_MapEntry** entries;
    int entry_count;
} AST_Expression_MapLiteral;

typedef struct {
    AST_Expression base;
    AST_Expression* left;
    char* operator;
    AST_Expression* right;
} AST_Expression_Infix;

typedef struct {
    AST_Expression base;
    char* operator;
    AST_Expression* right;
} AST_Expression_Prefix;

typedef struct {
    AST_Expression base;
    AST_Expression* function; // Identifier or other expression
    AST_Expression** arguments;
    int argument_count;
} AST_Expression_Call;

typedef struct AST_Expression_FnLiteral {
    AST_Expression base;
    AST_Expression_Identifier** parameters;
    int parameter_count;
    AST_Statement_Block* body;
} AST_Expression_FnLiteral;

typedef struct {
    AST_Expression base;
    // No specific fields needed for an empty expression beyond the base.
} AST_Expression_Empty; // New struct definition


// --- STATEMENTS ---

// `set <name> = <value>`
typedef struct {
    AST_Statement base;
    AST_Expression_Identifier* name;
    AST_Expression* value;
} AST_Statement_Set;

// A block of statements, e.g., an indented block
typedef struct AST_Statement_Block {
    AST_Statement base;
    AST_Statement** statements;
    int statement_count;
} AST_Statement_Block;

// `fn <name>(<params>): <block>`
typedef struct {
    AST_Statement base;
    AST_Expression_Identifier* name;
    AST_Expression_Identifier** parameters;
    int parameter_count;
    AST_Statement_Block* body;
} AST_Statement_FnDef;

// `class <name>: <block>`
typedef struct {
    AST_Statement base;
    AST_Expression_Identifier* name;
    AST_Statement_Block* body;
} AST_Statement_ClassDef;

// `if <condition>: <consequence> else: <alternative>`
typedef struct {
    AST_Statement base;
    AST_Expression* condition;
    AST_Statement_Block* consequence;
    AST_Statement* alternative; // Can be another IF_STATEMENT or a BLOCK_STATEMENT
} AST_Statement_If;

// `while <condition>: <body>`
typedef struct {
    AST_Statement base;
    AST_Expression* condition;
    AST_Statement_Block* body;
} AST_Statement_While;

// `for <iterator> in <iterable>: <body>`
typedef struct {
    AST_Statement base;
    AST_Expression_Identifier* iterator;
    AST_Expression* iterable;
    AST_Statement_Block* body;
} AST_Statement_For;

// A single case in a match statement: `case <pattern>: <consequence>`
typedef struct {
    AST_Statement base; // Inherit from AST_Statement
    AST_Expression* pattern;
    AST_Statement_Block* consequence;
} AST_Statement_MatchCase;

// `match <value>: ...cases`
typedef struct {
    AST_Statement base;
    AST_Expression* value;
    AST_Statement_MatchCase** cases;
    int case_count;
} AST_Statement_Match;

// `return <value>`
typedef struct {
    AST_Statement base;
    AST_Expression* return_value;
} AST_Statement_Return;

// A statement that is just an expression, e.g. a function call `print()`
typedef struct {
    AST_Statement base;
    AST_Expression* expression;
} AST_Statement_Expression;


// --- PROGRAM ---
// The root of every AST our parser produces.
typedef struct {
    AST_Statement** statements;
    int statement_count;
} AST_Program;


// --- Helper Functions (to be defined in ast.c) ---
// e.g., void free_program(AST_Program* program);

#endif //OMNIKARAI_AST_H