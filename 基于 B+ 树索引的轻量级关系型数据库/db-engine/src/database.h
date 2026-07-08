#ifndef DATABASE_H
#define DATABASE_H

#include "disk_manager.h"
#include "buffer_pool.h"
#include "bplus_tree.h"
#include <stddef.h>

typedef struct ResultSet {
    int affected_rows;
    char** rows;
    size_t row_count;
} ResultSet;

typedef struct Database {
    DiskManager* dm;
    BufferPool* pool;
    BPlusTree* index;
} Database;

Database* db_open(const char* filename);
void db_close(Database* db);
ResultSet* db_query(Database* db, const char* sql);
void result_set_destroy(ResultSet* rs);

#endif
