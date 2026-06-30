#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct PageTable PageTable;

typedef struct {
    uint32_t frame_number;

    bool present;
    bool writable;
    bool dirty;
    bool referenced;
    bool copy_on_write;

    /*
     * Quantidade de nós-folha que compartilham esta entrada.
     * É utilizado internamente pelo copy-on-write.
     */
    size_t reference_count;
} PageTableEntry;

PageTable *page_table_create(void);

/*
 * Cria outro objeto PageTable, inicialmente compartilhando
 * a mesma raiz e as mesmas subárvores.
 */
PageTable *page_table_clone_shared(
    const PageTable *source
);

bool page_table_shares_root(
    const PageTable *first,
    const PageTable *second
);

size_t page_table_root_reference_count(
    const PageTable *table
);

int page_table_map(
    PageTable *table,
    uint64_t virtual_address,
    uint32_t frame_number
);

const PageTableEntry *page_table_lookup(
    const PageTable *table,
    uint64_t virtual_address
);

const PageTableEntry *page_table_lookup_with_levels(
    const PageTable *table,
    uint64_t virtual_address,
    size_t *levels_visited
);

int page_table_unmap(
    PageTable *table,
    uint64_t virtual_address
);

size_t page_table_node_count(
    const PageTable *table
);

void page_table_destroy(PageTable *table);

#endif