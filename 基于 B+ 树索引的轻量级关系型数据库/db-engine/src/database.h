#ifndef DATABASE_H
#define DATABASE_H

#include "executor.h"
#include "wal.h"
#include <stdint.h>

typedef struct {
    Executor* executor;
    char* filename;
    WalManager* wal;
} Database;

Database* db_open(const char* filename);
ResultSet* db_query(Database* db, const char* sql);
void db_close(Database* db);
int db_begin_transaction(Database* db);
int db_commit(Database* db);
int db_rollback(Database* db);

#endif
