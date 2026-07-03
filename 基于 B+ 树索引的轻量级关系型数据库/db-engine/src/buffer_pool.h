#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include "types.h"
#include "disk_manager.h"

#define BUFFER_POOL_SIZE 64

typedef struct Frame {
    Page page;
    page_id_t page_id;
    int is_dirty;
    int pin_count;
    struct Frame* prev;
    struct Frame* next;
} Frame;

typedef struct {
    Frame* frames;
    void* hash_table;
    Frame* lru_head;
    Frame* lru_tail;
    DiskManager* dm;
    int num_frames;
    int max_frames;
} BufferPool;

DBStatus bp_init(BufferPool** bp, DiskManager* dm, int max_frames);
void bp_destroy(BufferPool* bp);

Page* bp_get_page(BufferPool* bp, page_id_t page_id);
void bp_release_page(BufferPool* bp, page_id_t page_id);
void bp_mark_dirty(BufferPool* bp, page_id_t page_id);
DBStatus bp_flush_page(BufferPool* bp, page_id_t page_id);
DBStatus bp_flush_all(BufferPool* bp);

#endif
