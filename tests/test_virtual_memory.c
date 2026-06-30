#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "page_table.h"
#include "physical_memory.h"
#include "process.h"
#include "virtual_memory.h"

static void test_create_and_destroy(void)
{
    VirtualMemory *memory =
        virtual_memory_create(4);

    assert(memory != NULL);

    const PhysicalMemory *physical =
        virtual_memory_get_physical_memory(memory);

    assert(physical != NULL);

    assert(
        physical_memory_frame_count(physical) == 4
    );

    virtual_memory_destroy(memory);
}

static void test_invalid_creation(void)
{
    assert(virtual_memory_create(0) == NULL);
}

static void test_first_access_causes_page_fault(void)
{
    VirtualMemory *memory =
        virtual_memory_create(4);

    Process *process =
        process_create(1);

    assert(memory != NULL);
    assert(process != NULL);

    VirtualMemoryAccessResult result =
        virtual_memory_access(
            memory,
            process,
            0x1000ULL,
            VM_ACCESS_READ
        );

    assert(result == VM_ACCESS_PAGE_FAULT);

    const PageTableEntry *entry =
        page_table_lookup(
            process_get_page_table(process),
            0x1000ULL
        );

    assert(entry != NULL);

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->total_accesses == 1);
    assert(stats->read_accesses == 1);
    assert(stats->write_accesses == 0);
    assert(stats->page_faults == 1);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_second_access_is_hit(void)
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
            0x2000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x2000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_OK
    );

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->total_accesses == 2);
    assert(stats->page_faults == 1);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_same_page_different_offsets(void)
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
            0x3000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x3123ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_OK
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x3FFFULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_OK
    );

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->total_accesses == 3);
    assert(stats->page_faults == 1);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_write_marks_frame_dirty(void)
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
            0x4000ULL,
            VM_ACCESS_WRITE
        ) == VM_ACCESS_PAGE_FAULT
    );

    const PageTableEntry *entry =
        page_table_lookup(
            process_get_page_table(process),
            0x4000ULL
        );

    assert(entry != NULL);

    const PhysicalMemory *physical =
        virtual_memory_get_physical_memory(memory);

    const PhysicalFrame *frame =
        physical_memory_get_frame(
            physical,
            entry->frame_number
        );

    assert(frame != NULL);
    assert(frame->occupied);
    assert(frame->referenced);
    assert(frame->dirty);

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->write_accesses == 1);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_no_free_frame(void)
{
    VirtualMemory *memory =
        virtual_memory_create(1);

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
            0x6000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_NO_FREE_FRAME
    );

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->total_accesses == 2);
    assert(stats->page_faults == 2);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_different_processes_use_different_frames(void)
{
    VirtualMemory *memory =
        virtual_memory_create(2);

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
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    const PageTableEntry *first_entry =
        page_table_lookup(
            process_get_page_table(first),
            0x1000ULL
        );

    const PageTableEntry *second_entry =
        page_table_lookup(
            process_get_page_table(second),
            0x1000ULL
        );

    assert(first_entry != NULL);
    assert(second_entry != NULL);

    assert(
        first_entry->frame_number !=
        second_entry->frame_number
    );

    const PhysicalMemory *physical =
        virtual_memory_get_physical_memory(memory);

    const PhysicalFrame *first_frame =
        physical_memory_get_frame(
            physical,
            first_entry->frame_number
        );

    const PhysicalFrame *second_frame =
        physical_memory_get_frame(
            physical,
            second_entry->frame_number
        );

    assert(first_frame != NULL);
    assert(second_frame != NULL);

    assert(first_frame->owner_pid == 1);
    assert(second_frame->owner_pid == 2);

    process_destroy(first);
    process_destroy(second);
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
    test_create_and_destroy();
    test_invalid_creation();
    test_first_access_causes_page_fault();
    test_second_access_is_hit();
    test_same_page_different_offsets();
    test_write_marks_frame_dirty();
    test_no_free_frame();
    test_different_processes_use_different_frames();
    test_invalid_arguments();

    printf(
        "Todos os testes de virtual_memory passaram.\n"
    );

    return 0;
}