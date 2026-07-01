#include "../src/disk_manager.h"
#include <stdio.h>
#include <string.h>

#define TEST_DB "test.db"

static void test_open_close() {
    DiskManager* dm = NULL;
    DBStatus status = dm_open(&dm, TEST_DB);
    if (status == DB_OK) {
        printf("[PASS] test_open_close: open database\n");
    } else {
        printf("[FAIL] test_open_close: open database failed\n");
        return;
    }

    status = dm_close(dm);
    if (status == DB_OK) {
        printf("[PASS] test_open_close: close database\n");
    } else {
        printf("[FAIL] test_open_close: close database failed\n");
    }
}

static void test_allocate_write_read() {
    DiskManager* dm = NULL;
    DBStatus status = dm_open(&dm, TEST_DB);
    if (status != DB_OK) {
        printf("[FAIL] test_allocate_write_read: open database failed\n");
        return;
    }

    page_id_t page_id = dm_allocate_page(dm);
    if (page_id != (page_id_t)-1) {
        printf("[PASS] test_allocate_write_read: allocate page (id=%u)\n", page_id);
    } else {
        printf("[FAIL] test_allocate_write_read: allocate page failed\n");
        dm_close(dm);
        return;
    }

    Page write_page;
    memset(&write_page, 0, sizeof(Page));
    const char* test_data = "Hello, Database!";
    strncpy((char*)write_page.data, test_data, strlen(test_data));

    status = dm_write_page(dm, page_id, &write_page);
    if (status == DB_OK) {
        printf("[PASS] test_allocate_write_read: write page\n");
    } else {
        printf("[FAIL] test_allocate_write_read: write page failed\n");
        dm_close(dm);
        return;
    }

    Page read_page;
    memset(&read_page, 0, sizeof(Page));
    status = dm_read_page(dm, page_id, &read_page);
    if (status == DB_OK) {
        printf("[PASS] test_allocate_write_read: read page\n");
    } else {
        printf("[FAIL] test_allocate_write_read: read page failed\n");
        dm_close(dm);
        return;
    }

    if (strcmp((char*)read_page.data, test_data) == 0) {
        printf("[PASS] test_allocate_write_read: data integrity\n");
    } else {
        printf("[FAIL] test_allocate_write_read: data mismatch\n");
    }

    dm_close(dm);
}

static void test_multiple_pages() {
    DiskManager* dm = NULL;
    DBStatus status = dm_open(&dm, TEST_DB);
    if (status != DB_OK) {
        printf("[FAIL] test_multiple_pages: open database failed\n");
        return;
    }

    page_id_t pages[5];
    for (int i = 0; i < 5; i++) {
        pages[i] = dm_allocate_page(dm);
        if (pages[i] == (page_id_t)-1) {
            printf("[FAIL] test_multiple_pages: allocate page %d failed\n", i);
            dm_close(dm);
            return;
        }

        Page page;
        memset(&page, 0, sizeof(Page));
        snprintf((char*)page.data, PAGE_SIZE, "Page %d data", i);
        status = dm_write_page(dm, pages[i], &page);
        if (status != DB_OK) {
            printf("[FAIL] test_multiple_pages: write page %d failed\n", i);
            dm_close(dm);
            return;
        }
    }
    printf("[PASS] test_multiple_pages: allocate and write 5 pages\n");

    for (int i = 0; i < 5; i++) {
        Page page;
        memset(&page, 0, sizeof(Page));
        status = dm_read_page(dm, pages[i], &page);
        if (status != DB_OK) {
            printf("[FAIL] test_multiple_pages: read page %d failed\n", i);
            dm_close(dm);
            return;
        }

        char expected[PAGE_SIZE];
        snprintf(expected, PAGE_SIZE, "Page %d data", i);
        if (strcmp((char*)page.data, expected) != 0) {
            printf("[FAIL] test_multiple_pages: page %d data mismatch\n", i);
            dm_close(dm);
            return;
        }
    }
    printf("[PASS] test_multiple_pages: verify 5 pages data\n");

    dm_close(dm);
}

static void test_free_page() {
    DiskManager* dm = NULL;
    DBStatus status = dm_open(&dm, TEST_DB);
    if (status != DB_OK) {
        printf("[FAIL] test_free_page: open database failed\n");
        return;
    }

    page_id_t page_id = dm_allocate_page(dm);
    if (page_id == (page_id_t)-1) {
        printf("[FAIL] test_free_page: allocate page failed\n");
        dm_close(dm);
        return;
    }

    status = dm_free_page(dm, page_id);
    if (status == DB_OK) {
        printf("[PASS] test_free_page: free page\n");
    } else {
        printf("[FAIL] test_free_page: free page failed\n");
        dm_close(dm);
        return;
    }

    page_id_t new_page_id = dm_allocate_page(dm);
    if (new_page_id == page_id) {
        printf("[PASS] test_free_page: reuse freed page\n");
    } else {
        printf("[FAIL] test_free_page: page not reused (expected %u, got %u)\n", page_id, new_page_id);
    }

    dm_close(dm);
}

static void test_persistence() {
    {
        DiskManager* dm = NULL;
        DBStatus status = dm_open(&dm, TEST_DB);
        if (status != DB_OK) {
            printf("[FAIL] test_persistence: open database failed\n");
            return;
        }

        page_id_t page_id = dm_allocate_page(dm);
        Page page;
        memset(&page, 0, sizeof(Page));
        strncpy((char*)page.data, "Persistent data", PAGE_SIZE - 1);
        page.data[PAGE_SIZE - 1] = '\0';
        dm_write_page(dm, page_id, &page);
        dm_close(dm);
    }

    {
        DiskManager* dm = NULL;
        DBStatus status = dm_open(&dm, TEST_DB);
        if (status != DB_OK) {
            printf("[FAIL] test_persistence: reopen database failed\n");
            return;
        }

        Page page;
        memset(&page, 0, sizeof(Page));
        status = dm_read_page(dm, 0, &page);
        if (status != DB_OK) {
            printf("[FAIL] test_persistence: read persisted page failed\n");
            dm_close(dm);
            return;
        }

        if (strcmp((char*)page.data, "Persistent data") == 0) {
            printf("[PASS] test_persistence: data persisted\n");
        } else {
            printf("[FAIL] test_persistence: data not persisted\n");
        }

        dm_close(dm);
    }
}

int main() {
    remove(TEST_DB);

    printf("Running disk manager tests...\n\n");

    test_open_close();
    printf("\n");

    remove(TEST_DB);
    test_allocate_write_read();
    printf("\n");

    remove(TEST_DB);
    test_multiple_pages();
    printf("\n");

    remove(TEST_DB);
    test_free_page();
    printf("\n");

    remove(TEST_DB);
    test_persistence();
    printf("\n");

    remove(TEST_DB);

    printf("All tests completed!\n");
    return 0;
}
