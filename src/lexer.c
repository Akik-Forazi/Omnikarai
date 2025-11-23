#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "lexer.h"

// Forward declarations
void read_char(Lexer* l);
char* read_identifier(Lexer* l);
char* read_number(Lexer* l);
void skip_whitespace(Lexer* l);
char peek_char(Lexer* l);
char* read_string(Lexer* l);

void lexer_init(Lexer* l, const char* source_code) {
    l->input = source_code;
    l->position = 0;
    l->readPosition = 0;
    read_char(l); // Initialize the first character
}

void read_char(Lexer* l) {
    if (l->readPosition >= strlen(l->input)) {
        l->ch = 0; // NUL character, signifies EOF
    } else {
        l->ch = l->input[l->readPosition];
    }
    l->position = l->readPosition;
    l->readPosition += 1;
}

char peek_char(Lexer* l) {
    if (l->readPosition >= strlen(l->input)) {
        return 0;
    } else {
        return l->input[l->readPosition];
    }
}


void skip_whitespace(Lexer* l) {
    while (isspace(l->ch)) {
        read_char(l);
    }
}

// Helper to check if a character is a letter or underscore
int is_letter(char ch) {
    return isalpha(ch) || ch == '_';
}

// Looks up an identifier to see if it's a keyword
TokenType lookup_ident(char* ident) {
    if (strcmp(ident, "fn") == 0) return TOKEN_FN;
    if (strcmp(ident, "let") == 0) return TOKEN_LET;
    if (strcmp(ident, "use") == 0) return TOKEN_USE;
    if (strcmp(ident, "if") == 0) return TOKEN_IF;
    if (strcmp(ident, "else") == 0) return TOKEN_ELSE;
    if (strcmp(ident, "for") == 0) return TOKEN_FOR;
    if (strcmp(ident, "in") == 0) return TOKEN_IN;
    if (strcmp(ident, "while") == 0) return TOKEN_WHILE;
    if (strcmp(ident, "return") == 0) return TOKEN_RETURN;
    if (strcmp(ident, "true") == 0) return TOKEN_TRUE;
    if (strcmp(ident, "false") == 0) return TOKEN_FALSE;
    if (strcmp(ident, "nil") == 0) return TOKEN_NIL;
    if (strcmp(ident, "string") == 0) return TOKEN_TYPE_STRING;
    if (strcmp(ident, "int") == 0) return TOKEN_TYPE_INT;
    if (strcmp(ident, "float") == 0) return TOKEN_TYPE_FLOAT;
    if (strcmp(ident, "bool") == 0) return TOKEN_TYPE_BOOL;
    return TOKEN_IDENT;
}

char* read_identifier(Lexer* l) {
    int start_pos = l->position;
    while (is_letter(l->ch)) {
        read_char(l);
    }
    int length = l->position - start_pos;
    char* ident = malloc(length + 1);
    strncpy(ident, &l->input[start_pos], length);
    ident[length] = '\0';
    return ident;
}

char* read_number(Lexer* l) {
    int start_pos = l->position;
    while (isdigit(l->ch)) {
        read_char(l);
    }
    int length = l->position - start_pos;
    char* num = malloc(length + 1);
    strncpy(num, &l->input[start_pos], length);
    num[length] = '\0';
    return num;
}

char* read_string(Lexer* l) {
    int start_pos = l->position + 1; // +1 to skip the opening "
    do {
        read_char(l);
    } while (l->ch != '"' && l->ch != 0);
    
    int length = l->position - start_pos;
    char* str = malloc(length + 1);
    strncpy(str, &l->input[start_pos], length);
    str[length] = '\0';
    return str;
}

Token get_next_token(Lexer* l) {
    Token tok;

    skip_whitespace(l);

    switch (l->ch) {
        case '=':
            if (peek_char(l) == '=') {
                read_char(l);
                tok = (Token){TOKEN_EQ, "=="};
            } else {
                tok = (Token){TOKEN_ASSIGN, "="};
            }
            break;
        case '!':
            if (peek_char(l) == '=') {
                read_char(l);
                tok = (Token){TOKEN_NOT_EQ, "!="};
            } else {
                tok = (Token){TOKEN_BANG, "!"}; // ! is a valid prefix operator
            }
            break;
        case '-': tok = (Token){TOKEN_MINUS, "-"}; break; // - is a valid prefix operator, and also infix
        case '.':
            if (peek_char(l) == '.') {
                read_char(l);
                tok = (Token){TOKEN_RANGE, ".."};
            } else {
                tok = (Token){TOKEN_ILLEGAL, "."};
            }
            break;
        case ';': tok = (Token){TOKEN_SEMICOLON, ";"}; break;
        case '(': tok = (Token){TOKEN_LPAREN, "("}; break;
        case ')': tok = (Token){TOKEN_RPAREN, ")"}; break;
        case '{': tok = (Token){TOKEN_LBRACE, "{"}; break;
        case '}': tok = (Token){TOKEN_RBRACE, "}"}; break;
        case ',': tok = (Token){TOKEN_COMMA, ","}; break;
        case '+': tok = (Token){TOKEN_PLUS, "+"}; break;
        case '<': tok = (Token){TOKEN_LT, "<"}; break;
        case '>': tok = (Token){TOKEN_GT, ">"}; break;
        case 0:   tok = (Token){TOKEN_EOF, ""}; break;
        default:
            if (is_letter(l->ch)) {
                tok.literal = read_identifier(l);
                tok.type = lookup_ident(tok.literal);
                return tok; // Special return because read_identifier already advanced the char
            } else if (isdigit(l->ch)) {
                tok.literal = read_number(l);
                tok.type = TOKEN_INT;
                return tok; // Special return because read_number already advanced the char
            } else {
                tok = (Token){TOKEN_ILLEGAL, ""};
            }
    }

    read_char(l); // Advance to the next character for single-char tokens
    return tok;
}