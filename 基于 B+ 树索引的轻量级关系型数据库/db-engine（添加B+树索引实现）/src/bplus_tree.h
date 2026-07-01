#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include "types.h"

// B+树阶数：每个节点最少 ORDER 个键，最多 2*ORDER 个键
#define BPT_ORDER 5
#define BPT_MAX_KEYS (2 * BPT_ORDER)
// 临时溢出容量：插入时节点可能短暂容纳 MAX+1 个键，分裂后再恢复
#define BPT_OVERFLOW (BPT_MAX_KEYS + 1)

// 键类型（当前只支持整数主键）
typedef int bpt_key_t;

// 前向声明
struct BPTNode;
typedef struct BPTNode BPTNode;

// B+树节点结构
struct BPTNode {
    int         is_leaf;       // 是否为叶子节点
    int         key_count;     // 当前键数量
    bpt_key_t   keys[BPT_OVERFLOW]; // 键数组（含1个溢出位）

    // 内部节点：子节点指针数组，容量 = BPT_OVERFLOW + 1
    BPTNode* children[BPT_OVERFLOW + 1];

    // 叶子节点：记录定位数组 + 双向链表
    RecordPointer  records[BPT_OVERFLOW]; // 每个键对应的记录定位（含1个溢出位）
    BPTNode* prev;  // 叶子链表前驱
    BPTNode* next;  // 叶子链表后继

    BPTNode* parent; // 父节点指针（分裂时用）
};

// B+树结构
typedef struct BPlusTree {
    BPTNode*  root;     // 根节点
    int       height;   // 树高度
    int       count;    // 总记录数（叶子节点键总数）
} BPlusTree;

// ===== API =====

// 创建一棵空B+树
BPlusTree* bpt_create(void);

// 销毁B+树，释放所有内存
void bpt_destroy(BPlusTree* tree);

// 插入键值对：key + 记录定位(rid)
DBStatus bpt_insert(BPlusTree* tree, bpt_key_t key, RecordPointer rid);

// 精确查找：按key查找，找到返回DB_OK并通过out_rid输出记录定位
DBStatus bpt_search(BPlusTree* tree, bpt_key_t key, RecordPointer* out_rid);

// 范围查询回调类型
typedef int (*BPTRangeCallback)(bpt_key_t key, RecordPointer rid, void* ctx);

// 范围查询：查找 low <= key <= high 的所有记录
DBStatus bpt_range_query(BPlusTree* tree, bpt_key_t low, bpt_key_t high,
                         BPTRangeCallback cb, void* ctx);

// 打印B+树结构（调试用）
void bpt_print(BPlusTree* tree);

// 验证B+树结构正确性（调试用）
DBStatus bpt_validate(BPlusTree* tree);

#endif
