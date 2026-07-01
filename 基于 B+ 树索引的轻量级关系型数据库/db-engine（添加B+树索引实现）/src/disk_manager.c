#include "disk_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 文件头部占用字节：总页数(uint32_t) + 空闲位图
#define FILE_HEADER_SIZE (sizeof(uint32_t) + BITMAP_SIZE)

static DBStatus dm_load_header(DiskManager* dm);
static DBStatus dm_save_header(DiskManager* dm);

// 创建或打开数据库文件
DBStatus db_open(DiskManager** dm, const char* filename) {
    if (filename == NULL || dm == NULL) {
        return DB_ERROR;
    }

    // 分配磁盘管理器结构体
    *dm = (DiskManager*)malloc(sizeof(DiskManager));
    if (*dm == NULL) {
        return DB_ERROR;
    }

    // 拷贝文件名
    size_t name_len = strlen(filename);
    (*dm)->filename = (char*)malloc(name_len + 1);
    if ((*dm)->filename == NULL) {
        free(*dm);
        *dm = NULL;
        return DB_ERROR;
    }
    strcpy((*dm)->filename, filename);

    // 判断文件是否存在
    FILE* test_fp = fopen(filename, "rb");
    int file_exists = (test_fp != NULL);
    if (test_fp != NULL) fclose(test_fp);

    // 读写模式打开，不存在则新建
    (*dm)->fd = fopen(filename, "r+b");
    if ((*dm)->fd == NULL) {
        (*dm)->fd = fopen(filename, "w+b");
    }
    if ((*dm)->fd == NULL) {
        free((*dm)->filename);
        free(*dm);
        *dm = NULL;
        return DB_FILE_ERROR;
    }

    memset((*dm)->bitmap, 0, BITMAP_SIZE);

    if (file_exists) {
        // 已有数据库，加载头部元数据
        if (dm_load_header(*dm) != DB_OK) {
            fclose((*dm)->fd);
            free((*dm)->filename);
            free(*dm);
            *dm = NULL;
            return DB_ERROR;
        }
    } else {
        // 新建数据库，初始化元数据并写入文件头
        (*dm)->num_pages = 0;
        memset((*dm)->bitmap, 0, BITMAP_SIZE);
        if (dm_save_header(*dm) != DB_OK) {
            fclose((*dm)->fd);
            free((*dm)->filename);
            free(*dm);
            *dm = NULL;
            return DB_ERROR;
        }
    }

    return DB_OK;
}

// 关闭磁盘管理器，持久化元数据并释放资源
DBStatus db_close(DiskManager* dm) {
    if (dm == NULL) return DB_OK;

    if (dm_save_header(dm) != DB_OK) {
        return DB_ERROR;
    }

    fclose(dm->fd);
    free(dm->filename);
    free(dm);
    return DB_OK;
}

// 从磁盘读取指定页到Page缓冲区
DBStatus db_read_page(DiskManager* dm, page_id_t page_id, Page* page) {
    if (dm == NULL || page == NULL) return DB_ERROR;
    if (page_id >= dm->num_pages || page_id >= MAX_PAGES) {
        return DB_PAGE_NOT_FOUND;
    }

    long page_offset = FILE_HEADER_SIZE + (long)page_id * PAGE_SIZE;
    if (fseek(dm->fd, page_offset, SEEK_SET) != 0) {
        return DB_FILE_ERROR;
    }

    size_t read_cnt = fread(page->data, sizeof(uint8_t), PAGE_SIZE, dm->fd);
    if (read_cnt != PAGE_SIZE) {
        return DB_FILE_ERROR;
    }
    return DB_OK;
}

// 将Page缓冲区写入磁盘指定页
DBStatus db_write_page(DiskManager* dm, page_id_t page_id, const Page* page) {
    if (dm == NULL || page == NULL) return DB_ERROR;
    if (page_id >= dm->num_pages || page_id >= MAX_PAGES) {
        return DB_PAGE_NOT_FOUND;
    }

    long page_offset = FILE_HEADER_SIZE + (long)page_id * PAGE_SIZE;
    if (fseek(dm->fd, page_offset, SEEK_SET) != 0) {
        return DB_FILE_ERROR;
    }

    size_t write_cnt = fwrite(page->data, sizeof(uint8_t), PAGE_SIZE, dm->fd);
    if (write_cnt != PAGE_SIZE) {
        return DB_FILE_ERROR;
    }
    fflush(dm->fd);
    return DB_OK;
}

// 分配一个新空闲页，返回page_id；无空闲页返回-1
page_id_t db_allocate_page(DiskManager* dm) {
    if (dm == NULL) return (page_id_t)-1;

    for (uint32_t pid = 0; pid < MAX_PAGES; pid++) {
        uint32_t byte_off = pid / 8;
        uint32_t bit_off  = pid % 8;

        if (!(dm->bitmap[byte_off] & (1U << bit_off))) {
            // 标记为已占用
            dm->bitmap[byte_off] |= (1U << bit_off);

            // 扩展总页数
            if (pid >= dm->num_pages) {
                dm->num_pages = pid + 1;
                dm_save_header(dm);
            }
            return pid;
        }
    }

    return (page_id_t)-1;
}

// 释放指定页，标记为空闲
DBStatus db_free_page(DiskManager* dm, page_id_t page_id) {
    if (dm == NULL) return DB_ERROR;
    if (page_id >= MAX_PAGES) return DB_ERROR;

    uint32_t byte_off = page_id / 8;
    uint32_t bit_off  = page_id % 8;

    // 当前页已是空闲，释放失败
    if (!(dm->bitmap[byte_off] & (1U << bit_off))) {
        return DB_ERROR;
    }

    dm->bitmap[byte_off] &= ~(1U << bit_off);
    return dm_save_header(dm);
}

// 获取当前数据库总页数
uint32_t dm_get_num_pages(DiskManager* dm) {
    if (dm == NULL) return 0;
    return dm->num_pages;
}

// 从磁盘读取文件头部元数据（总页数+位图）
static DBStatus dm_load_header(DiskManager* dm) {
    if (fseek(dm->fd, 0, SEEK_SET) != 0) {
        return DB_FILE_ERROR;
    }

    size_t read_num = fread(&dm->num_pages, sizeof(uint32_t), 1, dm->fd);
    if (read_num != 1) return DB_FILE_ERROR;

    size_t read_bitmap = fread(dm->bitmap, sizeof(uint8_t), BITMAP_SIZE, dm->fd);
    if (read_bitmap != BITMAP_SIZE) return DB_FILE_ERROR;

    return DB_OK;
}

// 将内存元数据写入文件头部持久化
static DBStatus dm_save_header(DiskManager* dm) {
    if (fseek(dm->fd, 0, SEEK_SET) != 0) {
        return DB_FILE_ERROR;
    }

    size_t write_num = fwrite(&dm->num_pages, sizeof(uint32_t), 1, dm->fd);
    if (write_num != 1) return DB_FILE_ERROR;

    size_t write_bitmap = fwrite(dm->bitmap, sizeof(uint8_t), BITMAP_SIZE, dm->fd);
    if (write_bitmap != BITMAP_SIZE) return DB_FILE_ERROR;

    fflush(dm->fd);
    return DB_OK;
}
