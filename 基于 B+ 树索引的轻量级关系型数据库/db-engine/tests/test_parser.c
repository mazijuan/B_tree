#include "parser.h"
#include "lexer.h"
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

static void test_select_all() {
    TEST("SELECT * FROM table");
    const char* sql = "SELECT * FROM users;";
    Lexer* lexer = lexer_init(sql);
    Parser* parser = parser_init(lexer);

    ASTNode* node = parser_parse(parser);
    if (!node) { FAIL("NULL AST"); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->type != NODE_SELECT) { FAIL("expected NODE_SELECT"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.select.column_count != 1) { FAIL("expected 1 column"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (strcmp(node->value.select.columns[0], "*") != 0) { FAIL("expected '*'"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (strcmp(node->value.select.table, "users") != 0) { FAIL("expected 'users'"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.select.where != NULL) { FAIL("expected NULL where"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }

    PASS();
    ast_destroy(node);
    parser_destroy(parser);
    lexer_destroy(lexer);
}

static void test_select_columns() {
    TEST("SELECT columns FROM table");
    const char* sql = "SELECT id, name FROM users;";
    Lexer* lexer = lexer_init(sql);
    Parser* parser = parser_init(lexer);

    ASTNode* node = parser_parse(parser);
    if (!node) { FAIL("NULL AST"); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->type != NODE_SELECT) { FAIL("expected NODE_SELECT"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.select.column_count != 2) { FAIL("expected 2 columns"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (strcmp(node->value.select.columns[0], "id") != 0) { FAIL("expected 'id'"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (strcmp(node->value.select.columns[1], "name") != 0) { FAIL("expected 'name'"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (strcmp(node->value.select.table, "users") != 0) { FAIL("expected 'users'"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }

    PASS();
    ast_destroy(node);
    parser_destroy(parser);
    lexer_destroy(lexer);
}

static void test_select_where() {
    TEST("SELECT with WHERE clause");
    const char* sql = "SELECT * FROM users WHERE id = 1;";
    Lexer* lexer = lexer_init(sql);
    Parser* parser = parser_init(lexer);

    ASTNode* node = parser_parse(parser);
    if (!node) { FAIL("NULL AST"); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->type != NODE_SELECT) { FAIL("expected NODE_SELECT"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.select.where == NULL) { FAIL("expected WHERE clause"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.select.where->op != OP_EQ) { FAIL("expected OP_EQ"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }

    PASS();
    ast_destroy(node);
    parser_destroy(parser);
    lexer_destroy(lexer);
}

static void test_insert() {
    TEST("INSERT INTO table");
    const char* sql = "INSERT INTO users (id, name) VALUES (1, 'Alice');";
    Lexer* lexer = lexer_init(sql);
    Parser* parser = parser_init(lexer);

    ASTNode* node = parser_parse(parser);
    if (!node) { FAIL("NULL AST"); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->type != NODE_INSERT) { FAIL("expected NODE_INSERT"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (strcmp(node->value.insert.table, "users") != 0) { FAIL("expected 'users'"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.insert.column_count != 2) { FAIL("expected 2 columns"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (strcmp(node->value.insert.columns[0], "id") != 0) { FAIL("expected 'id'"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (strcmp(node->value.insert.columns[1], "name") != 0) { FAIL("expected 'name'"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.insert.value_count != 2) { FAIL("expected 2 values"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }

    PASS();
    ast_destroy(node);
    parser_destroy(parser);
    lexer_destroy(lexer);
}

static void test_delete() {
    TEST("DELETE FROM table");
    const char* sql = "DELETE FROM users WHERE id > 10;";
    Lexer* lexer = lexer_init(sql);
    Parser* parser = parser_init(lexer);

    ASTNode* node = parser_parse(parser);
    if (!node) { FAIL("NULL AST"); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->type != NODE_DELETE) { FAIL("expected NODE_DELETE"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (strcmp(node->value.delete.table, "users") != 0) { FAIL("expected 'users'"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.delete.where == NULL) { FAIL("expected WHERE clause"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }

    PASS();
    ast_destroy(node);
    parser_destroy(parser);
    lexer_destroy(lexer);
}

static void test_update() {
    TEST("UPDATE table");
    const char* sql = "UPDATE users SET name = 'Bob' WHERE id = 1;";
    Lexer* lexer = lexer_init(sql);
    Parser* parser = parser_init(lexer);

    ASTNode* node = parser_parse(parser);
    if (!node) { FAIL("NULL AST"); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->type != NODE_UPDATE) { FAIL("expected NODE_UPDATE"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (strcmp(node->value.update.table, "users") != 0) { FAIL("expected 'users'"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.update.set_count != 1) { FAIL("expected 1 set"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (strcmp(node->value.update.columns[0], "name") != 0) { FAIL("expected 'name'"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.update.where == NULL) { FAIL("expected WHERE clause"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }

    PASS();
    ast_destroy(node);
    parser_destroy(parser);
    lexer_destroy(lexer);
}

static void test_expr_and() {
    TEST("expression with AND");
    const char* sql = "SELECT * FROM users WHERE id > 1 AND id < 10;";
    Lexer* lexer = lexer_init(sql);
    Parser* parser = parser_init(lexer);

    ASTNode* node = parser_parse(parser);
    if (!node) { FAIL("NULL AST"); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->type != NODE_SELECT) { FAIL("expected NODE_SELECT"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.select.where == NULL) { FAIL("expected WHERE clause"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.select.where->op != OP_AND) { FAIL("expected OP_AND"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }

    PASS();
    ast_destroy(node);
    parser_destroy(parser);
    lexer_destroy(lexer);
}

static void test_expr_or() {
    TEST("expression with OR");
    const char* sql = "SELECT * FROM users WHERE name = 'Alice' OR name = 'Bob';";
    Lexer* lexer = lexer_init(sql);
    Parser* parser = parser_init(lexer);

    ASTNode* node = parser_parse(parser);
    if (!node) { FAIL("NULL AST"); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->type != NODE_SELECT) { FAIL("expected NODE_SELECT"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.select.where == NULL) { FAIL("expected WHERE clause"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }
    if (node->value.select.where->op != OP_OR) { FAIL("expected OP_OR"); ast_destroy(node); parser_destroy(parser); lexer_destroy(lexer); return; }

    PASS();
    ast_destroy(node);
    parser_destroy(parser);
    lexer_destroy(lexer);
}

int main() {
    printf("Running Parser tests...\n\n");

    printf("=== SELECT Statements ===\n");
    test_select_all();
    test_select_columns();
    test_select_where();
    printf("\n");

    printf("=== INSERT Statements ===\n");
    test_insert();
    printf("\n");

    printf("=== DELETE Statements ===\n");
    test_delete();
    printf("\n");

    printf("=== UPDATE Statements ===\n");
    test_update();
    printf("\n");

    printf("=== Expressions ===\n");
    test_expr_and();
    test_expr_or();
    printf("\n");

    printf("=========================\n");
    printf("  Passed: %d\n", passed);
    printf("  Failed: %d\n", failed);
    printf("  Total:  %d\n", passed + failed);
    printf("=========================\n");

    return failed == 0 ? 0 : 1;
}
