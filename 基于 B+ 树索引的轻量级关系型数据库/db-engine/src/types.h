#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// 固定页大小4KB
#define PAGE_SIZE     4096
// 数据库最大支持页数
#define MAX_PAGES     65536
// 空闲页位图字节总大小
#define BITMAP_SIZE   (MAX_PAGES / 8)

// 页ID类型
typedef uint32_t page_id_t;
// 记录ID
typedef uint64_t record_id_t;

// 磁盘页结构，仅存储原始4KB二进制数据
typedef struct {
    uint8_t data[PAGE_SIZE];
} Page;

// 记录定位结构：页ID + 页内偏移
typedef struct {
    page_id_t page_id;
    uint32_t offset;
} RecordPointer;

// 数据库操作状态码
typedef enum {
    DB_OK            = 0,  // 执行成功
    DB_ERROR         = -1, // 通用未知错误
    DB_PAGE_NOT_FOUND= -2, // 目标页不存在
    DB_OUT_OF_PAGES  = -3, // 无空闲页可分配
    DB_FILE_ERROR    = -4  // 文件IO读写失败
} DBStatus;

#endif
