#include "../src/buffer_pool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST_DB "test_buffer_pool.db"

static void test_basic_dirty_flush() {
    DiskManager* dm = NULL;
    if (dm_open(&dm, TEST_DB) != DB_OK) {
        printf("[FAIL] test_basic_dirty_flush: open database failed\n");
        return;
    }

    BufferPool* pool = bp_create(dm, 2);
    if (pool == NULL) {
        printf("[FAIL] test_basic_dirty_flush: create buffer pool failed\n");
        dm_close(dm);
        return;
    }

    page_id_t page_id = dm_allocate_page(dm);
    Page page;
    memset(&page, 0, sizeof(Page));
    snprintf((char*)page.data, PAGE_SIZE, "buffer-pool-page");
    if (dm_write_page(dm, page_id, &page) != DB_OK) {
        printf("[FAIL] test_basic_dirty_flush: initial write failed\n");
        bp_close(pool);
        dm_close(dm);
        return;
    }

    Page* cached = bp_get_page(pool, page_id);
    if (cached == NULL) {
        printf("[FAIL] test_basic_dirty_flush: get page failed\n");
        bp_close(pool);
        dm_close(dm);
        return;
    }

    snprintf((char*)cached->data, PAGE_SIZE, "changed-by-buffer-pool");
    if (bp_mark_dirty(pool, page_id) != DB_OK) {
        printf("[FAIL] test_basic_dirty_flush: mark dirty failed\n");
        bp_close(pool);
        dm_close(dm);
        return;
    }

    if (bp_flush_page(pool, page_id) != DB_OK) {
        printf("[FAIL] test_basic_dirty_flush: flush page failed\n");
        bp_close(pool);
        dm_close(dm);
        return;
    }

    if (bp_close(pool) != DB_OK) {
        printf("[FAIL] test_basic_dirty_flush: close pool failed\n");
        dm_close(dm);
        return;
    }

    if (dm_open(&dm, TEST_DB) != DB_OK) {
        printf("[FAIL] test_basic_dirty_flush: reopen database failed\n");
        return;
    }

    Page reloaded;
    memset(&reloaded, 0, sizeof(Page));
    if (dm_read_page(dm, page_id, &reloaded) != DB_OK) {
        printf("[FAIL] test_basic_dirty_flush: read reloaded page failed\n");
        dm_close(dm);
        return;
    }

    if (strcmp((char*)reloaded.data, "changed-by-buffer-pool") == 0) {
        printf("[PASS] test_basic_dirty_flush: dirty page persisted\n");
    } else {
        printf("[FAIL] test_basic_dirty_flush: persisted data mismatch\n");
    }

    dm_close(dm);
}

static void test_lru_eviction() {
    DiskManager* dm = NULL;
    if (dm_open(&dm, TEST_DB) != DB_OK) {
        printf("[FAIL] test_lru_eviction: open database failed\n");
        return;
    }

    BufferPool* pool = bp_create(dm, 2);
    if (pool == NULL) {
        printf("[FAIL] test_lru_eviction: create buffer pool failed\n");
        dm_close(dm);
        return;
    }

    page_id_t p0 = dm_allocate_page(dm);
    page_id_t p1 = dm_allocate_page(dm);
    page_id_t p2 = dm_allocate_page(dm);

    Page page0, page1, page2;
    memset(&page0, 0, sizeof(Page));
    memset(&page1, 0, sizeof(Page));
    memset(&page2, 0, sizeof(Page));
    snprintf((char*)page0.data, PAGE_SIZE, "page0");
    snprintf((char*)page1.data, PAGE_SIZE, "page1");
    snprintf((char*)page2.data, PAGE_SIZE, "page2");

    dm_write_page(dm, p0, &page0);
    dm_write_page(dm, p1, &page1);
    dm_write_page(dm, p2, &page2);

    if (bp_get_page(pool, p0) == NULL || bp_get_page(pool, p1) == NULL || bp_get_page(pool, p2) == NULL) {
        printf("[FAIL] test_lru_eviction: load pages failed\n");
        bp_close(pool);
        dm_close(dm);
        return;
    }

    Page* reloaded = bp_get_page(pool, p0);
    if (reloaded != NULL && strcmp((char*)reloaded->data, "page0") == 0) {
        printf("[PASS] test_lru_eviction: LRU reload succeeded\n");
    } else {
        printf("[FAIL] test_lru_eviction: LRU reload failed\n");
    }

    bp_close(pool);
    dm_close(dm);
}

int main() {
    remove(TEST_DB);
    printf("Running buffer pool tests...\n\n");
    test_basic_dirty_flush();
    printf("\n");
    test_lru_eviction();
    printf("\n");
    remove(TEST_DB);
    return 0;
}
