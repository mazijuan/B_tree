#include "src/disk_manager.h"
#include "src/bplus_tree.h"
#include "src/buffer_pool.h"
#include <stdio.h>
#include <string.h>

void print_result(int key, record_id_t rid, void* ctx) {
    printf("  ├─ key=%d, record_id=%llu\n", key, (unsigned long long)rid);
}

int main() {
    printf("========================================\n");
    printf("  轻量级关系型数据库引擎 - 演示程序\n");
    printf("========================================\n\n");

    printf("【步骤1】打开数据库文件\n");
    DiskManager* dm = NULL;
    DBStatus status = dm_open(&dm, "demo.db");
    if (status != DB_OK) {
        printf("  ✗ 打开数据库失败!\n");
        return 1;
    }
    printf("  ✓ 数据库已打开: demo.db\n\n");

    printf("【步骤2】分配磁盘页面\n");
    page_id_t page1 = dm_allocate_page(dm);
    page_id_t page2 = dm_allocate_page(dm);
    printf("  ✓ 分配页面: page1=%u, page2=%u\n\n", page1, page2);

    printf("【步骤3】初始化缓冲池（4个帧）\n");
    BufferPool* bp = NULL;
    status = bp_init(&bp, dm, 4);
    if (status != DB_OK) {
        printf("  ✗ 初始化缓冲池失败!\n");
        dm_close(dm);
        return 1;
    }
    printf("  ✓ 缓冲池已初始化\n\n");

    printf("【步骤4】使用缓冲池写入数据\n");
    Page* p1 = bp_get_page(bp, page1);
    strcpy((char*)p1->data, "User: Alice, Age: 25, City: Beijing");
    bp_mark_dirty(bp, page1);
    bp_release_page(bp, page1);

    Page* p2 = bp_get_page(bp, page2);
    strcpy((char*)p2->data, "User: Bob, Age: 30, City: Shanghai");
    bp_mark_dirty(bp, page2);
    bp_release_page(bp, page2);
    printf("  ✓ 数据已写入缓冲池\n\n");

    printf("【步骤5】创建B+树索引\n");
    BPlusTree* tree = bpt_create();
    printf("  ✓ B+树已创建\n\n");

    printf("【步骤6】插入索引数据\n");
    bpt_insert(tree, 1001, page1);
    bpt_insert(tree, 1002, page2);
    bpt_insert(tree, 1003, page1);
    bpt_insert(tree, 1004, page2);
    bpt_insert(tree, 1005, page1);
    printf("  ✓ 已插入5条索引记录\n\n");

    printf("【步骤7】查询单条记录\n");
    record_id_t rid = bpt_search(tree, 1003);
    printf("  ├─ 查找 key=1003: record_id=%llu\n", (unsigned long long)rid);
    
    Page* found_page = bp_get_page(bp, (page_id_t)rid);
    printf("  └─ 页面内容: %s\n", found_page->data);
    bp_release_page(bp, (page_id_t)rid);
    printf("\n");

    printf("【步骤8】范围查询\n");
    printf("  查询 key 在 1002-1004 之间的记录:\n");
    bpt_range_query(tree, 1002, 1004, print_result, NULL);
    printf("\n");

    printf("【步骤9】删除记录\n");
    bpt_delete(tree, 1003);
    printf("  ✓ 已删除 key=1003\n");
    
    rid = bpt_search(tree, 1003);
    printf("  └─ 验证删除: key=1003, record_id=%llu (0表示不存在)\n\n", (unsigned long long)rid);

    printf("【步骤10】刷新缓冲池到磁盘\n");
    bp_flush_all(bp);
    printf("  ✓ 所有脏页已刷新到磁盘\n\n");

    printf("【步骤11】清理资源\n");
    bpt_destroy(tree);
    bp_destroy(bp);
    dm_close(dm);
    printf("  ✓ 所有资源已清理\n\n");

    printf("========================================\n");
    printf("  演示完成!\n");
    printf("========================================\n");

    return 0;
}
