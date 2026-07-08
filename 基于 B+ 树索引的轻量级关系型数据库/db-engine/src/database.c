#include "database.h"
#include "parser.h"
#include "executor.h"
#include <stdlib.h>
#include <string.h>

Database* db_open(const char* filename) {
    Database* db = (Database*)calloc(1, sizeof(Database));
    if (!db) return NULL;

    if (dm_open(&db->dm, filename) != DB_OK) {
        free(db);
        return NULL;
    }

    db->pool = bp_create(db->dm, 8);
    db->index = bpt_create();
    if (!db->pool || !db->index) {
        db_close(db);
        return NULL;
    }

    return db;
}

void db_close(Database* db) {
    if (!db) return;
    if (db->pool) {
        bp_close(db->pool);
        db->pool = NULL;
    }
    if (db->index) {
        bpt_destroy(db->index);
        db->index = NULL;
    }
    if (db->dm) {
        dm_close(db->dm);
        db->dm = NULL;
    }
    free(db);
}

ResultSet* db_query(Database* db, const char* sql) {
    if (!db || !sql) return NULL;

    AstNode* ast = parse_sql(sql);
    if (!ast) return NULL;

    ResultSet* rs = execute_ast(db, ast);
    ast_destroy(ast);
    return rs;
}

void result_set_destroy(ResultSet* rs) {
    if (!rs) return;
    if (rs->rows) {
        for (size_t i = 0; i < rs->row_count; ++i) {
            free(rs->rows[i]);
        }
        free(rs->rows);
    }
    free(rs);
}
