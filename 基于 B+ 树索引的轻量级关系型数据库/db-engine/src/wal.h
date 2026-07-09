#ifndef WAL_H
#define WAL_H

#include <stdint.h>
#include <stdio.h>

#define WAL_MAGIC 0x57414C00
#define WAL_VERSION 1
#define MAX_LOG_ENTRY_SIZE 4096

typedef enum {
    WAL_INSERT,
    WAL_UPDATE,
    WAL_DELETE,
    WAL_COMMIT,
    WAL_ROLLBACK
} WalRecordType;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint64_t last_txn_id;
} WalHeader;

typedef struct {
    uint64_t txn_id;
    WalRecordType type;
    uint64_t page_id;
    uint32_t data_len;
    char data[MAX_LOG_ENTRY_SIZE - 28];
} WalRecord;

typedef struct {
    FILE* file;
    char* filename;
    uint64_t txn_id;
    int is_open;
} WalManager;

WalManager* wal_init(const char* db_filename);
void wal_destroy(WalManager* wal);
int wal_begin_transaction(WalManager* wal);
int wal_commit(WalManager* wal);
int wal_rollback(WalManager* wal);
int wal_log_insert(WalManager* wal, uint64_t page_id, const char* data, uint32_t data_len);
int wal_log_update(WalManager* wal, uint64_t page_id, const char* old_data, const char* new_data, uint32_t data_len);
int wal_log_delete(WalManager* wal, uint64_t page_id, const char* data, uint32_t data_len);
int wal_recover(WalManager* wal);
uint64_t wal_get_current_txn_id(WalManager* wal);

#endif
