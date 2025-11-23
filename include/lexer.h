#ifndef OMNIKARAI_LEXER_H
#define OMNIKARAI_LEXER_H

// TOKEN TYPES
typedef enum {
    // SPECIAL TOKENS
    TOKEN_ILLEGAL, // A character we don't recognize
    TOKEN_EOF,     // End of File
    TOKEN_INDENT,  // Represents an increase in indentation
    TOKEN_DEDENT,  // Represents a decrease in indentation

    // IDENTIFIERS + LITERALS
    TOKEN_IDENT,   // my_variable, myFunction
    TOKEN_INT,     // 12345
    TOKEN_STRING,  // "hello world"

    // OPERATORS
    TOKEN_ASSIGN,    // =
    TOKEN_PLUS,      // +
    TOKEN_MINUS,     // -
    TOKEN_STAR,      // *
    TOKEN_SLASH,     // /
    TOKEN_LT,        // <
    TOKEN_GT,        // >
    TOKEN_EQ,        // ==
    TOKEN_NOT_EQ,    // !=
    TOKEN_GTE,       // >=
    TOKEN_LTE,       // <=

    // DELIMITERS
    TOKEN_COMMA,     // ,
    TOKEN_COLON,     // :
    TOKEN_LPAREN,    // (
    TOKEN_RPAREN,    // )
    TOKEN_LBRACKET,  // [
    TOKEN_RBRACKET,  // ]

    // KEYWORDS
    TOKEN_SET,       // set
    TOKEN_FN,        // fn
    TOKEN_CLASS,     // class
    TOKEN_IF,        // if
    TOKEN_ELIF,      // elif
    TOKEN_ELSE,      // else
    TOKEN_FOR,       // for
    TOKEN_IN,        // in
    TOKEN_WHILE,     // while
    TOKEN_RETURN,    // return
    TOKEN_USE,       // use
    TOKEN_AS,        // as
    TOKEN_MATCH,     // match
    TOKEN_CASE,      // case
    TOKEN_TRUE,      // true
    TOKEN_FALSE,     // false
    TOKEN_NIL,       // nil (like None)
} TokenType;

// TOKEN STRUCTURE
typedef struct {
    TokenType type;
    char *literal; // The actual characters of the token (e.g., "let", "my_variable", "5")
} Token;

// Lexer state
typedef struct {
    const char *input;
    size_t position;      // current position in input (points to current char)
    size_t readPosition;  // current reading position in input (after current char)
    char ch;           // current char under examination
} Lexer;

// --- Lexer API ---
// Initializes the lexer with a given source code string.
void lexer_init(Lexer* l, const char* source_code);

// Returns the next token from the source code.
Token get_next_token(Lexer* l);

#endif //OMNIKARAI_LEXER_H
