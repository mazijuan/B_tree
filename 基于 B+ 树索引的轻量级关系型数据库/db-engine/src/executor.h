#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"
#include "bplus_tree.h"
#include "types.h"
#include <stdint.h>

#define MAX_COLUMNS 32
#define MAX_ROWS 1000
#define MAX_TRANSACTION_LOGS 10000

typedef struct {
    char** rows;
    int row_count;
    char** columns;
    int column_count;
} ResultSet;

typedef struct {
    char* table_name;
    BPlusTree* index;
    char** records;
    int record_count;
    int max_records;
    ColumnDef* columns;
    int column_count;
} Table;

typedef enum {
    LOG_INSERT,
    LOG_UPDATE,
    LOG_DELETE
} LogType;

typedef struct {
    LogType type;
    char* table_name;
    int key;
    record_id_t rid;
    char* old_data;
    char* new_data;
} TransactionLog;

typedef struct {
    Table* tables;
    int table_count;
    int max_tables;
    TransactionLog* txn_logs;
    int txn_log_count;
    int txn_active;
} Executor;

Executor* executor_init();
void executor_destroy(Executor* executor);
Table* executor_get_table(Executor* executor, const char* table_name);
Table* executor_create_table(Executor* executor, const char* table_name);
int executor_begin_transaction(Executor* executor);
int executor_commit(Executor* executor);
int executor_rollback(Executor* executor);

ResultSet* execute_select(Executor* executor, SelectStmt* stmt);
int execute_insert(Executor* executor, InsertStmt* stmt);
int execute_delete(Executor* executor, DeleteStmt* stmt);
int execute_update(Executor* executor, UpdateStmt* stmt);
int execute_create_table(Executor* executor, CreateTableStmt* stmt);

void result_set_destroy(ResultSet* rs);

#endif
