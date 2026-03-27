#ifndef OMNIKARAI_AST_H
#define OMNIKARAI_AST_H

#include "lexer.h"

// --- FORWARD DECLARATIONS ---
struct AST_Statement;
typedef struct AST_Statement_Block AST_Statement_Block;

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
    USE_STATEMENT,

    // Expressions
    IDENTIFIER,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    BOOLEAN_LITERAL,
    NIL_LITERAL,
    ARRAY_LITERAL,      // Phase 4: [a, b, c]
    MAP_LITERAL,        // Phase 4: {"k": v}
    INDEX_EXPRESSION,   // Phase 4: obj[i]
    INFIX_EXPRESSION,
    PREFIX_EXPRESSION,
    CALL_EXPRESSION,
    MEMBER_ACCESS_EXPRESSION,
    FN_LITERAL,
    EMPTY_EXPRESSION
} AST_NodeType;

// --- BASE NODES ---
typedef struct AST_Node   { AST_NodeType type; } AST_Node;

typedef struct AST_Statement {
    AST_NodeType type;
    Token token;
} AST_Statement;

typedef struct AST_Expression {
    AST_NodeType type;
    Token token;
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
    double value;
} AST_Expression_FloatLiteral;

typedef struct {
    AST_Expression base;
    char* value;
} AST_Expression_StringLiteral;

typedef struct {
    AST_Expression base;
    int value;
} AST_Expression_Boolean;

typedef struct {
    AST_Expression base;
} AST_Expression_NilLiteral;

// Phase 4: list literal  [a, b, c]
typedef struct {
    AST_Expression base;
    AST_Expression** elements;
    int element_count;
} AST_Expression_ArrayLiteral;

// Phase 4: map entry  key: value
typedef struct {
    AST_Expression* key;
    AST_Expression* value;
} AST_MapEntry;

// Phase 4: dict literal  {"k": v, ...}
typedef struct {
    AST_Expression base;
    AST_MapEntry** entries;
    int entry_count;
} AST_Expression_MapLiteral;

// Phase 4: index expression  obj[i]
typedef struct {
    AST_Expression base;
    AST_Expression* left;   // the collection
    AST_Expression* index;  // the index expression
} AST_Expression_Index;

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
    AST_Expression* function;
    AST_Expression** arguments;
    int argument_count;
} AST_Expression_Call;

// member access:  obj.field
typedef struct {
    AST_Expression base;
    AST_Expression* object;   // the left side (e.g. identifier "time")
    char*           member;   // the field name (e.g. "now")
} AST_Expression_MemberAccess;

typedef struct AST_Expression_FnLiteral {
    AST_Expression base;
    AST_Expression_Identifier** parameters;
    int parameter_count;
    AST_Statement_Block* body;
} AST_Expression_FnLiteral;

typedef struct {
    AST_Expression base;
} AST_Expression_Empty;

// --- STATEMENTS ---

typedef struct {
    AST_Statement base;
    AST_Expression_Identifier* name;
    AST_Expression* value;
} AST_Statement_Set;

typedef struct AST_Statement_Block {
    AST_Statement base;
    AST_Statement** statements;
    int statement_count;
} AST_Statement_Block;

typedef struct {
    AST_Statement base;
    AST_Expression_Identifier* name;
    AST_Expression_Identifier** parameters;
    int parameter_count;
    AST_Statement_Block* body;
} AST_Statement_FnDef;

typedef struct {
    AST_Statement base;
    AST_Expression_Identifier* name;
    AST_Statement_Block* body;
} AST_Statement_ClassDef;

typedef struct {
    AST_Statement base;
    AST_Expression* condition;
    AST_Statement_Block* consequence;
    AST_Statement* alternative;
} AST_Statement_If;

typedef struct {
    AST_Statement base;
    AST_Expression* condition;
    AST_Statement_Block* body;
} AST_Statement_While;

typedef struct {
    AST_Statement base;
    AST_Expression_Identifier* iterator;
    AST_Expression* iterable;
    AST_Statement_Block* body;
} AST_Statement_For;

typedef struct {
    AST_Statement base;
    AST_Expression* pattern;
    AST_Statement_Block* consequence;
} AST_Statement_MatchCase;

typedef struct {
    AST_Statement base;
    AST_Expression* value;
    AST_Statement_MatchCase** cases;
    int case_count;
} AST_Statement_Match;

typedef struct {
    AST_Statement base;
    AST_Expression* return_value;
} AST_Statement_Return;

typedef struct {
    AST_Statement base;
    AST_Expression* expression;
} AST_Statement_Expression;

// use <module_name>
typedef struct {
    AST_Statement base;
    char* module_name;   // e.g. "time"
    char* alias;         // e.g. from "use time as t" → "t" (NULL if no alias)
} AST_Statement_Use;

// --- PROGRAM ---
typedef struct {
    AST_Statement** statements;
    int statement_count;
} AST_Program;

#endif // OMNIKARAI_AST_H
