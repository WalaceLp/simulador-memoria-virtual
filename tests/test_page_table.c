#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "page_table.h"

static void test_create_and_destroy(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    page_table_destroy(table);
}

static void test_lookup_unmapped_address(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    const PageTableEntry *entry =
        page_table_lookup(table, 0x1000ULL);

    assert(entry == NULL);

    page_table_destroy(table);
}

static void test_map_and_lookup(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    int result = page_table_map(
        table,
        0x0000000123456000ULL,
        7
    );

    assert(result == 0);

    const PageTableEntry *entry =
        page_table_lookup(
            table,
            0x0000000123456000ULL
        );

    assert(entry != NULL);
    assert(entry->present);
    assert(entry->frame_number == 7);

    page_table_destroy(table);
}

static void test_addresses_inside_same_page(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    uint64_t page_start = 0x0000000123456000ULL;

    assert(page_table_map(table, page_start, 12) == 0);

    const PageTableEntry *entry1 =
        page_table_lookup(table, page_start);

    const PageTableEntry *entry2 =
        page_table_lookup(table, page_start + 0x123ULL);

    const PageTableEntry *entry3 =
        page_table_lookup(table, page_start + 0xFFFULL);

    assert(entry1 != NULL);
    assert(entry2 != NULL);
    assert(entry3 != NULL);

    assert(entry1->frame_number == 12);
    assert(entry2->frame_number == 12);
    assert(entry3->frame_number == 12);

    assert(entry1 == entry2);
    assert(entry2 == entry3);

    page_table_destroy(table);
}

static void test_multiple_mappings(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    assert(page_table_map(table, 0x1000ULL, 1) == 0);
    assert(page_table_map(table, 0x2000ULL, 2) == 0);
    assert(page_table_map(table, 0x3000ULL, 3) == 0);

    const PageTableEntry *first =
        page_table_lookup(table, 0x1000ULL);

    const PageTableEntry *second =
        page_table_lookup(table, 0x2000ULL);

    const PageTableEntry *third =
        page_table_lookup(table, 0x3000ULL);

    assert(first != NULL);
    assert(second != NULL);
    assert(third != NULL);

    assert(first->frame_number == 1);
    assert(second->frame_number == 2);
    assert(third->frame_number == 3);

    page_table_destroy(table);
}

static void test_remap_existing_page(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    assert(page_table_map(table, 0x5000ULL, 10) == 0);
    assert(page_table_map(table, 0x5000ULL, 20) == 0);

    const PageTableEntry *entry =
        page_table_lookup(table, 0x5000ULL);

    assert(entry != NULL);
    assert(entry->frame_number == 20);

    page_table_destroy(table);
}

static void test_unmap_existing_page(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    assert(page_table_map(table, 0x1000ULL, 1) == 0);
    assert(page_table_lookup(table, 0x1000ULL) != NULL);

    assert(page_table_unmap(table, 0x1000ULL) == 0);
    assert(page_table_lookup(table, 0x1000ULL) == NULL);

    page_table_destroy(table);
}

static void test_unmap_unmapped_page(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    assert(page_table_unmap(table, 0x9000ULL) == -1);

    page_table_destroy(table);
}

static void test_unmap_preserves_other_page(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    assert(page_table_map(table, 0x1000ULL, 1) == 0);
    assert(page_table_map(table, 0x2000ULL, 2) == 0);

    assert(page_table_unmap(table, 0x1000ULL) == 0);

    assert(page_table_lookup(table, 0x1000ULL) == NULL);

    const PageTableEntry *remaining =
        page_table_lookup(table, 0x2000ULL);

    assert(remaining != NULL);
    assert(remaining->frame_number == 2);

    page_table_destroy(table);
}

static void test_unmap_prunes_empty_nodes(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    /*
     * Antes de qualquer mapeamento existe somente a raiz.
     */
    assert(page_table_node_count(table) == 1);

    assert(
        page_table_map(
            table,
            0x0000000123456000ULL,
            7
        ) == 0
    );

    /*
     * Raiz mais três nós criados de forma preguiçosa.
     */
    assert(page_table_node_count(table) == 4);

    assert(
        page_table_unmap(
            table,
            0x0000000123456000ULL
        ) == 0
    );

    /*
     * Após a poda, somente a raiz deve permanecer.
     */
    assert(page_table_node_count(table) == 1);

    page_table_destroy(table);
}

static void test_remap_after_unmap(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    assert(page_table_map(table, 0x5000ULL, 10) == 0);
    assert(page_table_unmap(table, 0x5000ULL) == 0);
    assert(page_table_map(table, 0x5000ULL, 20) == 0);

    const PageTableEntry *entry =
        page_table_lookup(table, 0x5000ULL);

    assert(entry != NULL);
    assert(entry->frame_number == 20);

    page_table_destroy(table);
}

static void test_unmap_address_inside_same_page(void)
{
    PageTable *table = page_table_create();

    assert(table != NULL);

    uint64_t page_start = 0x0000000123456000ULL;

    assert(page_table_map(table, page_start, 30) == 0);

    /*
     * O endereço usado na remoção difere apenas no offset.
     */
    assert(
        page_table_unmap(
            table,
            page_start + 0x321ULL
        ) == 0
    );

    assert(page_table_lookup(table, page_start) == NULL);
    assert(
        page_table_lookup(
            table,
            page_start + 0xFFFULL
        ) == NULL
    );

    page_table_destroy(table);
}

int main(void)
{
    test_create_and_destroy();
    test_lookup_unmapped_address();
    test_map_and_lookup();
    test_addresses_inside_same_page();
    test_multiple_mappings();
    test_remap_existing_page();

    test_unmap_existing_page();
    test_unmap_unmapped_page();
    test_unmap_preserves_other_page();
    test_unmap_prunes_empty_nodes();
    test_remap_after_unmap();
    test_unmap_address_inside_same_page();

    printf("Todos os testes de page_table passaram.\n");

    return 0;
}