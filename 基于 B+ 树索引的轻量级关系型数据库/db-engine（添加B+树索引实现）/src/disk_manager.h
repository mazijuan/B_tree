#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include <stdio.h>
#include "types.h"

// 磁盘管理器结构体
typedef struct {
    FILE*     fd;
    char*     filename;
    uint32_t  num_pages;
    uint8_t   bitmap[BITMAP_SIZE];
} DiskManager;

// 对外API
DBStatus db_open(DiskManager** dm, const char* filename);
DBStatus db_close(DiskManager* dm);
DBStatus db_read_page(DiskManager* dm, page_id_t page_id, Page* page);
DBStatus db_write_page(DiskManager* dm, page_id_t page_id, const Page* page);
page_id_t db_allocate_page(DiskManager* dm);
DBStatus db_free_page(DiskManager* dm, page_id_t page_id);
uint32_t dm_get_num_pages(DiskManager* dm);

// 内部静态工具函数声明
static DBStatus dm_load_header(DiskManager* dm);
static DBStatus dm_save_header(DiskManager* dm);

#endif
