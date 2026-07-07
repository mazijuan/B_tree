#include "database.h"
#include <stdio.h>
#include <string.h>

static int passed = 0;
static int failed = 0;

#define PASS() do { passed++; printf("  [TEST] %s ... PASS\n", __func__); } while(0)
#define FAIL(msg) do { failed++; printf("  [TEST] %s ... FAIL: %s\n", __func__, msg); } while(0)

static void test_db_open_close() {
    Database* db = db_open("test.db");
    if (db != NULL) {
        PASS();
        db_close(db);
    } else {
        FAIL("db_open returned NULL");
    }
}

static void test_db_insert_select() {
    Database* db = db_open("test.db");
    if (db == NULL) {
        FAIL("db_open failed");
        return;
    }
    
    db_query(db, "INSERT INTO users VALUES (1, 'Alice');");
    db_query(db, "INSERT INTO users VALUES (2, 'Bob');");
    
    ResultSet* rs = db_query(db, "SELECT * FROM users;");
    if (rs && rs->row_count == 2) {
        PASS();
        result_set_destroy(rs);
    } else {
        FAIL("SELECT returned wrong row count");
        if (rs) result_set_destroy(rs);
    }
    
    db_close(db);
}

static void test_db_select_where() {
    Database* db = db_open("test.db");
    if (db == NULL) {
        FAIL("db_open failed");
        return;
    }
    
    db_query(db, "INSERT INTO users VALUES (1, 'Alice');");
    db_query(db, "INSERT INTO users VALUES (2, 'Bob');");
    
    ResultSet* rs = db_query(db, "SELECT * FROM users WHERE id = 1;");
    if (rs && rs->row_count == 1) {
        PASS();
        result_set_destroy(rs);
    } else {
        FAIL("SELECT WHERE returned wrong row count");
        if (rs) result_set_destroy(rs);
    }
    
    db_close(db);
}

static void test_db_delete() {
    Database* db = db_open("test.db");
    if (db == NULL) {
        FAIL("db_open failed");
        return;
    }
    
    db_query(db, "INSERT INTO users VALUES (1, 'Alice');");
    db_query(db, "DELETE FROM users WHERE id = 1;");
    
    ResultSet* rs = db_query(db, "SELECT * FROM users;");
    if (rs && rs->row_count == 0) {
        PASS();
        result_set_destroy(rs);
    } else {
        FAIL("DELETE did not remove record");
        if (rs) result_set_destroy(rs);
    }
    
    db_close(db);
}

static void test_db_update() {
    Database* db = db_open("test.db");
    if (db == NULL) {
        FAIL("db_open failed");
        return;
    }
    
    db_query(db, "INSERT INTO users VALUES (1, 'Alice');");
    db_query(db, "UPDATE users SET id = 100 WHERE id = 1;");
    
    ResultSet* rs = db_query(db, "SELECT * FROM users WHERE id = 100;");
    if (rs && rs->row_count == 1) {
        PASS();
        result_set_destroy(rs);
    } else {
        FAIL("UPDATE did not work");
        if (rs) result_set_destroy(rs);
    }
    
    db_close(db);
}

static void test_db_full_crud() {
    Database* db = db_open("test.db");
    if (db == NULL) {
        FAIL("db_open failed");
        return;
    }
    
    db_query(db, "INSERT INTO users VALUES (1, 'Alice');");
    db_query(db, "INSERT INTO users VALUES (2, 'Bob');");
    db_query(db, "INSERT INTO users VALUES (3, 'Charlie');");
    
    ResultSet* rs1 = db_query(db, "SELECT * FROM users;");
    if (rs1->row_count != 3) {
        FAIL("INSERT failed");
        result_set_destroy(rs1);
        db_close(db);
        return;
    }
    result_set_destroy(rs1);
    
    db_query(db, "UPDATE users SET id = 10 WHERE id = 1;");
    
    ResultSet* rs2 = db_query(db, "SELECT * FROM users WHERE id = 10;");
    if (rs2->row_count != 1) {
        FAIL("UPDATE failed");
        result_set_destroy(rs2);
        db_close(db);
        return;
    }
    result_set_destroy(rs2);
    
    db_query(db, "DELETE FROM users WHERE id = 2;");
    
    ResultSet* rs3 = db_query(db, "SELECT * FROM users;");
    if (rs3->row_count != 2) {
        FAIL("DELETE failed");
        result_set_destroy(rs3);
        db_close(db);
        return;
    }
    result_set_destroy(rs3);
    
    PASS();
    db_close(db);
}

int main() {
    printf("Running Database API tests...\n\n");

    printf("=== Basic Operations ===\n");
    test_db_open_close();
    printf("\n");

    printf("=== INSERT & SELECT ===\n");
    test_db_insert_select();
    printf("\n");

    printf("=== SELECT WHERE ===\n");
    test_db_select_where();
    printf("\n");

    printf("=== DELETE ===\n");
    test_db_delete();
    printf("\n");

    printf("=== UPDATE ===\n");
    test_db_update();
    printf("\n");

    printf("=== Full CRUD ===\n");
    test_db_full_crud();
    printf("\n");

    printf("=========================\n");
    printf("  Passed: %d\n", passed);
    printf("  Failed: %d\n", failed);
    printf("  Total:  %d\n", passed + failed);
    printf("=========================\n");

    return failed == 0 ? 0 : 1;
}
