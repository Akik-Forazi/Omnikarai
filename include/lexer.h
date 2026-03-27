#ifndef OMNIKARAI_LEXER_H
#define OMNIKARAI_LEXER_H

typedef enum {
    // SPECIAL
    TOKEN_ILLEGAL,
    TOKEN_EOF,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_NL,

    // LITERALS
    TOKEN_IDENT,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_FSTRING,   // f"Hello {name}!"

    // OPERATORS
    TOKEN_ASSIGN,        // =
    TOKEN_PLUS,          // +
    TOKEN_MINUS,         // -
    TOKEN_STAR,          // *
    TOKEN_SLASH,         // /
    TOKEN_LT,            // <
    TOKEN_GT,            // >
    TOKEN_EQ,            // ==
    TOKEN_NOT_EQ,        // !=
    TOKEN_GTE,           // >=
    TOKEN_LTE,           // <=
    TOKEN_POWER,         // **
    TOKEN_PLUS_ASSIGN,   // +=
    TOKEN_MINUS_ASSIGN,  // -=
    TOKEN_STAR_ASSIGN,   // *=
    TOKEN_SLASH_ASSIGN,  // /=
    TOKEN_ARROW,         // ->

    // DELIMITERS
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_PERCENT,
    TOKEN_DOT,

    // KEYWORDS
    TOKEN_SET,
    TOKEN_FN,
    TOKEN_CLASS,
    TOKEN_IF,
    TOKEN_ELIF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_IN,
    TOKEN_WHILE,
    TOKEN_RETURN,
    TOKEN_USE,
    TOKEN_AS,
    TOKEN_MATCH,
    TOKEN_CASE,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NIL,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_SELF,
    TOKEN_CONST,     // const PI = 3.14
    TOKEN_EXTENDS,   // class Dog extends Animal
    TOKEN_RAISE,     // raise Error("msg")
    TOKEN_TRY,       // try:
    TOKEN_EXCEPT,    // except:
    TOKEN_IMPORT,    // import (alias for use)
    TOKEN_FROM,      // from x import y
} OmniTokenType;

typedef struct {
    OmniTokenType type;
    char *literal;
    int  line;
    int  col;
} Token;

typedef struct {
    const char *input;
    size_t input_len;
    size_t position;
    size_t readPosition;
    char ch;
    int at_bol;
    int line_num;
    int col_num;
    int* indent_stack;
    int indent_level;
    Token* pending_tokens;
    int pending_count;
} Lexer;

void lexer_init(Lexer* l, const char* source_code);
Token get_next_token(Lexer* l);

#endif
