#include "../src/buffer_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_DB "test_buffer.db"

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

static void test_init_destroy() {
    TEST("init and destroy");
    DiskManager* dm = NULL;
    if (dm_open(&dm, TEST_DB) != DB_OK) {
        FAIL("open disk manager failed");
        return;
    }

    BufferPool* bp = NULL;
    DBStatus status = bp_init(&bp, dm, 8);
    if (status != DB_OK) {
        FAIL("init buffer pool failed");
        dm_close(dm);
        return;
    }

    bp_destroy(bp);
    dm_close(dm);
    remove(TEST_DB);
    PASS();
}

static void test_get_release_page() {
    TEST("get and release page");
    DiskManager* dm = NULL;
    dm_open(&dm, TEST_DB);

    page_id_t page_id = dm_allocate_page(dm);
    Page page;
    memset(&page, 0, sizeof(Page));
    strcpy((char*)page.data, "Test data");
    dm_write_page(dm, page_id, &page);

    BufferPool* bp = NULL;
    bp_init(&bp, dm, 8);

    Page* got_page = bp_get_page(bp, page_id);
    if (got_page == NULL) {
        FAIL("get page failed");
        bp_destroy(bp);
        dm_close(dm);
        remove(TEST_DB);
        return;
    }

    if (strcmp((char*)got_page->data, "Test data") != 0) {
        FAIL("page data mismatch");
        bp_destroy(bp);
        dm_close(dm);
        remove(TEST_DB);
        return;
    }

    bp_release_page(bp, page_id);

    bp_destroy(bp);
    dm_close(dm);
    remove(TEST_DB);
    PASS();
}

static void test_mark_dirty_and_flush() {
    TEST("mark dirty and flush");
    DiskManager* dm = NULL;
    dm_open(&dm, TEST_DB);

    page_id_t page_id = dm_allocate_page(dm);

    BufferPool* bp = NULL;
    bp_init(&bp, dm, 8);

    Page* got_page = bp_get_page(bp, page_id);
    memset(got_page->data, 0, PAGE_SIZE);
    strcpy((char*)got_page->data, "Dirty data");

    bp_mark_dirty(bp, page_id);

    DBStatus status = bp_flush_page(bp, page_id);
    if (status != DB_OK) {
        FAIL("flush page failed");
        bp_destroy(bp);
        dm_close(dm);
        remove(TEST_DB);
        return;
    }

    Page read_back;
    memset(&read_back, 0, sizeof(Page));
    dm_read_page(dm, page_id, &read_back);
    if (strcmp((char*)read_back.data, "Dirty data") != 0) {
        FAIL("dirty data not persisted");
        bp_destroy(bp);
        dm_close(dm);
        remove(TEST_DB);
        return;
    }

    bp_destroy(bp);
    dm_close(dm);
    remove(TEST_DB);
    PASS();
}

static void test_lru_eviction() {
    TEST("LRU eviction");
    DiskManager* dm = NULL;
    dm_open(&dm, TEST_DB);

    page_id_t pages[10];
    for (int i = 0; i < 10; i++) {
        pages[i] = dm_allocate_page(dm);
        Page page;
        memset(&page, 0, sizeof(Page));
        snprintf((char*)page.data, PAGE_SIZE, "Page %d", i);
        dm_write_page(dm, pages[i], &page);
    }

    BufferPool* bp = NULL;
    bp_init(&bp, dm, 5);

    for (int i = 0; i < 5; i++) {
        Page* p = bp_get_page(bp, pages[i]);
        if (p == NULL) {
            FAIL("get page failed");
            bp_destroy(bp);
            dm_close(dm);
            remove(TEST_DB);
            return;
        }
        bp_release_page(bp, pages[i]);
    }

    Page* p5 = bp_get_page(bp, pages[5]);
    if (p5 == NULL) {
        FAIL("get new page after eviction failed");
        bp_destroy(bp);
        dm_close(dm);
        remove(TEST_DB);
        return;
    }

    Page* p0 = bp_get_page(bp, pages[0]);
    if (p0 == NULL) {
        FAIL("get evicted page failed");
        bp_destroy(bp);
        dm_close(dm);
        remove(TEST_DB);
        return;
    }

    bp_destroy(bp);
    dm_close(dm);
    remove(TEST_DB);
    PASS();
}

static void test_lru_eviction_order() {
    TEST("LRU eviction order");
    DiskManager* dm = NULL;
    dm_open(&dm, TEST_DB);

    page_id_t pages[5];
    for (int i = 0; i < 5; i++) {
        pages[i] = dm_allocate_page(dm);
        Page page;
        memset(&page, 0, sizeof(Page));
        snprintf((char*)page.data, PAGE_SIZE, "Page %d", i);
        dm_write_page(dm, pages[i], &page);
    }

    BufferPool* bp = NULL;
    bp_init(&bp, dm, 3);

    Page* p0 = bp_get_page(bp, pages[0]);
    bp_release_page(bp, pages[0]);

    Page* p1 = bp_get_page(bp, pages[1]);
    bp_release_page(bp, pages[1]);

    Page* p2 = bp_get_page(bp, pages[2]);
    bp_release_page(bp, pages[2]);

    p0 = bp_get_page(bp, pages[0]);
    bp_release_page(bp, pages[0]);

    Page* p3 = bp_get_page(bp, pages[3]);
    bp_release_page(bp, pages[3]);

    Page* p4 = bp_get_page(bp, pages[4]);
    bp_release_page(bp, pages[4]);

    Page* check_p1 = bp_get_page(bp, pages[1]);
    if (strcmp((char*)check_p1->data, "Page 1") != 0) {
        FAIL("LRU order violated - p1 should be evicted");
        bp_destroy(bp);
        dm_close(dm);
        remove(TEST_DB);
        return;
    }

    bp_destroy(bp);
    dm_close(dm);
    remove(TEST_DB);
    PASS();
}

static void test_pin_prevents_eviction() {
    TEST("pin prevents eviction");
    DiskManager* dm = NULL;
    dm_open(&dm, TEST_DB);

    page_id_t pages[5];
    for (int i = 0; i < 5; i++) {
        pages[i] = dm_allocate_page(dm);
        Page page;
        memset(&page, 0, sizeof(Page));
        snprintf((char*)page.data, PAGE_SIZE, "Page %d", i);
        dm_write_page(dm, pages[i], &page);
    }

    BufferPool* bp = NULL;
    bp_init(&bp, dm, 3);

    Page* p0 = bp_get_page(bp, pages[0]);
    Page* p1 = bp_get_page(bp, pages[1]);
    Page* p2 = bp_get_page(bp, pages[2]);

    bp_release_page(bp, pages[1]);
    bp_release_page(bp, pages[2]);

    Page* p3 = bp_get_page(bp, pages[3]);
    bp_release_page(bp, pages[3]);

    Page* p4 = bp_get_page(bp, pages[4]);
    bp_release_page(bp, pages[4]);

    if (strcmp((char*)p0->data, "Page 0") != 0) {
        FAIL("pinned page was evicted");
        bp_destroy(bp);
        dm_close(dm);
        remove(TEST_DB);
        return;
    }

    bp_release_page(bp, pages[0]);

    bp_destroy(bp);
    dm_close(dm);
    remove(TEST_DB);
    PASS();
}

static void test_flush_all() {
    TEST("flush all dirty pages");
    DiskManager* dm = NULL;
    dm_open(&dm, TEST_DB);

    page_id_t pages[3];
    for (int i = 0; i < 3; i++) {
        pages[i] = dm_allocate_page(dm);
    }

    BufferPool* bp = NULL;
    bp_init(&bp, dm, 3);

    Page* p0 = bp_get_page(bp, pages[0]);
    strcpy((char*)p0->data, "Dirty 0");
    bp_mark_dirty(bp, pages[0]);
    bp_release_page(bp, pages[0]);

    Page* p1 = bp_get_page(bp, pages[1]);
    strcpy((char*)p1->data, "Dirty 1");
    bp_mark_dirty(bp, pages[1]);
    bp_release_page(bp, pages[1]);

    Page* p2 = bp_get_page(bp, pages[2]);
    strcpy((char*)p2->data, "Dirty 2");
    bp_mark_dirty(bp, pages[2]);
    bp_release_page(bp, pages[2]);

    bp_flush_all(bp);

    for (int i = 0; i < 3; i++) {
        Page read_back;
        memset(&read_back, 0, sizeof(Page));
        dm_read_page(dm, pages[i], &read_back);
        char expected[PAGE_SIZE];
        snprintf(expected, PAGE_SIZE, "Dirty %d", i);
        if (strcmp((char*)read_back.data, expected) != 0) {
            FAIL("flush all failed");
            bp_destroy(bp);
            dm_close(dm);
            remove(TEST_DB);
            return;
        }
    }

    bp_destroy(bp);
    dm_close(dm);
    remove(TEST_DB);
    PASS();
}

static void test_dirty_eviction() {
    TEST("dirty page eviction persists");
    DiskManager* dm = NULL;
    dm_open(&dm, TEST_DB);

    page_id_t pages[5];
    for (int i = 0; i < 5; i++) {
        pages[i] = dm_allocate_page(dm);
    }

    BufferPool* bp = NULL;
    bp_init(&bp, dm, 2);

    Page* p0 = bp_get_page(bp, pages[0]);
    strcpy((char*)p0->data, "Modified 0");
    bp_mark_dirty(bp, pages[0]);
    bp_release_page(bp, pages[0]);

    Page* p1 = bp_get_page(bp, pages[1]);
    strcpy((char*)p1->data, "Modified 1");
    bp_mark_dirty(bp, pages[1]);
    bp_release_page(bp, pages[1]);

    Page* p2 = bp_get_page(bp, pages[2]);
    bp_release_page(bp, pages[2]);

    Page read_back;
    memset(&read_back, 0, sizeof(Page));
    dm_read_page(dm, pages[0], &read_back);
    if (strcmp((char*)read_back.data, "Modified 0") != 0) {
        FAIL("dirty page not persisted on eviction");
        bp_destroy(bp);
        dm_close(dm);
        remove(TEST_DB);
        return;
    }

    bp_destroy(bp);
    dm_close(dm);
    remove(TEST_DB);
    PASS();
}

static void test_stress() {
    TEST("stress test 1000 operations");
    DiskManager* dm = NULL;
    dm_open(&dm, TEST_DB);

    page_id_t pages[20];
    for (int i = 0; i < 20; i++) {
        pages[i] = dm_allocate_page(dm);
        Page page;
        memset(&page, 0, sizeof(Page));
        snprintf((char*)page.data, PAGE_SIZE, "Initial %d", i);
        dm_write_page(dm, pages[i], &page);
    }

    BufferPool* bp = NULL;
    bp_init(&bp, dm, 8);

    srand(42);
    for (int i = 0; i < 1000; i++) {
        int idx = rand() % 20;
        Page* p = bp_get_page(bp, pages[idx]);
        if (p == NULL) {
            FAIL("get page failed in stress test");
            bp_destroy(bp);
            dm_close(dm);
            remove(TEST_DB);
            return;
        }
        bp_release_page(bp, pages[idx]);
    }

    bp_destroy(bp);
    dm_close(dm);
    remove(TEST_DB);
    PASS();
}

int main() {
    remove(TEST_DB);

    printf("Running Buffer Pool tests...\n\n");

    printf("=== Basic Operations ===\n");
    test_init_destroy();
    test_get_release_page();
    test_mark_dirty_and_flush();
    test_flush_all();
    printf("\n");

    printf("=== LRU Eviction ===\n");
    test_lru_eviction();
    test_lru_eviction_order();
    test_dirty_eviction();
    printf("\n");

    printf("=== Pin Protection ===\n");
    test_pin_prevents_eviction();
    printf("\n");

    printf("=== Stress Test ===\n");
    test_stress();
    printf("\n");

    printf("=========================\n");
    printf("  Passed: %d\n", passed);
    printf("  Failed: %d\n", failed);
    printf("  Total:  %d\n", passed + failed);
    printf("=========================\n");

    return failed == 0 ? 0 : 1;
}
