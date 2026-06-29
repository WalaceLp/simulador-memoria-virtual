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
     * Os três primeiros índices apontam para nós intermediários.
     * O último índice aponta para uma PageTableEntry.
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

static void page_table_empty_node_destroy(PageTableNode *node)
{
    if (node == NULL) {
        return;
    }

    free(node->entries);
    free(node);
}

int page_table_unmap(
    PageTable *table,
    uint64_t virtual_address
)
{
    if (table == NULL || table->root == NULL) {
        return -1;
    }

    /*
     * nodes guarda cada nó visitado.
     *
     * nodes[0] = raiz
     * nodes[1] = segundo nível
     * nodes[2] = terceiro nível
     * nodes[3] = último nível
     */
    PageTableNode *nodes[PAGE_TABLE_LEVELS];

    /*
     * indices guarda qual entrada foi usada para sair
     * de cada nó e alcançar o próximo nível.
     */
    uint16_t indices[PAGE_TABLE_LEVELS];

    nodes[0] = table->root;

    for (int level = 0; level < PAGE_TABLE_LEVELS - 1; level++) {
        indices[level] = address_level_index(
            virtual_address,
            level
        );

        void *child = nodes[level]->entries[indices[level]];

        if (child == NULL) {
            return -1;
        }

        nodes[level + 1] = child;
    }

    indices[PAGE_TABLE_LEVELS - 1] = address_level_index(
        virtual_address,
        PAGE_TABLE_LEVELS - 1
    );

    PageTableNode *leaf_node = nodes[PAGE_TABLE_LEVELS - 1];

    PageTableEntry *entry =
        leaf_node->entries[indices[PAGE_TABLE_LEVELS - 1]];

    if (entry == NULL) {
        return -1;
    }

    free(entry);

    leaf_node->entries[indices[PAGE_TABLE_LEVELS - 1]] = NULL;
    leaf_node->active_entries--;

    /*
     * Percorre o caminho de baixo para cima.
     *
     * Se um nó ficou sem entradas, ele é removido do pai
     * e sua memória é liberada.
     *
     * A raiz nunca é removida.
     */
    for (int level = PAGE_TABLE_LEVELS - 1; level > 0; level--) {
        PageTableNode *current = nodes[level];

        if (current->active_entries != 0) {
            break;
        }

        PageTableNode *parent = nodes[level - 1];
        uint16_t parent_index = indices[level - 1];

        page_table_empty_node_destroy(current);

        parent->entries[parent_index] = NULL;
        parent->active_entries--;
    }

    return 0;
}

static size_t page_table_node_count_recursive(
    const PageTableNode *node
)
{
    if (node == NULL) {
        return 0;
    }

    size_t count = 1;

    /*
     * No último nível, as entradas são PageTableEntry,
     * não outros nós.
     */
    if (node->level == PAGE_TABLE_LEVELS - 1) {
        return count;
    }

    for (size_t index = 0; index < PAGE_TABLE_ENTRIES; index++) {
        const PageTableNode *child = node->entries[index];

        if (child != NULL) {
            count += page_table_node_count_recursive(child);
        }
    }

    return count;
}

size_t page_table_node_count(
    const PageTable *table
)
{
    if (table == NULL || table->root == NULL) {
        return 0;
    }

    return page_table_node_count_recursive(table->root);
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