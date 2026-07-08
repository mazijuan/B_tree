#include "../src/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_DB "test_sql.db"

int main(void) {
    remove(TEST_DB);
    Database* db = db_open(TEST_DB);
    if (!db) {
        printf("[FAIL] open database\n");
        return 1;
    }

    ResultSet* rs = db_query(db, "INSERT INTO users VALUES 1");
    if (!rs || rs->affected_rows != 1) {
        printf("[FAIL] INSERT query\n");
        db_close(db);
        return 1;
    }
    result_set_destroy(rs);

    rs = db_query(db, "SELECT id FROM users WHERE id = 1");
    if (!rs || rs->row_count != 1) {
        printf("[FAIL] SELECT query\n");
        db_close(db);
        return 1;
    }
    result_set_destroy(rs);

    rs = db_query(db, "DELETE FROM users WHERE id = 1");
    if (!rs || rs->affected_rows != 1) {
        printf("[FAIL] DELETE query\n");
        db_close(db);
        return 1;
    }
    result_set_destroy(rs);

    db_close(db);
    printf("[PASS] SQL integration smoke test\n");
    return 0;
}
