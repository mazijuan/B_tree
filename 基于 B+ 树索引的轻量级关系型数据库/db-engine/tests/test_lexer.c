#include "../src/lexer.h"
#include <stdio.h>
#include <string.h>

static int passed = 0;
static int failed = 0;

#define TEST(name) do { \
    printf("  [TEST] %s ... ", name); \
} while(0)

#define PASS() do { \
    printf("PASS\n"); \
    passed++; \
} while(0)

#define FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
    failed++; \
} while(0)

static void test_select_statement() {
    TEST("SELECT statement");
    const char* sql = "SELECT * FROM users WHERE id = 1;";
    Lexer* lexer = lexer_init(sql);

    Token* t = lexer_next_token(lexer);
    if (t->type != TK_SELECT) { FAIL("expected TK_SELECT"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_ASTERISK) { FAIL("expected TK_ASTERISK"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_FROM) { FAIL("expected TK_FROM"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_IDENTIFIER || strcmp(t->value, "users") != 0) { FAIL("expected 'users'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_WHERE) { FAIL("expected TK_WHERE"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_IDENTIFIER || strcmp(t->value, "id") != 0) { FAIL("expected 'id'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_EQ) { FAIL("expected TK_EQ"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_INTEGER || strcmp(t->value, "1") != 0) { FAIL("expected '1'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_SEMICOLON) { FAIL("expected TK_SEMICOLON"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_EOF) { FAIL("expected TK_EOF"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    PASS();
    lexer_destroy(lexer);
}

static void test_insert_statement() {
    TEST("INSERT statement");
    const char* sql = "INSERT INTO users (id, name) VALUES (1, 'Alice');";
    Lexer* lexer = lexer_init(sql);

    Token* t;
    t = lexer_next_token(lexer);
    if (t->type != TK_INSERT) { FAIL("expected TK_INSERT"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_INTO) { FAIL("expected TK_INTO"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_IDENTIFIER || strcmp(t->value, "users") != 0) { FAIL("expected 'users'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_LPAREN) { FAIL("expected TK_LPAREN"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_IDENTIFIER || strcmp(t->value, "id") != 0) { FAIL("expected 'id'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_COMMA) { FAIL("expected TK_COMMA"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_IDENTIFIER || strcmp(t->value, "name") != 0) { FAIL("expected 'name'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_RPAREN) { FAIL("expected TK_RPAREN"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_VALUES) { FAIL("expected TK_VALUES"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_LPAREN) { FAIL("expected TK_LPAREN"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_INTEGER || strcmp(t->value, "1") != 0) { FAIL("expected '1'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_COMMA) { FAIL("expected TK_COMMA"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_STRING || strcmp(t->value, "Alice") != 0) { FAIL("expected 'Alice'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_RPAREN) { FAIL("expected TK_RPAREN"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_SEMICOLON) { FAIL("expected TK_SEMICOLON"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_EOF) { FAIL("expected TK_EOF"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    PASS();
    lexer_destroy(lexer);
}

static void test_delete_statement() {
    TEST("DELETE statement");
    const char* sql = "DELETE FROM users WHERE id > 10;";
    Lexer* lexer = lexer_init(sql);

    Token* t;
    t = lexer_next_token(lexer);
    if (t->type != TK_DELETE) { FAIL("expected TK_DELETE"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_FROM) { FAIL("expected TK_FROM"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_IDENTIFIER || strcmp(t->value, "users") != 0) { FAIL("expected 'users'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_WHERE) { FAIL("expected TK_WHERE"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_IDENTIFIER || strcmp(t->value, "id") != 0) { FAIL("expected 'id'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_GT) { FAIL("expected TK_GT"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_INTEGER || strcmp(t->value, "10") != 0) { FAIL("expected '10'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_SEMICOLON) { FAIL("expected TK_SEMICOLON"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_EOF) { FAIL("expected TK_EOF"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    PASS();
    lexer_destroy(lexer);
}

static void test_update_statement() {
    TEST("UPDATE statement");
    const char* sql = "UPDATE users SET name = 'Bob' WHERE id = 1;";
    Lexer* lexer = lexer_init(sql);

    Token* t;
    t = lexer_next_token(lexer);
    if (t->type != TK_UPDATE) { FAIL("expected TK_UPDATE"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_IDENTIFIER || strcmp(t->value, "users") != 0) { FAIL("expected 'users'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_SET) { FAIL("expected TK_SET"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_IDENTIFIER || strcmp(t->value, "name") != 0) { FAIL("expected 'name'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_EQ) { FAIL("expected TK_EQ"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_STRING || strcmp(t->value, "Bob") != 0) { FAIL("expected 'Bob'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_WHERE) { FAIL("expected TK_WHERE"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_IDENTIFIER || strcmp(t->value, "id") != 0) { FAIL("expected 'id'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_EQ) { FAIL("expected TK_EQ"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_INTEGER || strcmp(t->value, "1") != 0) { FAIL("expected '1'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_SEMICOLON) { FAIL("expected TK_SEMICOLON"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_EOF) { FAIL("expected TK_EOF"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    PASS();
    lexer_destroy(lexer);
}

static void test_operators() {
    TEST("operators");
    const char* sql = "= != < > <= >= AND OR";
    Lexer* lexer = lexer_init(sql);

    Token* t;
    t = lexer_next_token(lexer);
    if (t->type != TK_EQ) { FAIL("expected TK_EQ"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_NE) { FAIL("expected TK_NE"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_LT) { FAIL("expected TK_LT"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_GT) { FAIL("expected TK_GT"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_LE) { FAIL("expected TK_LE"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_GE) { FAIL("expected TK_GE"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_AND) { FAIL("expected TK_AND"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_OR) { FAIL("expected TK_OR"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_EOF) { FAIL("expected TK_EOF"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    PASS();
    lexer_destroy(lexer);
}

static void test_integer() {
    TEST("integer parsing");
    const char* sql = "123 0 -1 99999";
    Lexer* lexer = lexer_init(sql);

    Token* t;
    t = lexer_next_token(lexer);
    if (t->type != TK_INTEGER || strcmp(t->value, "123") != 0) { FAIL("expected '123'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_INTEGER || strcmp(t->value, "0") != 0) { FAIL("expected '0'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_IDENTIFIER || strcmp(t->value, "-1") != 0) { FAIL("negative should be identifier"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_INTEGER || strcmp(t->value, "99999") != 0) { FAIL("expected '99999'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_EOF) { FAIL("expected TK_EOF"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    PASS();
    lexer_destroy(lexer);
}

static void test_empty_input() {
    TEST("empty input");
    const char* sql = "";
    Lexer* lexer = lexer_init(sql);

    Token* t = lexer_next_token(lexer);
    if (t->type != TK_EOF) { FAIL("expected TK_EOF"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    PASS();
    lexer_destroy(lexer);
}

static void test_whitespace() {
    TEST("whitespace handling");
    const char* sql = "  SELECT\t\n  *\nFROM users  ";
    Lexer* lexer = lexer_init(sql);

    Token* t;
    t = lexer_next_token(lexer);
    if (t->type != TK_SELECT) { FAIL("expected TK_SELECT"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_ASTERISK) { FAIL("expected TK_ASTERISK"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_FROM) { FAIL("expected TK_FROM"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_IDENTIFIER || strcmp(t->value, "users") != 0) { FAIL("expected 'users'"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    t = lexer_next_token(lexer);
    if (t->type != TK_EOF) { FAIL("expected TK_EOF"); token_destroy(t); lexer_destroy(lexer); return; }
    token_destroy(t);

    PASS();
    lexer_destroy(lexer);
}

int main() {
    printf("Running Lexer tests...\n\n");

    printf("=== SQL Statements ===\n");
    test_select_statement();
    printf("SELECT done\n");
    
    test_insert_statement();
    printf("INSERT done\n");
    
    test_delete_statement();
    printf("DELETE done\n");
    
    test_update_statement();
    printf("UPDATE done\n");
    printf("\n");

    printf("=== Operators ===\n");
    test_operators();
    printf("Operators done\n");
    printf("\n");

    printf("=== Edge Cases ===\n");
    test_integer();
    printf("Integer done\n");
    
    test_empty_input();
    printf("Empty input done\n");
    
    test_whitespace();
    printf("Whitespace done\n");
    printf("\n");

    printf("=========================\n");
    printf("  Passed: %d\n", passed);
    printf("  Failed: %d\n", failed);
    printf("  Total:  %d\n", passed + failed);
    printf("=========================\n");

    return failed == 0 ? 0 : 1;
}
