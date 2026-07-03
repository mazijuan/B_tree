#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

typedef enum {
    TK_EOF,
    TK_IDENTIFIER,
    TK_INTEGER,
    TK_STRING,
    TK_SELECT,
    TK_FROM,
    TK_WHERE,
    TK_INSERT,
    TK_INTO,
    TK_DELETE,
    TK_UPDATE,
    TK_SET,
    TK_VALUES,
    TK_AND,
    TK_OR,
    TK_EQ,
    TK_NE,
    TK_LT,
    TK_GT,
    TK_LE,
    TK_GE,
    TK_COMMA,
    TK_SEMICOLON,
    TK_LPAREN,
    TK_RPAREN,
    TK_STAR,
    TK_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char* text; /* owned by caller (free with token_free) */
    int64_t int_value;
} Token;

typedef struct Lexer Lexer;

Lexer* lexer_create(const char* input);
void lexer_destroy(Lexer* lx);
Token lexer_next(Lexer* lx);
void token_free(Token* t);

#endif
