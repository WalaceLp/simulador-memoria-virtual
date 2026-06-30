#include <assert.h>
#include <stdio.h>

#include "process.h"

static void test_create_and_destroy_process(void)
{
    Process *process = process_create(1);

    assert(process != NULL);
    assert(process_get_pid(process) == 1);
    assert(process_get_page_table(process) != NULL);

    process_destroy(process);
}

static void test_invalid_pid(void)
{
    Process *process = process_create(-1);

    assert(process == NULL);
}

static void test_null_process_operations(void)
{
    assert(process_get_pid(NULL) == -1);
    assert(process_get_page_table(NULL) == NULL);
    assert(process_get_page_table_const(NULL) == NULL);

    process_destroy(NULL);
}

static void test_process_page_table(void)
{
    Process *process = process_create(10);

    assert(process != NULL);

    PageTable *table = process_get_page_table(process);

    assert(table != NULL);

    assert(page_table_map(table, 0x1000ULL, 5) == 0);

    const PageTableEntry *entry =
        page_table_lookup(table, 0x1000ULL);

    assert(entry != NULL);
    assert(entry->frame_number == 5);

    process_destroy(process);
}

static void test_processes_have_independent_page_tables(void)
{
    Process *first = process_create(1);
    Process *second = process_create(2);

    assert(first != NULL);
    assert(second != NULL);

    PageTable *first_table =
        process_get_page_table(first);

    PageTable *second_table =
        process_get_page_table(second);

    assert(first_table != second_table);

    assert(page_table_map(
        first_table,
        0x1000ULL,
        7
    ) == 0);

    assert(page_table_lookup(
        first_table,
        0x1000ULL
    ) != NULL);

    assert(page_table_lookup(
        second_table,
        0x1000ULL
    ) == NULL);

    process_destroy(first);
    process_destroy(second);
}

static void test_fork_shares_page_table(void)
{
    Process *parent = process_create(1);

    assert(parent != NULL);

    PageTable *parent_table =
        process_get_page_table(parent);

    assert(page_table_map(
        parent_table,
        0x1000ULL,
        7
    ) == 0);

    Process *child = process_fork(parent, 2);

    assert(child != NULL);
    assert(process_get_pid(child) == 2);

    PageTable *child_table =
        process_get_page_table(child);

    assert(parent_table == child_table);

    assert(page_table_reference_count(
        parent_table
    ) == 2);

    const PageTableEntry *child_entry =
        page_table_lookup(
            child_table,
            0x1000ULL
        );

    assert(child_entry != NULL);
    assert(child_entry->frame_number == 7);

    process_destroy(child);

    assert(page_table_reference_count(
        parent_table
    ) == 1);

    process_destroy(parent);
}

static void test_destroy_parent_before_child(void)
{
    Process *parent = process_create(10);

    assert(parent != NULL);

    PageTable *table =
        process_get_page_table(parent);

    assert(page_table_map(
        table,
        0x2000ULL,
        9
    ) == 0);

    Process *child = process_fork(parent, 11);

    assert(child != NULL);

    process_destroy(parent);

    PageTable *child_table =
        process_get_page_table(child);

    assert(page_table_reference_count(
        child_table
    ) == 1);

    const PageTableEntry *entry =
        page_table_lookup(
            child_table,
            0x2000ULL
        );

    assert(entry != NULL);
    assert(entry->frame_number == 9);

    process_destroy(child);
}

static void test_invalid_fork(void)
{
    Process *parent = process_create(1);

    assert(parent != NULL);

    assert(process_fork(NULL, 2) == NULL);
    assert(process_fork(parent, -1) == NULL);

    process_destroy(parent);
}

static void test_shared_table_rejects_mutation(void)
{
    Process *parent = process_create(20);

    assert(parent != NULL);

    PageTable *table =
        process_get_page_table(parent);

    assert(page_table_map(
        table,
        0x3000ULL,
        4
    ) == 0);

    Process *child = process_fork(parent, 21);

    assert(child != NULL);

    PageTable *shared =
        process_get_page_table(child);

    assert(page_table_reference_count(
        shared
    ) == 2);

    /*
     * Até o copy-on-write ser implementado,
     * alterações em uma tabela compartilhada
     * são rejeitadas com -2.
     */
    assert(page_table_map(
        shared,
        0x4000ULL,
        5
    ) == -2);

    assert(page_table_unmap(
        shared,
        0x3000ULL
    ) == -2);

    assert(page_table_lookup(
        shared,
        0x3000ULL
    ) != NULL);

    process_destroy(child);
    process_destroy(parent);
}

int main(void)
{
    test_create_and_destroy_process();
    test_invalid_pid();
    test_null_process_operations();
    test_process_page_table();
    test_processes_have_independent_page_tables();

    test_fork_shares_page_table();
    test_destroy_parent_before_child();
    test_invalid_fork();
    test_shared_table_rejects_mutation();

    printf("Todos os testes de process passaram.\n");

    return 0;
}