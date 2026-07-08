#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include <stdint.h>
#include "types.h"
#include "disk_manager.h"

typedef struct BufferPool BufferPool;

typedef struct BufferFrame BufferFrame;

BufferPool* bp_create(DiskManager* dm, uint32_t capacity);
void bp_destroy(BufferPool* pool);
Page* bp_get_page(BufferPool* pool, page_id_t page_id);
DBStatus bp_mark_dirty(BufferPool* pool, page_id_t page_id);
DBStatus bp_flush_page(BufferPool* pool, page_id_t page_id);
DBStatus bp_close(BufferPool* pool);

#endif
