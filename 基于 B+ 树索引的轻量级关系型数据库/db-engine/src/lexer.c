#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char lexer_peek(Lexer* lexer) {
    return *lexer->current;
}

static char lexer_next(Lexer* lexer) {
    char c = *lexer->current;
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    lexer->current++;
    return c;
}

static int lexer_eof(Lexer* lexer) {
    return *lexer->current == '\0';
}

static void lexer_skip_whitespace(Lexer* lexer) {
    while (!lexer_eof(lexer) && isspace(lexer_peek(lexer))) {
        lexer_next(lexer);
    }
}

static Token* create_token(Lexer* lexer, TokenType type, const char* value) {
    Token* token = (Token*)malloc(sizeof(Token));
    token->type = type;
    token->value = value ? strdup(value) : NULL;
    token->line = lexer->line;
    token->column = lexer->column;
    return token;
}

static Token* lexer_read_identifier(Lexer* lexer) {
    const char* start = lexer->current;
    while (!lexer_eof(lexer) && (isalnum(lexer_peek(lexer)) || lexer_peek(lexer) == '_')) {
        lexer_next(lexer);
    }
    int len = lexer->current - start;
    char* value = (char*)malloc(len + 1);
    strncpy(value, start, len);
    value[len] = '\0';

    if (strcmp(value, "SELECT") == 0) return create_token(lexer, TK_SELECT, NULL);
    if (strcmp(value, "FROM") == 0) return create_token(lexer, TK_FROM, NULL);
    if (strcmp(value, "WHERE") == 0) return create_token(lexer, TK_WHERE, NULL);
    if (strcmp(value, "INSERT") == 0) return create_token(lexer, TK_INSERT, NULL);
    if (strcmp(value, "INTO") == 0) return create_token(lexer, TK_INTO, NULL);
    if (strcmp(value, "DELETE") == 0) return create_token(lexer, TK_DELETE, NULL);
    if (strcmp(value, "UPDATE") == 0) return create_token(lexer, TK_UPDATE, NULL);
    if (strcmp(value, "SET") == 0) return create_token(lexer, TK_SET, NULL);
    if (strcmp(value, "VALUES") == 0) return create_token(lexer, TK_VALUES, NULL);
    if (strcmp(value, "AND") == 0) return create_token(lexer, TK_AND, NULL);
    if (strcmp(value, "OR") == 0) return create_token(lexer, TK_OR, NULL);

    return create_token(lexer, TK_IDENTIFIER, value);
}

static Token* lexer_read_integer(Lexer* lexer) {
    const char* start = lexer->current;
    while (!lexer_eof(lexer) && isdigit(lexer_peek(lexer))) {
        lexer_next(lexer);
    }
    int len = lexer->current - start;
    char* value = (char*)malloc(len + 1);
    strncpy(value, start, len);
    value[len] = '\0';
    return create_token(lexer, TK_INTEGER, value);
}

static Token* lexer_read_string(Lexer* lexer) {
    lexer_next(lexer);
    const char* start = lexer->current;
    while (!lexer_eof(lexer) && lexer_peek(lexer) != '\'') {
        if (lexer_peek(lexer) == '\\') {
            lexer_next(lexer);
            if (!lexer_eof(lexer)) lexer_next(lexer);
        } else {
            lexer_next(lexer);
        }
    }
    if (!lexer_eof(lexer)) {
        lexer_next(lexer);
    }
    int len = lexer->current - start - 1;
    char* value = (char*)malloc(len + 1);
    strncpy(value, start, len);
    value[len] = '\0';
    return create_token(lexer, TK_STRING, value);
}

Lexer* lexer_init(const char* input) {
    Lexer* lexer = (Lexer*)malloc(sizeof(Lexer));
    lexer->input = input;
    lexer->current = input;
    lexer->line = 1;
    lexer->column = 1;
    return lexer;
}

void lexer_destroy(Lexer* lexer) {
    if (lexer) free(lexer);
}

Token* lexer_next_token(Lexer* lexer) {
    lexer_skip_whitespace(lexer);

    if (lexer_eof(lexer)) {
        return create_token(lexer, TK_EOF, NULL);
    }

    char c = lexer_peek(lexer);

    if (isalpha(c) || c == '_') {
        return lexer_read_identifier(lexer);
    }

    if (isdigit(c)) {
        return lexer_read_integer(lexer);
    }

    if (c == '\'') {
        return lexer_read_string(lexer);
    }

    switch (c) {
        case '=':
            lexer_next(lexer);
            return create_token(lexer, TK_EQ, NULL);
        case '!':
            lexer_next(lexer);
            if (lexer_peek(lexer) == '=') {
                lexer_next(lexer);
                return create_token(lexer, TK_NE, NULL);
            }
            break;
        case '<':
            lexer_next(lexer);
            if (lexer_peek(lexer) == '=') {
                lexer_next(lexer);
                return create_token(lexer, TK_LE, NULL);
            }
            return create_token(lexer, TK_LT, NULL);
        case '>':
            lexer_next(lexer);
            if (lexer_peek(lexer) == '=') {
                lexer_next(lexer);
                return create_token(lexer, TK_GE, NULL);
            }
            return create_token(lexer, TK_GT, NULL);
        case ',':
            lexer_next(lexer);
            return create_token(lexer, TK_COMMA, NULL);
        case ';':
            lexer_next(lexer);
            return create_token(lexer, TK_SEMICOLON, NULL);
        case '(':
            lexer_next(lexer);
            return create_token(lexer, TK_LPAREN, NULL);
        case ')':
            lexer_next(lexer);
            return create_token(lexer, TK_RPAREN, NULL);
        case '*':
            lexer_next(lexer);
            return create_token(lexer, TK_ASTERISK, NULL);
    }

    lexer_next(lexer);
    return create_token(lexer, TK_IDENTIFIER, NULL);
}

void token_destroy(Token* token) {
    if (token) {
        if (token->value) free(token->value);
        free(token);
    }
}

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TK_EOF: return "TK_EOF";
        case TK_IDENTIFIER: return "TK_IDENTIFIER";
        case TK_INTEGER: return "TK_INTEGER";
        case TK_STRING: return "TK_STRING";
        case TK_SELECT: return "TK_SELECT";
        case TK_FROM: return "TK_FROM";
        case TK_WHERE: return "TK_WHERE";
        case TK_INSERT: return "TK_INSERT";
        case TK_INTO: return "TK_INTO";
        case TK_DELETE: return "TK_DELETE";
        case TK_UPDATE: return "TK_UPDATE";
        case TK_SET: return "TK_SET";
        case TK_VALUES: return "TK_VALUES";
        case TK_EQ: return "TK_EQ";
        case TK_NE: return "TK_NE";
        case TK_LT: return "TK_LT";
        case TK_GT: return "TK_GT";
        case TK_LE: return "TK_LE";
        case TK_GE: return "TK_GE";
        case TK_AND: return "TK_AND";
        case TK_OR: return "TK_OR";
        case TK_COMMA: return "TK_COMMA";
        case TK_SEMICOLON: return "TK_SEMICOLON";
        case TK_LPAREN: return "TK_LPAREN";
        case TK_RPAREN: return "TK_RPAREN";
        case TK_ASTERISK: return "TK_ASTERISK";
        default: return "TK_UNKNOWN";
    }
}
