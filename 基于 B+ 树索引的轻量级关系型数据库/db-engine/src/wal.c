#include "wal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int wal_write_record(WalManager* wal, const WalRecord* record) {
    if (!wal || !record || !wal->file) return -1;
    
    fseek(wal->file, 0, SEEK_END);
    size_t written = fwrite(record, sizeof(WalRecord), 1, wal->file);
    fflush(wal->file);
    
    return written == 1 ? 0 : -1;
}

static int wal_read_record(WalManager* wal, WalRecord* record) {
    if (!wal || !record || !wal->file) return -1;
    
    size_t read = fread(record, sizeof(WalRecord), 1, wal->file);
    return read == 1 ? 0 : -1;
}

WalManager* wal_init(const char* db_filename) {
    if (!db_filename) return NULL;
    
    WalManager* wal = (WalManager*)malloc(sizeof(WalManager));
    if (!wal) return NULL;
    
    wal->filename = (char*)malloc(strlen(db_filename) + 5);
    sprintf(wal->filename, "%s.wal", db_filename);
    
    wal->file = fopen(wal->filename, "rb+");
    if (!wal->file) {
        wal->file = fopen(wal->filename, "wb+");
        if (!wal->file) {
            free(wal->filename);
            free(wal);
            return NULL;
        }
        
        WalHeader header = {WAL_MAGIC, WAL_VERSION, 0};
        fwrite(&header, sizeof(WalHeader), 1, wal->file);
        fflush(wal->file);
        wal->txn_id = 0;
    } else {
        WalHeader header;
        fread(&header, sizeof(WalHeader), 1, wal->file);
        
        if (header.magic != WAL_MAGIC || header.version != WAL_VERSION) {
            fclose(wal->file);
            wal->file = fopen(wal->filename, "wb+");
            WalHeader new_header = {WAL_MAGIC, WAL_VERSION, 0};
            fwrite(&new_header, sizeof(WalHeader), 1, wal->file);
            fflush(wal->file);
            wal->txn_id = 0;
        } else {
            wal->txn_id = header.last_txn_id;
        }
    }
    
    wal->is_open = 1;
    return wal;
}

void wal_destroy(WalManager* wal) {
    if (!wal) return;
    
    if (wal->file) {
        WalHeader header = {WAL_MAGIC, WAL_VERSION, wal->txn_id};
        fseek(wal->file, 0, SEEK_SET);
        fwrite(&header, sizeof(WalHeader), 1, wal->file);
        fclose(wal->file);
    }
    
    if (wal->filename) free(wal->filename);
    free(wal);
}

int wal_begin_transaction(WalManager* wal) {
    if (!wal || !wal->is_open) return -1;
    
    wal->txn_id++;
    return 0;
}

int wal_commit(WalManager* wal) {
    if (!wal || !wal->is_open) return -1;
    
    WalRecord record;
    memset(&record, 0, sizeof(WalRecord));
    record.txn_id = wal->txn_id;
    record.type = WAL_COMMIT;
    record.page_id = 0;
    record.data_len = 0;
    
    int ret = wal_write_record(wal, &record);
    
    WalHeader header = {WAL_MAGIC, WAL_VERSION, wal->txn_id};
    fseek(wal->file, 0, SEEK_SET);
    fwrite(&header, sizeof(WalHeader), 1, wal->file);
    fflush(wal->file);
    
    return ret;
}

int wal_rollback(WalManager* wal) {
    if (!wal || !wal->is_open) return -1;
    
    WalRecord record;
    memset(&record, 0, sizeof(WalRecord));
    record.txn_id = wal->txn_id;
    record.type = WAL_ROLLBACK;
    record.page_id = 0;
    record.data_len = 0;
    
    return wal_write_record(wal, &record);
}

int wal_log_insert(WalManager* wal, uint64_t page_id, const char* data, uint32_t data_len) {
    if (!wal || !data || !wal->is_open) return -1;
    
    WalRecord record;
    memset(&record, 0, sizeof(WalRecord));
    record.txn_id = wal->txn_id;
    record.type = WAL_INSERT;
    record.page_id = page_id;
    record.data_len = data_len > MAX_LOG_ENTRY_SIZE - 28 ? MAX_LOG_ENTRY_SIZE - 28 : data_len;
    memcpy(record.data, data, record.data_len);
    
    return wal_write_record(wal, &record);
}

int wal_log_update(WalManager* wal, uint64_t page_id, const char* old_data, const char* new_data, uint32_t data_len) {
    if (!wal || !old_data || !new_data || !wal->is_open) return -1;
    
    WalRecord record;
    memset(&record, 0, sizeof(WalRecord));
    record.txn_id = wal->txn_id;
    record.type = WAL_UPDATE;
    record.page_id = page_id;
    
    uint32_t half_len = data_len > (MAX_LOG_ENTRY_SIZE - 28) / 2 ? (MAX_LOG_ENTRY_SIZE - 28) / 2 : data_len;
    record.data_len = half_len * 2;
    memcpy(record.data, old_data, half_len);
    memcpy(record.data + half_len, new_data, half_len);
    
    return wal_write_record(wal, &record);
}

int wal_log_delete(WalManager* wal, uint64_t page_id, const char* data, uint32_t data_len) {
    if (!wal || !data || !wal->is_open) return -1;
    
    WalRecord record;
    memset(&record, 0, sizeof(WalRecord));
    record.txn_id = wal->txn_id;
    record.type = WAL_DELETE;
    record.page_id = page_id;
    record.data_len = data_len > MAX_LOG_ENTRY_SIZE - 28 ? MAX_LOG_ENTRY_SIZE - 28 : data_len;
    memcpy(record.data, data, record.data_len);
    
    return wal_write_record(wal, &record);
}

int wal_recover(WalManager* wal) {
    if (!wal || !wal->file) return -1;
    
    fseek(wal->file, sizeof(WalHeader), SEEK_SET);
    
    WalRecord record;
    int committed = 0;
    
    while (wal_read_record(wal, &record) == 0) {
        switch (record.type) {
            case WAL_COMMIT:
                committed = 1;
                break;
            case WAL_ROLLBACK:
                committed = 0;
                break;
            case WAL_INSERT:
            case WAL_UPDATE:
            case WAL_DELETE:
                if (committed) {
                }
                break;
            default:
                break;
        }
    }
    
    return 0;
}

uint64_t wal_get_current_txn_id(WalManager* wal) {
    return wal ? wal->txn_id : 0;
}
