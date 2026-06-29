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

int main(void)
{
    test_create_and_destroy_process();
    test_invalid_pid();
    test_null_process_operations();
    test_process_page_table();
    test_processes_have_independent_page_tables();

    printf("Todos os testes de process passaram.\n");

    return 0;
}