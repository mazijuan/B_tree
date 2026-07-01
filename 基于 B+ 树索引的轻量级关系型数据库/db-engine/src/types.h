#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 65536
#define BITMAP_SIZE (MAX_PAGES / 8)

typedef uint32_t page_id_t;
typedef uint64_t record_id_t;

typedef struct {
    uint8_t data[PAGE_SIZE];
} Page;

typedef struct {
    page_id_t page_id;
    uint32_t offset;
} RecordPointer;

typedef enum {
    DB_OK = 0,
    DB_ERROR = -1,
    DB_PAGE_NOT_FOUND = -2,
    DB_OUT_OF_PAGES = -3,
    DB_FILE_ERROR = -4
} DBStatus;

#endif
