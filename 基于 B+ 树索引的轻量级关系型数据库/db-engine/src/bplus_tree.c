#include "bplus_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BPT_MAX_KEYS_ARRAY (BPT_MAX_KEYS + 1)

static BPlusTreeNode* create_node(int is_leaf) {
    BPlusTreeNode* node = (BPlusTreeNode*)malloc(sizeof(BPlusTreeNode));
    if (node == NULL) return NULL;
    node->is_leaf = is_leaf;
    node->key_count = 0;
    node->next = NULL;
    node->prev = NULL;
    memset(node->keys, 0, sizeof(node->keys));
    memset(node->children, 0, sizeof(node->children));
    memset(node->records, 0, sizeof(node->records));
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

static BPlusTreeNode* find_leaf(BPlusTreeNode* root, int key) {
    BPlusTreeNode* node = root;
    while (!node->is_leaf) {
        int i = 0;
        while (i < node->key_count && key >= node->keys[i]) {
            i++;
        }
        node = node->children[i];
    }
    return node;
}

record_id_t bpt_search(BPlusTree* tree, int key) {
    if (tree == NULL || tree->root == NULL) return (record_id_t)-1;

    BPlusTreeNode* leaf = find_leaf(tree->root, key);
    for (int i = 0; i < leaf->key_count; i++) {
        if (leaf->keys[i] == key) {
            return leaf->records[i];
        }
    }
    return (record_id_t)-1;
}

static void insert_into_leaf(BPlusTreeNode* leaf, int key, record_id_t rid) {
    int i = leaf->key_count - 1;
    while (i >= 0 && leaf->keys[i] > key) {
        leaf->keys[i + 1] = leaf->keys[i];
        leaf->records[i + 1] = leaf->records[i];
        i--;
    }
    leaf->keys[i + 1] = key;
    leaf->records[i + 1] = rid;
    leaf->key_count++;
}

static BPlusTreeNode* split_leaf(BPlusTreeNode* leaf) {
    BPlusTreeNode* new_leaf = create_node(1);
    if (new_leaf == NULL) return NULL;

    int mid = (BPT_MAX_KEYS + 1) / 2;
    int j = 0;
    for (int i = mid; i < leaf->key_count; i++) {
        new_leaf->keys[j] = leaf->keys[i];
        new_leaf->records[j] = leaf->records[i];
        j++;
    }
    leaf->key_count = mid;
    new_leaf->key_count = j;

    new_leaf->next = leaf->next;
    new_leaf->prev = leaf;
    if (leaf->next != NULL) {
        leaf->next->prev = new_leaf;
    }
    leaf->next = new_leaf;

    return new_leaf;
}

static void insert_into_internal(BPlusTreeNode* node, int pos, int key, BPlusTreeNode* child) {
    for (int i = node->key_count; i > pos; i--) {
        node->keys[i] = node->keys[i - 1];
        node->children[i + 1] = node->children[i];
    }
    node->keys[pos] = key;
    node->children[pos + 1] = child;
    node->key_count++;
}

static BPlusTreeNode* split_internal(BPlusTreeNode* node, int* mid_key) {
    BPlusTreeNode* new_node = create_node(0);
    if (new_node == NULL) return NULL;

    int mid = node->key_count / 2;
    *mid_key = node->keys[mid];

    int j = 0;
    for (int i = mid + 1; i < node->key_count; i++) {
        new_node->keys[j] = node->keys[i];
        new_node->children[j] = node->children[i];
        j++;
    }
    new_node->children[j] = node->children[node->key_count];
    new_node->key_count = j;
    node->key_count = mid;

    return new_node;
}

static DBStatus insert_recursive(BPlusTreeNode* node, int key, record_id_t rid,
                                 int* new_key, BPlusTreeNode** new_child, int* inserted) {
    *inserted = 0;
    if (node->is_leaf) {
        for (int i = 0; i < node->key_count; i++) {
            if (node->keys[i] == key) {
                node->records[i] = rid;
                *new_child = NULL;
                return DB_OK;
            }
        }

        insert_into_leaf(node, key, rid);
        *inserted = 1;

        if (node->key_count > BPT_MAX_KEYS) {
            BPlusTreeNode* new_leaf = split_leaf(node);
            if (new_leaf == NULL) return DB_ERROR;
            *new_key = new_leaf->keys[0];
            *new_child = new_leaf;
            return DB_OK;
        }
        *new_child = NULL;
        return DB_OK;
    }

    int i = 0;
    while (i < node->key_count && key >= node->keys[i]) {
        i++;
    }

    int child_new_key;
    BPlusTreeNode* child_new_node = NULL;
    int child_inserted = 0;
    DBStatus status = insert_recursive(node->children[i], key, rid,
                                       &child_new_key, &child_new_node, &child_inserted);
    if (status != DB_OK) return status;
    *inserted = child_inserted;

    if (child_new_node != NULL) {
        insert_into_internal(node, i, child_new_key, child_new_node);

        if (node->key_count > BPT_MAX_KEYS) {
            int mid_key;
            BPlusTreeNode* new_node = split_internal(node, &mid_key);
            if (new_node == NULL) return DB_ERROR;
            *new_key = mid_key;
            *new_child = new_node;
            return DB_OK;
        }
    }

    *new_child = NULL;
    return DB_OK;
}

DBStatus bpt_insert(BPlusTree* tree, int key, record_id_t rid) {
    if (tree == NULL || tree->root == NULL) return DB_ERROR;

    int new_key;
    BPlusTreeNode* new_child = NULL;
    int inserted = 0;
    DBStatus status = insert_recursive(tree->root, key, rid, &new_key, &new_child, &inserted);
    if (status != DB_OK) return status;

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

    if (inserted) {
        tree->size++;
    }
    return DB_OK;
}

static int find_index_in_node(BPlusTreeNode* node, int key) {
    for (int i = 0; i < node->key_count; i++) {
        if (node->keys[i] == key) return i;
    }
    return -1;
}

static BPlusTreeNode* get_left_sibling(BPlusTreeNode* parent, int index) {
    if (index <= 0) return NULL;
    return parent->children[index - 1];
}

static BPlusTreeNode* get_right_sibling(BPlusTreeNode* parent, int index) {
    if (index >= parent->key_count) return NULL;
    return parent->children[index + 1];
}

static void borrow_from_left_leaf(BPlusTreeNode* node, BPlusTreeNode* left,
                                  BPlusTreeNode* parent, int index) {
    for (int i = node->key_count; i > 0; i--) {
        node->keys[i] = node->keys[i - 1];
        node->records[i] = node->records[i - 1];
    }
    node->keys[0] = left->keys[left->key_count - 1];
    node->records[0] = left->records[left->key_count - 1];
    node->key_count++;
    left->key_count--;
    parent->keys[index - 1] = node->keys[0];
}

static void borrow_from_right_leaf(BPlusTreeNode* node, BPlusTreeNode* right,
                                   BPlusTreeNode* parent, int index) {
    node->keys[node->key_count] = right->keys[0];
    node->records[node->key_count] = right->records[0];
    node->key_count++;
    for (int i = 0; i < right->key_count - 1; i++) {
        right->keys[i] = right->keys[i + 1];
        right->records[i] = right->records[i + 1];
    }
    right->key_count--;
    parent->keys[index] = right->keys[0];
}

static void merge_leaf(BPlusTreeNode* left, BPlusTreeNode* right) {
    int j = left->key_count;
    for (int i = 0; i < right->key_count; i++) {
        left->keys[j + i] = right->keys[i];
        left->records[j + i] = right->records[i];
    }
    left->key_count += right->key_count;
    left->next = right->next;
    if (right->next != NULL) {
        right->next->prev = left;
    }
    free(right);
}

static void borrow_from_left_internal(BPlusTreeNode* node, BPlusTreeNode* left,
                                      BPlusTreeNode* parent, int index) {
    for (int i = node->key_count; i > 0; i--) {
        node->keys[i] = node->keys[i - 1];
        node->children[i + 1] = node->children[i];
    }
    node->children[1] = node->children[0];

    node->keys[0] = parent->keys[index - 1];
    node->children[0] = left->children[left->key_count];
    parent->keys[index - 1] = left->keys[left->key_count - 1];

    node->key_count++;
    left->key_count--;
}

static void borrow_from_right_internal(BPlusTreeNode* node, BPlusTreeNode* right,
                                       BPlusTreeNode* parent, int index) {
    node->keys[node->key_count] = parent->keys[index];
    node->children[node->key_count + 1] = right->children[0];
    parent->keys[index] = right->keys[0];

    for (int i = 0; i < right->key_count - 1; i++) {
        right->keys[i] = right->keys[i + 1];
        right->children[i] = right->children[i + 1];
    }
    right->children[right->key_count - 1] = right->children[right->key_count];

    node->key_count++;
    right->key_count--;
}

static void merge_internal(BPlusTreeNode* left, BPlusTreeNode* right, int mid_key) {
    left->keys[left->key_count] = mid_key;
    int j = left->key_count + 1;
    for (int i = 0; i < right->key_count; i++) {
        left->keys[j + i] = right->keys[i];
        left->children[j + i] = right->children[i];
    }
    left->children[j + right->key_count] = right->children[right->key_count];
    left->key_count += right->key_count + 1;
    free(right);
}

static DBStatus delete_recursive(BPlusTreeNode* node, int key,
                                 BPlusTreeNode* parent, int parent_index,
                                 int* deleted) {
    if (node->is_leaf) {
        int idx = find_index_in_node(node, key);
        if (idx == -1) {
            *deleted = 0;
            return DB_OK;
        }

        for (int i = idx; i < node->key_count - 1; i++) {
            node->keys[i] = node->keys[i + 1];
            node->records[i] = node->records[i + 1];
        }
        node->key_count--;
        *deleted = 1;

        if (parent == NULL) {
            return DB_OK;
        }

        if (node->key_count < BPT_MIN_KEYS) {
            BPlusTreeNode* left_sib = get_left_sibling(parent, parent_index);
            BPlusTreeNode* right_sib = get_right_sibling(parent, parent_index);

            if (left_sib != NULL && left_sib->key_count > BPT_MIN_KEYS) {
                borrow_from_left_leaf(node, left_sib, parent, parent_index);
            } else if (right_sib != NULL && right_sib->key_count > BPT_MIN_KEYS) {
                borrow_from_right_leaf(node, right_sib, parent, parent_index);
            } else if (left_sib != NULL) {
                merge_leaf(left_sib, node);
                for (int i = parent_index - 1; i < parent->key_count - 1; i++) {
                    parent->keys[i] = parent->keys[i + 1];
                    parent->children[i + 1] = parent->children[i + 2];
                }
                parent->key_count--;
            } else if (right_sib != NULL) {
                merge_leaf(node, right_sib);
                for (int i = parent_index; i < parent->key_count - 1; i++) {
                    parent->keys[i] = parent->keys[i + 1];
                    parent->children[i + 1] = parent->children[i + 2];
                }
                parent->key_count--;
            }
        }

        return DB_OK;
    }

    int i = 0;
    while (i < node->key_count && key >= node->keys[i]) {
        i++;
    }

    int child_deleted = 0;
    DBStatus status = delete_recursive(node->children[i], key, node, i, &child_deleted);
    if (status != DB_OK) return status;
    *deleted = child_deleted;

    if (parent == NULL) {
        return DB_OK;
    }

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
            for (int i = parent_index - 1; i < parent->key_count - 1; i++) {
                parent->keys[i] = parent->keys[i + 1];
                parent->children[i + 1] = parent->children[i + 2];
            }
            parent->key_count--;
        } else if (right_sib != NULL) {
            int mid_key = parent->keys[parent_index];
            merge_internal(node, right_sib, mid_key);
            for (int i = parent_index; i < parent->key_count - 1; i++) {
                parent->keys[i] = parent->keys[i + 1];
                parent->children[i + 1] = parent->children[i + 2];
            }
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

    if (tree->root->key_count == 0 && !tree->root->is_leaf) {
        BPlusTreeNode* old_root = tree->root;
        tree->root = tree->root->children[0];
        tree->height--;
        free(old_root);
    }

    if (deleted) {
        tree->size--;
    }
    return DB_OK;
}

void bpt_range_query(BPlusTree* tree, int low, int high, BPTCallback cb, void* ctx) {
    if (tree == NULL || tree->root == NULL || cb == NULL) return;

    BPlusTreeNode* leaf = find_leaf(tree->root, low);

    while (leaf != NULL) {
        int i = 0;
        while (i < leaf->key_count && leaf->keys[i] < low) {
            i++;
        }
        while (i < leaf->key_count && leaf->keys[i] <= high) {
            cb(leaf->keys[i], leaf->records[i], ctx);
            i++;
        }
        if (i < leaf->key_count) {
            break;
        }
        leaf = leaf->next;
    }
}

static void print_node(BPlusTreeNode* node, int level) {
    if (node == NULL) return;

    for (int i = 0; i < level; i++) {
        printf("  ");
    }

    printf("[");
    for (int i = 0; i < node->key_count; i++) {
        printf("%d", node->keys[i]);
        if (i < node->key_count - 1) printf(", ");
    }
    printf("] (%s, %d keys)\n", node->is_leaf ? "leaf" : "internal", node->key_count);

    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            print_node(node->children[i], level + 1);
        }
    }
}

void bpt_print(BPlusTree* tree) {
    if (tree == NULL || tree->root == NULL) {
        printf("Empty tree\n");
        return;
    }
    printf("B+ Tree (height=%d, size=%d):\n", tree->height, tree->size);
    print_node(tree->root, 0);
}
