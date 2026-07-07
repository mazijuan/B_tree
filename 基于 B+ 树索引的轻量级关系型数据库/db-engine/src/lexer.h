#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

typedef enum {
    TK_BAD_TOKEN,
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
    TK_EQ,
    TK_NE,
    TK_LT,
    TK_GT,
    TK_LE,
    TK_GE,
    TK_AND,
    TK_OR,
    TK_COMMA,
    TK_SEMICOLON,
    TK_LPAREN,
    TK_RPAREN,
    TK_ASTERISK,
    TK_MINUS
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

typedef struct {
    const char* input;
    const char* current;
    int line;
    int column;
} Lexer;

Lexer* lexer_init(const char* input);
void lexer_destroy(Lexer* lexer);
Token* lexer_next_token(Lexer* lexer);
void token_destroy(Token* token);
const char* token_type_to_string(TokenType type);

#endif
