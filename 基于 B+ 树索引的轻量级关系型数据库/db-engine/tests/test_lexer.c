#include "../src/lexer.h"
#include <stdio.h>
#include <string.h>

static void expect(TokenType got, TokenType want, const char* msg) {
    if (got != want) {
        printf("[FAIL] %s (got=%d want=%d)\n", msg, (int)got, (int)want);
        return;
    }
    printf("[PASS] %s\n", msg);
}

int main() {
    const char* sql = "SELECT id, name FROM users WHERE id >= 10 AND name != 'admin';";
    Lexer* lx = lexer_create(sql);
    if (!lx) {
        printf("[FAIL] create lexer\n");
        return 1;
    }

    Token t;
    t = lexer_next(lx); expect(t.type, TK_SELECT, "token SELECT"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_IDENTIFIER, "token id"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_COMMA, "token comma"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_IDENTIFIER, "token name"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_FROM, "token FROM"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_IDENTIFIER, "token users"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_WHERE, "token WHERE"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_IDENTIFIER, "token id2"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_GE, "token >="); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_INTEGER, "token 10"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_AND, "token AND"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_IDENTIFIER, "token name2"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_NE, "token != or <> (recognized as NE)"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_STRING, "token string"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_SEMICOLON, "token semicolon"); token_free(&t);
    t = lexer_next(lx); expect(t.type, TK_EOF, "token EOF"); token_free(&t);

    lexer_destroy(lx);
    return 0;
}
