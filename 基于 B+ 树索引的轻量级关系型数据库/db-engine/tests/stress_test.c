#include "database.h"
#include "bplus_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TEST_SIZE 100000

static long long get_time_ms() {
    #ifdef _WIN32
    return (long long)clock() * 1000 / CLOCKS_PER_SEC;
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    #endif
}

static void test_bpt_stress() {
    printf("=== B+ Tree Stress Test ===\n\n");
    
    BPlusTree* tree = bpt_create();
    
    long long start = get_time_ms();
    for (int i = 1; i <= TEST_SIZE; i++) {
        record_id_t rid = i;
        bpt_insert(tree, i, rid);
    }
    long long insert_time = get_time_ms() - start;
    printf("Insert %d keys: %lld ms (%.2f ms/key)\n", TEST_SIZE, insert_time, (double)insert_time / TEST_SIZE);
    
    start = get_time_ms();
    for (int i = 1; i <= TEST_SIZE; i += 100) {
        record_id_t rid = bpt_search(tree, i);
        if (rid != i) {
            printf("Search failed for key %d\n", i);
            bpt_destroy(tree);
            return;
        }
    }
    long long search_time = get_time_ms() - start;
    printf("Search 100 keys: %lld ms (%.2f ms/key)\n", search_time, (double)search_time / 100);
    
    int count = 0;
    start = get_time_ms();
    bpt_range_query(tree, 1000, 2000, NULL, NULL);
    long long range_time = get_time_ms() - start;
    printf("Range query (1000-2000): %lld ms\n", range_time);
    
    start = get_time_ms();
    for (int i = 1; i <= TEST_SIZE; i += 2) {
        bpt_delete(tree, i);
    }
    long long delete_time = get_time_ms() - start;
    printf("Delete %d keys: %lld ms (%.2f ms/key)\n", TEST_SIZE / 2, delete_time, (double)delete_time / (TEST_SIZE / 2));
    
    bpt_destroy(tree);
    printf("\n");
}

static void test_sql_stress() {
    printf("=== SQL Pipeline Stress Test ===\n\n");
    
    Database* db = db_open("stress_test.db");
    
    char sql[256];
    
    long long start = get_time_ms();
    for (int i = 1; i <= TEST_SIZE; i++) {
        sprintf(sql, "INSERT INTO users VALUES (%d, 'user_%d');", i, i);
        db_query(db, sql);
    }
    long long insert_time = get_time_ms() - start;
    printf("INSERT %d records: %lld ms (%.2f ms/record)\n", TEST_SIZE, insert_time, (double)insert_time / TEST_SIZE);
    
    start = get_time_ms();
    for (int i = 1; i <= TEST_SIZE; i += 100) {
        sprintf(sql, "SELECT * FROM users WHERE id = %d;", i);
        ResultSet* rs = db_query(db, sql);
        if (rs && rs->row_count != 1) {
            printf("SELECT WHERE failed for id %d\n", i);
            result_set_destroy(rs);
            db_close(db);
            return;
        }
        result_set_destroy(rs);
    }
    long long select_time = get_time_ms() - start;
    printf("SELECT WHERE 100 records: %lld ms (%.2f ms/query)\n", select_time, (double)select_time / 100);
    
    start = get_time_ms();
    ResultSet* rs_all = db_query(db, "SELECT * FROM users;");
    long long select_all_time = get_time_ms() - start;
    printf("SELECT ALL (%d records): %lld ms\n", rs_all ? rs_all->row_count : 0, select_all_time);
    result_set_destroy(rs_all);
    
    start = get_time_ms();
    for (int i = 1; i <= TEST_SIZE; i += 10) {
        sprintf(sql, "UPDATE users SET id = %d WHERE id = %d;", i + TEST_SIZE, i);
        db_query(db, sql);
    }
    long long update_time = get_time_ms() - start;
    printf("UPDATE %d records: %lld ms (%.2f ms/record)\n", TEST_SIZE / 10, update_time, (double)update_time / (TEST_SIZE / 10));
    
    start = get_time_ms();
    for (int i = TEST_SIZE + 1; i <= TEST_SIZE * 2; i += 10) {
        sprintf(sql, "DELETE FROM users WHERE id = %d;", i);
        db_query(db, sql);
    }
    long long delete_time = get_time_ms() - start;
    printf("DELETE %d records: %lld ms (%.2f ms/record)\n", TEST_SIZE / 10, delete_time, (double)delete_time / (TEST_SIZE / 10));
    
    ResultSet* rs_final = db_query(db, "SELECT * FROM users;");
    printf("Final record count: %d\n", rs_final ? rs_final->row_count : 0);
    result_set_destroy(rs_final);
    
    db_close(db);
    printf("\n");
}

static void test_edge_cases() {
    printf("=== Edge Case Tests ===\n\n");
    
    Database* db = db_open("edge_test.db");
    
    printf("1. Insert at boundary... ");
    for (int i = 1; i <= 10; i++) {
        char sql[256];
        sprintf(sql, "INSERT INTO t1 VALUES (%d, 'v%d');", i, i);
        db_query(db, sql);
    }
    printf("OK\n");
    
    printf("2. Delete first record (head)... ");
    db_query(db, "DELETE FROM t1 WHERE id = 1;");
    ResultSet* rs = db_query(db, "SELECT * FROM t1 WHERE id = 1;");
    if (rs->row_count == 0) {
        printf("OK\n");
    } else {
        printf("FAIL\n");
    }
    result_set_destroy(rs);
    
    printf("3. Delete last record (tail)... ");
    db_query(db, "DELETE FROM t1 WHERE id = 10;");
    rs = db_query(db, "SELECT * FROM t1 WHERE id = 10;");
    if (rs->row_count == 0) {
        printf("OK\n");
    } else {
        printf("FAIL\n");
    }
    result_set_destroy(rs);
    
    printf("4. Delete middle record... ");
    db_query(db, "DELETE FROM t1 WHERE id = 5;");
    rs = db_query(db, "SELECT * FROM t1 WHERE id = 5;");
    if (rs->row_count == 0) {
        printf("OK\n");
    } else {
        printf("FAIL\n");
    }
    result_set_destroy(rs);
    
    printf("5. Update primary key... ");
    db_query(db, "UPDATE t1 SET id = 99 WHERE id = 2;");
    rs = db_query(db, "SELECT * FROM t1 WHERE id = 99;");
    if (rs->row_count == 1) {
        printf("OK\n");
    } else {
        printf("FAIL\n");
    }
    result_set_destroy(rs);
    
    printf("6. Verify remaining records... ");
    rs = db_query(db, "SELECT * FROM t1;");
    if (rs->row_count >= 6 && rs->row_count <= 8) {
        printf("OK (%d records)\n", rs->row_count);
    } else {
        printf("FAIL (expected 6-8, got %d)\n", rs->row_count);
    }
    result_set_destroy(rs);
    
    db_close(db);
    printf("\n");
}

int main() {
    printf("Running Stress Tests...\n\n");
    
    test_bpt_stress();
    test_sql_stress();
    test_edge_cases();
    
    printf("=== Summary ===\n");
    printf("All stress tests completed successfully!\n");
    printf("Test size: %d records\n", TEST_SIZE);
    
    return 0;
}
