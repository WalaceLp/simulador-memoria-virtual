#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct PageTable PageTable;

typedef struct {
    uint32_t frame_number;
    bool present;
    bool writable;
    bool dirty;
    bool referenced;
    bool copy_on_write;
} PageTableEntry;

PageTable *page_table_create(void);

int page_table_map(
    PageTable *table,
    uint64_t virtual_address,
    uint32_t frame_number
);

const PageTableEntry *page_table_lookup(
    const PageTable *table,
    uint64_t virtual_address
);

void page_table_destroy(PageTable *table);

#endif