#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include <stdint.h>
#include "types.h"

#define BPT_ORDER 5
#define BPT_MAX_KEYS (2 * BPT_ORDER)
#define BPT_MIN_KEYS BPT_ORDER
#define BPT_MAX_KEYS_ARRAY (BPT_MAX_KEYS + 1)

typedef struct BPlusTreeNode {
    int is_leaf;
    int key_count;
    int keys[BPT_MAX_KEYS_ARRAY];
    struct BPlusTreeNode* children[BPT_MAX_KEYS_ARRAY + 1];
    struct BPlusTreeNode* next;
    struct BPlusTreeNode* prev;
    record_id_t records[BPT_MAX_KEYS_ARRAY];
} BPlusTreeNode;

typedef struct {
    BPlusTreeNode* root;
    int height;
    int size;
} BPlusTree;

typedef void (*BPTCallback)(int key, record_id_t rid, void* ctx);

BPlusTree* bpt_create();
void bpt_destroy(BPlusTree* tree);

DBStatus bpt_insert(BPlusTree* tree, int key, record_id_t rid);
record_id_t bpt_search(BPlusTree* tree, int key);
DBStatus bpt_delete(BPlusTree* tree, int key);
void bpt_range_query(BPlusTree* tree, int low, int high, BPTCallback cb, void* ctx);

int bpt_size(BPlusTree* tree);
void bpt_print(BPlusTree* tree);

#endif
