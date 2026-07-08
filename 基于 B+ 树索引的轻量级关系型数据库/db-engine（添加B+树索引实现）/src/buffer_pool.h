#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include "types.h"
#include "disk_manager.h"

// 默认缓冲池容量（缓存的页帧数量）
#define DEFAULT_POOL_SIZE 1024

// 页帧结构：包装一个Page，附带元信息
typedef struct Frame {
    Page      page;         // 页数据
    page_id_t page_id;      // 对应的磁盘页ID
    int       is_dirty;     // 是否被修改过（脏页）
    int       pin_count;    // 引用计数（被使用时>0）
    struct Frame* prev;     // LRU双向链表前驱
    struct Frame* next;     // LRU双向链表后继
} Frame;

// 缓冲池结构
typedef struct BufferPool {
    DiskManager* dm;          // 关联的磁盘管理器
    int          pool_size;   // 缓冲池容量
    int          used_count;  // 当前已使用的页帧数
    Frame*       frames;      // 页帧数组
    Frame*       free_list;   // 空闲页帧链表头
    Frame*       lru_head;    // LRU链表头（最近使用）
    Frame*       lru_tail;    // LRU链表尾（最久未用）
    Frame**      page_table;  // 哈希表：page_id -> Frame*（开放寻址法）
    int          hash_size;   // 哈希表大小
} BufferPool;

// ===== API =====

// 创建缓冲池，关联磁盘管理器，指定缓存页数
DBStatus bp_create(BufferPool** bp, DiskManager* dm, int pool_size);

// 获取指定页（若不在缓存则从磁盘加载），返回Frame指针
// 调用者使用完毕后需调用bp_unpin_page释放
DBStatus bp_get_page(BufferPool* bp, page_id_t page_id, Frame** out_frame);

// 标记页为脏页（内容已修改，需写回磁盘）
DBStatus bp_mark_dirty(BufferPool* bp, page_id_t page_id);

// 释放页的引用（pin_count减1）
DBStatus bp_unpin_page(BufferPool* bp, page_id_t page_id);

// 分配一个新页（通过磁盘管理器），并加载到缓存
DBStatus bp_allocate_page(BufferPool* bp, page_id_t* out_page_id, Frame** out_frame);

// 将指定脏页刷新到磁盘
DBStatus bp_flush_page(BufferPool* bp, page_id_t page_id);

// 刷新所有脏页到磁盘
DBStatus bp_flush_all(BufferPool* bp);

// 销毁缓冲池（刷新所有脏页，释放内存）
DBStatus bp_destroy(BufferPool* bp);

// 打印缓冲池状态（调试用）
void bp_print_status(BufferPool* bp);

#endif
