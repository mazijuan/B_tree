#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct Lexer {
    const char* input;
    size_t pos;
    size_t len;
};

Lexer* lexer_create(const char* input) {
    if (!input) return NULL;
    Lexer* lx = (Lexer*)calloc(1, sizeof(Lexer));
    if (!lx) return NULL;
    lx->input = input;
    lx->pos = 0;
    lx->len = strlen(input);
    return lx;
}

void lexer_destroy(Lexer* lx) {
    if (!lx) return;
    free(lx);
}

static void skip_ws(Lexer* lx) {
    while (lx->pos < lx->len && isspace((unsigned char)lx->input[lx->pos])) lx->pos++;
}

static char peek(Lexer* lx) {
    if (lx->pos >= lx->len) return '\0';
    return lx->input[lx->pos];
}

static char advance(Lexer* lx) {
    if (lx->pos >= lx->len) return '\0';
    return lx->input[lx->pos++];
}

static char* make_text(const char* start, size_t n) {
    char* s = (char*)malloc(n + 1);
    if (!s) return NULL;
    memcpy(s, start, n);
    s[n] = '\0';
    return s;
}

static int is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static int is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '$';
}

static Token make_token(TokenType type, char* text) {
    Token t;
    t.type = type;
    t.text = text;
    t.int_value = 0;
    return t;
}

static Token token_from_number(const char* start, size_t len) {
    char* txt = make_text(start, len);
    Token t = make_token(TK_INTEGER, txt);
    if (txt) t.int_value = atoll(txt);
    return t;
}

static Token token_from_string(const char* start, size_t len) {
    /* strip surrounding quotes if present */
    char* txt = make_text(start, len);
    Token t = make_token(TK_STRING, txt);
    return t;
}

static Token token_from_ident_or_keyword(const char* start, size_t len) {
    char* txt = make_text(start, len);
    /* uppercase copy for comparison */
    for (size_t i = 0; i < len; i++) txt[i] = (char)toupper((unsigned char)txt[i]);

    TokenType type = TK_IDENTIFIER;
    if (strcmp(txt, "SELECT") == 0) type = TK_SELECT;
    else if (strcmp(txt, "FROM") == 0) type = TK_FROM;
    else if (strcmp(txt, "WHERE") == 0) type = TK_WHERE;
    else if (strcmp(txt, "INSERT") == 0) type = TK_INSERT;
    else if (strcmp(txt, "INTO") == 0) type = TK_INTO;
    else if (strcmp(txt, "DELETE") == 0) type = TK_DELETE;
    else if (strcmp(txt, "UPDATE") == 0) type = TK_UPDATE;
    else if (strcmp(txt, "SET") == 0) type = TK_SET;
    else if (strcmp(txt, "VALUES") == 0) type = TK_VALUES;
    else if (strcmp(txt, "AND") == 0) type = TK_AND;
    else if (strcmp(txt, "OR") == 0) type = TK_OR;

    /* if it's a keyword, keep original case text */
    if (type != TK_IDENTIFIER) {
        char* kept = make_text(start, len);
        free(txt);
        return make_token(type, kept);
    }

    return make_token(TK_IDENTIFIER, txt);
}

Token lexer_next(Lexer* lx) {
    Token t;
    t.type = TK_EOF;
    t.text = NULL;
    t.int_value = 0;
    if (!lx) return t;

    skip_ws(lx);
    if (lx->pos >= lx->len) return make_token(TK_EOF, NULL);

    char c = peek(lx);

    /* single char tokens and operators */
    if (c == '*') { advance(lx); return make_token(TK_STAR, make_text("*",1)); }
    if (c == ',') { advance(lx); return make_token(TK_COMMA, make_text(",",1)); }
    if (c == ';') { advance(lx); return make_token(TK_SEMICOLON, make_text(";",1)); }
    if (c == '(') { advance(lx); return make_token(TK_LPAREN, make_text("(",1)); }
    if (c == ')') { advance(lx); return make_token(TK_RPAREN, make_text(")",1)); }

    if (c == '=') { advance(lx); return make_token(TK_EQ, make_text("=",1)); }
    if (c == '<') {
        advance(lx);
        if (peek(lx) == '=') { advance(lx); return make_token(TK_LE, make_text("<=",2)); }
        if (peek(lx) == '>') { advance(lx); return make_token(TK_NE, make_text("<>",2)); }
        return make_token(TK_LT, make_text("<",1));
    }
    if (c == '>') {
        advance(lx);
        if (peek(lx) == '=') { advance(lx); return make_token(TK_GE, make_text(">=",2)); }
        return make_token(TK_GT, make_text(">",1));
    }

    /* numbers */
    if (isdigit((unsigned char)c)) {
        size_t start = lx->pos;
        while (lx->pos < lx->len && isdigit((unsigned char)lx->input[lx->pos])) lx->pos++;
        return token_from_number(&lx->input[start], lx->pos - start);
    }

    /* identifiers / keywords */
    if (is_ident_start(c)) {
        size_t start = lx->pos;
        advance(lx);
        while (lx->pos < lx->len && is_ident_char(lx->input[lx->pos])) lx->pos++;
        return token_from_ident_or_keyword(&lx->input[start], lx->pos - start);
    }

    /* strings single-quoted */
    if (c == '\'' || c == '"') {
        char quote = advance(lx);
        size_t start = lx->pos;
        while (lx->pos < lx->len && lx->input[lx->pos] != quote) {
            /* support escape of quote by doubling */
            if (lx->input[lx->pos] == '\\' && lx->pos + 1 < lx->len) lx->pos += 2;
            else lx->pos++;
        }
        size_t len = lx->pos - start;
        Token tok = token_from_string(&lx->input[start], len);
        if (peek(lx) == quote) advance(lx);
        return tok;
    }

    /* unknown single char */
    advance(lx);
    char* txt = make_text(&lx->input[lx->pos-1], 1);
    return make_token(TK_UNKNOWN, txt);
}

void token_free(Token* t) {
    if (!t) return;
    if (t->text) free(t->text);
    t->text = NULL;
}
