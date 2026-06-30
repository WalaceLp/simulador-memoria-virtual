#include <stddef.h>
#include <stdlib.h>

#include "address.h"
#include "page_table.h"

typedef struct PageTableNode {
    void **entries;
    size_t active_entries;
    unsigned int level;
    size_t reference_count;
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

    node->entries = calloc(
        PAGE_TABLE_ENTRIES,
        sizeof(void *)
    );

    if (node->entries == NULL) {
        free(node);
        return NULL;
    }

    node->active_entries = 0;
    node->level = level;
    node->reference_count = 1;

    return node;
}

static PageTableEntry *page_table_entry_create(
    uint32_t frame_number
)
{
    PageTableEntry *entry = calloc(
        1,
        sizeof(PageTableEntry)
    );

    if (entry == NULL) {
        return NULL;
    }

    entry->frame_number = frame_number;
    entry->present = true;
    entry->writable = true;
    entry->dirty = false;
    entry->referenced = false;
    entry->copy_on_write = false;
    entry->reference_count = 1;

    return entry;
}

static void page_table_entry_retain(
    PageTableEntry *entry
)
{
    if (entry == NULL) {
        return;
    }

    entry->reference_count++;
}

static void page_table_entry_release(
    PageTableEntry *entry
)
{
    if (entry == NULL) {
        return;
    }

    if (entry->reference_count > 1) {
        entry->reference_count--;
        return;
    }

    free(entry);
}

static void page_table_node_retain(
    PageTableNode *node
)
{
    if (node == NULL) {
        return;
    }

    node->reference_count++;
}

static void page_table_node_release(
    PageTableNode *node
)
{
    if (node == NULL) {
        return;
    }

    if (node->reference_count > 1) {
        node->reference_count--;
        return;
    }

    for (
        size_t index = 0;
        index < PAGE_TABLE_ENTRIES;
        index++
    ) {
        if (node->entries[index] == NULL) {
            continue;
        }

        if (node->level < PAGE_TABLE_LEVELS - 1) {
            page_table_node_release(
                node->entries[index]
            );
        } else {
            page_table_entry_release(
                node->entries[index]
            );
        }
    }

    free(node->entries);
    free(node);
}

/*
 * Faz uma cópia superficial do nó.
 *
 * O novo nó recebe outro vetor de entradas, mas inicialmente
 * aponta para os mesmos filhos ou PageTableEntry.
 *
 * Por isso, as referências dos objetos compartilhados são
 * incrementadas.
 */
static PageTableNode *page_table_node_clone_shallow(
    const PageTableNode *source
)
{
    if (source == NULL) {
        return NULL;
    }

    PageTableNode *copy =
        page_table_node_create(source->level);

    if (copy == NULL) {
        return NULL;
    }

    copy->active_entries = source->active_entries;

    for (
        size_t index = 0;
        index < PAGE_TABLE_ENTRIES;
        index++
    ) {
        void *entry = source->entries[index];

        if (entry == NULL) {
            continue;
        }

        copy->entries[index] = entry;

        if (source->level < PAGE_TABLE_LEVELS - 1) {
            page_table_node_retain(entry);
        } else {
            page_table_entry_retain(entry);
        }
    }

    return copy;
}

static PageTableEntry *page_table_entry_clone(
    const PageTableEntry *source
)
{
    if (source == NULL) {
        return NULL;
    }

    PageTableEntry *copy = malloc(
        sizeof(PageTableEntry)
    );

    if (copy == NULL) {
        return NULL;
    }

    *copy = *source;
    copy->reference_count = 1;
    copy->copy_on_write = false;

    return copy;
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

PageTable *page_table_clone_shared(
    const PageTable *source
)
{
    if (source == NULL || source->root == NULL) {
        return NULL;
    }

    PageTable *copy = calloc(1, sizeof(PageTable));

    if (copy == NULL) {
        return NULL;
    }

    copy->root = source->root;
    page_table_node_retain(copy->root);

    return copy;
}

bool page_table_shares_root(
    const PageTable *first,
    const PageTable *second
)
{
    if (
        first == NULL ||
        second == NULL ||
        first->root == NULL ||
        second->root == NULL
    ) {
        return false;
    }

    return first->root == second->root;
}

size_t page_table_root_reference_count(
    const PageTable *table
)
{
    if (table == NULL || table->root == NULL) {
        return 0;
    }

    return table->root->reference_count;
}

/*
 * Garante que a raiz da tabela pertence exclusivamente
 * a este objeto PageTable.
 */
static int page_table_ensure_private_root(
    PageTable *table
)
{
    if (table == NULL || table->root == NULL) {
        return -1;
    }

    if (table->root->reference_count == 1) {
        return 0;
    }

    PageTableNode *old_root = table->root;

    PageTableNode *new_root =
        page_table_node_clone_shallow(old_root);

    if (new_root == NULL) {
        return -1;
    }

    table->root = new_root;
    page_table_node_release(old_root);

    return 0;
}

/*
 * Garante que um filho de um nó seja privado antes
 * que ele seja modificado.
 *
 * O nó pai já deve pertencer exclusivamente à tabela atual.
 */
static PageTableNode *page_table_ensure_private_child(
    PageTableNode *parent,
    uint16_t index
)
{
    if (parent == NULL) {
        return NULL;
    }

    PageTableNode *child = parent->entries[index];

    if (child == NULL) {
        return NULL;
    }

    if (child->reference_count == 1) {
        return child;
    }

    PageTableNode *copy =
        page_table_node_clone_shallow(child);

    if (copy == NULL) {
        return NULL;
    }

    parent->entries[index] = copy;
    page_table_node_release(child);

    return copy;
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

    if (page_table_ensure_private_root(table) != 0) {
        return -1;
    }

    PageTableNode *current = table->root;

    for (
        int level = 0;
        level < PAGE_TABLE_LEVELS - 1;
        level++
    ) {
        uint16_t index = address_level_index(
            virtual_address,
            level
        );

        if (current->entries[index] == NULL) {
            PageTableNode *child =
                page_table_node_create(
                    (unsigned int)(level + 1)
                );

            if (child == NULL) {
                return -1;
            }

            current->entries[index] = child;
            current->active_entries++;
            current = child;

            continue;
        }

        PageTableNode *private_child =
            page_table_ensure_private_child(
                current,
                index
            );

        if (private_child == NULL) {
            return -1;
        }

        current = private_child;
    }

    uint16_t leaf_index = address_level_index(
        virtual_address,
        PAGE_TABLE_LEVELS - 1
    );

    PageTableEntry *entry =
        current->entries[leaf_index];

    if (entry == NULL) {
        entry = page_table_entry_create(
            frame_number
        );

        if (entry == NULL) {
            return -1;
        }

        current->entries[leaf_index] = entry;
        current->active_entries++;

        return 0;
    }

    if (entry->reference_count > 1) {
        PageTableEntry *copy =
            page_table_entry_clone(entry);

        if (copy == NULL) {
            return -1;
        }

        page_table_entry_release(entry);
        current->entries[leaf_index] = copy;
        entry = copy;
    }

    entry->frame_number = frame_number;
    entry->present = true;

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

    for (
        int level = 0;
        level < PAGE_TABLE_LEVELS - 1;
        level++
    ) {
        uint16_t index = address_level_index(
            virtual_address,
            level
        );

        if (current->entries[index] == NULL) {
            return NULL;
        }

        current = current->entries[index];
    }

    uint16_t leaf_index = address_level_index(
        virtual_address,
        PAGE_TABLE_LEVELS - 1
    );

    const PageTableEntry *entry =
        current->entries[leaf_index];

    if (entry == NULL || !entry->present) {
        return NULL;
    }

    return entry;
}

static bool page_table_mapping_exists(
    const PageTable *table,
    uint64_t virtual_address
)
{
    return page_table_lookup(
        table,
        virtual_address
    ) != NULL;
}

static void page_table_empty_node_destroy(
    PageTableNode *node
)
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
     * Verifica antes se o mapeamento existe.
     * Isso evita copiar caminhos compartilhados para uma
     * remoção que não poderá ser realizada.
     */
    if (!page_table_mapping_exists(
        table,
        virtual_address
    )) {
        return -1;
    }

    if (page_table_ensure_private_root(table) != 0) {
        return -1;
    }

    PageTableNode *nodes[PAGE_TABLE_LEVELS];
    uint16_t indices[PAGE_TABLE_LEVELS];

    nodes[0] = table->root;

    for (
        int level = 0;
        level < PAGE_TABLE_LEVELS - 1;
        level++
    ) {
        indices[level] = address_level_index(
            virtual_address,
            level
        );

        PageTableNode *child =
            page_table_ensure_private_child(
                nodes[level],
                indices[level]
            );

        if (child == NULL) {
            return -1;
        }

        nodes[level + 1] = child;
    }

    indices[PAGE_TABLE_LEVELS - 1] =
        address_level_index(
            virtual_address,
            PAGE_TABLE_LEVELS - 1
        );

    PageTableNode *leaf_node =
        nodes[PAGE_TABLE_LEVELS - 1];

    PageTableEntry *entry =
        leaf_node->entries[
            indices[PAGE_TABLE_LEVELS - 1]
        ];

    if (entry == NULL) {
        return -1;
    }

    page_table_entry_release(entry);

    leaf_node->entries[
        indices[PAGE_TABLE_LEVELS - 1]
    ] = NULL;

    leaf_node->active_entries--;

    /*
     * Realiza a poda dos nós vazios de baixo para cima.
     * Todos esses nós já são privados da tabela atual.
     */
    for (
        int level = PAGE_TABLE_LEVELS - 1;
        level > 0;
        level--
    ) {
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

    if (node->level == PAGE_TABLE_LEVELS - 1) {
        return count;
    }

    for (
        size_t index = 0;
        index < PAGE_TABLE_ENTRIES;
        index++
    ) {
        const PageTableNode *child =
            node->entries[index];

        if (child != NULL) {
            count += page_table_node_count_recursive(
                child
            );
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

    return page_table_node_count_recursive(
        table->root
    );
}

void page_table_destroy(PageTable *table)
{
    if (table == NULL) {
        return;
    }

    page_table_node_release(table->root);
    free(table);
}