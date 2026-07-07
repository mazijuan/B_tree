#include "executor.h"
#include "parser.h"
#include "bplus_tree.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int get_expr_integer_value(Expr* expr) {
    if (expr == NULL) return 0;
    switch (expr->op) {
        case OP_INTEGER:
            return expr->value.integer;
        case OP_IDENTIFIER:
        case OP_STRING:
        default:
            return 0;
    }
}

static char* get_expr_string_value(Expr* expr) {
    if (expr == NULL) return NULL;
    switch (expr->op) {
        case OP_STRING:
            return expr->value.string;
        case OP_IDENTIFIER:
            return expr->value.identifier;
        default:
            return NULL;
    }
}

static int compare_expr(Expr* expr) {
    if (expr == NULL || expr->op != OP_EQ) return 0;
    
    Expr* left = expr->value.binary.left;
    Expr* right = expr->value.binary.right;
    
    if (left->op == OP_IDENTIFIER && right->op == OP_INTEGER) {
        return right->value.integer;
    }
    return 0;
}

Executor* executor_init() {
    Executor* executor = (Executor*)malloc(sizeof(Executor));
    executor->tables = NULL;
    executor->table_count = 0;
    executor->max_tables = 0;
    return executor;
}

void executor_destroy(Executor* executor) {
    if (executor == NULL) return;
    
    for (int i = 0; i < executor->table_count; i++) {
        Table* table = &executor->tables[i];
        if (table->table_name) free(table->table_name);
        if (table->index) bpt_destroy(table->index);
        for (int j = 0; j < table->record_count; j++) {
            if (table->records[j]) free(table->records[j]);
        }
        if (table->records) free(table->records);
    }
    if (executor->tables) free(executor->tables);
    free(executor);
}

Table* executor_get_table(Executor* executor, const char* table_name) {
    if (executor == NULL || table_name == NULL) return NULL;
    
    for (int i = 0; i < executor->table_count; i++) {
        if (strcmp(executor->tables[i].table_name, table_name) == 0) {
            return &executor->tables[i];
        }
    }
    return NULL;
}

Table* executor_create_table(Executor* executor, const char* table_name) {
    if (executor == NULL || table_name == NULL) return NULL;
    
    if (executor_get_table(executor, table_name) != NULL) {
        return executor_get_table(executor, table_name);
    }
    
    if (executor->table_count >= executor->max_tables) {
        executor->max_tables = executor->max_tables == 0 ? 8 : executor->max_tables * 2;
        executor->tables = (Table*)realloc(executor->tables, executor->max_tables * sizeof(Table));
    }
    
    Table* table = &executor->tables[executor->table_count++];
    table->table_name = strdup(table_name);
    table->index = bpt_create();
    table->records = NULL;
    table->record_count = 0;
    table->max_records = 0;
    
    return table;
}

ResultSet* execute_select(Executor* executor, SelectStmt* stmt) {
    if (executor == NULL || stmt == NULL) return NULL;
    
    Table* table = executor_get_table(executor, stmt->table);
    if (table == NULL) return NULL;
    
    ResultSet* rs = (ResultSet*)malloc(sizeof(ResultSet));
    rs->rows = NULL;
    rs->row_count = 0;
    rs->columns = NULL;
    rs->column_count = 0;
    
    if (stmt->columns != NULL && stmt->column_count > 0) {
        rs->column_count = stmt->column_count;
        rs->columns = (char**)malloc(rs->column_count * sizeof(char*));
        for (int i = 0; i < stmt->column_count; i++) {
            rs->columns[i] = strdup(stmt->columns[i]);
        }
    } else {
        rs->column_count = 1;
        rs->columns = (char**)malloc(sizeof(char*));
        rs->columns[0] = strdup("*");
    }
    
    if (stmt->where != NULL) {
        int key = compare_expr(stmt->where);
        if (key > 0) {
            record_id_t rid = bpt_search(table->index, key);
            if (rid > 0 && rid <= table->record_count) {
                rs->row_count = 1;
                rs->rows = (char**)malloc(sizeof(char*));
                rs->rows[0] = strdup(table->records[rid - 1]);
            }
        } else {
            rs->row_count = table->record_count;
            rs->rows = (char**)malloc(rs->row_count * sizeof(char*));
            for (int i = 0; i < rs->row_count; i++) {
                rs->rows[i] = strdup(table->records[i]);
            }
        }
    } else {
        rs->row_count = table->record_count;
        rs->rows = (char**)malloc(rs->row_count * sizeof(char*));
        for (int i = 0; i < rs->row_count; i++) {
            rs->rows[i] = strdup(table->records[i]);
        }
    }
    
    return rs;
}

int execute_insert(Executor* executor, InsertStmt* stmt) {
    if (executor == NULL || stmt == NULL) return DB_ERROR;
    
    Table* table = executor_create_table(executor, stmt->table);
    if (table == NULL) return DB_ERROR;
    
    if (stmt->value_count == 0) return DB_ERROR;
    
    char* record = (char*)malloc(256);
    record[0] = '\0';
    
    for (int i = 0; i < stmt->value_count; i++) {
        if (i > 0) strcat(record, ",");
        
        Expr* value = stmt->values[i];
        switch (value->op) {
            case OP_INTEGER:
                char num_str[32];
                sprintf(num_str, "%d", value->value.integer);
                strcat(record, num_str);
                break;
            case OP_STRING:
                strcat(record, value->value.string);
                break;
            case OP_IDENTIFIER:
                strcat(record, value->value.identifier);
                break;
            default:
                strcat(record, "NULL");
        }
    }
    
    if (table->record_count >= table->max_records) {
        table->max_records = table->max_records == 0 ? 100 : table->max_records * 2;
        table->records = (char**)realloc(table->records, table->max_records * sizeof(char*));
    }
    
    table->records[table->record_count] = record;
    record_id_t rid = table->record_count + 1;
    
    if (stmt->value_count > 0) {
        Expr* first_value = stmt->values[0];
        if (first_value->op == OP_INTEGER) {
            bpt_insert(table->index, first_value->value.integer, rid);
        }
    }
    
    table->record_count++;
    return DB_OK;
}

int execute_delete(Executor* executor, DeleteStmt* stmt) {
    if (executor == NULL || stmt == NULL) return DB_ERROR;
    
    Table* table = executor_get_table(executor, stmt->table);
    if (table == NULL) return DB_ERROR;
    
    if (stmt->where != NULL) {
        int key = compare_expr(stmt->where);
        if (key > 0) {
            record_id_t rid = bpt_search(table->index, key);
            if (rid > 0 && rid <= table->record_count) {
                free(table->records[rid - 1]);
                table->records[rid - 1] = NULL;
                bpt_delete(table->index, key);
                
                for (int i = rid - 1; i < table->record_count - 1; i++) {
                    table->records[i] = table->records[i + 1];
                }
                table->record_count--;
                return DB_OK;
            }
        }
    }
    
    return DB_ERROR;
}

int execute_update(Executor* executor, UpdateStmt* stmt) {
    if (executor == NULL || stmt == NULL) return DB_ERROR;
    
    Table* table = executor_get_table(executor, stmt->table);
    if (table == NULL) return DB_ERROR;
    
    if (stmt->set_count == 0) return DB_ERROR;
    
    int key = 0;
    if (stmt->where != NULL) {
        key = compare_expr(stmt->where);
    }
    
    if (key > 0) {
        record_id_t rid = bpt_search(table->index, key);
        if (rid > 0 && rid <= table->record_count) {
            bpt_delete(table->index, key);
            
            char* record = (char*)malloc(256);
            record[0] = '\0';
            
            for (int i = 0; i < stmt->set_count; i++) {
                if (i > 0) strcat(record, ",");
                
                Expr* value = stmt->values[i];
                switch (value->op) {
                    case OP_INTEGER:
                        char num_str[32];
                        sprintf(num_str, "%d", value->value.integer);
                        strcat(record, num_str);
                        break;
                    case OP_STRING:
                        strcat(record, value->value.string);
                        break;
                    case OP_IDENTIFIER:
                        strcat(record, value->value.identifier);
                        break;
                    default:
                        strcat(record, "NULL");
                }
            }
            
            free(table->records[rid - 1]);
            table->records[rid - 1] = record;
            
            Expr* first_value = stmt->values[0];
            if (first_value->op == OP_INTEGER) {
                bpt_insert(table->index, first_value->value.integer, rid);
            }
            
            return DB_OK;
        }
    }
    
    return DB_ERROR;
}

void result_set_destroy(ResultSet* rs) {
    if (rs == NULL) return;
    
    for (int i = 0; i < rs->row_count; i++) {
        if (rs->rows[i]) free(rs->rows[i]);
    }
    if (rs->rows) free(rs->rows);
    
    for (int i = 0; i < rs->column_count; i++) {
        if (rs->columns[i]) free(rs->columns[i]);
    }
    if (rs->columns) free(rs->columns);
    
    free(rs);
}
