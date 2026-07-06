#include "disk_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_HEADER_SIZE (sizeof(uint32_t) + BITMAP_SIZE)

static DBStatus dm_load_header(DiskManager* dm);
static DBStatus dm_save_header(DiskManager* dm);

DBStatus dm_open(DiskManager** dm, const char* filename) {
    // 入参空校验
    if (dm == NULL || filename == NULL) {
        return DB_ERROR;
    }

    *dm = (DiskManager*)malloc(sizeof(DiskManager));
    if (*dm == NULL) {
        return DB_ERROR;
    }

    size_t name_len = strlen(filename);
    (*dm)->filename = (char*)malloc(name_len + 1);
    if ((*dm)->filename == NULL) {
        goto clean_dm_only;
    }
    strcpy((*dm)->filename, filename);

    // 尝试读写打开，不存在则创建
    FILE* fp = fopen(filename, "r+b");
    if (fp == NULL) {
        fp = fopen(filename, "w+b");
    }
    if (fp == NULL) {
        goto clean_all;
    }
    (*dm)->fd = fp;

    // 判断文件是否已有内容：读取文件头部判断是否有效
    int file_exists = 0;
    long file_size = 0;
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (file_size >= (long)FILE_HEADER_SIZE) {
        file_exists = 1;
    }

    memset((*dm)->bitmap, 0, BITMAP_SIZE);
    (*dm)->num_pages = 0;

    if (file_exists) {
        if (dm_load_header(*dm) != DB_OK) {
            goto clean_all;
        }
    } else {
        if (dm_save_header(*dm) != DB_OK) {
            goto clean_all;
        }
    }

    return DB_OK;

clean_all:
    fclose((*dm)->fd);
    free((*dm)->filename);
clean_dm_only:
    free(*dm);
    *dm = NULL;
    return DB_ERROR;
}

DBStatus dm_close(DiskManager* dm) {
    if (dm == NULL) {
        return DB_OK;
    }

    DBStatus ret = dm_save_header(dm);
    fclose(dm->fd);
    free(dm->filename);
    free(dm);
    return ret;
}

DBStatus dm_read_page(DiskManager* dm, page_id_t page_id, Page* page) {
    if (dm == NULL || page == NULL) {
        return DB_ERROR;
    }
    if ((uint32_t)page_id >= dm->num_pages) {
        return DB_PAGE_NOT_FOUND;
    }

    long offset = (long)FILE_HEADER_SIZE + (long)page_id * PAGE_SIZE;
    if (fseek(dm->fd, offset, SEEK_SET) != 0) {
        return DB_FILE_ERROR;
    }

    size_t read_cnt = fread(page->data, 1, PAGE_SIZE, dm->fd);
    if (read_cnt != PAGE_SIZE) {
        memset(page->data, 0, PAGE_SIZE);
    }

    return DB_OK;
}

DBStatus dm_write_page(DiskManager* dm, page_id_t page_id, const Page* page) {
    if (dm == NULL || page == NULL) {
        return DB_ERROR;
    }

    long offset = (long)FILE_HEADER_SIZE + (long)page_id * PAGE_SIZE;
    if (fseek(dm->fd, offset, SEEK_SET) != 0) {
        return DB_FILE_ERROR;
    }

    size_t write_cnt = fwrite(page->data, 1, PAGE_SIZE, dm->fd);
    if (write_cnt != PAGE_SIZE) {
        return DB_FILE_ERROR;
    }

    uint32_t pid = (uint32_t)page_id;
    if (pid >= dm->num_pages) {
        dm->num_pages = pid + 1;
        // 校验头部保存结果
        if (dm_save_header(dm) != DB_OK) {
            return DB_FILE_ERROR;
        }
    }

    if (fflush(dm->fd) != 0) {
        return DB_FILE_ERROR;
    }
    return DB_OK;
}

page_id_t dm_allocate_page(DiskManager* dm) {
    if (dm == NULL) {
        return (page_id_t)-1;
    }

    for (uint32_t i = 0; i < MAX_PAGES; i++) {
        uint32_t byte_idx = i / 8;
        uint8_t bit_mask = (uint8_t)(1U << (i % 8));
        if (!(dm->bitmap[byte_idx] & bit_mask)) {
            dm->bitmap[byte_idx] |= bit_mask;

            if (i >= dm->num_pages) {
                dm->num_pages = i + 1;
                dm_save_header(dm);
            }

            Page empty_page;
            memset(&empty_page, 0, sizeof(Page));
            dm_write_page(dm, i, &empty_page);

            return (page_id_t)i;
        }
    }

    return (page_id_t)-1;
}

DBStatus dm_free_page(DiskManager* dm, page_id_t page_id) {
    if (dm == NULL) {
        return DB_ERROR;
    }
    uint32_t pid = (uint32_t)page_id;
    if (pid >= MAX_PAGES) {
        return DB_ERROR;
    }

    uint32_t byte_idx = pid / 8;
    uint8_t bit_mask = (uint8_t)(1U << (pid % 8));
    if (!(dm->bitmap[byte_idx] & bit_mask)) {
        return DB_ERROR;
    }

    dm->bitmap[byte_idx] &= ~bit_mask;
    return dm_save_header(dm);
}

uint32_t dm_get_num_pages(DiskManager* dm) {
    return (dm == NULL) ? 0 : dm->num_pages;
}

static DBStatus dm_load_header(DiskManager* dm) {
    if (fseek(dm->fd, 0, SEEK_SET) != 0) {
        return DB_FILE_ERROR;
    }

    size_t read_cnt = fread(&dm->num_pages, sizeof(uint32_t), 1, dm->fd);
    if (read_cnt != 1) {
        return DB_FILE_ERROR;
    }

    read_cnt = fread(dm->bitmap, 1, BITMAP_SIZE, dm->fd);
    if (read_cnt != BITMAP_SIZE) {
        return DB_FILE_ERROR;
    }

    return DB_OK;
}

static DBStatus dm_save_header(DiskManager* dm) {
    if (fseek(dm->fd, 0, SEEK_SET) != 0) {
        return DB_FILE_ERROR;
    }

    size_t write_cnt = fwrite(&dm->num_pages, sizeof(uint32_t), 1, dm->fd);
    if (write_cnt != 1) {
        return DB_FILE_ERROR;
    }

    write_cnt = fwrite(dm->bitmap, 1, BITMAP_SIZE, dm->fd);
    if (write_cnt != BITMAP_SIZE) {
        return DB_FILE_ERROR;
    }

    fflush(dm->fd);
    return DB_OK;
}
