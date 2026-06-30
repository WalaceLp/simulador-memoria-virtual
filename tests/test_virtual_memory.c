#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "page_table.h"
#include "physical_memory.h"
#include "process.h"
#include "replacement.h"
#include "virtual_memory.h"

static void test_create_with_policy(void)
{
    VirtualMemory *memory =
        virtual_memory_create_with_policy(
            3,
            REPLACEMENT_LRU
        );

    assert(memory != NULL);
    assert(
        strcmp(
            virtual_memory_policy_name(memory),
            "LRU"
        ) == 0
    );

    virtual_memory_destroy(memory);
}

static void test_first_access_causes_page_fault(void)
{
    VirtualMemory *memory =
        virtual_memory_create(4);

    Process *process =
        process_create(1);

    assert(memory != NULL);
    assert(process != NULL);

    assert(
        virtual_memory_access(
            memory,
            process,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_OK
    );

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->total_accesses == 2);
    assert(stats->page_faults == 1);
    assert(stats->replacements == 0);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_fifo_replacement(void)
{
    VirtualMemory *memory =
        virtual_memory_create_with_policy(
            3,
            REPLACEMENT_FIFO
        );

    Process *process =
        process_create(1);

    assert(memory != NULL);
    assert(process != NULL);

    assert(
        virtual_memory_access(
            memory,
            process,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x2000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x3000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    /*
     * FIFO deve remover a primeira página carregada.
     */
    assert(
        virtual_memory_access(
            memory,
            process,
            0x4000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    PageTable *table =
        process_get_page_table(process);

    assert(page_table_lookup(table, 0x1000ULL) == NULL);
    assert(page_table_lookup(table, 0x2000ULL) != NULL);
    assert(page_table_lookup(table, 0x3000ULL) != NULL);
    assert(page_table_lookup(table, 0x4000ULL) != NULL);

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->page_faults == 4);
    assert(stats->replacements == 1);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_lru_replacement(void)
{
    VirtualMemory *memory =
        virtual_memory_create_with_policy(
            3,
            REPLACEMENT_LRU
        );

    Process *process =
        process_create(1);

    assert(memory != NULL);
    assert(process != NULL);

    assert(
        virtual_memory_access(
            memory,
            process,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x2000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x3000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    /*
     * A página 0x1000 volta a ser a mais recente.
     */
    assert(
        virtual_memory_access(
            memory,
            process,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_OK
    );

    /*
     * A página 0x2000 passa a ser a menos recente.
     */
    assert(
        virtual_memory_access(
            memory,
            process,
            0x4000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    PageTable *table =
        process_get_page_table(process);

    assert(page_table_lookup(table, 0x1000ULL) != NULL);
    assert(page_table_lookup(table, 0x2000ULL) == NULL);
    assert(page_table_lookup(table, 0x3000ULL) != NULL);
    assert(page_table_lookup(table, 0x4000ULL) != NULL);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_clock_replacement(void)
{
    VirtualMemory *memory =
        virtual_memory_create_with_policy(
            2,
            REPLACEMENT_CLOCK
        );

    Process *process =
        process_create(1);

    assert(memory != NULL);
    assert(process != NULL);

    assert(
        virtual_memory_access(
            memory,
            process,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x2000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x3000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->replacements == 1);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_aging_replacement(void)
{
    VirtualMemory *memory =
        virtual_memory_create_with_policy(
            2,
            REPLACEMENT_AGING
        );

    Process *process =
        process_create(1);

    assert(memory != NULL);
    assert(process != NULL);

    assert(
        virtual_memory_access(
            memory,
            process,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x2000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    /*
     * Mantém 0x2000 mais recente.
     */
    assert(
        virtual_memory_access(
            memory,
            process,
            0x2000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_OK
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x3000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->replacements == 1);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_dirty_eviction(void)
{
    VirtualMemory *memory =
        virtual_memory_create_with_policy(
            1,
            REPLACEMENT_FIFO
        );

    Process *process =
        process_create(1);

    assert(memory != NULL);
    assert(process != NULL);

    assert(
        virtual_memory_access(
            memory,
            process,
            0x1000ULL,
            VM_ACCESS_WRITE
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x2000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->replacements == 1);
    assert(stats->dirty_evictions == 1);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_replacement_between_processes(void)
{
    VirtualMemory *memory =
        virtual_memory_create_with_policy(
            1,
            REPLACEMENT_FIFO
        );

    Process *first =
        process_create(1);

    Process *second =
        process_create(2);

    assert(memory != NULL);
    assert(first != NULL);
    assert(second != NULL);

    assert(
        virtual_memory_access(
            memory,
            first,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            second,
            0x2000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        page_table_lookup(
            process_get_page_table(first),
            0x1000ULL
        ) == NULL
    );

    assert(
        page_table_lookup(
            process_get_page_table(second),
            0x2000ULL
        ) != NULL
    );

    const PhysicalMemory *physical =
        virtual_memory_get_physical_memory(memory);

    const PhysicalFrame *frame =
        physical_memory_get_frame(
            physical,
            0
        );

    assert(frame != NULL);
    assert(frame->owner_pid == 2);
    assert(frame->owner_process == second);

    process_destroy(first);
    process_destroy(second);
    virtual_memory_destroy(memory);
}

static void test_same_page_different_offsets(void)
{
    VirtualMemory *memory =
        virtual_memory_create(2);

    Process *process =
        process_create(1);

    assert(memory != NULL);
    assert(process != NULL);

    assert(
        virtual_memory_access(
            memory,
            process,
            0x5000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x5123ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_OK
    );

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->page_faults == 1);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_invalid_arguments(void)
{
    VirtualMemory *memory =
        virtual_memory_create(1);

    Process *process =
        process_create(1);

    assert(memory != NULL);
    assert(process != NULL);

    assert(
        virtual_memory_access(
            NULL,
            process,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_INVALID_ARGUMENT
    );

    assert(
        virtual_memory_access(
            memory,
            NULL,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_INVALID_ARGUMENT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x1000ULL,
            (VirtualMemoryAccessType)99
        ) == VM_ACCESS_INVALID_ARGUMENT
    );

    process_destroy(process);
    virtual_memory_destroy(memory);
}

int main(void)
{
    test_create_with_policy();
    test_first_access_causes_page_fault();
    test_fifo_replacement();
    test_lru_replacement();
    test_clock_replacement();
    test_aging_replacement();
    test_dirty_eviction();
    test_replacement_between_processes();
    test_same_page_different_offsets();
    test_invalid_arguments();

    printf(
        "Todos os testes de virtual_memory passaram.\n"
    );

    return 0;
}