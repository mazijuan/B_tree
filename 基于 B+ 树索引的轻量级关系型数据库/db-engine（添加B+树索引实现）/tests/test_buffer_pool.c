#include "../src/buffer_pool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "test_bp.db"

// ===== 测试1：创建和销毁 =====
static void test_create_destroy() {
    printf("--- Test: Create & Destroy ---\n");

    DiskManager* dm = NULL;
    if (db_open(&dm, TEST_DB) != DB_OK) {
        printf("[FAIL] db_open\n");
        return;
    }

    BufferPool* bp = NULL;
    DBStatus rc = bp_create(&bp, dm, 16);
    if (rc != DB_OK || bp == NULL) {
        printf("[FAIL] bp_create\n");
        db_close(dm);
        return;
    }
    printf("[PASS] bp_create (pool_size=16)\n");

    rc = bp_destroy(bp);
    if (rc == DB_OK) {
        printf("[PASS] bp_destroy\n");
    } else {
        printf("[FAIL] bp_destroy\n");
    }

    db_close(dm);
    remove(TEST_DB);
    printf("\n");
}

// ===== 测试2：写入和读取（通过缓冲池） =====
static void test_write_read() {
    printf("--- Test: Write & Read via BufferPool ---\n");

    DiskManager* dm = NULL;
    db_open(&dm, TEST_DB);

    BufferPool* bp = NULL;
    bp_create(&bp, dm, 16);

    // 分配一个新页
    page_id_t pid;
    Frame* frame = NULL;
    DBStatus rc = bp_allocate_page(bp, &pid, &frame);
    if (rc != DB_OK || frame == NULL) {
        printf("[FAIL] bp_allocate_page\n");
        bp_destroy(bp);
        db_close(dm);
        remove(TEST_DB);
        return;
    }
    printf("[PASS] bp_allocate_page (pid=%u)\n", pid);

    // 写入数据
    const char* msg = "Hello, Buffer Pool!";
    memset(frame->page.data, 0, PAGE_SIZE);
    strncpy((char*)frame->page.data, msg, strlen(msg));
    bp_mark_dirty(bp, pid);
    bp_unpin_page(bp, pid);
    printf("[PASS] wrote data and marked dirty\n");

    // 刷新到磁盘
    rc = bp_flush_page(bp, pid);
    if (rc == DB_OK) {
        printf("[PASS] bp_flush_page\n");
    } else {
        printf("[FAIL] bp_flush_page\n");
    }

    // 通过缓冲池重新读取
    Frame* frame2 = NULL;
    rc = bp_get_page(bp, pid, &frame2);
    if (rc != DB_OK || frame2 == NULL) {
        printf("[FAIL] bp_get_page after flush\n");
    } else if (strcmp((char*)frame2->page.data, msg) == 0) {
        printf("[PASS] data integrity verified via buffer pool\n");
    } else {
        printf("[FAIL] data mismatch\n");
    }

    if (frame2) bp_unpin_page(bp, pid);

    bp_destroy(bp);
    db_close(dm);
    remove(TEST_DB);
    printf("\n");
}

// ===== 测试3：LRU淘汰 =====
static void test_lru_eviction() {
    printf("--- Test: LRU Eviction ---\n");

    DiskManager* dm = NULL;
    db_open(&dm, TEST_DB);

    // 创建一个很小的缓冲池（只能缓存4页）
    BufferPool* bp = NULL;
    bp_create(&bp, dm, 4);

    // 分配8个页，写入不同数据
    page_id_t pids[8];
    for (int i = 0; i < 8; i++) {
        Frame* frame = NULL;
        DBStatus rc = bp_allocate_page(bp, &pids[i], &frame);
        if (rc != DB_OK) {
            printf("[FAIL] bp_allocate_page %d\n", i);
            bp_destroy(bp);
            db_close(dm);
            remove(TEST_DB);
            return;
        }
        memset(frame->page.data, 0, PAGE_SIZE);
        snprintf((char*)frame->page.data, PAGE_SIZE, "Page-%d-data", i);
        bp_mark_dirty(bp, pids[i]);
        bp_unpin_page(bp, pids[i]);
    }
    printf("[PASS] allocated 8 pages (pool_size=4)\n");

    // 读取第0页（应该被淘汰了，需要从磁盘重新加载）
    Frame* frame = NULL;
    DBStatus rc = bp_get_page(bp, pids[0], &frame);
    if (rc != DB_OK) {
        printf("[FAIL] bp_get_page pids[0] after eviction\n");
    } else {
        char expected[32];
        snprintf(expected, sizeof(expected), "Page-0-data");
        if (strncmp((char*)frame->page.data, expected, strlen(expected)) == 0) {
            printf("[PASS] page 0 data correct after eviction (loaded from disk)\n");
        } else {
            printf("[FAIL] page 0 data mismatch: got '%.20s'\n", (char*)frame->page.data);
        }
        bp_unpin_page(bp, pids[0]);
    }

    // 验证所有页的数据都能正确读取（通过LRU淘汰+磁盘回读）
    int fail = 0;
    for (int i = 0; i < 8; i++) {
        Frame* f = NULL;
        rc = bp_get_page(bp, pids[i], &f);
        if (rc != DB_OK) {
            printf("[FAIL] bp_get_page pids[%d]\n", i);
            fail++;
            continue;
        }
        char expected[32];
        snprintf(expected, sizeof(expected), "Page-%d-data", i);
        if (strncmp((char*)f->page.data, expected, strlen(expected)) != 0) {
            printf("[FAIL] page %d data mismatch\n", i);
            fail++;
        }
        bp_unpin_page(bp, pids[i]);
    }
    if (fail == 0) printf("[PASS] all 8 pages data verified\n");

    bp_destroy(bp);
    db_close(dm);
    remove(TEST_DB);
    printf("\n");
}

// ===== 测试4：持久化测试 =====
static void test_persistence() {
    printf("--- Test: Persistence ---\n");

    const char* data_msg = "Persistent buffer pool data";

    // 第一次会话：写入数据
    {
        DiskManager* dm = NULL;
        db_open(&dm, TEST_DB);

        BufferPool* bp = NULL;
        bp_create(&bp, dm, 16);

        page_id_t pid;
        Frame* frame = NULL;
        bp_allocate_page(bp, &pid, &frame);
        memset(frame->page.data, 0, PAGE_SIZE);
        strncpy((char*)frame->page.data, data_msg, strlen(data_msg));
        bp_mark_dirty(bp, pid);
        bp_unpin_page(bp, pid);

        // 销毁缓冲池（会刷新所有脏页）
        bp_destroy(bp);
        db_close(dm);
    }
    printf("[PASS] session 1: wrote data and closed\n");

    // 第二次会话：验证数据
    {
        DiskManager* dm = NULL;
        db_open(&dm, TEST_DB);

        BufferPool* bp = NULL;
        bp_create(&bp, dm, 16);

        // 读取之前写入的页（page_id=0）
        Frame* frame = NULL;
        DBStatus rc = bp_get_page(bp, 0, &frame);
        if (rc != DB_OK) {
            printf("[FAIL] bp_get_page in session 2\n");
        } else if (strcmp((char*)frame->page.data, data_msg) == 0) {
            printf("[PASS] data persisted across sessions\n");
        } else {
            printf("[FAIL] data mismatch in session 2\n");
        }

        if (frame) bp_unpin_page(bp, 0);

        bp_destroy(bp);
        db_close(dm);
    }

    remove(TEST_DB);
    printf("\n");
}

// ===== 测试5：pin_count测试 =====
static void test_pin_count() {
    printf("--- Test: Pin Count ---\n");

    DiskManager* dm = NULL;
    db_open(&dm, TEST_DB);

    BufferPool* bp = NULL;
    bp_create(&bp, dm, 4);

    // 分配3个页
    page_id_t pids[3];
    Frame* frames[3];
    for (int i = 0; i < 3; i++) {
        bp_allocate_page(bp, &pids[i], &frames[i]);
        bp_mark_dirty(bp, pids[i]);
        // 注意：不unpin
    }
    printf("[PASS] allocated 3 pages (all pinned)\n");

    // 尝试分配第4个页（应该成功，还有1个空闲帧）
    page_id_t pid4;
    Frame* frame4 = NULL;
    DBStatus rc = bp_allocate_page(bp, &pid4, &frame4);
    if (rc == DB_OK) {
        printf("[PASS] allocated 4th page (used last free frame)\n");
        bp_mark_dirty(bp, pid4);
        bp_unpin_page(bp, pid4);
    } else {
        printf("[FAIL] allocate 4th page\n");
    }

    // 尝试分配第5个页（3个pinned + 1个unpinned可淘汰，应该成功）
    page_id_t pid5;
    Frame* frame5 = NULL;
    rc = bp_allocate_page(bp, &pid5, &frame5);
    if (rc == DB_OK) {
        printf("[PASS] 5th allocation succeeded (evicted unpinned page)\n");
        bp_unpin_page(bp, pid5);
    } else {
        printf("[FAIL] 5th allocation should have succeeded (1 unpinned page available for eviction)\n");
    }

    // 现在所有4个帧都被占用：3个pinned + 1个unpinned（刚evict回来的）
    // 再分配第6个页：只有1个unpinned可淘汰，应该成功
    page_id_t pid6;
    Frame* frame6 = NULL;
    rc = bp_allocate_page(bp, &pid6, &frame6);
    if (rc == DB_OK) {
        printf("[PASS] 6th allocation succeeded (evicted another unpinned page)\n");
        bp_unpin_page(bp, pid6);
    } else {
        printf("[FAIL] 6th allocation should have succeeded\n");
    }

    // 释放第1个页
    bp_unpin_page(bp, pids[0]);
    printf("[PASS] unpinned page 0\n");

    // 再次尝试分配（应该成功）
    rc = bp_allocate_page(bp, &pid5, &frame5);
    if (rc == DB_OK) {
        printf("[PASS] allocation succeeded after unpinning page 0\n");
        bp_unpin_page(bp, pid5);
    } else {
        printf("[FAIL] allocation should have succeeded\n");
    }

    // 清理
    for (int i = 1; i < 3; i++) {
        bp_unpin_page(bp, pids[i]);
    }

    bp_destroy(bp);
    db_close(dm);
    remove(TEST_DB);
    printf("\n");
}

// ===== 测试6：批量操作 =====
static void test_batch_operations() {
    printf("--- Test: Batch Operations ---\n");

    DiskManager* dm = NULL;
    db_open(&dm, TEST_DB);

    BufferPool* bp = NULL;
    bp_create(&bp, dm, 32);

    // 批量分配100个页并写入数据
    const int N = 100;
    page_id_t* pids = (page_id_t*)malloc(N * sizeof(page_id_t));

    for (int i = 0; i < N; i++) {
        Frame* frame = NULL;
        DBStatus rc = bp_allocate_page(bp, &pids[i], &frame);
        if (rc != DB_OK) {
            printf("[FAIL] bp_allocate_page %d\n", i);
            free(pids);
            bp_destroy(bp);
            db_close(dm);
            remove(TEST_DB);
            return;
        }
        // 写入页号
        memset(frame->page.data, 0, PAGE_SIZE);
        snprintf((char*)frame->page.data, PAGE_SIZE, "BatchPage-%d", i);
        bp_mark_dirty(bp, pids[i]);
        bp_unpin_page(bp, pids[i]);
    }
    printf("[PASS] allocated and wrote %d pages\n", N);

    // 刷新所有脏页
    bp_flush_all(bp);
    printf("[PASS] bp_flush_all\n");

    // 批量读取验证
    int fail = 0;
    for (int i = 0; i < N; i++) {
        Frame* frame = NULL;
        DBStatus rc = bp_get_page(bp, pids[i], &frame);
        if (rc != DB_OK) {
            printf("[FAIL] bp_get_page %d\n", i);
            fail++;
            continue;
        }
        char expected[32];
        snprintf(expected, sizeof(expected), "BatchPage-%d", i);
        if (strncmp((char*)frame->page.data, expected, strlen(expected)) != 0) {
            printf("[FAIL] page %d data mismatch\n", i);
            fail++;
        }
        bp_unpin_page(bp, pids[i]);
    }
    if (fail == 0) printf("[PASS] all %d pages verified\n", N);

    free(pids);
    bp_destroy(bp);
    db_close(dm);
    remove(TEST_DB);
    printf("\n");
}

// ===== 测试7：状态打印 =====
static void test_print_status() {
    printf("--- Test: Print Status ---\n");

    DiskManager* dm = NULL;
    db_open(&dm, TEST_DB);

    BufferPool* bp = NULL;
    bp_create(&bp, dm, 8);

    // 分配几个页
    for (int i = 0; i < 5; i++) {
        page_id_t pid;
        Frame* frame = NULL;
        bp_allocate_page(bp, &pid, &frame);
        if (i < 3) {
            bp_mark_dirty(bp, pid);
        }
        bp_unpin_page(bp, pid);
    }

    printf("Buffer pool status after 5 allocations:\n");
    bp_print_status(bp);

    bp_destroy(bp);
    db_close(dm);
    remove(TEST_DB);
    printf("\n");
}

int main() {
    printf("=== Buffer Pool Tests ===\n\n");

    // 清理可能残留的测试文件
    remove(TEST_DB);

    test_create_destroy();
    test_write_read();
    test_lru_eviction();
    test_persistence();
    test_pin_count();
    test_batch_operations();
    test_print_status();

    printf("=== All Buffer Pool tests completed! ===\n");
    return 0;
}
