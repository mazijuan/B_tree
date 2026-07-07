#ifndef DATABASE_H
#define DATABASE_H

#include "executor.h"
#include <stdint.h>

typedef struct {
    Executor* executor;
    char* filename;
} Database;

Database* db_open(const char* filename);
ResultSet* db_query(Database* db, const char* sql);
void db_close(Database* db);

#endif
