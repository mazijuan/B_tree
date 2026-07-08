#include "buffer_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===== 内部辅助函数 =====

// 哈希函数：page_id -> 哈希表槽位
static int bp_hash(BufferPool* bp, page_id_t page_id) {
    return (int)(page_id % (page_id_t)bp->hash_size);
}

// 在哈希表中查找页帧
static Frame* bp_hash_lookup(BufferPool* bp, page_id_t page_id) {
    int slot = bp_hash(bp, page_id);
    int i = slot;
    do {
        Frame* f = bp->page_table[i];
        if (f == NULL) return NULL;
        if (f == (Frame*)-1) {
            // 墓碑标记，继续探测
            i = (i + 1) % bp->hash_size;
            continue;
        }
        if (f->page_id == page_id && f->pin_count >= 0) {
            return f;
        }
        i = (i + 1) % bp->hash_size;
    } while (i != slot);
    return NULL;
}

// 向哈希表插入页帧
static DBStatus bp_hash_insert(BufferPool* bp, Frame* frame) {
    int slot = bp_hash(bp, frame->page_id);
    int i = slot;
    do {
        if (bp->page_table[i] == NULL || bp->page_table[i] == (Frame*)-1) {
            bp->page_table[i] = frame;
            return DB_OK;
        }
        i = (i + 1) % bp->hash_size;
    } while (i != slot);
    return DB_ERROR; // 哈希表已满
}

// 从哈希表删除页帧
static void bp_hash_remove(BufferPool* bp, page_id_t page_id) {
    int slot = bp_hash(bp, page_id);
    int i = slot;
    do {
        Frame* f = bp->page_table[i];
        if (f == NULL) return;
        if (f != (Frame*)-1 && f->page_id == page_id) {
            bp->page_table[i] = (Frame*)-1; // 墓碑
            return;
        }
        i = (i + 1) % bp->hash_size;
    } while (i != slot);
}

// 将frame移动到LRU链表头部（表示最近使用）
static void bp_lru_move_to_head(BufferPool* bp, Frame* frame) {
    // 如果已经在头部，无需操作
    if (bp->lru_head == frame) return;

    // 从当前位置摘除
    if (frame->prev != NULL) {
        frame->prev->next = frame->next;
    }
    if (frame->next != NULL) {
        frame->next->prev = frame->prev;
    }
    if (bp->lru_tail == frame) {
        bp->lru_tail = frame->prev;
    }

    // 插入到头部
    frame->prev = NULL;
    frame->next = bp->lru_head;
    if (bp->lru_head != NULL) {
        bp->lru_head->prev = frame;
    }
    bp->lru_head = frame;
    if (bp->lru_tail == NULL) {
        bp->lru_tail = frame;
    }
}

// 将frame从LRU链表摘除
static void bp_lru_remove(BufferPool* bp, Frame* frame) {
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

// 从空闲链表获取一个页帧
static Frame* bp_free_list_pop(BufferPool* bp) {
    if (bp->free_list == NULL) return NULL;
    Frame* frame = bp->free_list;
    bp->free_list = frame->next;
    frame->prev = NULL;
    frame->next = NULL;
    return frame;
}

// 向空闲链表添加一个页帧
static void bp_free_list_push(BufferPool* bp, Frame* frame) {
    frame->prev = NULL;
    frame->next = bp->free_list;
    bp->free_list = frame;
}

// 选择一个牺牲页（LRU淘汰）
// 返回被淘汰的Frame，已写回脏页；返回NULL表示没有可淘汰的页
static Frame* bp_evict(BufferPool* bp) {
    Frame* victim = bp->lru_tail;
    while (victim != NULL) {
        if (victim->pin_count == 0) {
            // 找到可淘汰的页
            // 从LRU链表摘除
            bp_lru_remove(bp, victim);

            // 如果是脏页，写回磁盘
            if (victim->is_dirty) {
                db_write_page(bp->dm, victim->page_id, &victim->page);
                victim->is_dirty = 0;
            }

            // 从哈希表移除
            bp_hash_remove(bp, victim->page_id);

            return victim;
        }
        victim = victim->prev;
    }
    return NULL; // 所有页都被pin住，无法淘汰
}

// ===== 公开API =====

// 创建缓冲池
DBStatus bp_create(BufferPool** bp, DiskManager* dm, int pool_size) {
    if (bp == NULL || dm == NULL || pool_size <= 0) return DB_ERROR;

    *bp = (BufferPool*)calloc(1, sizeof(BufferPool));
    if (*bp == NULL) return DB_ERROR;

    (*bp)->dm = dm;
    (*bp)->pool_size = pool_size;
    (*bp)->used_count = 0;
    (*bp)->free_list = NULL;
    (*bp)->lru_head = NULL;
    (*bp)->lru_tail = NULL;

    // 分配页帧数组
    (*bp)->frames = (Frame*)calloc(pool_size, sizeof(Frame));
    if ((*bp)->frames == NULL) {
        free(*bp);
        *bp = NULL;
        return DB_ERROR;
    }

    // 初始化每个页帧并加入空闲链表
    for (int i = 0; i < pool_size; i++) {
        (*bp)->frames[i].page_id = (page_id_t)-1;
        (*bp)->frames[i].is_dirty = 0;
        (*bp)->frames[i].pin_count = 0;
        (*bp)->frames[i].prev = NULL;
        (*bp)->frames[i].next = NULL;
        bp_free_list_push(*bp, &(*bp)->frames[i]);
    }

    // 创建哈希表（大小为pool_size的2倍，减少冲突）
    (*bp)->hash_size = pool_size * 2;
    (*bp)->page_table = (Frame**)calloc((*bp)->hash_size, sizeof(Frame*));
    if ((*bp)->page_table == NULL) {
        free((*bp)->frames);
        free(*bp);
        *bp = NULL;
        return DB_ERROR;
    }

    return DB_OK;
}

// 获取指定页
DBStatus bp_get_page(BufferPool* bp, page_id_t page_id, Frame** out_frame) {
    if (bp == NULL || out_frame == NULL) return DB_ERROR;

    // 1. 先在哈希表中查找
    Frame* frame = bp_hash_lookup(bp, page_id);
    if (frame != NULL) {
        // 缓存命中
        frame->pin_count++;
        bp_lru_move_to_head(bp, frame);
        *out_frame = frame;
        return DB_OK;
    }

    // 2. 缓存未命中，需要从磁盘加载
    Frame* new_frame = NULL;

    // 2a. 优先从空闲链表获取
    if (bp->free_list != NULL) {
        new_frame = bp_free_list_pop(bp);
        bp->used_count++;
    } else {
        // 2b. 空闲链表为空，尝试淘汰
        new_frame = bp_evict(bp);
        if (new_frame == NULL) {
            return DB_ERROR; // 所有页都被pin住
        }
    }

    // 3. 从磁盘读取页数据
    DBStatus rc = db_read_page(bp->dm, page_id, &new_frame->page);
    if (rc != DB_OK) {
        // 读取失败，归还页帧
        bp_free_list_push(bp, new_frame);
        bp->used_count--;
        return rc;
    }

    // 4. 初始化页帧
    new_frame->page_id = page_id;
    new_frame->is_dirty = 0;
    new_frame->pin_count = 1;

    // 5. 插入哈希表
    bp_hash_insert(bp, new_frame);

    // 6. 加入LRU链表头部
    bp_lru_move_to_head(bp, new_frame);

    *out_frame = new_frame;
    return DB_OK;
}

// 标记脏页
DBStatus bp_mark_dirty(BufferPool* bp, page_id_t page_id) {
    if (bp == NULL) return DB_ERROR;
    Frame* frame = bp_hash_lookup(bp, page_id);
    if (frame == NULL) return DB_PAGE_NOT_FOUND;
    frame->is_dirty = 1;
    return DB_OK;
}

// 释放页引用
DBStatus bp_unpin_page(BufferPool* bp, page_id_t page_id) {
    if (bp == NULL) return DB_ERROR;
    Frame* frame = bp_hash_lookup(bp, page_id);
    if (frame == NULL) return DB_PAGE_NOT_FOUND;
    if (frame->pin_count > 0) {
        frame->pin_count--;
    }
    return DB_OK;
}

// 分配新页
DBStatus bp_allocate_page(BufferPool* bp, page_id_t* out_page_id, Frame** out_frame) {
    if (bp == NULL || out_page_id == NULL || out_frame == NULL) return DB_ERROR;

    // 1. 通过磁盘管理器分配
    page_id_t new_pid = db_allocate_page(bp->dm);
    if (new_pid == (page_id_t)-1) {
        return DB_OUT_OF_PAGES;
    }

    // 2. 获取一个页帧来存放新页
    Frame* new_frame = NULL;

    // 2a. 优先从空闲链表获取
    if (bp->free_list != NULL) {
        new_frame = bp_free_list_pop(bp);
        bp->used_count++;
    } else {
        // 2b. 尝试淘汰
        new_frame = bp_evict(bp);
        if (new_frame == NULL) {
            return DB_ERROR;
        }
    }

    // 3. 初始化新页帧（内容清零）
    memset(new_frame->page.data, 0, PAGE_SIZE);
    new_frame->page_id = new_pid;
    new_frame->is_dirty = 1; // 新分配的页标记为脏，需要写回
    new_frame->pin_count = 1;

    // 4. 插入哈希表
    bp_hash_insert(bp, new_frame);

    // 5. 加入LRU链表头部
    bp_lru_move_to_head(bp, new_frame);

    *out_page_id = new_pid;
    *out_frame = new_frame;
    return DB_OK;
}

// 刷新指定页到磁盘
DBStatus bp_flush_page(BufferPool* bp, page_id_t page_id) {
    if (bp == NULL) return DB_ERROR;
    Frame* frame = bp_hash_lookup(bp, page_id);
    if (frame == NULL) return DB_PAGE_NOT_FOUND;

    if (frame->is_dirty) {
        DBStatus rc = db_write_page(bp->dm, frame->page_id, &frame->page);
        if (rc != DB_OK) return rc;
        frame->is_dirty = 0;
    }
    return DB_OK;
}

// 刷新所有脏页
DBStatus bp_flush_all(BufferPool* bp) {
    if (bp == NULL) return DB_ERROR;

    for (int i = 0; i < bp->pool_size; i++) {
        Frame* f = &bp->frames[i];
        // 只处理有效页帧（page_id != -1 且在哈希表中）
        if (f->page_id != (page_id_t)-1 && f->is_dirty) {
            // 检查是否仍在哈希表中（未被淘汰）
            Frame* found = bp_hash_lookup(bp, f->page_id);
            if (found == f) {
                db_write_page(bp->dm, f->page_id, &f->page);
                f->is_dirty = 0;
            }
        }
    }
    return DB_OK;
}

// 销毁缓冲池
DBStatus bp_destroy(BufferPool* bp) {
    if (bp == NULL) return DB_OK;

    // 刷新所有脏页
    bp_flush_all(bp);

    // 释放资源
    free(bp->frames);
    free(bp->page_table);
    free(bp);
    return DB_OK;
}

// 打印缓冲池状态
void bp_print_status(BufferPool* bp) {
    if (bp == NULL) {
        printf("BufferPool: (null)\n");
        return;
    }
    printf("BufferPool: pool_size=%d, used=%d, free=%d\n",
           bp->pool_size, bp->used_count, bp->pool_size - bp->used_count);

    // 统计脏页
    int dirty_count = 0;
    int pinned_count = 0;
    for (int i = 0; i < bp->pool_size; i++) {
        if (bp->frames[i].page_id != (page_id_t)-1) {
            if (bp->frames[i].is_dirty) dirty_count++;
            if (bp->frames[i].pin_count > 0) pinned_count++;
        }
    }
    printf("  dirty frames: %d, pinned frames: %d\n", dirty_count, pinned_count);

    // 打印LRU链表（从head到tail）
    printf("  LRU list (head->tail): ");
    Frame* cur = bp->lru_head;
    int count = 0;
    while (cur != NULL && count < 20) {
        printf("[pid=%u%s%s] ", cur->page_id,
               cur->is_dirty ? ",D" : "",
               cur->pin_count > 0 ? ",P" : "");
        cur = cur->next;
        count++;
    }
    if (cur != NULL) printf("...");
    printf("\n");
}
