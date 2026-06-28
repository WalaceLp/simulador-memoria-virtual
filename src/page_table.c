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

static void page_table_node_destroy(PageTableNode *node)
{
    if (node == NULL) {
        return;
    }

    free(node->entries);
    free(node);
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

void page_table_destroy(PageTable *table)
{
    if (table == NULL) {
        return;
    }

    page_table_node_destroy(table->root);
    free(table);
}