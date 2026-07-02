#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include <stdio.h>
#include "types.h"

typedef struct {
    FILE* fd;
    char* filename;
    uint32_t num_pages;
    uint8_t bitmap[BITMAP_SIZE];
} DiskManager;

DBStatus dm_open(DiskManager** dm, const char* filename);
DBStatus dm_close(DiskManager* dm);
DBStatus dm_read_page(DiskManager* dm, page_id_t page_id, Page* page);
DBStatus dm_write_page(DiskManager* dm, page_id_t page_id, const Page* page);
page_id_t dm_allocate_page(DiskManager* dm);
DBStatus dm_free_page(DiskManager* dm, page_id_t page_id);
uint32_t dm_get_num_pages(DiskManager* dm);

#endif
