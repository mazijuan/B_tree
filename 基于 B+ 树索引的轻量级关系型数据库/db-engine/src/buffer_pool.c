#include "buffer_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 哈希链表节点
typedef struct HashEntry {
    page_id_t page_id;
    Frame* frame;
    struct HashEntry* next;
} HashEntry;

// 计算哈希下标
static unsigned int hash_page_id(page_id_t page_id, int capacity) {
    return (unsigned int)(page_id % capacity);
}

// 将frame从LRU双向链表摘除
static void lru_remove(BufferPool* bp, Frame* frame) {
    if (frame->prev == NULL && frame->next == NULL) {
        if (bp->lru_head == frame) {
            bp->lru_head = NULL;
            bp->lru_tail = NULL;
        }
        return;
    }

    // 修改前驱节点后继
    if (frame->prev != NULL) {
        frame->prev->next = frame->next;
    } else {
        bp->lru_head = frame->next;
    }
    // 修改后继节点前驱
    if (frame->next != NULL) {
        frame->next->prev = frame->prev;
    } else {
        bp->lru_tail = frame->prev;
    }

    // 清空当前frame链表指针
    frame->prev = NULL;
    frame->next = NULL;
}

// 将frame移动到LRU链表头部（最近使用）
static void lru_add_to_head(BufferPool* bp, Frame* frame) {
    lru_remove(bp, frame);
    frame->next = bp->lru_head;
    if (bp->lru_head != NULL) {
        bp->lru_head->prev = frame;
    }
    bp->lru_head = frame;
    // 空链表时头尾都指向当前帧
    if (bp->lru_tail == NULL) {
        bp->lru_tail = frame;
    }
}

// 哈希表查找页对应的Frame，找不到返回NULL
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

// 哈希表插入页帧映射
static void bp_insert_hash(BufferPool* bp, Frame* frame) {
    HashEntry** hash_table = (HashEntry**)bp->hash_table;
    unsigned int idx = hash_page_id(frame->page_id, bp->max_frames);
    HashEntry* entry = (HashEntry*)malloc(sizeof(HashEntry));
    // 内存分配失败直接返回，避免野指针写入
    if (entry == NULL) return;

    entry->page_id = frame->page_id;
    entry->frame = frame;
    entry->next = hash_table[idx];
    hash_table[idx] = entry;
}

// 哈希表删除指定页ID映射
static void bp_remove_hash(BufferPool* bp, page_id_t page_id) {
    HashEntry** hash_table = (HashEntry**)bp->hash_table;
    unsigned int idx = hash_page_id(page_id, bp->max_frames);
    HashEntry** pp_entry = &hash_table[idx];
    HashEntry* entry = hash_table[idx];

    while (entry != NULL) {
        if (entry->page_id == page_id) {
            *pp_entry = entry->next;
            free(entry);
            return;
        }
        pp_entry = &entry->next;
        entry = entry->next;
    }
}

// LRU淘汰：从尾部寻找可驱逐（pin_count=0）的帧
static Frame* bp_evict(BufferPool* bp) {
    Frame* frame = bp->lru_tail;
    // 从尾部向前遍历，找到第一个无引用帧
    for (; frame != NULL; frame = frame->prev) {
        if (frame->pin_count == 0) {
            lru_remove(bp, frame);
            bp_remove_hash(bp, frame->page_id);
            // 脏页落盘
            if (frame->is_dirty) {
                dm_write_page(bp->dm, frame->page_id, &frame->page);
                frame->is_dirty = 0;
            }
            // 重置页ID标记为空帧
            frame->page_id = (page_id_t)-1;
            return frame;
        }
    }
    // 所有帧都被pin住，无法驱逐
    return NULL;
}

// 初始化缓冲池
DBStatus bp_init(BufferPool** bp, DiskManager* dm, int max_frames) {
    // 入参基础校验
    if (bp == NULL || dm == NULL || max_frames <= 0) {
        return DB_ERROR;
    }

    *bp = (BufferPool*)malloc(sizeof(BufferPool));
    if (*bp == NULL) return DB_ERROR;

    // 分配帧数组
    (*bp)->frames = (Frame*)calloc(max_frames, sizeof(Frame));
    if ((*bp)->frames == NULL) {
        free(*bp);
        return DB_ERROR;
    }

    // 哈希表数组初始化（桶数组）
    (*bp)->hash_table = calloc(max_frames, sizeof(HashEntry*));
    if ((*bp)->hash_table == NULL) {
        free((*bp)->frames);
        free(*bp);
        return DB_ERROR;
    }

    // 基础元数据赋值
    (*bp)->dm = dm;
    (*bp)->max_frames = max_frames;
    (*bp)->num_frames = 0;
    (*bp)->lru_head = NULL;
    (*bp)->lru_tail = NULL;

    // calloc已自动零初始化，仅需标记空页ID，省去完整memset
    for (int i = 0; i < max_frames; i++) {
        (*bp)->frames[i].page_id = (page_id_t)-1;
    }

    return DB_OK;
}

// 销毁缓冲池，释放全部资源
void bp_destroy(BufferPool* bp) {
    if (bp == NULL) return;
    // 先刷新所有脏页落盘
    bp_flush_all(bp);

    // 释放哈希链表所有节点
    HashEntry** hash_table = (HashEntry**)bp->hash_table;
    for (int i = 0; i < bp->max_frames; i++) {
        HashEntry* entry = hash_table[i];
        while (entry != NULL) {
            HashEntry* next_entry = entry->next;
            free(entry);
            entry = next_entry;
        }
    }
    free(bp->hash_table);
    free(bp->frames);
    free(bp);
}

// 获取指定页，自动加载/驱逐/增加引用计数
Page* bp_get_page(BufferPool* bp, page_id_t page_id) {
    if (bp == NULL) return NULL;

    // 1. 缓存命中，直接提升LRU热度
    Frame* frame = bp_find_frame(bp, page_id);
    if (frame != NULL) {
        frame->pin_count++;
        lru_add_to_head(bp, frame);
        return &frame->page;
    }

    Frame* new_frame = NULL;
    // 2. 缓存已满，执行驱逐
    if (bp->num_frames >= bp->max_frames) {
        new_frame = bp_evict(bp);
        if (new_frame == NULL) {
            // 所有页面被锁定，驱逐失败
            return NULL;
        }
    } else {
        // 缓存尚有空闲帧，取用新帧
        new_frame = &bp->frames[bp->num_frames];
        bp->num_frames++;
    }

    // 3. 从磁盘读取页面数据
    DBStatus read_stat = dm_read_page(bp->dm, page_id, &new_frame->page);
    if (read_stat != DB_OK) {
        // 读取失败，若为新分配帧则回收计数
        if (bp->num_frames > 0 && new_frame == &bp->frames[bp->num_frames - 1]) {
            bp->num_frames--;
        }
        return NULL;
    }

    // 4. 初始化帧元数据
    new_frame->page_id = page_id;
    new_frame->is_dirty = 0;
    new_frame->pin_count = 1;
    bp_insert_hash(bp, new_frame);
    lru_add_to_head(bp, new_frame);

    return &new_frame->page;
}

// 释放页面引用，减少pin计数
void bp_release_page(BufferPool* bp, page_id_t page_id) {
    if (bp == NULL) return;
    Frame* frame = bp_find_frame(bp, page_id);
    if (frame != NULL && frame->pin_count > 0) {
        frame->pin_count--;
    }
}

// 标记页面为脏页，等待落盘
void bp_mark_dirty(BufferPool* bp, page_id_t page_id) {
    if (bp == NULL) return;
    Frame* frame = bp_find_frame(bp, page_id);
    if (frame != NULL) {
        frame->is_dirty = 1;
    }
}

// 强制刷新单页到磁盘
DBStatus bp_flush_page(BufferPool* bp, page_id_t page_id) {
    if (bp == NULL) return DB_ERROR;
    Frame* frame = bp_find_frame(bp, page_id);
    if (frame == NULL) return DB_PAGE_NOT_FOUND;
    // 干净页无需刷盘，直接成功
    if (!frame->is_dirty) return DB_OK;

    DBStatus write_stat = dm_write_page(bp->dm, frame->page_id, &frame->page);
    if (write_stat == DB_OK) {
        frame->is_dirty = 0;
    }
    return write_stat;
}

// 刷新缓冲池内所有脏页
DBStatus bp_flush_all(BufferPool* bp) {
    if (bp == NULL) return DB_ERROR;
    for (int i = 0; i < bp->num_frames; i++) {
        Frame* frame = &bp->frames[i];
        if (frame->page_id == (page_id_t)-1 || !frame->is_dirty) {
            continue;
        }
        DBStatus stat = dm_write_page(bp->dm, frame->page_id, &frame->page);
        if (stat != DB_OK) {
            return stat;
        }
        frame->is_dirty = 0;
    }
    return DB_OK;
}
