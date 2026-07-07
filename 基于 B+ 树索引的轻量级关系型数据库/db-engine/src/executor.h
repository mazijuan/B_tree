#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"
#include "bplus_tree.h"
#include "types.h"
#include <stdint.h>

#define MAX_COLUMNS 32
#define MAX_ROWS 1000

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
} Table;

typedef struct {
    Table* tables;
    int table_count;
    int max_tables;
} Executor;

Executor* executor_init();
void executor_destroy(Executor* executor);
Table* executor_get_table(Executor* executor, const char* table_name);
Table* executor_create_table(Executor* executor, const char* table_name);

ResultSet* execute_select(Executor* executor, SelectStmt* stmt);
int execute_insert(Executor* executor, InsertStmt* stmt);
int execute_delete(Executor* executor, DeleteStmt* stmt);
int execute_update(Executor* executor, UpdateStmt* stmt);

void result_set_destroy(ResultSet* rs);

#endif
