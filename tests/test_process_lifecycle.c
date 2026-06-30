#include <assert.h>
#include <stdio.h>

#include "page_table.h"
#include "physical_memory.h"
#include "process.h"
#include "replacement.h"
#include "swap.h"
#include "virtual_memory.h"

#define PROCESS_LIFECYCLE_SWAP \
    "bin/process_lifecycle.swap"

static void test_release_process_frames(void)
{
    VirtualMemory *memory =
        virtual_memory_create_complete(
            4,
            REPLACEMENT_FIFO,
            4,
            PROCESS_LIFECYCLE_SWAP,
            16,
            true
        );

    Process *process =
        process_create(10);

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
            VM_ACCESS_WRITE
        ) == VM_ACCESS_PAGE_FAULT
    );

    const PhysicalMemory *physical =
        virtual_memory_get_physical_memory(
            memory
        );

    assert(
        physical_memory_free_frame_count(
            physical
        ) == 2
    );

    assert(
        virtual_memory_release_process(
            memory,
            process
        ) == 0
    );

    assert(
        physical_memory_free_frame_count(
            physical
        ) == 4
    );

    assert(
        page_table_lookup(
            process_get_page_table(process),
            0x1000ULL
        ) == NULL
    );

    assert(
        page_table_lookup(
            process_get_page_table(process),
            0x2000ULL
        ) == NULL
    );

    assert(virtual_memory_validate(memory));

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_release_only_selected_process(void)
{
    VirtualMemory *memory =
        virtual_memory_create_complete(
            4,
            REPLACEMENT_LRU,
            4,
            PROCESS_LIFECYCLE_SWAP,
            16,
            true
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
        virtual_memory_release_process(
            memory,
            first
        ) == 0
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

    assert(virtual_memory_validate(memory));

    process_destroy(first);

    assert(
        virtual_memory_access(
            memory,
            second,
            0x2000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_OK
    );

    assert(
        virtual_memory_release_process(
            memory,
            second
        ) == 0
    );

    process_destroy(second);
    virtual_memory_destroy(memory);
}

int main(void)
{
    test_release_process_frames();
    test_release_only_selected_process();

    printf(
        "Todos os testes de ciclo de vida passaram.\n"
    );

    return 0;
}