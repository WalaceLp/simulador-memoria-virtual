#include <stddef.h>
#include <stdlib.h>

#include "address.h"
#include "page_table.h"

typedef struct PageTableNode {
    void **entries;
    size_t active_entries;
    unsigned int level;
} PageTableNode;

struct PageTable {
    PageTableNode *root;
};

static PageTableNode *page_table_node_create(unsigned int level)
{
    PageTableNode *node = calloc(1, sizeof(PageTableNode));

    if (node == NULL) {
        return NULL;
    }

    node->entries = calloc(PAGE_TABLE_ENTRIES, sizeof(void *));

    if (node->entries == NULL) {
        free(node);
        return NULL;
    }

    node->active_entries = 0;
    node->level = level;

    return node;
}

static PageTableEntry *page_table_entry_create(uint32_t frame_number)
{
    PageTableEntry *entry = calloc(1, sizeof(PageTableEntry));

    if (entry == NULL) {
        return NULL;
    }

    entry->frame_number = frame_number;
    entry->present = true;
    entry->writable = true;
    entry->dirty = false;
    entry->referenced = false;
    entry->copy_on_write = false;

    return entry;
}

PageTable *page_table_create(void)
{
    PageTable *table = calloc(1, sizeof(PageTable));

    if (table == NULL) {
        return NULL;
    }

    table->root = page_table_node_create(0);

    if (table->root == NULL) {
        free(table);
        return NULL;
    }

    return table;
}

int page_table_map(
    PageTable *table,
    uint64_t virtual_address,
    uint32_t frame_number
)
{
    if (table == NULL || table->root == NULL) {
        return -1;
    }

    PageTableNode *current = table->root;

    /*
     * Os níveis 0, 1 e 2 armazenam ponteiros para outros nós.
     * O nível 3 armazena a entrada final da página.
     */
    for (int level = 0; level < PAGE_TABLE_LEVELS - 1; level++) {
        uint16_t index = address_level_index(virtual_address, level);

        if (current->entries[index] == NULL) {
            PageTableNode *child =
                page_table_node_create((unsigned int)(level + 1));

            if (child == NULL) {
                return -1;
            }

            current->entries[index] = child;
            current->active_entries++;
        }

        current = current->entries[index];
    }

    uint16_t leaf_index = address_level_index(
        virtual_address,
        PAGE_TABLE_LEVELS - 1
    );

    PageTableEntry *entry = current->entries[leaf_index];

    if (entry == NULL) {
        entry = page_table_entry_create(frame_number);

        if (entry == NULL) {
            return -1;
        }

        current->entries[leaf_index] = entry;
        current->active_entries++;
    } else {
        entry->frame_number = frame_number;
        entry->present = true;
    }

    return 0;
}

const PageTableEntry *page_table_lookup(
    const PageTable *table,
    uint64_t virtual_address
)
{
    if (table == NULL || table->root == NULL) {
        return NULL;
    }

    const PageTableNode *current = table->root;

    for (int level = 0; level < PAGE_TABLE_LEVELS - 1; level++) {
        uint16_t index = address_level_index(virtual_address, level);

        if (current->entries[index] == NULL) {
            return NULL;
        }

        current = current->entries[index];
    }

    uint16_t leaf_index = address_level_index(
        virtual_address,
        PAGE_TABLE_LEVELS - 1
    );

    const PageTableEntry *entry = current->entries[leaf_index];

    if (entry == NULL || !entry->present) {
        return NULL;
    }

    return entry;
}

static void page_table_node_destroy(PageTableNode *node)
{
    if (node == NULL) {
        return;
    }

    for (size_t index = 0; index < PAGE_TABLE_ENTRIES; index++) {
        if (node->entries[index] == NULL) {
            continue;
        }

        if (node->level < PAGE_TABLE_LEVELS - 1) {
            page_table_node_destroy(node->entries[index]);
        } else {
            free(node->entries[index]);
        }
    }

    free(node->entries);
    free(node);
}

void page_table_destroy(PageTable *table)
{
    if (table == NULL) {
        return;
    }

    page_table_node_destroy(table->root);
    free(table);
}