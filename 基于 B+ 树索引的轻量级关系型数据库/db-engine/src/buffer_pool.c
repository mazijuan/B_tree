#include "buffer_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct HashEntry {
    page_id_t page_id;
    Frame* frame;
    struct HashEntry* next;
} HashEntry;

static unsigned int hash_page_id(page_id_t page_id, int capacity) {
    return (unsigned int)(page_id % capacity);
}

static void lru_remove(BufferPool* bp, Frame* frame) {
    if (frame->prev == NULL && frame->next == NULL) {
        if (bp->lru_head == frame) {
            bp->lru_head = NULL;
            bp->lru_tail = NULL;
        }
        return;
    }
    if (frame->prev != NULL) {
        frame->prev->next = frame->next;
    } else {
        bp->lru_head = frame->next;
    }
    if (frame->next != NULL) {
        frame->next->prev = frame->prev;
    } else {
        bp->lru_tail = frame->prev;
    }
    frame->prev = NULL;
    frame->next = NULL;
}

static void lru_add_to_head(BufferPool* bp, Frame* frame) {
    lru_remove(bp, frame);
    frame->next = bp->lru_head;
    if (bp->lru_head != NULL) {
        bp->lru_head->prev = frame;
    }
    bp->lru_head = frame;
    if (bp->lru_tail == NULL) {
        bp->lru_tail = frame;
    }
}

static Frame* bp_find_frame(BufferPool* bp, page_id_t page_id) {
    HashEntry** hash_table = (HashEntry**)bp->hash_table;
    unsigned int idx = hash_page_id(page_id, bp->max_frames);
    HashEntry* entry = hash_table[idx];
    while (entry != NULL) {
        if (entry->page_id == page_id) {
            return entry->frame;
        }
        entry = entry->next;
    }
    return NULL;
}

static void bp_insert_hash(BufferPool* bp, Frame* frame) {
    HashEntry** hash_table = (HashEntry**)bp->hash_table;
    unsigned int idx = hash_page_id(frame->page_id, bp->max_frames);
    HashEntry* entry = (HashEntry*)malloc(sizeof(HashEntry));
    entry->page_id = frame->page_id;
    entry->frame = frame;
    entry->next = hash_table[idx];
    hash_table[idx] = entry;
}

static void bp_remove_hash(BufferPool* bp, page_id_t page_id) {
    HashEntry** hash_table = (HashEntry**)bp->hash_table;
    unsigned int idx = hash_page_id(page_id, bp->max_frames);
    HashEntry** prev = &hash_table[idx];
    HashEntry* entry = hash_table[idx];
    while (entry != NULL) {
        if (entry->page_id == page_id) {
            *prev = entry->next;
            free(entry);
            return;
        }
        prev = &entry->next;
        entry = entry->next;
    }
}

static Frame* bp_evict(BufferPool* bp) {
    Frame* frame = bp->lru_tail;
    while (frame != NULL) {
        if (frame->pin_count == 0) {
            lru_remove(bp, frame);
            bp_remove_hash(bp, frame->page_id);
            if (frame->is_dirty) {
                dm_write_page(bp->dm, frame->page_id, &frame->page);
                frame->is_dirty = 0;
            }
            return frame;
        }
        frame = frame->prev;
    }
    return NULL;
}

DBStatus bp_init(BufferPool** bp, DiskManager* dm, int max_frames) {
    *bp = (BufferPool*)malloc(sizeof(BufferPool));
    if (*bp == NULL) return DB_ERROR;

    (*bp)->frames = (Frame*)malloc(max_frames * sizeof(Frame));
    if ((*bp)->frames == NULL) {
        free(*bp);
        return DB_ERROR;
    }

    (*bp)->hash_table = (HashEntry**)calloc(max_frames, sizeof(HashEntry*));
    if ((*bp)->hash_table == NULL) {
        free((*bp)->frames);
        free(*bp);
        return DB_ERROR;
    }

    (*bp)->dm = dm;
    (*bp)->max_frames = max_frames;
    (*bp)->num_frames = 0;
    (*bp)->lru_head = NULL;
    (*bp)->lru_tail = NULL;

    memset((*bp)->frames, 0, max_frames * sizeof(Frame));
    for (int i = 0; i < max_frames; i++) {
        (*bp)->frames[i].page_id = (page_id_t)-1;
        (*bp)->frames[i].is_dirty = 0;
        (*bp)->frames[i].pin_count = 0;
        (*bp)->frames[i].prev = NULL;
        (*bp)->frames[i].next = NULL;
    }

    return DB_OK;
}

void bp_destroy(BufferPool* bp) {
    if (bp == NULL) return;
    bp_flush_all(bp);

    HashEntry** hash_table = (HashEntry**)bp->hash_table;
    for (int i = 0; i < bp->max_frames; i++) {
        HashEntry* entry = hash_table[i];
        while (entry != NULL) {
            HashEntry* next = entry->next;
            free(entry);
            entry = next;
        }
    }
    free(bp->hash_table);
    free(bp->frames);
    free(bp);
}

Page* bp_get_page(BufferPool* bp, page_id_t page_id) {
    if (bp == NULL) return NULL;

    Frame* frame = bp_find_frame(bp, page_id);
    if (frame != NULL) {
        frame->pin_count++;
        lru_add_to_head(bp, frame);
        return &frame->page;
    }

    Frame* evicted = NULL;
    if (bp->num_frames >= bp->max_frames) {
        evicted = bp_evict(bp);
        if (evicted == NULL) {
            return NULL;
        }
    }

    Frame* new_frame;
    if (evicted != NULL) {
        new_frame = evicted;
    } else {
        new_frame = &bp->frames[bp->num_frames];
        bp->num_frames++;
    }

    DBStatus status = dm_read_page(bp->dm, page_id, &new_frame->page);
    if (status != DB_OK) {
        if (evicted == NULL) {
            bp->num_frames--;
        }
        return NULL;
    }

    new_frame->page_id = page_id;
    new_frame->is_dirty = 0;
    new_frame->pin_count = 1;
    bp_insert_hash(bp, new_frame);
    lru_add_to_head(bp, new_frame);

    return &new_frame->page;
}

void bp_release_page(BufferPool* bp, page_id_t page_id) {
    if (bp == NULL) return;
    Frame* frame = bp_find_frame(bp, page_id);
    if (frame != NULL && frame->pin_count > 0) {
        frame->pin_count--;
    }
}

void bp_mark_dirty(BufferPool* bp, page_id_t page_id) {
    if (bp == NULL) return;
    Frame* frame = bp_find_frame(bp, page_id);
    if (frame != NULL) {
        frame->is_dirty = 1;
    }
}

DBStatus bp_flush_page(BufferPool* bp, page_id_t page_id) {
    if (bp == NULL) return DB_ERROR;
    Frame* frame = bp_find_frame(bp, page_id);
    if (frame == NULL) return DB_PAGE_NOT_FOUND;
    if (!frame->is_dirty) return DB_OK;

    DBStatus status = dm_write_page(bp->dm, frame->page_id, &frame->page);
    if (status == DB_OK) {
        frame->is_dirty = 0;
    }
    return status;
}

DBStatus bp_flush_all(BufferPool* bp) {
    if (bp == NULL) return DB_ERROR;
    for (int i = 0; i < bp->num_frames; i++) {
        Frame* frame = &bp->frames[i];
        if (frame->page_id != (page_id_t)-1 && frame->is_dirty) {
            DBStatus status = dm_write_page(bp->dm, frame->page_id, &frame->page);
            if (status != DB_OK) return status;
            frame->is_dirty = 0;
        }
    }
    return DB_OK;
}
