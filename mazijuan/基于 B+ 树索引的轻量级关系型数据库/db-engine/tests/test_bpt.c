#include "../src/bplus_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int passed = 0;
static int failed = 0;

#define TEST(name) do { \
    printf("  [TEST] %s ... ", name); \
} while(0)

#define PASS() do { \
    printf("PASS\n"); \
    passed++; \
} while(0)

#define FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
    failed++; \
} while(0)

static void test_create_destroy() {
    TEST("create and destroy");
    BPlusTree* tree = bpt_create();
    if (tree == NULL) {
        FAIL("tree is NULL");
        return;
    }
    if (tree->root == NULL) {
        FAIL("root is NULL");
        bpt_destroy(tree);
        return;
    }
    if (bpt_size(tree) != 0) {
        FAIL("initial size should be 0");
        bpt_destroy(tree);
        return;
    }
    bpt_destroy(tree);
    PASS();
}

static void test_insert_single() {
    TEST("insert single key");
    BPlusTree* tree = bpt_create();
    DBStatus status = bpt_insert(tree, 42, 100);
    if (status != DB_OK) {
        FAIL("insert failed");
        bpt_destroy(tree);
        return;
    }
    if (bpt_size(tree) != 1) {
        FAIL("size should be 1");
        bpt_destroy(tree);
        return;
    }
    record_id_t rid = bpt_search(tree, 42);
    if (rid != 100) {
        FAIL("search returned wrong record id");
        bpt_destroy(tree);
        return;
    }
    bpt_destroy(tree);
    PASS();
}

static void test_insert_multiple() {
    TEST("insert multiple keys");
    BPlusTree* tree = bpt_create();
    int keys[] = {5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
    int n = sizeof(keys) / sizeof(keys[0]);

    for (int i = 0; i < n; i++) {
        DBStatus status = bpt_insert(tree, keys[i], keys[i] * 10);
        if (status != DB_OK) {
            FAIL("insert failed");
            bpt_destroy(tree);
            return;
        }
    }

    if (bpt_size(tree) != n) {
        FAIL("size mismatch");
        bpt_destroy(tree);
        return;
    }

    for (int i = 0; i < n; i++) {
        record_id_t rid = bpt_search(tree, keys[i]);
        if (rid != (record_id_t)keys[i] * 10) {
            FAIL("search returned wrong value");
            bpt_destroy(tree);
            return;
        }
    }

    bpt_destroy(tree);
    PASS();
}

static void test_insert_duplicate() {
    TEST("insert duplicate key (update)");
    BPlusTree* tree = bpt_create();
    bpt_insert(tree, 42, 100);
    bpt_insert(tree, 42, 200);

    if (bpt_size(tree) != 1) {
        FAIL("size should still be 1");
        bpt_destroy(tree);
        return;
    }

    record_id_t rid = bpt_search(tree, 42);
    if (rid != 200) {
        FAIL("duplicate insert should update value");
        bpt_destroy(tree);
        return;
    }

    bpt_destroy(tree);
    PASS();
}

static void test_search_not_found() {
    TEST("search non-existent key");
    BPlusTree* tree = bpt_create();
    bpt_insert(tree, 42, 100);

    record_id_t rid = bpt_search(tree, 99);
    if (rid != (record_id_t)-1) {
        FAIL("should return -1 for non-existent key");
        bpt_destroy(tree);
        return;
    }

    bpt_destroy(tree);
    PASS();
}

static void test_leaf_split() {
    TEST("leaf node split");
    BPlusTree* tree = bpt_create();

    for (int i = 0; i < 15; i++) {
        bpt_insert(tree, i, i * 100);
    }

    if (bpt_size(tree) != 15) {
        FAIL("size should be 15");
        bpt_destroy(tree);
        return;
    }

    for (int i = 0; i < 15; i++) {
        record_id_t rid = bpt_search(tree, i);
        if (rid != (record_id_t)i * 100) {
            FAIL("search after split failed");
            bpt_destroy(tree);
            return;
        }
    }

    bpt_destroy(tree);
    PASS();
}

static void test_root_split() {
    TEST("root node split (tree grows height)");
    BPlusTree* tree = bpt_create();
    int initial_height = tree->height;

    for (int i = 0; i < 50; i++) {
        bpt_insert(tree, i, i);
    }

    if (tree->height <= initial_height) {
        FAIL("tree height should increase");
        bpt_destroy(tree);
        return;
    }

    for (int i = 0; i < 50; i++) {
        record_id_t rid = bpt_search(tree, i);
        if (rid != (record_id_t)i) {
            FAIL("search after root split failed");
            bpt_destroy(tree);
            return;
        }
    }

    bpt_destroy(tree);
    PASS();
}

static void test_random_insert() {
    TEST("random insert 1000 keys");
    BPlusTree* tree = bpt_create();
    int n = 1000;
    int* keys = (int*)malloc(n * sizeof(int));

    for (int i = 0; i < n; i++) {
        keys[i] = rand() % 100000;
        DBStatus status = bpt_insert(tree, keys[i], keys[i] * 2);
        if (status != DB_OK) {
            FAIL("random insert failed");
            free(keys);
            bpt_destroy(tree);
            return;
        }
    }

    for (int i = 0; i < n; i++) {
        record_id_t rid = bpt_search(tree, keys[i]);
        if (rid != (record_id_t)keys[i] * 2) {
            FAIL("random search failed");
            free(keys);
            bpt_destroy(tree);
            return;
        }
    }

    free(keys);
    bpt_destroy(tree);
    PASS();
}

static void test_delete_single() {
    TEST("delete single key");
    BPlusTree* tree = bpt_create();
    bpt_insert(tree, 42, 100);

    DBStatus status = bpt_delete(tree, 42);
    if (status != DB_OK) {
        FAIL("delete failed");
        bpt_destroy(tree);
        return;
    }

    if (bpt_size(tree) != 0) {
        FAIL("size should be 0 after delete");
        bpt_destroy(tree);
        return;
    }

    record_id_t rid = bpt_search(tree, 42);
    if (rid != (record_id_t)-1) {
        FAIL("should not find deleted key");
        bpt_destroy(tree);
        return;
    }

    bpt_destroy(tree);
    PASS();
}

static void test_delete_not_found() {
    TEST("delete non-existent key");
    BPlusTree* tree = bpt_create();
    bpt_insert(tree, 42, 100);

    DBStatus status = bpt_delete(tree, 99);
    if (status != DB_OK) {
        FAIL("delete non-existent should return OK");
        bpt_destroy(tree);
        return;
    }

    if (bpt_size(tree) != 1) {
        FAIL("size should remain 1");
        bpt_destroy(tree);
        return;
    }

    bpt_destroy(tree);
    PASS();
}

static void test_delete_multiple() {
    TEST("delete multiple keys");
    BPlusTree* tree = bpt_create();
    int n = 20;

    for (int i = 0; i < n; i++) {
        bpt_insert(tree, i, i * 10);
    }

    for (int i = 0; i < n; i += 2) {
        bpt_delete(tree, i);
    }

    if (bpt_size(tree) != n / 2) {
        FAIL("size should be n/2");
        bpt_destroy(tree);
        return;
    }

    for (int i = 0; i < n; i++) {
        record_id_t rid = bpt_search(tree, i);
        if (i % 2 == 0) {
            if (rid != (record_id_t)-1) {
                FAIL("deleted key should not be found");
                bpt_destroy(tree);
                return;
            }
        } else {
            if (rid != (record_id_t)i * 10) {
                FAIL("remaining key should be found");
                bpt_destroy(tree);
                return;
            }
        }
    }

    bpt_destroy(tree);
    PASS();
}

static void test_delete_with_merge() {
    TEST("delete triggering merge");
    BPlusTree* tree = bpt_create();
    int n = 30;

    for (int i = 0; i < n; i++) {
        bpt_insert(tree, i, i);
    }

    for (int i = 0; i < 25; i++) {
        bpt_delete(tree, i);
    }

    if (bpt_size(tree) != 5) {
        FAIL("size should be 5");
        bpt_destroy(tree);
        return;
    }

    for (int i = 25; i < n; i++) {
        record_id_t rid = bpt_search(tree, i);
        if (rid != (record_id_t)i) {
            FAIL("remaining keys should be found");
            bpt_destroy(tree);
            return;
        }
    }

    bpt_destroy(tree);
    PASS();
}

static void test_delete_all() {
    TEST("delete all keys");
    BPlusTree* tree = bpt_create();
    int n = 50;

    for (int i = 0; i < n; i++) {
        bpt_insert(tree, i, i);
    }

    for (int i = 0; i < n; i++) {
        bpt_delete(tree, i);
    }

    if (bpt_size(tree) != 0) {
        FAIL("size should be 0");
        bpt_destroy(tree);
        return;
    }

    bpt_destroy(tree);
    PASS();
}

static int range_count = 0;
static int range_sum = 0;

static void range_callback(int key, record_id_t rid, void* ctx) {
    range_count++;
    range_sum += key;
    (void)rid;
    (void)ctx;
}

static void test_range_query_small() {
    TEST("range query small range");
    BPlusTree* tree = bpt_create();

    for (int i = 0; i < 20; i++) {
        bpt_insert(tree, i, i * 10);
    }

    range_count = 0;
    range_sum = 0;
    bpt_range_query(tree, 5, 10, range_callback, NULL);

    if (range_count != 6) {
        FAIL("range count should be 6");
        bpt_destroy(tree);
        return;
    }

    int expected_sum = 5 + 6 + 7 + 8 + 9 + 10;
    if (range_sum != expected_sum) {
        FAIL("range sum mismatch");
        bpt_destroy(tree);
        return;
    }

    bpt_destroy(tree);
    PASS();
}

static void test_range_query_full() {
    TEST("range query full range");
    BPlusTree* tree = bpt_create();
    int n = 100;

    for (int i = 0; i < n; i++) {
        bpt_insert(tree, i, i);
    }

    range_count = 0;
    range_sum = 0;
    bpt_range_query(tree, 0, 99, range_callback, NULL);

    if (range_count != n) {
        FAIL("full range count mismatch");
        bpt_destroy(tree);
        return;
    }

    bpt_destroy(tree);
    PASS();
}

static void test_range_query_empty() {
    TEST("range query empty result");
    BPlusTree* tree = bpt_create();

    for (int i = 0; i < 10; i++) {
        bpt_insert(tree, i, i);
    }

    range_count = 0;
    bpt_range_query(tree, 20, 30, range_callback, NULL);

    if (range_count != 0) {
        FAIL("range should be empty");
        bpt_destroy(tree);
        return;
    }

    bpt_destroy(tree);
    PASS();
}

static void test_range_after_delete() {
    TEST("range query after delete");
    BPlusTree* tree = bpt_create();

    for (int i = 0; i < 30; i++) {
        bpt_insert(tree, i, i);
    }

    for (int i = 10; i < 20; i++) {
        bpt_delete(tree, i);
    }

    range_count = 0;
    range_sum = 0;
    bpt_range_query(tree, 0, 29, range_callback, NULL);

    if (range_count != 20) {
        FAIL("range count should be 20 after delete");
        bpt_destroy(tree);
        return;
    }

    bpt_destroy(tree);
    PASS();
}

static void test_insert_reverse_order() {
    TEST("insert in reverse order");
    BPlusTree* tree = bpt_create();
    int n = 50;

    for (int i = n - 1; i >= 0; i--) {
        DBStatus status = bpt_insert(tree, i, i);
        if (status != DB_OK) {
            FAIL("reverse insert failed");
            bpt_destroy(tree);
            return;
        }
    }

    if (bpt_size(tree) != n) {
        FAIL("size mismatch");
        bpt_destroy(tree);
        return;
    }

    for (int i = 0; i < n; i++) {
        record_id_t rid = bpt_search(tree, i);
        if (rid != (record_id_t)i) {
            FAIL("search failed");
            bpt_destroy(tree);
            return;
        }
    }

    bpt_destroy(tree);
    PASS();
}

static void test_random_insert_delete() {
    TEST("random insert and delete 2000 operations");
    BPlusTree* tree = bpt_create();
    int n = 2000;
    int* keys = (int*)malloc(n * sizeof(int));
    int* present = (int*)malloc(n * sizeof(int));

    for (int i = 0; i < n; i++) {
        keys[i] = i;
        present[i] = 0;
    }

    srand(42);
    for (int i = 0; i < n; i++) {
        int idx = rand() % n;
        if (!present[idx]) {
            bpt_insert(tree, keys[idx], keys[idx] * 10);
            present[idx] = 1;
        } else {
            bpt_delete(tree, keys[idx]);
            present[idx] = 0;
        }
    }

    int expected_size = 0;
    for (int i = 0; i < n; i++) {
        if (present[i]) expected_size++;
    }

    if (bpt_size(tree) != expected_size) {
        FAIL("size mismatch after random operations");
        free(keys);
        free(present);
        bpt_destroy(tree);
        return;
    }

    for (int i = 0; i < n; i++) {
        record_id_t rid = bpt_search(tree, keys[i]);
        if (present[i]) {
            if (rid != (record_id_t)keys[i] * 10) {
                FAIL("present key not found correctly");
                free(keys);
                free(present);
                bpt_destroy(tree);
                return;
            }
        } else {
            if (rid != (record_id_t)-1) {
                FAIL("absent key should not be found");
                free(keys);
                free(present);
                bpt_destroy(tree);
                return;
            }
        }
    }

    free(keys);
    free(present);
    bpt_destroy(tree);
    PASS();
}

int main() {
    srand(time(NULL));

    printf("Running B+ Tree tests...\n\n");

    printf("=== Creation ===\n");
    test_create_destroy();
    printf("\n");

    printf("=== Insertion ===\n");
    test_insert_single();
    test_insert_multiple();
    test_insert_duplicate();
    test_insert_reverse_order();
    test_leaf_split();
    test_root_split();
    test_random_insert();
    printf("\n");

    printf("=== Search ===\n");
    test_search_not_found();
    printf("\n");

    printf("=== Deletion ===\n");
    test_delete_single();
    test_delete_not_found();
    test_delete_multiple();
    test_delete_with_merge();
    test_delete_all();
    printf("\n");

    printf("=== Range Query ===\n");
    test_range_query_small();
    test_range_query_full();
    test_range_query_empty();
    test_range_after_delete();
    printf("\n");

    printf("=== Stress Test ===\n");
    test_random_insert_delete();
    printf("\n");

    printf("=========================\n");
    printf("  Passed: %d\n", passed);
    printf("  Failed: %d\n", failed);
    printf("  Total:  %d\n", passed + failed);
    printf("=========================\n");

    return failed == 0 ? 0 : 1;
}
