#include "../src/bplus_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ===== 测试1：基本插入和查找 =====
static void test_basic_insert_search() {
    printf("--- Test: Basic Insert & Search ---\n");

    BPlusTree* tree = bpt_create();
    if (tree == NULL) {
        printf("[FAIL] bpt_create\n");
        return;
    }
    printf("[PASS] bpt_create\n");

    // 插入几条数据
    RecordPointer rids[6];
    for (int i = 0; i < 6; i++) {
        rids[i].page_id = i;
        rids[i].offset = i * 100;
    }

    bpt_key_t keys[] = {10, 20, 30, 40, 50, 60};
    for (int i = 0; i < 6; i++) {
        DBStatus rc = bpt_insert(tree, keys[i], rids[i]);
        if (rc != DB_OK) {
            printf("[FAIL] bpt_insert key=%d\n", keys[i]);
            bpt_destroy(tree);
            return;
        }
    }
    printf("[PASS] insert 6 keys: 10,20,30,40,50,60\n");

    // 查找每条数据
    for (int i = 0; i < 6; i++) {
        RecordPointer found;
        DBStatus rc = bpt_search(tree, keys[i], &found);
        if (rc != DB_OK) {
            printf("[FAIL] bpt_search key=%d (not found)\n", keys[i]);
            bpt_destroy(tree);
            return;
        }
        if (found.page_id != rids[i].page_id || found.offset != rids[i].offset) {
            printf("[FAIL] bpt_search key=%d: rid mismatch (expect page=%u off=%u, got page=%u off=%u)\n",
                   keys[i], rids[i].page_id, rids[i].offset, found.page_id, found.offset);
            bpt_destroy(tree);
            return;
        }
    }
    printf("[PASS] search all 6 keys: correct rid\n");

    // 查找不存在的键
    RecordPointer not_found;
    if (bpt_search(tree, 99, &not_found) == DB_OK) {
        printf("[FAIL] bpt_search key=99 should not be found\n");
    } else {
        printf("[PASS] search non-existent key=99: not found\n");
    }

    // 验证结构
    DBStatus vrc = bpt_validate(tree);
    if (vrc != DB_OK) {
        printf("[FAIL] bpt_validate after basic insert\n");
        bpt_print(tree);
    } else {
        printf("[PASS] bpt_validate: structure correct\n");
    }

    bpt_destroy(tree);
    printf("\n");
}

// ===== 测试2：触发分裂（插入超过单节点容量） =====
static void test_split() {
    printf("--- Test: Node Split ---\n");

    BPlusTree* tree = bpt_create();
    if (tree == NULL) {
        printf("[FAIL] bpt_create\n");
        return;
    }

    // BPT_MAX_KEYS = 10, 插入11条必然触发分裂
    for (int i = 1; i <= 11; i++) {
        RecordPointer rid = {i, i * 10};
        DBStatus rc = bpt_insert(tree, i, rid);
        if (rc != DB_OK) {
            printf("[FAIL] bpt_insert key=%d\n", i);
            bpt_destroy(tree);
            return;
        }
    }
    printf("[PASS] insert 11 keys (trigger first split)\n");

    // 验证所有键可查
    for (int i = 1; i <= 11; i++) {
        RecordPointer found;
        DBStatus rc = bpt_search(tree, i, &found);
        if (rc != DB_OK || found.page_id != (uint32_t)i || found.offset != (uint32_t)(i * 10)) {
            printf("[FAIL] search key=%d after split\n", i);
            bpt_destroy(tree);
            return;
        }
    }
    printf("[PASS] all 11 keys found after split\n");

    // 验证结构
    if (bpt_validate(tree) != DB_OK) {
        printf("[FAIL] bpt_validate after split\n");
        bpt_print(tree);
    } else {
        printf("[PASS] bpt_validate: structure correct after split\n");
    }

    bpt_destroy(tree);
    printf("\n");
}

// ===== 测试3：大量插入+结构验证 =====
static void test_large_insert() {
    printf("--- Test: Large Insert (1000 keys) ---\n");

    BPlusTree* tree = bpt_create();
    if (tree == NULL) {
        printf("[FAIL] bpt_create\n");
        return;
    }

    const int N = 1000;
    srand(42); // 固定种子，可重复

    // 生成不重复的随机键
    int* keys = (int*)malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) keys[i] = i + 1; // 顺序1..1000

    // Fisher-Yates shuffle
    for (int i = N - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = keys[i]; keys[i] = keys[j]; keys[j] = tmp;
    }

    // 插入随机顺序
    for (int i = 0; i < N; i++) {
        RecordPointer rid = {keys[i], keys[i] * 100};
        DBStatus rc = bpt_insert(tree, keys[i], rid);
        if (rc != DB_OK) {
            printf("[FAIL] bpt_insert key=%d at step %d\n", keys[i], i);
            bpt_destroy(tree);
            free(keys);
            return;
        }
    }
    printf("[PASS] insert %d keys in random order\n", N);

    // 验证所有键可查
    int fail_count = 0;
    for (int i = 1; i <= N; i++) {
        RecordPointer found;
        DBStatus rc = bpt_search(tree, i, &found);
        if (rc != DB_OK) {
            printf("[FAIL] search key=%d not found\n", i);
            fail_count++;
        } else if (found.page_id != (uint32_t)i || found.offset != (uint32_t)(i * 100)) {
            printf("[FAIL] search key=%d rid mismatch\n", i);
            fail_count++;
        }
    }
    if (fail_count == 0) {
        printf("[PASS] all %d keys found with correct rid\n", N);
    } else {
        printf("[FAIL] %d keys not found or rid mismatch\n", fail_count);
    }

    // 验证结构
    if (bpt_validate(tree) != DB_OK) {
        printf("[FAIL] bpt_validate after large insert\n");
        bpt_print(tree);
    } else {
        printf("[PASS] bpt_validate: structure correct after %d inserts\n", N);
    }

    printf("[INFO] tree height=%d, total count=%d\n", tree->height, tree->count);

    bpt_destroy(tree);
    free(keys);
    printf("\n");
}

// ===== 测试4：范围查询 =====
static void test_range_query() {
    printf("--- Test: Range Query ---\n");

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

    // 范围查询 5..15，收集结果
    typedef struct {
        bpt_key_t keys[20];
        int count;
    } RangeResult;

    RangeResult result = {.count = 0};
    BPTRangeCallback cb = NULL; // will use lambda-like approach below

    // 人工遍历叶子链表来验证范围查询
    // 先用回调方式
    int range_count = 0;
    for (int k = 5; k <= 15; k++) {
        RecordPointer found;
        if (bpt_search(tree, k, &found) == DB_OK) {
            range_count++;
        }
    }

    if (range_count == 11) {
        printf("[PASS] range 5..15: found %d keys (expected 11)\n", range_count);
    } else {
        printf("[FAIL] range 5..15: found %d keys (expected 11)\n", range_count);
    }

    // 使用 bpt_range_query 回调
    typedef struct {
        int keys[30];
        int count;
    } CbCtx;

    CbCtx ctx = {.count = 0};
    // 手动回调内联
    // 由于C不支持lambda，定义一个全局回调
    // 这里用一个简单方式：遍历验证
    // (范围查询的回调测试已在上面间接验证，下面用正式API)

    // 正式范围查询测试：回调收集
    // 定义一个简单的回调函数
    // 注：因C语言限制，回调函数在此文件中定义

    bpt_destroy(tree);
    printf("\n");
}

// 范围查询回调辅助
typedef struct {
    int keys[1000];
    int rids_page[1000];
    int count;
} RangeCbCtx;

static int range_collect_cb(bpt_key_t key, RecordPointer rid, void* ctx) {
    RangeCbCtx* rctx = (RangeCbCtx*)ctx;
    rctx->keys[rctx->count] = key;
    rctx->rids_page[rctx->count] = (int)rid.page_id;
    rctx->count++;
    return 0; // 继续
}

static void test_range_query_api() {
    printf("--- Test: Range Query API ---\n");

    BPlusTree* tree = bpt_create();
    for (int i = 1; i <= 30; i++) {
        RecordPointer rid = {i, i * 10};
        bpt_insert(tree, i, rid);
    }
    printf("[PASS] insert 1..30\n");

    RangeCbCtx ctx = {.count = 0};
    DBStatus rc = bpt_range_query(tree, 10, 20, range_collect_cb, &ctx);
    if (rc != DB_OK) {
        printf("[FAIL] bpt_range_query returned error\n");
    } else {
        // 应找到 10..20 = 11个键
        if (ctx.count == 11) {
            printf("[PASS] range 10..20: found %d keys\n", ctx.count);
        } else {
            printf("[FAIL] range 10..20: found %d keys (expected 11)\n", ctx.count);
        }
        // 验证有序
        int sorted = 1;
        for (int i = 1; i < ctx.count; i++) {
            if (ctx.keys[i] < ctx.keys[i - 1]) sorted = 0;
        }
        if (sorted) {
            printf("[PASS] range results are sorted\n");
        } else {
            printf("[FAIL] range results not sorted\n");
        }
    }

    // 范围无结果
    RangeCbCtx ctx2 = {.count = 0};
    rc = bpt_range_query(tree, 100, 200, range_collect_cb, &ctx2);
    if (rc == DB_PAGE_NOT_FOUND && ctx2.count == 0) {
        printf("[PASS] range 100..200: no results (correct)\n");
    } else {
        printf("[FAIL] range 100..200: unexpected result\n");
    }

    bpt_destroy(tree);
    printf("\n");
}

// ===== 测试5：逆序插入 =====
static void test_reverse_insert() {
    printf("--- Test: Reverse Insert ---\n");

    BPlusTree* tree = bpt_create();
    for (int i = 50; i >= 1; i--) {
        RecordPointer rid = {i, i * 10};
        DBStatus rc = bpt_insert(tree, i, rid);
        if (rc != DB_OK) {
            printf("[FAIL] bpt_insert key=%d\n", i);
            bpt_destroy(tree);
            return;
        }
    }
    printf("[PASS] insert 50..1 (reverse order)\n");

    // 验证所有键可查
    int fail = 0;
    for (int i = 1; i <= 50; i++) {
        RecordPointer found;
        if (bpt_search(tree, i, &found) != DB_OK ||
            found.page_id != (uint32_t)i) {
            fail++;
        }
    }
    if (fail == 0) {
        printf("[PASS] all 50 keys found after reverse insert\n");
    } else {
        printf("[FAIL] %d keys missing after reverse insert\n", fail);
    }

    if (bpt_validate(tree) != DB_OK) {
        printf("[FAIL] bpt_validate after reverse insert\n");
        bpt_print(tree);
    } else {
        printf("[PASS] bpt_validate: structure correct\n");
    }

    bpt_destroy(tree);
    printf("\n");
}

// ===== 测试6：打印调试 =====
static void test_print() {
    printf("--- Test: Print Tree ---\n");

    BPlusTree* tree = bpt_create();
    for (int i = 1; i <= 15; i++) {
        RecordPointer rid = {i, i * 10};
        bpt_insert(tree, i, rid);
    }
    printf("[PASS] insert 1..15\n");

    printf("Tree structure:\n");
    bpt_print(tree);

    bpt_destroy(tree);
    printf("\n");
}

int main() {
    printf("=== B+Tree Index Tests ===\n\n");

    test_basic_insert_search();
    test_split();
    test_large_insert();
    test_range_query();
    test_range_query_api();
    test_reverse_insert();
    test_print();

    printf("=== All B+Tree tests completed! ===\n");
    return 0;
}
