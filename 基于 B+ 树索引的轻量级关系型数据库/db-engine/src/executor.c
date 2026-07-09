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
    if (expr == NULL) return 0;
    
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
    executor->txn_logs = (TransactionLog*)malloc(MAX_TRANSACTION_LOGS * sizeof(TransactionLog));
    executor->txn_log_count = 0;
    executor->txn_active = 0;
    return executor;
}

void executor_destroy(Executor* executor) {
    if (executor == NULL) return;
    
    for (int i = 0; i < executor->txn_log_count; i++) {
        if (executor->txn_logs[i].table_name) free(executor->txn_logs[i].table_name);
        if (executor->txn_logs[i].old_data) free(executor->txn_logs[i].old_data);
        if (executor->txn_logs[i].new_data) free(executor->txn_logs[i].new_data);
    }
    if (executor->txn_logs) free(executor->txn_logs);
    
    for (int i = 0; i < executor->table_count; i++) {
        Table* table = &executor->tables[i];
        if (table->table_name) free(table->table_name);
        if (table->index) bpt_destroy(table->index);
        for (int j = 0; j < table->record_count; j++) {
            if (table->records[j]) free(table->records[j]);
        }
        if (table->records) free(table->records);
        for (int j = 0; j < table->column_count; j++) {
            if (table->columns[j].name) free(table->columns[j].name);
        }
        if (table->columns) free(table->columns);
    }
    if (executor->tables) free(executor->tables);
    free(executor);
}

int executor_begin_transaction(Executor* executor) {
    if (!executor) return -1;
    executor->txn_log_count = 0;
    executor->txn_active = 1;
    return 0;
}

int executor_commit(Executor* executor) {
    if (!executor) return -1;
    for (int i = 0; i < executor->txn_log_count; i++) {
        if (executor->txn_logs[i].table_name) free(executor->txn_logs[i].table_name);
        if (executor->txn_logs[i].old_data) free(executor->txn_logs[i].old_data);
        if (executor->txn_logs[i].new_data) free(executor->txn_logs[i].new_data);
    }
    executor->txn_log_count = 0;
    executor->txn_active = 0;
    return 0;
}

int executor_rollback(Executor* executor) {
    if (!executor || !executor->txn_active) return -1;
    
    for (int i = executor->txn_log_count - 1; i >= 0; i--) {
        TransactionLog* log = &executor->txn_logs[i];
        Table* table = executor_get_table(executor, log->table_name);
        
        if (!table) continue;
        
        switch (log->type) {
            case LOG_INSERT: {
                record_id_t rid = bpt_search(table->index, log->key);
                if (rid > 0 && rid <= table->record_count) {
                    free(table->records[rid - 1]);
                    table->records[rid - 1] = NULL;
                    bpt_delete(table->index, log->key);
                    
                    for (int j = rid - 1; j < table->record_count - 1; j++) {
                        table->records[j] = table->records[j + 1];
                    }
                    table->record_count--;
                }
                break;
            }
            case LOG_UPDATE: {
                record_id_t rid = bpt_search(table->index, log->key);
                if (rid > 0 && rid <= table->record_count) {
                    free(table->records[rid - 1]);
                    table->records[rid - 1] = strdup(log->old_data);
                }
                break;
            }
            case LOG_DELETE: {
                if (table->record_count >= table->max_records) {
                    table->max_records = table->max_records == 0 ? 100 : table->max_records * 2;
                    table->records = (char**)realloc(table->records, table->max_records * sizeof(char*));
                }
                table->records[table->record_count] = strdup(log->old_data);
                record_id_t new_rid = table->record_count + 1;
                bpt_insert(table->index, log->key, new_rid);
                table->record_count++;
                break;
            }
        }
    }
    
    for (int i = 0; i < executor->txn_log_count; i++) {
        if (executor->txn_logs[i].table_name) free(executor->txn_logs[i].table_name);
        if (executor->txn_logs[i].old_data) free(executor->txn_logs[i].old_data);
        if (executor->txn_logs[i].new_data) free(executor->txn_logs[i].new_data);
    }
    executor->txn_log_count = 0;
    executor->txn_active = 0;
    return 0;
}

static void add_transaction_log(Executor* executor, LogType type, const char* table_name, 
                                int key, record_id_t rid, const char* old_data, const char* new_data) {
    if (!executor || !executor->txn_active || executor->txn_log_count >= MAX_TRANSACTION_LOGS) {
        return;
    }
    
    TransactionLog* log = &executor->txn_logs[executor->txn_log_count++];
    log->type = type;
    log->table_name = table_name ? strdup(table_name) : NULL;
    log->key = key;
    log->rid = rid;
    log->old_data = old_data ? strdup(old_data) : NULL;
    log->new_data = new_data ? strdup(new_data) : NULL;
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
    table->columns = NULL;
    table->column_count = 0;
    
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
    
    int key = 0;
    if (stmt->value_count > 0) {
        Expr* first_value = stmt->values[0];
        if (first_value->op == OP_INTEGER) {
            key = first_value->value.integer;
            bpt_insert(table->index, key, rid);
        }
    }
    
    add_transaction_log(executor, LOG_INSERT, table->table_name, key, rid, NULL, record);
    
    table->record_count++;
    return DB_OK;
}

typedef struct {
    Table* table;
    int* delete_keys;
    int delete_count;
    int max_keys;
} DeleteContext;

static void delete_callback(int key, record_id_t rid, void* ctx) {
    DeleteContext* context = (DeleteContext*)ctx;
    if (context->delete_count < context->max_keys) {
        context->delete_keys[context->delete_count++] = key;
    }
}

int execute_delete(Executor* executor, DeleteStmt* stmt) {
    if (executor == NULL || stmt == NULL) return DB_ERROR;
    
    Table* table = executor_get_table(executor, stmt->table);
    if (table == NULL) return DB_ERROR;
    
    if (stmt->where != NULL) {
        int key = compare_expr(stmt->where);
        
        if (stmt->where->op == OP_EQ && key > 0) {
            record_id_t rid = bpt_search(table->index, key);
            if (rid > 0 && rid <= table->record_count) {
                add_transaction_log(executor, LOG_DELETE, table->table_name, key, rid, table->records[rid - 1], NULL);
                
                free(table->records[rid - 1]);
                table->records[rid - 1] = NULL;
                bpt_delete(table->index, key);
                
                for (int i = rid - 1; i < table->record_count - 1; i++) {
                    table->records[i] = table->records[i + 1];
                }
                table->record_count--;
                return DB_OK;
            }
        } else {
            int delete_keys[1000];
            DeleteContext context = {
                .table = table,
                .delete_keys = delete_keys,
                .delete_count = 0,
                .max_keys = 1000
            };
            
            int op = stmt->where->op;
            int low = 0, high = 0;
            
            if (op == OP_GT) {
                low = key + 1;
                high = 1000000;
            } else if (op == OP_LT) {
                low = 0;
                high = key - 1;
            } else if (op == OP_GE) {
                low = key;
                high = 1000000;
            } else if (op == OP_LE) {
                low = 0;
                high = key;
            }
            
            bpt_range_query(table->index, low, high, delete_callback, &context);
            
            if (context.delete_count > 0) {
                for (int i = context.delete_count - 1; i >= 0; i--) {
                    int delete_key = context.delete_keys[i];
                    record_id_t rid = bpt_search(table->index, delete_key);
                    if (rid > 0 && rid <= table->record_count) {
                        add_transaction_log(executor, LOG_DELETE, table->table_name, delete_key, rid, table->records[rid - 1], NULL);
                        
                        free(table->records[rid - 1]);
                        table->records[rid - 1] = NULL;
                        bpt_delete(table->index, delete_key);
                        
                        for (int j = rid - 1; j < table->record_count - 1; j++) {
                            table->records[j] = table->records[j + 1];
                        }
                        table->record_count--;
                    }
                }
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
            char* old_record = table->records[rid - 1];
            char* new_record = (char*)malloc(256);
            strcpy(new_record, old_record);
            
            int column_count = table->column_count;
            if (column_count == 0) column_count = 4;
            
            char* parts[16];
            int part_count = 0;
            char* token = strtok((char*)old_record, ",");
            while (token != NULL && part_count < 16) {
                parts[part_count++] = strdup(token);
                token = strtok(NULL, ",");
            }
            
            for (int i = 0; i < stmt->set_count; i++) {
                const char* col_name = stmt->columns[i];
                Expr* value = stmt->values[i];
                
                for (int j = 0; j < part_count; j++) {
                    if (table->column_count > 0 && j < table->column_count) {
                        if (strcmp(table->columns[j].name, col_name) == 0) {
                            free(parts[j]);
                            if (value->op == OP_INTEGER) {
                                char num_str[32];
                                sprintf(num_str, "%d", value->value.integer);
                                parts[j] = strdup(num_str);
                            } else if (value->op == OP_STRING) {
                                parts[j] = strdup(value->value.string);
                            } else {
                                parts[j] = strdup("NULL");
                            }
                            break;
                        }
                    }
                }
            }
            
            new_record[0] = '\0';
            for (int i = 0; i < part_count; i++) {
                if (i > 0) strcat(new_record, ",");
                strcat(new_record, parts[i]);
                free(parts[i]);
            }
            
            add_transaction_log(executor, LOG_UPDATE, table->table_name, key, rid, old_record, new_record);
            
            free(table->records[rid - 1]);
            table->records[rid - 1] = new_record;
            
            return DB_OK;
        }
    }
    
    return DB_ERROR;
}

int execute_create_table(Executor* executor, CreateTableStmt* stmt) {
    if (executor == NULL || stmt == NULL) return DB_ERROR;
    
    if (executor_get_table(executor, stmt->table) != NULL) {
        return DB_ERROR;
    }
    
    if (executor->table_count >= executor->max_tables) {
        executor->max_tables = executor->max_tables == 0 ? 8 : executor->max_tables * 2;
        executor->tables = (Table*)realloc(executor->tables, executor->max_tables * sizeof(Table));
    }
    
    Table* table = &executor->tables[executor->table_count++];
    table->table_name = strdup(stmt->table);
    table->index = bpt_create();
    table->records = NULL;
    table->record_count = 0;
    table->max_records = 0;
    table->column_count = stmt->column_count;
    table->columns = NULL;
    
    if (stmt->column_count > 0 && stmt->columns != NULL) {
        table->columns = (ColumnDef*)malloc(stmt->column_count * sizeof(ColumnDef));
        for (int i = 0; i < stmt->column_count; i++) {
            table->columns[i].name = strdup(stmt->columns[i].name);
            table->columns[i].type = stmt->columns[i].type;
        }
    }
    
    return DB_OK;
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
