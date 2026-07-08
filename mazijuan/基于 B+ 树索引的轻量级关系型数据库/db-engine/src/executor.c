#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* dup_string(const char* s) {
    size_t n = strlen(s) + 1;
    char* out = (char*)malloc(n);
    if (out) memcpy(out, s, n);
    return out;
}

static int expr_to_int(const Expr* expr) {
    if (!expr) return 0;
    if (expr->type == EXPR_LITERAL_INT) return (int)expr->value.int_value;
    return 0;
}

static int match_simple_condition(const Expr* expr) {
    if (!expr || expr->type != EXPR_BINARY) return 0;
    if (expr->value.binary.left && expr->value.binary.left->type == EXPR_IDENTIFIER && expr->value.binary.right) {
        return expr->value.binary.op == TK_EQ && expr->value.binary.right->type == EXPR_LITERAL_INT;
    }
    return 0;
}

static ResultSet* execute_select(Database* db, const SelectStmt* stmt) {
    if (!db || !stmt) return NULL;
    ResultSet* rs = (ResultSet*)calloc(1, sizeof(ResultSet));
    if (!rs) return NULL;
    rs->affected_rows = 0;
    rs->rows = (char**)calloc(1, sizeof(char*));
    rs->row_count = 1;
    if (stmt->where && match_simple_condition(stmt->where)) {
        int key = expr_to_int(stmt->where->value.binary.right);
        record_id_t rid = bpt_search(db->index, key);
        if (rid != (record_id_t)-1) {
            rs->rows[0] = dup_string("SELECT found");
            return rs;
        }
    }
    rs->rows[0] = dup_string("SELECT executed");
    return rs;
}

static ResultSet* execute_insert(Database* db, const InsertStmt* stmt) {
    if (!db || !stmt) return NULL;
    ResultSet* rs = (ResultSet*)calloc(1, sizeof(ResultSet));
    if (!rs) return NULL;
    if (stmt->value_count > 0 && stmt->values[0] && stmt->values[0]->type == EXPR_LITERAL_INT) {
        int key = (int)stmt->values[0]->value.int_value;
        (void)bpt_insert(db->index, key, (record_id_t)key);
    }
    rs->affected_rows = 1;
    rs->rows = (char**)calloc(1, sizeof(char*));
    rs->row_count = 1;
    rs->rows[0] = dup_string("INSERT executed");
    return rs;
}

static ResultSet* execute_delete(Database* db, const DeleteStmt* stmt) {
    if (!db || !stmt) return NULL;
    ResultSet* rs = (ResultSet*)calloc(1, sizeof(ResultSet));
    if (!rs) return NULL;
    if (stmt->where && match_simple_condition(stmt->where)) {
        int key = expr_to_int(stmt->where->value.binary.right);
        (void)bpt_delete(db->index, key);
    }
    rs->affected_rows = 1;
    rs->rows = (char**)calloc(1, sizeof(char*));
    rs->row_count = 1;
    rs->rows[0] = dup_string("DELETE executed");
    return rs;
}

static ResultSet* execute_update(Database* db, const UpdateStmt* stmt) {
    if (!db || !stmt) return NULL;
    ResultSet* rs = (ResultSet*)calloc(1, sizeof(ResultSet));
    if (!rs) return NULL;
    rs->affected_rows = 1;
    rs->rows = (char**)calloc(1, sizeof(char*));
    rs->row_count = 1;
    rs->rows[0] = dup_string("UPDATE executed");
    return rs;
}

ResultSet* execute_ast(Database* db, const AstNode* ast) {
    if (!db || !ast) return NULL;
    switch (ast->type) {
        case AST_SELECT:
            return execute_select(db, &ast->value.select);
        case AST_INSERT:
            return execute_insert(db, &ast->value.insert);
        case AST_DELETE:
            return execute_delete(db, &ast->value.delete);
        case AST_UPDATE:
            return execute_update(db, &ast->value.update);
        default:
            return NULL;
    }
}
