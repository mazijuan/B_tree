#include "../src/bplus_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ===== 测试1：基本删除 =====
static void test_basic_delete() {
    printf("--- Test: Basic Delete ---\n");

    BPlusTree* tree = bpt_create();
    if (tree == NULL) {
        printf("[FAIL] bpt_create\n");
        return;
    }

    // 插入 1..20
    for (int i = 1; i <= 20; i++) {
        RecordPointer rid = {i, i * 10};
        bpt_insert(tree, i, rid);
    }
    printf("[PASS] insert 1..20\n");

    // 删除 key=10
    DBStatus rc = bpt_delete(tree, 10);
    if (rc != DB_OK) {
        printf("[FAIL] delete key=10\n");
        bpt_destroy(tree);
        return;
    }
    printf("[PASS] delete key=10\n");

    // 验证 key=10 已删除
    RecordPointer found;
    if (bpt_search(tree, 10, &found) == DB_OK) {
        printf("[FAIL] key=10 still found after delete\n");
        bpt_destroy(tree);
        return;
    }
    printf("[PASS] key=10 not found after delete\n");

    // 验证其他键仍可查
    int fail = 0;
    for (int i = 1; i <= 20; i++) {
        if (i == 10) continue;
        if (bpt_search(tree, i, &found) != DB_OK) {
            printf("[FAIL] key=%d not found after deleting key=10\n", i);
            fail++;
        }
    }
    if (fail == 0) printf("[PASS] all other keys still found\n");

    // 验证结构
    if (bpt_validate(tree) != DB_OK) {
        printf("[FAIL] bpt_validate after basic delete\n");
        bpt_print(tree);
    } else {
        printf("[PASS] bpt_validate: structure correct\n");
    }

    bpt_destroy(tree);
    printf("\n");
}

// ===== 测试2：删除不存在的键 =====
static void test_delete_nonexistent() {
    printf("--- Test: Delete Non-existent Key ---\n");

    BPlusTree* tree = bpt_create();
    for (int i = 1; i <= 10; i++) {
        RecordPointer rid = {i, i * 10};
        bpt_insert(tree, i, rid);
    }

    DBStatus rc = bpt_delete(tree, 999);
    if (rc == DB_PAGE_NOT_FOUND) {
        printf("[PASS] delete non-existent key=999 returned DB_PAGE_NOT_FOUND\n");
    } else {
        printf("[FAIL] delete non-existent key=999 returned %d\n", rc);
    }

    // 确保树结构未受影响
    if (bpt_validate(tree) == DB_OK) {
        printf("[PASS] tree structure intact after failed delete\n");
    } else {
        printf("[FAIL] tree structure corrupted after failed delete\n");
    }

    bpt_destroy(tree);
    printf("\n");
}

// ===== 测试3：删除触发借键 =====
static void test_delete_borrow() {
    printf("--- Test: Delete Triggering Borrow ---\n");

    BPlusTree* tree = bpt_create();

    // 插入足够多的键以形成多叶节点
    for (int i = 1; i <= 30; i++) {
        RecordPointer rid = {i, i * 10};
        bpt_insert(tree, i, rid);
    }
    printf("[PASS] insert 1..30 (count=%d, height=%d)\n", tree->count, tree->height);

    // 连续删除前6个键，使第一个叶子节点键数不足，触发借键
    for (int i = 1; i <= 6; i++) {
        DBStatus rc = bpt_delete(tree, i);
        if (rc != DB_OK) {
            printf("[FAIL] delete key=%d\n", i);
            bpt_destroy(tree);
            return;
        }
    }
    printf("[PASS] delete keys 1..6\n");

    // 验证已删除的键不存在
    int fail = 0;
    for (int i = 1; i <= 6; i++) {
        RecordPointer found;
        if (bpt_search(tree, i, &found) == DB_OK) {
            printf("[FAIL] key=%d still found\n", i);
            fail++;
        }
    }
    if (fail == 0) printf("[PASS] deleted keys not found\n");

    // 验证剩余键仍可查
    fail = 0;
    for (int i = 7; i <= 30; i++) {
        RecordPointer found;
        if (bpt_search(tree, i, &found) != DB_OK) {
            printf("[FAIL] key=%d not found after borrow\n", i);
            fail++;
        }
    }
    if (fail == 0) printf("[PASS] remaining keys all found\n");

    // 验证结构
    if (bpt_validate(tree) == DB_OK) {
        printf("[PASS] bpt_validate: structure correct after borrow\n");
    } else {
        printf("[FAIL] bpt_validate after borrow\n");
        bpt_print(tree);
    }

    bpt_destroy(tree);
    printf("\n");
}

// ===== 测试4：删除触发合并 =====
static void test_delete_merge() {
    printf("--- Test: Delete Triggering Merge ---\n");

    BPlusTree* tree = bpt_create();

    // 插入 1..50
    for (int i = 1; i <= 50; i++) {
        RecordPointer rid = {i, i * 10};
        bpt_insert(tree, i, rid);
    }
    printf("[PASS] insert 1..50 (count=%d, height=%d)\n", tree->count, tree->height);

    // 连续删除大量键，触发合并
    for (int i = 1; i <= 35; i++) {
        DBStatus rc = bpt_delete(tree, i);
        if (rc != DB_OK) {
            printf("[FAIL] delete key=%d\n", i);
            bpt_destroy(tree);
            return;
        }
    }
    printf("[PASS] delete keys 1..35\n");

    // 验证剩余键
    int fail = 0;
    for (int i = 36; i <= 50; i++) {
        RecordPointer found;
        if (bpt_search(tree, i, &found) != DB_OK) {
            printf("[FAIL] key=%d not found after merge\n", i);
            fail++;
        }
    }
    if (fail == 0) printf("[PASS] remaining keys 36..50 all found\n");

    // 验证结构
    if (bpt_validate(tree) == DB_OK) {
        printf("[PASS] bpt_validate: structure correct after merge\n");
    } else {
        printf("[FAIL] bpt_validate after merge\n");
        bpt_print(tree);
    }

    printf("[INFO] tree height=%d, count=%d\n", tree->height, tree->count);

    bpt_destroy(tree);
    printf("\n");
}

// ===== 测试5：删除直到树为空 =====
static void test_delete_all() {
    printf("--- Test: Delete All Keys ---\n");

    BPlusTree* tree = bpt_create();

    // 插入 1..20
    for (int i = 1; i <= 20; i++) {
        RecordPointer rid = {i, i * 10};
        bpt_insert(tree, i, rid);
    }
    printf("[PASS] insert 1..20\n");

    // 删除所有键
    int fail = 0;
    for (int i = 1; i <= 20; i++) {
        DBStatus rc = bpt_delete(tree, i);
        if (rc != DB_OK) {
            printf("[FAIL] delete key=%d\n", i);
            fail++;
        }
    }
    if (fail == 0) printf("[PASS] deleted all 20 keys\n");

    // 验证树为空
    if (tree->count == 0) {
        printf("[PASS] tree count is 0\n");
    } else {
        printf("[FAIL] tree count=%d (expected 0)\n", tree->count);
    }

    // 验证结构
    if (bpt_validate(tree) == DB_OK) {
        printf("[PASS] bpt_validate: structure correct (empty tree)\n");
    } else {
        printf("[FAIL] bpt_validate on empty tree\n");
        bpt_print(tree);
    }

    bpt_destroy(tree);
    printf("\n");
}

// ===== 测试6：随机插入后随机删除 =====
static void test_random_insert_delete() {
    printf("--- Test: Random Insert & Delete ---\n");

    BPlusTree* tree = bpt_create();
    srand(123);

    const int N = 200;
    int* keys = (int*)malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) keys[i] = i + 1;

    // 打乱顺序
    for (int i = N - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = keys[i]; keys[i] = keys[j]; keys[j] = tmp;
    }

    // 随机插入
    for (int i = 0; i < N; i++) {
        RecordPointer rid = {keys[i], keys[i] * 100};
        bpt_insert(tree, keys[i], rid);
    }
    printf("[PASS] inserted %d keys in random order\n", N);

    // 验证结构
    if (bpt_validate(tree) != DB_OK) {
        printf("[FAIL] bpt_validate after random insert\n");
        bpt_print(tree);
        free(keys);
        bpt_destroy(tree);
        return;
    }
    printf("[PASS] bpt_validate after random insert\n");

    // 随机删除一半
    int half = N / 2;
    for (int i = 0; i < half; i++) {
        DBStatus rc = bpt_delete(tree, keys[i]);
        if (rc != DB_OK) {
            printf("[FAIL] delete key=%d\n", keys[i]);
            bpt_print(tree);
            free(keys);
            bpt_destroy(tree);
            return;
        }
    }
    printf("[PASS] deleted %d keys\n", half);

    // 验证已删除的键不存在
    int fail = 0;
    for (int i = 0; i < half; i++) {
        RecordPointer found;
        if (bpt_search(tree, keys[i], &found) == DB_OK) {
            printf("[FAIL] deleted key=%d still found\n", keys[i]);
            fail++;
        }
    }
    if (fail == 0) printf("[PASS] all deleted keys confirmed gone\n");

    // 验证未删除的键仍存在
    fail = 0;
    for (int i = half; i < N; i++) {
        RecordPointer found;
        if (bpt_search(tree, keys[i], &found) != DB_OK) {
            printf("[FAIL] key=%d not found\n", keys[i]);
            fail++;
        }
    }
    if (fail == 0) printf("[PASS] all remaining keys confirmed present\n");

    // 验证结构
    if (bpt_validate(tree) == DB_OK) {
        printf("[PASS] bpt_validate: structure correct after random delete\n");
    } else {
        printf("[FAIL] bpt_validate after random delete\n");
        bpt_print(tree);
    }

    printf("[INFO] tree height=%d, count=%d\n", tree->height, tree->count);

    free(keys);
    bpt_destroy(tree);
    printf("\n");
}

// ===== 测试7：删除后再插入 =====
static void test_delete_then_reinsert() {
    printf("--- Test: Delete Then Reinsert ---\n");

    BPlusTree* tree = bpt_create();

    for (int i = 1; i <= 15; i++) {
        RecordPointer rid = {i, i * 10};
        bpt_insert(tree, i, rid);
    }
    printf("[PASS] insert 1..15\n");

    // 删除 5, 10, 15
    bpt_delete(tree, 5);
    bpt_delete(tree, 10);
    bpt_delete(tree, 15);
    printf("[PASS] delete 5, 10, 15\n");

    // 重新插入
    RecordPointer rid5 = {5, 555};
    RecordPointer rid10 = {10, 1010};
    RecordPointer rid15 = {15, 1515};
    bpt_insert(tree, 5, rid5);
    bpt_insert(tree, 10, rid10);
    bpt_insert(tree, 15, rid15);
    printf("[PASS] reinsert 5, 10, 15 with new rids\n");

    // 验证新值
    RecordPointer found;
    int fail = 0;

    if (bpt_search(tree, 5, &found) != DB_OK || found.offset != 555) {
        printf("[FAIL] key=5 rid mismatch\n");
        fail++;
    }
    if (bpt_search(tree, 10, &found) != DB_OK || found.offset != 1010) {
        printf("[FAIL] key=10 rid mismatch\n");
        fail++;
    }
    if (bpt_search(tree, 15, &found) != DB_OK || found.offset != 1515) {
        printf("[FAIL] key=15 rid mismatch\n");
        fail++;
    }
    if (fail == 0) printf("[PASS] reinserted keys have correct new rids\n");

    if (bpt_validate(tree) == DB_OK) {
        printf("[PASS] bpt_validate: structure correct\n");
    } else {
        printf("[FAIL] bpt_validate after reinsert\n");
        bpt_print(tree);
    }

    bpt_destroy(tree);
    printf("\n");
}

// ===== 测试8：删除边界情况（删除最小/最大键） =====
static void test_delete_boundary() {
    printf("--- Test: Delete Boundary Keys ---\n");

    BPlusTree* tree = bpt_create();

    for (int i = 10; i <= 100; i += 10) {
        RecordPointer rid = {i, i * 10};
        bpt_insert(tree, i, rid);
    }
    printf("[PASS] insert 10,20,...,100\n");

    // 删除最小键
    DBStatus rc = bpt_delete(tree, 10);
    if (rc == DB_OK) {
        printf("[PASS] delete min key=10\n");
    } else {
        printf("[FAIL] delete min key=10\n");
    }

    // 删除最大键
    rc = bpt_delete(tree, 100);
    if (rc == DB_OK) {
        printf("[PASS] delete max key=100\n");
    } else {
        printf("[FAIL] delete max key=100\n");
    }

    // 验证结构
    if (bpt_validate(tree) == DB_OK) {
        printf("[PASS] bpt_validate: structure correct\n");
    } else {
        printf("[FAIL] bpt_validate after boundary delete\n");
        bpt_print(tree);
    }

    // 验证中间键仍可查
    int fail = 0;
    for (int i = 20; i <= 90; i += 10) {
        RecordPointer found;
        if (bpt_search(tree, i, &found) != DB_OK) fail++;
    }
    if (fail == 0) printf("[PASS] middle keys still found\n");

    bpt_destroy(tree);
    printf("\n");
}

int main() {
    printf("=== B+Tree Delete Tests ===\n\n");

    test_basic_delete();
    test_delete_nonexistent();
    test_delete_borrow();
    test_delete_merge();
    test_delete_all();
    test_random_insert_delete();
    test_delete_then_reinsert();
    test_delete_boundary();

    printf("=== All B+Tree Delete tests completed! ===\n");
    return 0;
}
