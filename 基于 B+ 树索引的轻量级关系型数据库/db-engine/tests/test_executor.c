#include "executor.h"
#include "parser.h"
#include "lexer.h"
#include <stdio.h>
#include <string.h>

static int passed = 0;
static int failed = 0;

#define PASS() do { passed++; printf("  [TEST] %s ... PASS\n", __func__); } while(0)
#define FAIL(msg) do { failed++; printf("  [TEST] %s ... FAIL: %s\n", __func__, msg); } while(0)

static Executor* create_test_executor() {
    return executor_init();
}

static void destroy_test_executor(Executor* executor) {
    executor_destroy(executor);
}

static ASTNode* parse_sql(const char* sql) {
    Lexer* lexer = lexer_init(sql);
    Parser* parser = parser_init(lexer);
    ASTNode* node = parser_parse(parser);
    parser_destroy(parser);
    lexer_destroy(lexer);
    return node;
}

static void test_insert() {
    Executor* executor = create_test_executor();
    ASTNode* node = parse_sql("INSERT INTO users VALUES (1, 'Alice');");
    
    if (node && node->type == NODE_INSERT) {
        int result = execute_insert(executor, &node->value.insert);
        if (result == DB_OK) {
            Table* table = executor_get_table(executor, "users");
            if (table && table->record_count == 1) {
                PASS();
            } else {
                FAIL("record count mismatch");
            }
        } else {
            FAIL("insert failed");
        }
    } else {
        FAIL("parse failed");
    }
    
    ast_destroy(node);
    destroy_test_executor(executor);
}

static void test_select_all() {
    Executor* executor = create_test_executor();
    
    ASTNode* insert1 = parse_sql("INSERT INTO users VALUES (1, 'Alice');");
    ASTNode* insert2 = parse_sql("INSERT INTO users VALUES (2, 'Bob');");
    execute_insert(executor, &insert1->value.insert);
    execute_insert(executor, &insert2->value.insert);
    ast_destroy(insert1);
    ast_destroy(insert2);
    
    ASTNode* select = parse_sql("SELECT * FROM users;");
    if (select && select->type == NODE_SELECT) {
        ResultSet* rs = execute_select(executor, &select->value.select);
        if (rs && rs->row_count == 2) {
            PASS();
        } else {
            FAIL("row count mismatch");
        }
        result_set_destroy(rs);
    } else {
        FAIL("parse failed");
    }
    
    ast_destroy(select);
    destroy_test_executor(executor);
}

static void test_select_where() {
    Executor* executor = create_test_executor();
    
    ASTNode* insert1 = parse_sql("INSERT INTO users VALUES (1, 'Alice');");
    ASTNode* insert2 = parse_sql("INSERT INTO users VALUES (2, 'Bob');");
    execute_insert(executor, &insert1->value.insert);
    execute_insert(executor, &insert2->value.insert);
    ast_destroy(insert1);
    ast_destroy(insert2);
    
    ASTNode* select = parse_sql("SELECT * FROM users WHERE id = 1;");
    if (select && select->type == NODE_SELECT) {
        ResultSet* rs = execute_select(executor, &select->value.select);
        if (rs && rs->row_count == 1) {
            PASS();
        } else {
            FAIL("row count mismatch");
        }
        result_set_destroy(rs);
    } else {
        FAIL("parse failed");
    }
    
    ast_destroy(select);
    destroy_test_executor(executor);
}

static void test_delete() {
    Executor* executor = create_test_executor();
    
    ASTNode* insert = parse_sql("INSERT INTO users VALUES (1, 'Alice');");
    execute_insert(executor, &insert->value.insert);
    ast_destroy(insert);
    
    ASTNode* delete_node = parse_sql("DELETE FROM users WHERE id = 1;");
    if (delete_node && delete_node->type == NODE_DELETE) {
        int result = execute_delete(executor, &delete_node->value.delete);
        if (result == DB_OK) {
            Table* table = executor_get_table(executor, "users");
            if (table && table->record_count == 0) {
                PASS();
            } else {
                FAIL("record count mismatch");
            }
        } else {
            FAIL("delete failed");
        }
    } else {
        FAIL("parse failed");
    }
    
    ast_destroy(delete_node);
    destroy_test_executor(executor);
}

static void test_update() {
    Executor* executor = create_test_executor();
    
    ASTNode* insert = parse_sql("INSERT INTO users VALUES (1, 'Alice');");
    execute_insert(executor, &insert->value.insert);
    ast_destroy(insert);
    
    ASTNode* update = parse_sql("UPDATE users SET id = 100 WHERE id = 1;");
    if (update && update->type == NODE_UPDATE) {
        int result = execute_update(executor, &update->value.update);
        if (result == DB_OK) {
            PASS();
        } else {
            FAIL("update failed");
        }
    } else {
        FAIL("parse failed");
    }
    
    ast_destroy(update);
    destroy_test_executor(executor);
}

int main() {
    printf("Running Executor tests...\n\n");

    printf("=== INSERT ===\n");
    test_insert();
    printf("\n");

    printf("=== SELECT ===\n");
    test_select_all();
    test_select_where();
    printf("\n");

    printf("=== DELETE ===\n");
    test_delete();
    printf("\n");

    printf("=== UPDATE ===\n");
    test_update();
    printf("\n");

    printf("=========================\n");
    printf("  Passed: %d\n", passed);
    printf("  Failed: %d\n", failed);
    printf("  Total:  %d\n", passed + failed);
    printf("=========================\n");

    return failed == 0 ? 0 : 1;
}
