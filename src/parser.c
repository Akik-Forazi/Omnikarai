#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "ast.h"
#include "lexer.h"

// --- Function Prototypes ---
static AST_Statement* parse_statement(Parser* p);
static AST_Statement_Block* parse_block_statement(Parser* p);
static AST_Expression* parse_expression(Parser* p, int precedence);
// ... other prototypes will be added as needed

// --- Error Handling ---
static void parser_add_error(Parser* p, const char* msg) {
    // A real implementation would have a list of errors.
    fprintf(stderr, "ParseError (line %d): %s. Got %d\n", p->lexer->line_num, msg, p->currentToken.type);
}

// --- Token Management ---
static void parser_next_token(Parser* p) {
    p->currentToken = p->peekToken;
    p->peekToken = get_next_token(p->lexer);
}

static int current_token_is(Parser* p, TokenType t) {
    return p->currentToken.type == t;
}

static int peek_token_is(Parser* p, TokenType t) {
    return p->peekToken.type == t;
}

static int expect_peek(Parser* p, TokenType t) {
    if (peek_token_is(p, t)) {
        parser_next_token(p);
        return 1;
    } else {
        char err[100];
        sprintf(err, "Expected next token to be %d", t);
        parser_add_error(p, err);
        return 0;
    }
}


// --- Statement Parsers ---

static AST_Statement* parse_set_statement(Parser* p) {
    // Consumes "set"
    AST_Statement_Set* stmt = malloc(sizeof(AST_Statement_Set));
    stmt->base.type = SET_STATEMENT;
    stmt->base.token = p->currentToken;

    if (!expect_peek(p, TOKEN_IDENT)) {
        free(stmt);
        return NULL;
    }

    AST_Expression_Identifier* name = malloc(sizeof(AST_Expression_Identifier));
    name->base.type = IDENTIFIER;
    name->base.token = p->currentToken;
    name->value = strdup(p->currentToken.literal);
    stmt->name = name;

    if (!expect_peek(p, TOKEN_ASSIGN)) {
        free(name->value);
        free(name);
        free(stmt);
        return NULL;
    }
    
    parser_next_token(p); // consume '='
    
    stmt->value = parse_expression(p, 0); // TODO: Precedence LOWEST

    return (AST_Statement*)stmt;
}

static AST_Statement_Block* parse_block_statement(Parser* p) {
    AST_Statement_Block* block = malloc(sizeof(AST_Statement_Block));
    block->base.type = BLOCK_STATEMENT;
    block->base.token = p->currentToken; // The ':' token
    block->statements = NULL;
    block->statement_count = 0;

    if (!current_token_is(p, TOKEN_COLON)) {
        parser_add_error(p, "Expected ':' to start a block");
        free(block);
        return NULL;
    }

    if (!expect_peek(p, TOKEN_INDENT)) {
        parser_add_error(p, "Expected indented block after ':'");
        free(block);
        return NULL;
    }
    
    parser_next_token(p); // consume INDENT

    while(!current_token_is(p, TOKEN_DEDENT) && !current_token_is(p, TOKEN_EOF)) {
        AST_Statement* stmt = parse_statement(p);
        if (stmt) {
            block->statement_count++;
            block->statements = realloc(block->statements, block->statement_count * sizeof(AST_Statement*));
            if (!block->statements) {
                parser_add_error(p, "Memory allocation failed");
                free(block);
                return NULL;
            }
            block->statements[block->statement_count - 1] = stmt;
        }
        parser_next_token(p);
    }
    
    if (!current_token_is(p, TOKEN_DEDENT)) {
        parser_add_error(p, "Expected dedent to end block");
        // TODO: Free block contents
        free(block);
        return NULL;
    }

    return block;
}

// A simple placeholder for expression parsing.
// A real implementation requires a Pratt parser or similar.
static AST_Expression* parse_expression(Parser* p, int precedence) {
    // For now, just parse identifiers and literals
    if (current_token_is(p, TOKEN_IDENT)) {
        AST_Expression_Identifier* ident = malloc(sizeof(AST_Expression_Identifier));
        ident->base.type = IDENTIFIER;
        ident->base.token = p->currentToken;
        ident->value = strdup(p->currentToken.literal);
        return (AST_Expression*)ident;
    }
    if (current_token_is(p, TOKEN_INT)) {
        AST_Expression_IntegerLiteral* lit = malloc(sizeof(AST_Expression_IntegerLiteral));
        lit->base.type = INTEGER_LITERAL;
        lit->base.token = p->currentToken;
        lit->value = atoll(p->currentToken.literal);
        return (AST_Expression*)lit;
    }
    // ... other expression types
    parser_add_error(p, "Expression parsing not fully implemented");
    return NULL;
}


static AST_Statement* parse_statement(Parser* p) {
    switch (p->currentToken.type) {
        case TOKEN_SET:
            return parse_set_statement(p);
        // case TOKEN_FN:
        //     return parse_fn_definition(p);
        // case TOKEN_IF:
        //     return parse_if_statement(p);
        default:
            // Placeholder for expression statements
            //return parse_expression_statement(p);
            return NULL;
    }
}


// --- Public API ---

Parser* new_parser(Lexer* l) {
    Parser* p = malloc(sizeof(Parser));
    p->lexer = l;
    
    // Read two tokens, so currentToken and peekToken are both set
    parser_next_token(p);
    parser_next_token(p);

    return p;
}

AST_Program* parse_program(Parser* p) {
    AST_Program* program = malloc(sizeof(AST_Program));
    program->statements = NULL;
    program->statement_count = 0;

    while (!current_token_is(p, TOKEN_EOF)) {
        AST_Statement* stmt = parse_statement(p);
        if (stmt) {
            program->statement_count++;
            program->statements = realloc(program->statements, program->statement_count * sizeof(AST_Statement*));
            if (!program->statements) {
                parser_add_error(p, "Memory allocation failed");
                free(program);
                return NULL;
            }
            program->statements[program->statement_count - 1] = stmt;
        }
        parser_next_token(p);
    }
    
    return program;
}
