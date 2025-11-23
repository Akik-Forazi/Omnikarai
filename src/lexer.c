#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "lexer.h"

#define INDENT_STACK_SIZE 100
#define PENDING_TOKEN_SIZE 20

// --- Forward declarations ---
static void read_char(Lexer* l);
static char peek_char(Lexer* l);
static Token new_token(TokenType type, const char* literal);
static char* read_identifier(Lexer* l);
static char* read_number(Lexer* l);
static char* read_string(Lexer* l);
static TokenType lookup_ident(const char* ident);
static void handle_indentation(Lexer* l); // Changed to void

// --- Lexer Initialization ---
void lexer_init(Lexer* l, const char* source_code) {

    l->input = source_code;
    l->position = 0;
    l->readPosition = 0;
    l->ch = 0;
    l->at_bol = 1; // At beginning of line
    l->line_num = 1;

    l->indent_stack = malloc(sizeof(int) * INDENT_STACK_SIZE);
    if (l->indent_stack == NULL) {
        fprintf(stderr, "Fatal: Memory allocation failed for indent_stack\n");
        exit(1);
    }
    l->indent_level = 0;
    l->indent_stack[0] = 0;

    l->pending_tokens = malloc(sizeof(Token) * PENDING_TOKEN_SIZE);
    if (l->pending_tokens == NULL) {
        fprintf(stderr, "Fatal: Memory allocation failed for pending_tokens\n");
        exit(1);
    }
    l->pending_count = 0; // Initialize pending_count

    read_char(l); // Initialize the first character

}

// --- Helper Functions ---

static void read_char(Lexer* l) {
    if (l->readPosition >= strlen(l->input)) {
        l->ch = 0; // NUL character, signifies EOF
    } else {
        l->ch = l->input[l->readPosition];
    }
    l->position = l->readPosition;
    l->readPosition += 1;
}

static char peek_char(Lexer* l) {
    if (l->readPosition >= strlen(l->input)) {
        return 0;
    } else {
        return l->input[l->readPosition];
    }
}

static Token new_token(TokenType type, const char* literal) {
    Token tok;
    tok.type = type;
    // Note: In a real compiler, you'd copy the literal to manage memory.
    // For simplicity here, we are using string literals directly.
    tok.literal = (char*)literal;
    return tok;
}

static int is_letter(char ch) {
    return isalpha(ch) || ch == '_';
}

static void skip_inline_whitespace(Lexer* l) {
    while (l->ch == ' ' || l->ch == '\t') {
        read_char(l);
    }
}

// --- Keyword Lookup ---

static TokenType lookup_ident(const char* ident) {
    if (strcmp(ident, "set") == 0) return TOKEN_SET;
    if (strcmp(ident, "fn") == 0) return TOKEN_FN;
    if (strcmp(ident, "class") == 0) return TOKEN_CLASS;
    if (strcmp(ident, "if") == 0) return TOKEN_IF;
    if (strcmp(ident, "elif") == 0) return TOKEN_ELIF;
    if (strcmp(ident, "else") == 0) return TOKEN_ELSE;
    if (strcmp(ident, "for") == 0) return TOKEN_FOR;
    if (strcmp(ident, "in") == 0) return TOKEN_IN;
    if (strcmp(ident, "while") == 0) return TOKEN_WHILE;
    if (strcmp(ident, "return") == 0) return TOKEN_RETURN;
    if (strcmp(ident, "use") == 0) return TOKEN_USE;
    if (strcmp(ident, "as") == 0) return TOKEN_AS;
    if (strcmp(ident, "match") == 0) return TOKEN_MATCH;
    if (strcmp(ident, "case") == 0) return TOKEN_CASE;
    if (strcmp(ident, "true") == 0) return TOKEN_TRUE;
    if (strcmp(ident, "false") == 0) return TOKEN_FALSE;
    if (strcmp(ident, "nil") == 0) return TOKEN_NIL;
    return TOKEN_IDENT;
}

// --- Main Tokenization Logic ---

static char* read_identifier(Lexer* l) {

    size_t start_pos = l->position;
    while (is_letter(l->ch) || isdigit(l->ch)) {
        read_char(l);
    }
    size_t length = l->position - start_pos;
    char* ident = malloc(length + 1);
    if (ident == NULL) {
        fprintf(stderr, "Fatal: Memory allocation failed for identifier literal\n");
        exit(1);
    }
    strncpy(ident, &l->input[start_pos], length);
    ident[length] = '\0';

    return ident;
}

static char* read_number(Lexer* l) {

    size_t start_pos = l->position;
    while (isdigit(l->ch)) {
        read_char(l);
    }
    size_t length = l->position - start_pos;
    char* num = malloc(length + 1);
    if (num == NULL) {
        fprintf(stderr, "Fatal: Memory allocation failed for number literal\n");
        exit(1);
    }
    strncpy(num, &l->input[start_pos], length);
    num[length] = '\0';

    return num;
}

static char* read_string(Lexer* l) {

    char quote_char = l->ch;
    size_t start_pos = l->position + 1;
    do {
        read_char(l);
    } while (l->ch != quote_char && l->ch != 0);
    
    size_t length = l->position - start_pos;
    char* str = malloc(length + 1);
    if (str == NULL) {
        fprintf(stderr, "Fatal: Memory allocation failed for string literal\n");
        exit(1);
    }
    strncpy(str, &l->input[start_pos], length);
    str[length] = '\0';
    read_char(l); // Consume the closing quote

    return str;
}



static void handle_indentation(Lexer* l) {


    int current_indent = l->indent_stack[l->indent_level];
    int new_indent = 0;

    // Consume all leading whitespace, blank lines, and comments
    while (l->ch != 0) {
        // Skip current line if it's blank or comment
        if (l->ch == ' ' || l->ch == '\t') {
            new_indent = 0; // Recalculate indentation for current line
            while (l->ch == ' ' || l->ch == '\t') {
                new_indent += (l->ch == ' ') ? 1 : 4;
                read_char(l);
            }
        }

        if (l->ch == '\n') {
            l->line_num++;
            read_char(l);
            l->at_bol = 1; // At beginning of line
            new_indent = 0; // Reset indent for new line

            continue; // Go back and process leading whitespace of new line
        }

        if (l->ch == '#') {
            if (peek_char(l) == '|') {
                read_char(l); read_char(l); // consume '#|'
                while (l->ch != 0) {
                    if (l->ch == '|' && peek_char(l) == '#') {
                        read_char(l); read_char(l); // consume '|#'
                        break;
                    }
                    if (l->ch == '\n') l->line_num++;
                    read_char(l);
                }
            } else {
                // Single line comment, skip to end of line
                while(l->ch != '\n' && l->ch != 0) read_char(l);
            }
            l->at_bol = 1; // Comment line, still at BOL for next line
            new_indent = 0; // Reset indent for new line

            continue; // Go back and process leading whitespace of new line
        }
        break; // Found a significant character
    }

    // Now l->ch is the first significant character of the line, or EOF.
    // Determine INDENT/DEDENT tokens if not EOF
    if (l->ch != 0) {


        if (new_indent > current_indent) {

            l->indent_level++;
            if (l->indent_level >= INDENT_STACK_SIZE) {
                fprintf(stderr, "Fatal: Indentation stack overflow\n");
                exit(1);
            }
            l->indent_stack[l->indent_level] = new_indent;
            if (l->pending_count >= PENDING_TOKEN_SIZE) { // Add INDENT to pending
                fprintf(stderr, "Fatal: Pending token stack overflow\n");
                exit(1);
            }
            l->pending_tokens[l->pending_count++] = new_token(TOKEN_INDENT, "INDENT");

        } else if (new_indent < current_indent) {

            while (l->indent_stack[l->indent_level] > new_indent) {
                l->indent_level--;
                if (l->pending_count >= PENDING_TOKEN_SIZE) {
                    fprintf(stderr, "Fatal: Pending token stack overflow\n");
                    exit(1);
                }
                l->pending_tokens[l->pending_count++] = new_token(TOKEN_DEDENT, "DEDENT");

            }
            // If the new indentation level is not on the stack, it's an error
            if (l->indent_stack[l->indent_level] != new_indent) {
                fprintf(stderr, "Fatal: IndentationError: inconsistent dedent at line %d\n", l->line_num);
                exit(1);
            }
        }
    } else { // It's EOF
        // Emit DEDENTs for any open blocks at EOF
        while (l->indent_stack[l->indent_level] > 0) {
            l->indent_level--;
            if (l->pending_count >= PENDING_TOKEN_SIZE) {
                fprintf(stderr, "Fatal: Pending token stack overflow\n");
                exit(1);
            }
            l->pending_tokens[l->pending_count++] = new_token(TOKEN_DEDENT, "DEDENT");

        }
    }
    l->at_bol = 0; // Handled BOL, processing actual tokens now

}


// --- Main Public API ---

Token get_next_token(Lexer* l) {
    Token tok;

    if (l->pending_count > 0) {
        l->pending_count--;
        return l->pending_tokens[l->pending_count];
    }

    skip_inline_whitespace(l); // NEW POSITION

    if (l->at_bol) {

    switch (l->ch) {
        case '=': tok = (peek_char(l) == '=') ? (read_char(l), new_token(TOKEN_EQ, "==")) : new_token(TOKEN_ASSIGN, "="); break;
        case '!': tok = (peek_char(l) == '=') ? (read_char(l), new_token(TOKEN_NOT_EQ, "!=")) : new_token(TOKEN_ILLEGAL, "!"); break;
        case '<': tok = (peek_char(l) == '=') ? (read_char(l), new_token(TOKEN_LTE, "<=")) : new_token(TOKEN_LT, "<"); break;
        case '>': tok = (peek_char(l) == '=') ? (read_char(l), new_token(TOKEN_GTE, ">=")) : new_token(TOKEN_GT, ">"); break;

        case '+': tok = new_token(TOKEN_PLUS, "+"); break;
        case '-': tok = new_token(TOKEN_MINUS, "-"); break;
        case '*': tok = new_token(TOKEN_STAR, "*"); break;
        case '/': tok = new_token(TOKEN_SLASH, "/"); break;

        case '.':
            if (peek_char(l) == '.') {
                read_char(l); // consume first '.'
                // Not implemented: .. range operator, for now illegal
                tok = new_token(TOKEN_ILLEGAL, "..");
            } else {
                tok = new_token(TOKEN_ILLEGAL, ".");
            }
            break;

        case ',': tok = new_token(TOKEN_COMMA, ","); break;
        case ':': tok = new_token(TOKEN_COLON, ":"); break;
        case '(': tok = new_token(TOKEN_LPAREN, "("); break;
        case ')': tok = new_token(TOKEN_RPAREN, ")"); break;
        case '[': tok = new_token(TOKEN_LBRACKET, "["); break;
        case ']': tok = new_token(TOKEN_RBRACKET, "]"); break;
        case '{': tok = new_token(TOKEN_LBRACE, "{"); break; // New
        case '}': tok = new_token(TOKEN_RBRACE, "}"); break; // New
        case ';': tok = new_token(TOKEN_SEMICOLON, ";"); break; // New

        case '"':
        case '\'':
            tok.literal = read_string(l);
            tok.type = TOKEN_STRING;
            return tok; // Special return

        case 0:
            tok = new_token(TOKEN_EOF, "");
            return tok;

        default:
            if (is_letter(l->ch)) {
                tok.literal = read_identifier(l);
                tok.type = lookup_ident(tok.literal);
                return tok; // Special return
            } else if (isdigit(l->ch)) {
                tok.literal = read_number(l);
                tok.type = TOKEN_INT;
                return tok; // Special return
            } else {
                tok = new_token(TOKEN_ILLEGAL, "");
                // No read_char(l) here, rely on the final one
            }
    }

    read_char(l); // Advance for simple tokens that haven't already advanced.
    return tok;
}
