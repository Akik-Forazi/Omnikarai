#ifndef OMNIKARAI_LEXER_H
#define OMNIKARAI_LEXER_H

// TOKEN TYPES
typedef enum {
    // SPECIAL TOKENS
    TOKEN_ILLEGAL, // A character we don't recognize
    TOKEN_EOF,     // End of File

    // IDENTIFIERS + LITERALS
    TOKEN_IDENT,   // my_variable, myFunction
    TOKEN_INT,     // 12345
    TOKEN_STRING,  // "hello world"

    // OPERATORS
    TOKEN_ASSIGN,  // =
    TOKEN_PLUS,    // +
    TOKEN_MINUS,   // -
    TOKEN_STAR,    // *
    TOKEN_SLASH,   // /
    TOKEN_LT,      // <
    TOKEN_GT,      // >
    TOKEN_BANG,    // !

    // DELIMITERS
    TOKEN_COMMA,     // ,
    TOKEN_SEMICOLON, // ;

    TOKEN_LPAREN,    // (
    TOKEN_RPAREN,    // )
    TOKEN_LBRACE,    // {
    TOKEN_RBRACE,    // }

    // KEYWORDS
    TOKEN_LET,       // let
    TOKEN_FN,        // fn
    TOKEN_USE,       // use
    TOKEN_IF,        // if
    TOKEN_ELSE,      // else
    TOKEN_FOR,       // for
    TOKEN_IN,        // in
    TOKEN_WHILE,     // while
    TOKEN_RETURN,    // return
    TOKEN_TRUE,      // true
    TOKEN_FALSE,     // false
    TOKEN_NIL,       // nil

    // TYPES
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_INT,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_BOOL,

    // OPERATORS (Two-character)
    TOKEN_EQ,        // ==
    TOKEN_NOT_EQ,    // !=
    TOKEN_RANGE      // ..
} TokenType;

// TOKEN STRUCTURE
typedef struct {
    TokenType type;
    char *literal; // The actual characters of the token (e.g., "let", "my_variable", "5")
} Token;

// Lexer state
typedef struct {
    const char *input;
    int position;      // current position in input (points to current char)
    int readPosition;  // current reading position in input (after current char)
    char ch;           // current char under examination
} Lexer;

// --- Lexer API ---
// Initializes the lexer with a given source code string.
void lexer_init(Lexer* l, const char* source_code);

// Returns the next token from the source code.
Token get_next_token(Lexer* l);

#endif //OMNIKARAI_LEXER_H
