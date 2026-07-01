#include "bplus_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===== 内部辅助函数 =====

// 创建新节点
static BPTNode* bpt_create_node(int is_leaf) {
    BPTNode* node = (BPTNode*)calloc(1, sizeof(BPTNode));
    if (node == NULL) return NULL;
    node->is_leaf = is_leaf;
    node->key_count = 0;
    node->parent = NULL;
    node->prev = NULL;
    node->next = NULL;
    for (int i = 0; i < BPT_OVERFLOW + 1; i++) {
        node->children[i] = NULL;
    }
    return node;
}

// 在节点中查找键的位置（返回第一个 >= key 的索引）
static int bpt_find_key_index(BPTNode* node, bpt_key_t key) {
    int i = 0;
    while (i < node->key_count && node->keys[i] < key) {
        i++;
    }
    return i;
}

// 在内部节点中查找应该进入的子节点索引
static int bpt_find_child_index(BPTNode* node, bpt_key_t key) {
    int i = 0;
    while (i < node->key_count && node->keys[i] <= key) {
        i++;
    }
    return i;
}

// 向叶子节点插入键和记录（假设节点未满或即将分裂后处理）
static void bpt_leaf_insert_at(BPTNode* leaf, int pos,
                                bpt_key_t key, RecordPointer rid) {
    // 向右腾出空间
    for (int i = leaf->key_count; i > pos; i--) {
        leaf->keys[i] = leaf->keys[i - 1];
        leaf->records[i] = leaf->records[i - 1];
    }
    leaf->keys[pos] = key;
    leaf->records[pos] = rid;
    leaf->key_count++;
}

// 向内部节点插入键和子节点（分裂时使用）
static void bpt_internal_insert_at(BPTNode* node, int pos,
                                    bpt_key_t key, BPTNode* child) {
    // 向右腾出空间
    for (int i = node->key_count; i > pos; i--) {
        node->keys[i] = node->keys[i - 1];
        node->children[i + 1] = node->children[i];
    }
    node->keys[pos] = key;
    node->children[pos + 1] = child;
    node->key_count++;
    if (child != NULL) {
        child->parent = node;
    }
}

// ===== 分裂逻辑 =====

// 分裂叶子节点：左半留在原节点，右半移到新节点
// 返回新节点，new_key 为新节点的最小键（要插入父节点）
static BPTNode* bpt_split_leaf(BPTNode* leaf, bpt_key_t* new_key) {
    BPTNode* new_leaf = bpt_create_node(1);
    if (new_leaf == NULL) return NULL;

    // 分裂点：左节点保留 BPT_ORDER 个键（满足最少键数要求）
    int split_pos = BPT_ORDER;

    // 复制后半部分到新节点
    for (int i = split_pos; i < leaf->key_count; i++) {
        new_leaf->keys[i - split_pos] = leaf->keys[i];
        new_leaf->records[i - split_pos] = leaf->records[i];
    }
    new_leaf->key_count = leaf->key_count - split_pos;

    // 更新原节点键数
    leaf->key_count = split_pos;

    // 新节点的最小键作为父节点中的分隔键
    *new_key = new_leaf->keys[0];

    // 维护叶子链表
    new_leaf->next = leaf->next;
    new_leaf->prev = leaf;
    if (leaf->next != NULL) {
        leaf->next->prev = new_leaf;
    }
    leaf->next = new_leaf;

    // 父节点指针暂不设置，由调用方处理
    new_leaf->parent = leaf->parent;

    return new_leaf;
}

// 分裂内部节点：左半留在原节点，中间键上提，右半移到新节点
// 返回新节点，up_key 为上提到父节点的键
static BPTNode* bpt_split_internal(BPTNode* node, bpt_key_t* up_key) {
    BPTNode* new_node = bpt_create_node(0);
    if (new_node == NULL) return NULL;

    // 分裂点：中间键上提，左节点保留 BPT_ORDER 个键
    int split_pos = BPT_ORDER;

    // 上提的键
    *up_key = node->keys[split_pos];

    // 复制后半部分到新节点（跳过上提的键）
    for (int i = split_pos + 1; i < node->key_count; i++) {
        new_node->keys[i - split_pos - 1] = node->keys[i];
        new_node->children[i - split_pos - 1] = node->children[i];
        if (new_node->children[i - split_pos - 1] != NULL) {
            new_node->children[i - split_pos - 1]->parent = new_node;
        }
    }
    // 最后一个子节点
    new_node->children[node->key_count - split_pos - 1] = node->children[node->key_count];
    if (new_node->children[node->key_count - split_pos - 1] != NULL) {
        new_node->children[node->key_count - split_pos - 1]->parent = new_node;
    }

    new_node->key_count = node->key_count - split_pos - 1;

    // 更新原节点键数
    node->key_count = split_pos;

    // 父节点指针
    new_node->parent = node->parent;

    return new_node;
}

// 向父节点插入分裂产生的新键和子节点，可能级联分裂
static DBStatus bpt_insert_into_parent(BPlusTree* tree, BPTNode* left,
                                        bpt_key_t key, BPTNode* right) {
    BPTNode* parent = left->parent;

    // 情况1：没有父节点，需要创建新根
    if (parent == NULL) {
        BPTNode* new_root = bpt_create_node(0);
        if (new_root == NULL) return DB_ERROR;

        new_root->keys[0] = key;
        new_root->children[0] = left;
        new_root->children[1] = right;
        new_root->key_count = 1;

        left->parent = new_root;
        right->parent = new_root;

        tree->root = new_root;
        tree->height++;
        return DB_OK;
    }

    // 情况2：父节点有空位，直接插入
    int pos = bpt_find_key_index(parent, key);
    bpt_internal_insert_at(parent, pos, key, right);

    // 父节点溢出，需要分裂
    if (parent->key_count > BPT_MAX_KEYS) {
        bpt_key_t up_key;
        BPTNode* new_internal = bpt_split_internal(parent, &up_key);
        if (new_internal == NULL) return DB_ERROR;

        // 递归向上插入
        return bpt_insert_into_parent(tree, parent, up_key, new_internal);
    }

    return DB_OK;
}

// ===== 公开API =====

// 创建一棵空B+树
BPlusTree* bpt_create(void) {
    BPlusTree* tree = (BPlusTree*)calloc(1, sizeof(BPlusTree));
    if (tree == NULL) return NULL;

    // 初始根节点为空叶子
    tree->root = bpt_create_node(1);
    if (tree->root == NULL) {
        free(tree);
        return NULL;
    }
    tree->height = 1;
    tree->count = 0;
    return tree;
}

// 递归销毁节点
static void bpt_destroy_node(BPTNode* node) {
    if (node == NULL) return;
    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            bpt_destroy_node(node->children[i]);
        }
    }
    free(node);
}

// 销毁B+树
void bpt_destroy(BPlusTree* tree) {
    if (tree == NULL) return;
    bpt_destroy_node(tree->root);
    free(tree);
}

// 插入键值对
DBStatus bpt_insert(BPlusTree* tree, bpt_key_t key, RecordPointer rid) {
    if (tree == NULL) return DB_ERROR;

    // 1. 找到目标叶子节点
    BPTNode* cur = tree->root;
    while (!cur->is_leaf) {
        int idx = bpt_find_child_index(cur, key);
        cur = cur->children[idx];
    }

    // 2. 在叶子节点中查找插入位置（允许重复键）
    int pos = bpt_find_key_index(cur, key);

    // 3. 插入到叶子节点
    bpt_leaf_insert_at(cur, pos, key, rid);
    tree->count++;

    // 4. 检查是否溢出，需要分裂
    if (cur->key_count > BPT_MAX_KEYS) {
        bpt_key_t new_key;
        BPTNode* new_leaf = bpt_split_leaf(cur, &new_key);
        if (new_leaf == NULL) return DB_ERROR;

        // 将新节点插入父节点
        DBStatus rc = bpt_insert_into_parent(tree, cur, new_key, new_leaf);
        if (rc != DB_OK) return rc;
    }

    return DB_OK;
}

// 精确查找
DBStatus bpt_search(BPlusTree* tree, bpt_key_t key, RecordPointer* out_rid) {
    if (tree == NULL || out_rid == NULL) return DB_ERROR;

    BPTNode* cur = tree->root;
    while (!cur->is_leaf) {
        int idx = bpt_find_child_index(cur, key);
        cur = cur->children[idx];
    }

    // 在叶子节点中查找
    int pos = bpt_find_key_index(cur, key);
    if (pos < cur->key_count && cur->keys[pos] == key) {
        *out_rid = cur->records[pos];
        return DB_OK;
    }

    // 如果当前叶子没找到且链表有下一个，检查下一个叶子
    // （处理边界情况：key等于下一叶子的第一个键）
    if (cur->next != NULL && cur->next->keys[0] == key) {
        *out_rid = cur->next->records[0];
        return DB_OK;
    }

    return DB_PAGE_NOT_FOUND; // 未找到
}

// 范围查询
DBStatus bpt_range_query(BPlusTree* tree, bpt_key_t low, bpt_key_t high,
                          BPTRangeCallback cb, void* ctx) {
    if (tree == NULL || cb == NULL) return DB_ERROR;

    // 找到 low 所在的叶子节点
    BPTNode* cur = tree->root;
    while (!cur->is_leaf) {
        int idx = bpt_find_child_index(cur, low);
        cur = cur->children[idx];
    }

    // 从该叶子开始遍历
    int found = 0;
    while (cur != NULL) {
        for (int i = 0; i < cur->key_count; i++) {
            if (cur->keys[i] > high) {
                return DB_OK; // 超出范围，停止
            }
            if (cur->keys[i] >= low) {
                int rc = cb(cur->keys[i], cur->records[i], ctx);
                if (rc != 0) return DB_OK; // 回调要求停止
                found++;
            }
        }
        cur = cur->next;
    }

    if (found == 0) return DB_PAGE_NOT_FOUND;
    return DB_OK;
}

// ===== 调试辅助 =====

// 递归打印节点
static void bpt_print_node(BPTNode* node, int level) {
    if (node == NULL) return;

    // 缩进
    for (int i = 0; i < level; i++) printf("  ");

    if (node->is_leaf) {
        printf("Leaf[%d]: ", node->key_count);
        for (int i = 0; i < node->key_count; i++) {
            printf("%d(page=%u,off=%u) ", node->keys[i],
                   node->records[i].page_id, node->records[i].offset);
        }
        printf("\n");
    } else {
        printf("Internal[%d]: ", node->key_count);
        for (int i = 0; i < node->key_count; i++) {
            printf("%d ", node->keys[i]);
        }
        printf("\n");
        for (int i = 0; i <= node->key_count; i++) {
            bpt_print_node(node->children[i], level + 1);
        }
    }
}

// 打印B+树
void bpt_print(BPlusTree* tree) {
    if (tree == NULL) {
        printf("(empty tree)\n");
        return;
    }
    printf("B+Tree (height=%d, count=%d):\n", tree->height, tree->count);
    bpt_print_node(tree->root, 0);
}

// 验证节点键数合法性
static DBStatus bpt_validate_node(BPTNode* node, int is_root, bpt_key_t* min_out, bpt_key_t* max_out) {
    if (node == NULL) return DB_OK;

    // 检查键数范围
    if (!is_root) {
        if (node->key_count < BPT_ORDER || node->key_count > BPT_MAX_KEYS) {
            printf("VALIDATE FAIL: node key_count=%d (min=%d, max=%d)\n",
                   node->key_count, BPT_ORDER, BPT_MAX_KEYS);
            return DB_ERROR;
        }
    } else {
        // 根节点至少1个键（除非空树）
        if (node->key_count < 1 || node->key_count > BPT_MAX_KEYS) {
            printf("VALIDATE FAIL: root key_count=%d\n", node->key_count);
            return DB_ERROR;
        }
    }

    // 检查键有序
    for (int i = 1; i < node->key_count; i++) {
        if (node->keys[i] < node->keys[i - 1]) {
            printf("VALIDATE FAIL: keys not sorted at index %d: %d < %d\n",
                   i, node->keys[i], node->keys[i - 1]);
            return DB_ERROR;
        }
    }

    *min_out = node->keys[0];
    *max_out = node->keys[node->key_count - 1];

    if (!node->is_leaf) {
        // 检查子节点数量 = key_count + 1
        // 检查子节点键边界
        for (int i = 0; i <= node->key_count; i++) {
            bpt_key_t cmin, cmax;
            DBStatus rc = bpt_validate_node(node->children[i], 0, &cmin, &cmax);
            if (rc != DB_OK) return rc;

            // 子节点parent指针
            if (node->children[i]->parent != node) {
                printf("VALIDATE FAIL: child[%d] parent mismatch\n", i);
                return DB_ERROR;
            }

            // 键边界约束
            if (i > 0 && cmin < node->keys[i - 1]) {
                printf("VALIDATE FAIL: child[%d] min=%d < parent key[%d]=%d\n",
                       i, cmin, i - 1, node->keys[i - 1]);
                return DB_ERROR;
            }
        }
    } else {
        // 检查叶子链表
        if (node->prev != NULL && node->prev->next != node) {
            printf("VALIDATE FAIL: leaf prev-next mismatch\n");
            return DB_ERROR;
        }
        if (node->next != NULL && node->next->prev != node) {
            printf("VALIDATE FAIL: leaf next-prev mismatch\n");
            return DB_ERROR;
        }
    }

    return DB_OK;
}

// 验证B+树结构
DBStatus bpt_validate(BPlusTree* tree) {
    if (tree == NULL) return DB_ERROR;
    bpt_key_t min, max;
    return bpt_validate_node(tree->root, 1, &min, &max);
}
