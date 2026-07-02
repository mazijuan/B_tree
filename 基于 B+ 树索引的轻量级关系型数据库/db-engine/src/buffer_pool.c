#include "buffer_pool.h"
#include <stdlib.h>
#include <string.h>

typedef struct BufferFrame {
    page_id_t page_id;
    Page page;
    int valid;
    int dirty;
    int pin_count;
    struct BufferFrame* prev;
    struct BufferFrame* next;
} BufferFrame;

struct BufferPool {
    DiskManager* dm;
    uint32_t capacity;
    uint32_t size;
    BufferFrame* head;
    BufferFrame* tail;
    BufferFrame* frames;
};

static void bp_detach(BufferPool* pool, BufferFrame* frame) {
    if (frame == NULL || pool == NULL) return;

    if (frame->prev != NULL) {
        frame->prev->next = frame->next;
    } else {
        pool->head = frame->next;
    }

    if (frame->next != NULL) {
        frame->next->prev = frame->prev;
    } else {
        pool->tail = frame->prev;
    }

    frame->prev = NULL;
    frame->next = NULL;
}

static void bp_attach_front(BufferPool* pool, BufferFrame* frame) {
    if (pool == NULL || frame == NULL) return;

    bp_detach(pool, frame);
    frame->prev = NULL;
    frame->next = pool->head;
    if (pool->head != NULL) {
        pool->head->prev = frame;
    } else {
        pool->tail = frame;
    }
    pool->head = frame;
}

static BufferFrame* bp_find_frame(BufferPool* pool, page_id_t page_id) {
    if (pool == NULL) return NULL;

    for (BufferFrame* frame = pool->head; frame != NULL; frame = frame->next) {
        if (frame->valid && frame->page_id == page_id) {
            return frame;
        }
    }
    return NULL;
}

static BufferFrame* bp_find_free_frame(BufferPool* pool) {
    if (pool == NULL || pool->frames == NULL) return NULL;

    for (uint32_t i = 0; i < pool->capacity; i++) {
        BufferFrame* frame = &pool->frames[i];
        if (!frame->valid) {
            return frame;
        }
    }
    return NULL;
}

static BufferFrame* bp_evict_lru(BufferPool* pool) {
    if (pool == NULL || pool->tail == NULL) return NULL;

    BufferFrame* victim = pool->tail;
    bp_detach(pool, victim);

    if (victim->dirty && victim->valid) {
        DBStatus status = dm_write_page(pool->dm, victim->page_id, &victim->page);
        if (status != DB_OK) {
            return NULL;
        }
    }

    victim->valid = 0;
    victim->dirty = 0;
    victim->pin_count = 0;
    victim->page_id = 0;
    memset(&victim->page, 0, sizeof(Page));
    return victim;
}

BufferPool* bp_create(DiskManager* dm, uint32_t capacity) {
    if (dm == NULL || capacity == 0) return NULL;

    BufferPool* pool = (BufferPool*)calloc(1, sizeof(BufferPool));
    if (pool == NULL) return NULL;

    pool->frames = (BufferFrame*)calloc(capacity, sizeof(BufferFrame));
    if (pool->frames == NULL) {
        free(pool);
        return NULL;
    }

    pool->dm = dm;
    pool->capacity = capacity;
    pool->size = 0;
    return pool;
}

void bp_destroy(BufferPool* pool) {
    if (pool == NULL) return;

    free(pool->frames);
    free(pool);
}

Page* bp_get_page(BufferPool* pool, page_id_t page_id) {
    if (pool == NULL || pool->dm == NULL) return NULL;

    BufferFrame* frame = bp_find_frame(pool, page_id);
    if (frame != NULL) {
        bp_attach_front(pool, frame);
        return &frame->page;
    }

    frame = bp_find_free_frame(pool);
    if (frame == NULL) {
        frame = bp_evict_lru(pool);
    }

    if (frame == NULL) return NULL;

    if (dm_read_page(pool->dm, page_id, &frame->page) != DB_OK) {
        return NULL;
    }

    frame->page_id = page_id;
    frame->valid = 1;
    frame->dirty = 0;
    frame->pin_count = 0;
    bp_attach_front(pool, frame);
    if (pool->size < pool->capacity) {
        pool->size++;
    }

    return &frame->page;
}

DBStatus bp_mark_dirty(BufferPool* pool, page_id_t page_id) {
    if (pool == NULL) return DB_ERROR;

    BufferFrame* frame = bp_find_frame(pool, page_id);
    if (frame == NULL) return DB_PAGE_NOT_FOUND;

    frame->dirty = 1;
    return DB_OK;
}

DBStatus bp_flush_page(BufferPool* pool, page_id_t page_id) {
    if (pool == NULL) return DB_ERROR;

    BufferFrame* frame = bp_find_frame(pool, page_id);
    if (frame == NULL) return DB_PAGE_NOT_FOUND;

    if (frame->dirty) {
        DBStatus status = dm_write_page(pool->dm, page_id, &frame->page);
        if (status != DB_OK) return status;
        frame->dirty = 0;
    }
    return DB_OK;
}

DBStatus bp_close(BufferPool* pool) {
    if (pool == NULL) return DB_OK;

    for (BufferFrame* frame = pool->head; frame != NULL; frame = frame->next) {
        if (frame->dirty && frame->valid) {
            DBStatus status = dm_write_page(pool->dm, frame->page_id, &frame->page);
            if (status != DB_OK) {
                bp_destroy(pool);
                return status;
            }
            frame->dirty = 0;
        }
    }

    bp_destroy(pool);
    return DB_OK;
}
