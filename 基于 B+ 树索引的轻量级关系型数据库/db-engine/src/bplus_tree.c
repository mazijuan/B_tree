#include "bplus_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BPlusTreeNode* create_node(int is_leaf) {
    // calloc自动清零内存，省去memset，避免脏数据
    BPlusTreeNode* node = (BPlusTreeNode*)calloc(1, sizeof(BPlusTreeNode));
    if (node == NULL) {
        return NULL;
    }
    node->is_leaf = is_leaf;
    node->key_count = 0;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

static void destroy_node(BPlusTreeNode* node) {
    if (node == NULL) return;
    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            destroy_node(node->children[i]);
        }
    }
    free(node);
}

BPlusTree* bpt_create() {
    BPlusTree* tree = (BPlusTree*)malloc(sizeof(BPlusTree));
    if (tree == NULL) return NULL;
    tree->root = create_node(1);
    if (tree->root == NULL) {
        free(tree);
        return NULL;
    }
    tree->height = 0;
    tree->size = 0;
    return tree;
}

void bpt_destroy(BPlusTree* tree) {
    if (tree == NULL) return;
    destroy_node(tree->root);
    free(tree);
}

int bpt_size(BPlusTree* tree) {
    if (tree == NULL) return 0;
    return tree->size;
}

// 二分查找：获取第一个 >= key 的下标，替代原线性遍历
static int binary_search_pos(BPlusTreeNode* node, int key) {
    int l = 0, r = node->key_count;
    while (l < r) {
        int mid = (l + r) / 2;
        if (node->keys[mid] >= key)
            r = mid;
        else
            l = mid + 1;
    }
    return l;
}

// 二分查找key，存在返回下标，不存在返回-1
static int find_index_in_node(BPlusTreeNode* node, int key) {
    int pos = binary_search_pos(node, key);
    if (pos < node->key_count && node->keys[pos] == key)
        return pos;
    return -1;
}

static BPlusTreeNode* find_leaf(BPlusTreeNode* root, int key) {
    BPlusTreeNode* node = root;
    while (!node->is_leaf) {
        int i = binary_search_pos(node, key);
        node = node->children[i];
    }
    return node;
}

record_id_t bpt_search(BPlusTree* tree, int key) {
    if (tree == NULL || tree->root == NULL) return (record_id_t)-1;

    BPlusTreeNode* leaf = find_leaf(tree->root, key);
    int idx = find_index_in_node(leaf, key);
    return idx == -1 ? (record_id_t)-1 : leaf->records[idx];
}

static void insert_into_leaf(BPlusTreeNode* leaf, int key, record_id_t rid) {
    int pos = binary_search_pos(leaf, key);
    // memmove处理内存重叠，安全后移数据
    memmove(&leaf->keys[pos + 1], &leaf->keys[pos], sizeof(int) * (leaf->key_count - pos));
    memmove(&leaf->records[pos + 1], &leaf->records[pos], sizeof(record_id_t) * (leaf->key_count - pos));
    leaf->keys[pos] = key;
    leaf->records[pos] = rid;
    leaf->key_count++;
}

static BPlusTreeNode* split_leaf(BPlusTreeNode* leaf) {
    BPlusTreeNode* new_leaf = create_node(1);
    if (new_leaf == NULL) return NULL;

    int mid = (BPT_MAX_KEYS + 1) / 2;
    int copy_cnt = leaf->key_count - mid;
    memcpy(new_leaf->keys, leaf->keys + mid, sizeof(int) * copy_cnt);
    memcpy(new_leaf->records, leaf->records + mid, sizeof(record_id_t) * copy_cnt);

    leaf->key_count = mid;
    new_leaf->key_count = copy_cnt;

    // 维护叶子双向链表
    new_leaf->next = leaf->next;
    new_leaf->prev = leaf;
    if (leaf->next != NULL) {
        leaf->next->prev = new_leaf;
    }
    leaf->next = new_leaf;
    return new_leaf;
}

static void insert_into_internal(BPlusTreeNode* node, int pos, int key, BPlusTreeNode* child) {
    int move_len = node->key_count - pos;
    memmove(&node->keys[pos + 1], &node->keys[pos], sizeof(int) * move_len);
    memmove(&node->children[pos + 2], &node->children[pos + 1], sizeof(BPlusTreeNode*) * move_len);

    node->keys[pos] = key;
    node->children[pos + 1] = child;
    node->key_count++;
}

static BPlusTreeNode* split_internal(BPlusTreeNode* node, int* mid_key) {
    BPlusTreeNode* new_node = create_node(0);
    if (new_node == NULL) return NULL;

    int mid = node->key_count / 2;
    *mid_key = node->keys[mid];
    int copy_cnt = node->key_count - (mid + 1);

    memcpy(new_node->keys, node->keys + mid + 1, sizeof(int) * copy_cnt);
    memcpy(new_node->children, node->children + mid + 1, sizeof(BPlusTreeNode*) * (copy_cnt + 1));

    node->key_count = mid;
    new_node->key_count = copy_cnt;
    return new_node;
}

static DBStatus insert_recursive(BPlusTreeNode* node, int key, record_id_t rid,
                                 int* new_key, BPlusTreeNode** new_child, int* inserted) {
    *inserted = 0;
    *new_child = NULL;

    if (node->is_leaf) {
        int idx = find_index_in_node(node, key);
        // 重复key直接覆盖，不新增计数
        if (idx != -1) {
            node->records[idx] = rid;
            return DB_OK;
        }
        insert_into_leaf(node, key, rid);
        *inserted = 1;

        if (node->key_count > BPT_MAX_KEYS) {
            BPlusTreeNode* new_leaf = split_leaf(node);
            if (new_leaf == NULL) return DB_ERROR;
            *new_key = new_leaf->keys[0];
            *new_child = new_leaf;
        }
        return DB_OK;
    }

    // 内部节点递归向下
    int child_pos = binary_search_pos(node, key);
    int child_new_key;
    BPlusTreeNode* child_new_node = NULL;
    int child_inserted = 0;

    DBStatus status = insert_recursive(node->children[child_pos], key, rid,
                                       &child_new_key, &child_new_node, &child_inserted);
    if (status != DB_OK) return status;
    *inserted = child_inserted;

    // 子节点分裂，当前节点插入提升key
    if (child_new_node != NULL) {
        insert_into_internal(node, child_pos, child_new_key, child_new_node);
        if (node->key_count > BPT_MAX_KEYS) {
            int mid_key;
            BPlusTreeNode* new_node = split_internal(node, &mid_key);
            if (new_node == NULL) return DB_ERROR;
            *new_key = mid_key;
            *new_child = new_node;
        }
    }
    return DB_OK;
}

DBStatus bpt_insert(BPlusTree* tree, int key, record_id_t rid) {
    if (tree == NULL || tree->root == NULL) return DB_ERROR;

    int new_key = 0;
    BPlusTreeNode* new_child = NULL;
    int inserted = 0;

    DBStatus status = insert_recursive(tree->root, key, rid, &new_key, &new_child, &inserted);
    if (status != DB_OK) return status;

    // 根节点分裂，新建根
    if (new_child != NULL) {
        BPlusTreeNode* new_root = create_node(0);
        if (new_root == NULL) return DB_ERROR;
        new_root->keys[0] = new_key;
        new_root->children[0] = tree->root;
        new_root->children[1] = new_child;
        new_root->key_count = 1;
        tree->root = new_root;
        tree->height++;
    }

    if (inserted) tree->size++;
    return DB_OK;
}

// 删除辅助：获取左右兄弟
static BPlusTreeNode* get_left_sibling(BPlusTreeNode* parent, int index) {
    return (index > 0) ? parent->children[index - 1] : NULL;
}

static BPlusTreeNode* get_right_sibling(BPlusTreeNode* parent, int index) {
    return (index < parent->key_count) ? parent->children[index + 1] : NULL;
}

// 叶子向左借
static void borrow_from_left_leaf(BPlusTreeNode* node, BPlusTreeNode* left,
                                  BPlusTreeNode* parent, int index) {
    int take_key = left->keys[left->key_count - 1];
    record_id_t take_rid = left->records[left->key_count - 1];

    memmove(&node->keys[1], &node->keys[0], sizeof(int) * node->key_count);
    memmove(&node->records[1], &node->records[0], sizeof(record_id_t) * node->key_count);
    node->keys[0] = take_key;
    node->records[0] = take_rid;
    node->key_count++;
    left->key_count--;
    parent->keys[index - 1] = node->keys[0];
}

// 叶子向右借
static void borrow_from_right_leaf(BPlusTreeNode* node, BPlusTreeNode* right,
                                   BPlusTreeNode* parent, int index) {
    node->keys[node->key_count] = right->keys[0];
    node->records[node->key_count] = right->records[0];
    node->key_count++;

    memmove(&right->keys[0], &right->keys[1], sizeof(int) * (right->key_count - 1));
    memmove(&right->records[0], &right->records[1], sizeof(record_id_t) * (right->key_count - 1));
    right->key_count--;
    parent->keys[index] = right->keys[0];
}

// 合并叶子，释放右节点
static void merge_leaf(BPlusTreeNode* left, BPlusTreeNode* right) {
    int copy_cnt = right->key_count;
    memcpy(left->keys + left->key_count, right->keys, sizeof(int) * copy_cnt);
    memcpy(left->records + left->key_count, right->records, sizeof(record_id_t) * copy_cnt);
    left->key_count += copy_cnt;

    left->next = right->next;
    if (right->next != NULL) {
        right->next->prev = left;
    }
    free(right);
}

// 内部节点左借
static void borrow_from_left_internal(BPlusTreeNode* node, BPlusTreeNode* left,
                                      BPlusTreeNode* parent, int index) {
    memmove(&node->keys[1], &node->keys[0], sizeof(int) * node->key_count);
    memmove(&node->children[1], &node->children[0], sizeof(BPlusTreeNode*) * (node->key_count + 1));

    node->keys[0] = parent->keys[index - 1];
    node->children[0] = left->children[left->key_count];
    parent->keys[index - 1] = left->keys[left->key_count - 1];

    node->key_count++;
    left->key_count--;
}

// 内部节点右借
static void borrow_from_right_internal(BPlusTreeNode* node, BPlusTreeNode* right,
                                       BPlusTreeNode* parent, int index) {
    node->keys[node->key_count] = parent->keys[index];
    node->children[node->key_count + 1] = right->children[0];
    parent->keys[index] = right->keys[0];

    memmove(&right->keys[0], &right->keys[1], sizeof(int) * (right->key_count - 1));
    memmove(&right->children[0], &right->children[1], sizeof(BPlusTreeNode*) * right->key_count);
    right->key_count--;
    node->key_count++;
}

// 合并内部节点
static void merge_internal(BPlusTreeNode* left, BPlusTreeNode* right, int mid_key) {
    left->keys[left->key_count] = mid_key;
    int copy_cnt = right->key_count;
    memcpy(left->keys + left->key_count + 1, right->keys, sizeof(int) * copy_cnt);
    memcpy(left->children + left->key_count + 1, right->children, sizeof(BPlusTreeNode*) * (copy_cnt + 1));
    left->key_count += copy_cnt + 1;
    free(right);
}

static DBStatus delete_recursive(BPlusTreeNode* node, int key,
                                 BPlusTreeNode* parent, int parent_index,
                                 int* deleted) {
    *deleted = 0;
    if (node->is_leaf) {
        int idx = find_index_in_node(node, key);
        if (idx == -1) return DB_OK;

        // 删除元素，前移覆盖
        memmove(&node->keys[idx], &node->keys[idx + 1], sizeof(int) * (node->key_count - idx - 1));
        memmove(&node->records[idx], &node->records[idx + 1], sizeof(record_id_t) * (node->key_count - idx - 1));
        node->key_count--;
        *deleted = 1;

        if (parent == NULL) return DB_OK;
        // 节点不足最小key，平衡
        if (node->key_count < BPT_MIN_KEYS) {
            BPlusTreeNode* left_sib = get_left_sibling(parent, parent_index);
            BPlusTreeNode* right_sib = get_right_sibling(parent, parent_index);

            if (left_sib != NULL && left_sib->key_count > BPT_MIN_KEYS) {
                borrow_from_left_leaf(node, left_sib, parent, parent_index);
            } else if (right_sib != NULL && right_sib->key_count > BPT_MIN_KEYS) {
                borrow_from_right_leaf(node, right_sib, parent, parent_index);
            } else if (left_sib != NULL) {
                merge_leaf(left_sib, node);
                // 删除父节点分隔key
                memmove(&parent->keys[parent_index - 1], &parent->keys[parent_index], sizeof(int) * (parent->key_count - parent_index));
                memmove(&parent->children[parent_index], &parent->children[parent_index + 1], sizeof(BPlusTreeNode*) * (parent->key_count - parent_index));
                parent->key_count--;
            } else if (right_sib != NULL) {
                merge_leaf(node, right_sib);
                memmove(&parent->keys[parent_index], &parent->keys[parent_index + 1], sizeof(int) * (parent->key_count - parent_index - 1));
                memmove(&parent->children[parent_index + 1], &parent->children[parent_index + 2], sizeof(BPlusTreeNode*) * (parent->key_count - parent_index - 1));
                parent->key_count--;
            }
        }
        return DB_OK;
    }

    // 内部节点递归删除
    int child_pos = binary_search_pos(node, key);
    int child_deleted = 0;
    DBStatus status = delete_recursive(node->children[child_pos], key, node, child_pos, &child_deleted);
    if (status != DB_OK) return status;
    *deleted = child_deleted;

    if (parent == NULL) return DB_OK;
    if (node->key_count < BPT_MIN_KEYS) {
        BPlusTreeNode* left_sib = get_left_sibling(parent, parent_index);
        BPlusTreeNode* right_sib = get_right_sibling(parent, parent_index);

        if (left_sib != NULL && left_sib->key_count > BPT_MIN_KEYS) {
            borrow_from_left_internal(node, left_sib, parent, parent_index);
        } else if (right_sib != NULL && right_sib->key_count > BPT_MIN_KEYS) {
            borrow_from_right_internal(node, right_sib, parent, parent_index);
        } else if (left_sib != NULL) {
            int mid_key = parent->keys[parent_index - 1];
            merge_internal(left_sib, node, mid_key);
            memmove(&parent->keys[parent_index - 1], &parent->keys[parent_index], sizeof(int) * (parent->key_count - parent_index));
            memmove(&parent->children[parent_index], &parent->children[parent_index + 1], sizeof(BPlusTreeNode*) * (parent->key_count - parent_index));
            parent->key_count--;
        } else if (right_sib != NULL) {
            int mid_key = parent->keys[parent_index];
            merge_internal(node, right_sib, mid_key);
            memmove(&parent->keys[parent_index], &parent->keys[parent_index + 1], sizeof(int) * (parent->key_count - parent_index - 1));
            memmove(&parent->children[parent_index + 1], &parent->children[parent_index + 2], sizeof(BPlusTreeNode*) * (parent->key_count - parent_index - 1));
            parent->key_count--;
        }
    }
    return DB_OK;
}

DBStatus bpt_delete(BPlusTree* tree, int key) {
    if (tree == NULL || tree->root == NULL) return DB_ERROR;
    if (tree->size == 0) return DB_OK;

    int deleted = 0;
    DBStatus status = delete_recursive(tree->root, key, NULL, 0, &deleted);
    if (status != DB_OK) return status;

    // 根节点降级
    if (tree->root->key_count == 0 && !tree->root->is_leaf) {
        BPlusTreeNode* old_root = tree->root;
        tree->root = tree->root->children[0];
        tree->height--;
        free(old_root);
    }

    if (deleted) tree->size--;
    return DB_OK;
}

void bpt_range_query(BPlusTree* tree, int low, int high, BPTCallback cb, void* ctx) {
    if (tree == NULL || tree->root == NULL || cb == NULL) return;

    BPlusTreeNode* leaf = find_leaf(tree->root, low);
    while (leaf != NULL) {
        int start_idx = binary_search_pos(leaf, low);
        for (int i = start_idx; i < leaf->key_count; i++) {
            int cur_key = leaf->keys[i];
            if (cur_key > high)
                return;
            cb(cur_key, leaf->records[i], ctx);
        }
        leaf = leaf->next;
    }
}

static void print_node(BPlusTreeNode* node, int level) {
    if (node == NULL) return;
    for (int i = 0; i < level; i++)
        printf("  ");
    printf("[");
    for (int i = 0; i < node->key_count; i++) {
        printf("%d", node->keys[i]);
        if (i < node->key_count - 1)
            printf(", ");
    }
    printf("] (%s, cnt=%d)\n", node->is_leaf ? "leaf" : "internal", node->key_count);
    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            print_node(node->children[i], level + 1);
        }
    }
}

void bpt_print(BPlusTree* tree) {
    if (tree == NULL || tree->root == NULL) {
        printf("Empty B+ Tree\n");
        return;
    }
    printf("B+ Tree height=%d, total size=%d\n", tree->height, tree->size);
    print_node(tree->root, 0);
}
