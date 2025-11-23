#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // For isdigit and isalpha

#include "parser.h"
#include "ast.h"
#include "lexer.h"

// --- PRECEDENCE ---
// This will be used to map token types to their precedence levels
// For now, we only care about prefix and infix operators.
// We'll fill this out as we add more operators.
Precedence token_precedence(TokenType type) {
    switch (type) {
        case TOKEN_EQ: return EQUALS;
        case TOKEN_NOT_EQ: return EQUALS;
        case TOKEN_LT: return LESSGREATER;
        case TOKEN_GT: return LESSGREATER;
        case TOKEN_PLUS: return SUM;
        case TOKEN_MINUS: return SUM; // Also prefix for now
        case TOKEN_SLASH: return PRODUCT;
        case TOKEN_STAR: return PRODUCT;
        default: return LOWEST;
    }
}

// --- PARSING FUNCTIONS ---
typedef AST_Expression* (*prefix_parse_fn)(Parser* p);
typedef AST_Expression* (*infix_parse_fn)(Parser* p, AST_Expression* left);

// A lookup table for prefix parsing functions
prefix_parse_fn prefix_parse_fns[TOKEN_EOF + 1] = {NULL};
infix_parse_fn infix_parse_fns[TOKEN_EOF + 1] = {NULL};

void register_prefix(TokenType type, prefix_parse_fn fn) {
    prefix_parse_fns[type] = fn;
}

void register_infix(TokenType type, infix_parse_fn fn) {
    infix_parse_fns[type] = fn;
}

// --- ERROR HANDLING ---
void parser_add_error(Parser* p, const char* msg) {
    fprintf(stderr, "Parser Error: %s at token %d (Literal: %s)\n", 
            msg, p->currentToken.type, p->currentToken.literal);
}

// Helper function prototypes (defined in this file)
AST_Statement* parse_statement(Parser* p);
AST_Statement* parse_let_statement(Parser* p);
AST_Expression* parse_expression(Parser* p, Precedence precedence);
AST_Expression* parse_identifier(Parser* p);
AST_Expression* parse_integer_literal(Parser* p);
AST_Expression* parse_boolean(Parser* p);
AST_Expression* parse_string_literal(Parser* p);
AST_Expression* parse_prefix_expression(Parser* p);
AST_Expression* parse_function_literal(Parser* p);
AST_Expression** parse_call_arguments(Parser* p);
AST_Expression* parse_call_expression(Parser* p, AST_Expression* function);

int expect_peek(Parser* p, TokenType t);
int current_token_is(Parser* p, TokenType t);
int peek_token_is(Parser* p, TokenType t);
Precedence peek_precedence(Parser* p);
Precedence current_precedence(Parser* p);

// The core function to advance the tokens
void parser_next_token(Parser* p) {
    p->currentToken = p->peekToken;
    p->peekToken = get_next_token(p->lexer);
}

Parser* new_parser(Lexer* l) {
    Parser* p = malloc(sizeof(Parser));
    p->lexer = l;

    // Read two tokens, so currentToken and peekToken are both set
    parser_next_token(p);
    parser_next_token(p);

    // Register prefix parsing functions
    register_prefix(TOKEN_IDENT, parse_identifier);
    register_prefix(TOKEN_INT, parse_integer_literal);
    register_prefix(TOKEN_BANG, parse_prefix_expression);
    register_prefix(TOKEN_MINUS, parse_prefix_expression);
    register_prefix(TOKEN_TRUE, parse_boolean);
    register_prefix(TOKEN_FALSE, parse_boolean);
    register_prefix(TOKEN_STRING, parse_string_literal);
    register_prefix(TOKEN_FN, parse_function_literal);
    
    // Register infix parsing functions
    register_infix(TOKEN_LPAREN, parse_call_expression);

    return p;
}

Precedence peek_precedence(Parser* p) {
    return token_precedence(p->peekToken.type);
}

Precedence current_precedence(Parser* p) {
    return token_precedence(p->currentToken.type);
}

int current_token_is(Parser* p, TokenType t) {
    return p->currentToken.type == t;
}

int peek_token_is(Parser* p, TokenType t) {
    return p->peekToken.type == t;
}

int expect_peek(Parser* p, TokenType t) {
    if (peek_token_is(p, t)) {
        parser_next_token(p);
        return 1;
    } else {
        parser_add_error(p, "Expected next token to be of a different type");
        return 0;
    }
}

AST_Expression* parse_identifier(Parser* p) {
    AST_Expression_Identifier* ident = malloc(sizeof(AST_Expression_Identifier));
    ident->type = AST_IDENTIFIER;
    ident->token = p->currentToken;
    ident->value = malloc(strlen(p->currentToken.literal) + 1);
    strcpy(ident->value, p->currentToken.literal);
    return (AST_Expression*)ident;
}

AST_Expression* parse_integer_literal(Parser* p) {
    AST_Expression_IntegerLiteral* lit = malloc(sizeof(AST_Expression_IntegerLiteral));
    lit->type = AST_INTEGER_LITERAL;
    lit->token = p->currentToken;
    lit->value = atoll(p->currentToken.literal); // Convert string to long long
    return (AST_Expression*)lit;
}

static AST_Expression* parse_boolean(Parser* p) {
    AST_Expression_Boolean* b = malloc(sizeof(AST_Expression_Boolean));
    b->type = AST_BOOLEAN_LITERAL;
    b->token = p->currentToken;
    b->value = current_token_is(p, TOKEN_TRUE) ? 1 : 0;
    return (AST_Expression*)b;
}

static AST_Expression* parse_string_literal(Parser* p) {
    AST_Expression_StringLiteral* str_lit = malloc(sizeof(AST_Expression_StringLiteral));
    str_lit->type = AST_STRING_LITERAL;
    str_lit->token = p->currentToken;
    str_lit->value = malloc(strlen(p->currentToken.literal) + 1);
    strcpy(str_lit->value, p->currentToken.literal);
    return (AST_Expression*)str_lit;
}

static AST_Expression* parse_function_literal(Parser* p) {
    AST_Expression_FunctionLiteral* fn_lit = malloc(sizeof(AST_Expression_FunctionLiteral));
    fn_lit->type = AST_FUNCTION_LITERAL;
    fn_lit->token = p->currentToken; // The 'fn' token
    fn_lit->parameters = NULL;
    fn_lit->parameter_count = 0;

    if (!expect_peek(p, TOKEN_LPAREN)) {
        free(fn_lit);
        return NULL;
    }

    // TODO: Parse function parameters
    while (!current_token_is(p, TOKEN_RPAREN) && !current_token_is(p, TOKEN_EOF)) {
        parser_next_token(p);
    }

    if (!expect_peek(p, TOKEN_LBRACE)) {
        free(fn_lit);
        return NULL;
    }
    // TODO: Parse function body (BlockStatement)
    while (!current_token_is(p, TOKEN_RBRACE) && !current_token_is(p, TOKEN_EOF)) {
        parser_next_token(p);
    }

    return (AST_Expression*)fn_lit;
}

static AST_Expression** parse_call_arguments(Parser* p) {
    AST_Expression** args = NULL;
    int arg_count = 0;

    if (peek_token_is(p, TOKEN_RPAREN)) {
        parser_next_token(p);
        return args;
    }

    parser_next_token(p);
    args = realloc(args, (arg_count + 1) * sizeof(AST_Expression*));
    args[arg_count++] = parse_expression(p, LOWEST);

    while (peek_token_is(p, TOKEN_COMMA)) {
        parser_next_token(p); // Move past comma
        parser_next_token(p); // Move to next argument
        args = realloc(args, (arg_count + 1) * sizeof(AST_Expression*));
        args[arg_count++] = parse_expression(p, LOWEST);
    }

    if (!expect_peek(p, TOKEN_RPAREN)) {
        // TODO: Free arguments
        return NULL;
    }

    return args; // The caller will assign arg_count
}

static AST_Expression* parse_call_expression(Parser* p, AST_Expression* function) {
    AST_Expression_Call* call_expr = malloc(sizeof(AST_Expression_Call));
    call_expr->type = AST_CALL_EXPRESSION;
    call_expr->token = p->currentToken; // The '(' token
    call_expr->function = function;
    call_expr->arguments = parse_call_arguments(p);
    call_expr->argument_count = 0; // TODO: Get actual count from parse_call_arguments
    if (call_expr->arguments == NULL && !peek_token_is(p, TOKEN_RPAREN)) {
        // Error in parsing arguments, or missing RPAREN
        free(call_expr);
        return NULL;
    }
    if (call_expr->arguments != NULL) {
        // Count arguments (this is a simple placeholder for now)
        int i = 0;
        while (call_expr->arguments[i] != NULL) { i++; }
        call_expr->argument_count = i;
    }
    return (AST_Expression*)call_expr;
}

static AST_Expression* parse_prefix_expression(Parser* p) {
    AST_Expression_Prefix* expr = malloc(sizeof(AST_Expression_Prefix));
    expr->type = AST_PREFIX_EXPRESSION;
    expr->token = p->currentToken;
    expr->operator = malloc(strlen(p->currentToken.literal) + 1);
    strcpy(expr->operator, p->currentToken.literal);

    parser_next_token(p);
    expr->right = parse_expression(p, PREFIX);
    return (AST_Expression*)expr;
}

static AST_Expression* parse_expression(Parser* p, Precedence precedence) {
    prefix_parse_fn prefix = prefix_parse_fns[p->currentToken.type];
    if (prefix == NULL) {
        parser_add_error(p, "No prefix parsing function for current token");
        return NULL;
    }
    AST_Expression* left_expr = prefix(p);

    while (!peek_token_is(p, TOKEN_SEMICOLON) && precedence < peek_precedence(p)) {
        infix_parse_fn infix = infix_parse_fns[p->peekToken.type];
        if (infix == NULL) {
            return left_expr;
        }
        parser_next_token(p);
        left_expr = infix(p, left_expr);
    }

    return left_expr;
}

static AST_Statement* parse_let_statement(Parser* p) {
    AST_Statement_Let* stmt = malloc(sizeof(AST_Statement_Let));
    stmt->type = AST_LET_STATEMENT;
    stmt->token = p->currentToken; // The 'let' token

    if (!expect_peek(p, TOKEN_IDENT)) {
        free(stmt);
        return NULL;
    }
    
    stmt->name = parse_identifier(p);

    if (!expect_peek(p, TOKEN_ASSIGN)) {
        // TODO: Free the identifier
        free(((AST_Expression_Identifier*)stmt->name)->value);
        free(stmt->name);
        free(stmt);
        return NULL;
    }

    parser_next_token(p); // Move past the assignment operator
    stmt->value = parse_expression(p, LOWEST); // Parse the expression for the value

    if (peek_token_is(p, TOKEN_SEMICOLON)) {
        parser_next_token(p);
    }

    return (AST_Statement*)stmt;
}

static AST_Statement* parse_statement(Parser* p) {
    switch (p->currentToken.type) {
        case TOKEN_LET:
            return parse_let_statement(p);
        default:
            return NULL;
    }
}

AST_Program* parse_program(Parser* p) {
    AST_Program* program = malloc(sizeof(AST_Program));
    program->statements = NULL;
    program->statement_count = 0;

    while (p->currentToken.type != TOKEN_EOF) {
        AST_Statement* stmt = parse_statement(p);

        if (stmt != NULL) {
            program->statement_count++;
            program->statements = realloc(program->statements, program->statement_count * sizeof(AST_Statement*));
            program->statements[program->statement_count - 1] = stmt;
        }
        parser_next_token(p);
    }
    
    return program;
}