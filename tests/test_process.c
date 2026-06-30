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

static void test_invalid_fork(void)
{
    Process *parent = process_create(1);

    assert(parent != NULL);

    assert(process_fork(NULL, 2) == NULL);
    assert(process_fork(parent, -1) == NULL);

    process_destroy(parent);
}

static void test_fork_shares_root_initially(void)
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

    /*
     * Pai e filho possuem objetos PageTable diferentes.
     */
    assert(parent_table != child_table);

    /*
     * Entretanto, a raiz da Trie é inicialmente compartilhada.
     */
    assert(page_table_shares_root(
        parent_table,
        child_table
    ));

    assert(page_table_root_reference_count(
        parent_table
    ) == 2);

    const PageTableEntry *parent_entry =
        page_table_lookup(
            parent_table,
            0x1000ULL
        );

    const PageTableEntry *child_entry =
        page_table_lookup(
            child_table,
            0x1000ULL
        );

    assert(parent_entry != NULL);
    assert(child_entry != NULL);

    /*
     * Antes de qualquer modificação, a entrada também é
     * compartilhada entre pai e filho.
     */
    assert(parent_entry == child_entry);
    assert(parent_entry->frame_number == 7);
    assert(child_entry->frame_number == 7);

    process_destroy(child);
    process_destroy(parent);
}

static void test_destroy_parent_before_child(void)
{
    Process *parent = process_create(10);

    assert(parent != NULL);

    PageTable *parent_table =
        process_get_page_table(parent);

    assert(page_table_map(
        parent_table,
        0x2000ULL,
        9
    ) == 0);

    Process *child = process_fork(parent, 11);

    assert(child != NULL);

    PageTable *child_table =
        process_get_page_table(child);

    assert(parent_table != child_table);

    assert(page_table_shares_root(
        parent_table,
        child_table
    ));

    assert(page_table_root_reference_count(
        child_table
    ) == 2);

    process_destroy(parent);

    /*
     * Após destruir o pai, o filho passa a ser o único
     * proprietário da raiz.
     */
    assert(page_table_root_reference_count(
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

static void test_child_map_triggers_copy_on_write(void)
{
    Process *parent = process_create(10);

    assert(parent != NULL);

    PageTable *parent_table =
        process_get_page_table(parent);

    assert(page_table_map(
        parent_table,
        0x1000ULL,
        3
    ) == 0);

    Process *child = process_fork(parent, 11);

    assert(child != NULL);

    PageTable *child_table =
        process_get_page_table(child);

    assert(page_table_shares_root(
        parent_table,
        child_table
    ));

    /*
     * A escrita do filho deve criar um caminho privado.
     */
    assert(page_table_map(
        child_table,
        0x2000ULL,
        8
    ) == 0);

    assert(!page_table_shares_root(
        parent_table,
        child_table
    ));

    const PageTableEntry *child_new =
        page_table_lookup(
            child_table,
            0x2000ULL
        );

    assert(child_new != NULL);
    assert(child_new->frame_number == 8);

    /*
     * O pai não deve enxergar o novo mapeamento do filho.
     */
    assert(page_table_lookup(
        parent_table,
        0x2000ULL
    ) == NULL);

    /*
     * O mapeamento anterior deve continuar existindo
     * nas duas tabelas.
     */
    assert(page_table_lookup(
        parent_table,
        0x1000ULL
    ) != NULL);

    assert(page_table_lookup(
        child_table,
        0x1000ULL
    ) != NULL);

    process_destroy(child);
    process_destroy(parent);
}

static void test_child_remap_does_not_change_parent(void)
{
    Process *parent = process_create(20);

    assert(parent != NULL);

    PageTable *parent_table =
        process_get_page_table(parent);

    assert(page_table_map(
        parent_table,
        0x3000ULL,
        4
    ) == 0);

    Process *child = process_fork(parent, 21);

    assert(child != NULL);

    PageTable *child_table =
        process_get_page_table(child);

    assert(page_table_map(
        child_table,
        0x3000ULL,
        15
    ) == 0);

    const PageTableEntry *parent_entry =
        page_table_lookup(
            parent_table,
            0x3000ULL
        );

    const PageTableEntry *child_entry =
        page_table_lookup(
            child_table,
            0x3000ULL
        );

    assert(parent_entry != NULL);
    assert(child_entry != NULL);

    assert(parent_entry->frame_number == 4);
    assert(child_entry->frame_number == 15);

    /*
     * A entrada deve ter sido copiada antes da alteração.
     */
    assert(parent_entry != child_entry);

    process_destroy(child);
    process_destroy(parent);
}

static void test_child_unmap_does_not_change_parent(void)
{
    Process *parent = process_create(30);

    assert(parent != NULL);

    PageTable *parent_table =
        process_get_page_table(parent);

    assert(page_table_map(
        parent_table,
        0x4000ULL,
        6
    ) == 0);

    assert(page_table_map(
        parent_table,
        0x5000ULL,
        7
    ) == 0);

    Process *child = process_fork(parent, 31);

    assert(child != NULL);

    PageTable *child_table =
        process_get_page_table(child);

    assert(page_table_unmap(
        child_table,
        0x4000ULL
    ) == 0);

    /*
     * O mapeamento foi removido apenas da tabela do filho.
     */
    assert(page_table_lookup(
        child_table,
        0x4000ULL
    ) == NULL);

    assert(page_table_lookup(
        parent_table,
        0x4000ULL
    ) != NULL);

    /*
     * O segundo mapeamento continua disponível nos dois.
     */
    assert(page_table_lookup(
        child_table,
        0x5000ULL
    ) != NULL);

    assert(page_table_lookup(
        parent_table,
        0x5000ULL
    ) != NULL);

    process_destroy(child);
    process_destroy(parent);
}

static void test_parent_modification_does_not_change_child(void)
{
    Process *parent = process_create(40);

    assert(parent != NULL);

    PageTable *parent_table =
        process_get_page_table(parent);

    assert(page_table_map(
        parent_table,
        0x6000ULL,
        10
    ) == 0);

    Process *child = process_fork(parent, 41);

    assert(child != NULL);

    PageTable *child_table =
        process_get_page_table(child);

    assert(page_table_map(
        parent_table,
        0x7000ULL,
        11
    ) == 0);

    assert(page_table_lookup(
        parent_table,
        0x7000ULL
    ) != NULL);

    assert(page_table_lookup(
        child_table,
        0x7000ULL
    ) == NULL);

    assert(page_table_lookup(
        child_table,
        0x6000ULL
    ) != NULL);

    process_destroy(parent);

    /*
     * A tabela do filho deve continuar válida após a
     * destruição do processo pai.
     */
    const PageTableEntry *child_entry =
        page_table_lookup(
            child_table,
            0x6000ULL
        );

    assert(child_entry != NULL);
    assert(child_entry->frame_number == 10);

    process_destroy(child);
}

static void test_multiple_forks(void)
{
    Process *parent = process_create(50);

    assert(parent != NULL);

    PageTable *parent_table =
        process_get_page_table(parent);

    assert(page_table_map(
        parent_table,
        0x8000ULL,
        12
    ) == 0);

    Process *first_child =
        process_fork(parent, 51);

    Process *second_child =
        process_fork(parent, 52);

    assert(first_child != NULL);
    assert(second_child != NULL);

    PageTable *first_table =
        process_get_page_table(first_child);

    PageTable *second_table =
        process_get_page_table(second_child);

    assert(page_table_root_reference_count(
        parent_table
    ) == 3);

    assert(page_table_shares_root(
        parent_table,
        first_table
    ));

    assert(page_table_shares_root(
        parent_table,
        second_table
    ));

    /*
     * Apenas o primeiro filho modifica sua tabela.
     */
    assert(page_table_map(
        first_table,
        0x9000ULL,
        13
    ) == 0);

    assert(page_table_lookup(
        first_table,
        0x9000ULL
    ) != NULL);

    assert(page_table_lookup(
        parent_table,
        0x9000ULL
    ) == NULL);

    assert(page_table_lookup(
        second_table,
        0x9000ULL
    ) == NULL);

    /*
     * Pai e segundo filho continuam compartilhando a raiz.
     */
    assert(page_table_shares_root(
        parent_table,
        second_table
    ));

    assert(!page_table_shares_root(
        parent_table,
        first_table
    ));

    process_destroy(first_child);
    process_destroy(second_child);
    process_destroy(parent);
}

int main(void)
{
    test_create_and_destroy_process();
    test_invalid_pid();
    test_null_process_operations();
    test_process_page_table();
    test_processes_have_independent_page_tables();

    test_invalid_fork();
    test_fork_shares_root_initially();
    test_destroy_parent_before_child();
    test_child_map_triggers_copy_on_write();
    test_child_remap_does_not_change_parent();
    test_child_unmap_does_not_change_parent();
    test_parent_modification_does_not_change_child();
    test_multiple_forks();

    printf("Todos os testes de process passaram.\n");

    return 0;
}
