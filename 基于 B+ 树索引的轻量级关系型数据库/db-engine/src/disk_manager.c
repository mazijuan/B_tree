#include "disk_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_HEADER_SIZE (sizeof(uint32_t) + BITMAP_SIZE)

static DBStatus dm_load_header(DiskManager* dm);
static DBStatus dm_save_header(DiskManager* dm);

DBStatus dm_open(DiskManager** dm, const char* filename) {
    *dm = (DiskManager*)malloc(sizeof(DiskManager));
    if (*dm == NULL) return DB_ERROR;

    (*dm)->filename = (char*)malloc(strlen(filename) + 1);
    if ((*dm)->filename == NULL) {
        free(*dm);
        return DB_ERROR;
    }
    strcpy((*dm)->filename, filename);

    FILE* f = fopen(filename, "rb");
    int exists = (f != NULL);
    if (exists) fclose(f);

    (*dm)->fd = fopen(filename, "r+b");
    if ((*dm)->fd == NULL) {
        (*dm)->fd = fopen(filename, "w+b");
    }
    if ((*dm)->fd == NULL) {
        free((*dm)->filename);
        free(*dm);
        return DB_FILE_ERROR;
    }

    memset((*dm)->bitmap, 0, BITMAP_SIZE);

    if (exists) {
        if (dm_load_header(*dm) != DB_OK) {
            fclose((*dm)->fd);
            free((*dm)->filename);
            free(*dm);
            return DB_ERROR;
        }
    } else {
        (*dm)->num_pages = 0;
        memset((*dm)->bitmap, 0, BITMAP_SIZE);
        if (dm_save_header(*dm) != DB_OK) {
            fclose((*dm)->fd);
            free((*dm)->filename);
            free(*dm);
            return DB_ERROR;
        }
    }

    return DB_OK;
}

DBStatus dm_close(DiskManager* dm) {
    if (dm == NULL) return DB_OK;

    if (dm_save_header(dm) != DB_OK) return DB_ERROR;

    fclose(dm->fd);
    free(dm->filename);
    free(dm);
    return DB_OK;
}

DBStatus dm_read_page(DiskManager* dm, page_id_t page_id, Page* page) {
    if (dm == NULL || page == NULL) return DB_ERROR;
    if (page_id >= dm->num_pages) return DB_PAGE_NOT_FOUND;

    long offset = FILE_HEADER_SIZE + (long)page_id * PAGE_SIZE;
    if (fseek(dm->fd, offset, SEEK_SET) != 0) return DB_FILE_ERROR;

    size_t bytes_read = fread(page->data, 1, PAGE_SIZE, dm->fd);
    if (bytes_read != PAGE_SIZE) {
        memset(page->data, 0, PAGE_SIZE);
    }

    return DB_OK;
}

DBStatus dm_write_page(DiskManager* dm, page_id_t page_id, const Page* page) {
    if (dm == NULL || page == NULL) return DB_ERROR;

    long offset = FILE_HEADER_SIZE + (long)page_id * PAGE_SIZE;
    if (fseek(dm->fd, offset, SEEK_SET) != 0) return DB_FILE_ERROR;

    size_t bytes_written = fwrite(page->data, 1, PAGE_SIZE, dm->fd);
    if (bytes_written != PAGE_SIZE) return DB_FILE_ERROR;

    if (page_id >= dm->num_pages) {
        dm->num_pages = page_id + 1;
        dm_save_header(dm);
    }

    if (fflush(dm->fd) != 0) return DB_FILE_ERROR;
    return DB_OK;
}

page_id_t dm_allocate_page(DiskManager* dm) {
    if (dm == NULL) return (page_id_t)-1;

    for (uint32_t i = 0; i < MAX_PAGES; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        if (!(dm->bitmap[byte_idx] & (1 << bit_idx))) {
            dm->bitmap[byte_idx] |= (1 << bit_idx);
            if (i >= dm->num_pages) {
                dm->num_pages = i + 1;
                dm_save_header(dm);
            }

            Page empty_page;
            memset(&empty_page, 0, sizeof(Page));
            dm_write_page(dm, i, &empty_page);

            return i;
        }
    }

    return (page_id_t)-1;
}

DBStatus dm_free_page(DiskManager* dm, page_id_t page_id) {
    if (dm == NULL) return DB_ERROR;
    if (page_id >= MAX_PAGES) return DB_ERROR;

    uint32_t byte_idx = page_id / 8;
    uint32_t bit_idx = page_id % 8;

    if (!(dm->bitmap[byte_idx] & (1 << bit_idx))) {
        return DB_ERROR;
    }

    dm->bitmap[byte_idx] &= ~(1 << bit_idx);
    return dm_save_header(dm);
}

uint32_t dm_get_num_pages(DiskManager* dm) {
    if (dm == NULL) return 0;
    return dm->num_pages;
}

static DBStatus dm_load_header(DiskManager* dm) {
    if (fseek(dm->fd, 0, SEEK_SET) != 0) return DB_FILE_ERROR;

    size_t bytes_read = fread(&dm->num_pages, sizeof(uint32_t), 1, dm->fd);
    if (bytes_read != 1) return DB_FILE_ERROR;

    bytes_read = fread(dm->bitmap, 1, BITMAP_SIZE, dm->fd);
    if (bytes_read != BITMAP_SIZE) return DB_FILE_ERROR;

    return DB_OK;
}

static DBStatus dm_save_header(DiskManager* dm) {
    if (fseek(dm->fd, 0, SEEK_SET) != 0) return DB_FILE_ERROR;

    size_t bytes_written = fwrite(&dm->num_pages, sizeof(uint32_t), 1, dm->fd);
    if (bytes_written != 1) return DB_FILE_ERROR;

    bytes_written = fwrite(dm->bitmap, 1, BITMAP_SIZE, dm->fd);
    if (bytes_written != BITMAP_SIZE) return DB_FILE_ERROR;

    fflush(dm->fd);
    return DB_OK;
}
