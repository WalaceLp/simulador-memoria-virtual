#include <stdbool.h>
#include <stdlib.h>

#include "address.h"
#include "page_table.h"
#include "physical_memory.h"
#include "process.h"
#include "virtual_memory.h"

struct VirtualMemory {
    PhysicalMemory *physical_memory;
    VirtualMemoryStats stats;
};

static bool virtual_memory_access_type_is_valid(
    VirtualMemoryAccessType access_type
)
{
    return (
        access_type == VM_ACCESS_READ ||
        access_type == VM_ACCESS_WRITE
    );
}

VirtualMemory *virtual_memory_create(
    size_t frame_count
)
{
    if (frame_count == 0) {
        return NULL;
    }

    VirtualMemory *memory = calloc(
        1,
        sizeof(VirtualMemory)
    );

    if (memory == NULL) {
        return NULL;
    }

    memory->physical_memory =
        physical_memory_create(frame_count);

    if (memory->physical_memory == NULL) {
        free(memory);
        return NULL;
    }

    return memory;
}

void virtual_memory_destroy(
    VirtualMemory *memory
)
{
    if (memory == NULL) {
        return;
    }

    physical_memory_destroy(
        memory->physical_memory
    );

    free(memory);
}

static void virtual_memory_register_access(
    VirtualMemory *memory,
    VirtualMemoryAccessType access_type
)
{
    memory->stats.total_accesses++;

    if (access_type == VM_ACCESS_READ) {
        memory->stats.read_accesses++;
    } else {
        memory->stats.write_accesses++;
    }
}

VirtualMemoryAccessResult virtual_memory_access(
    VirtualMemory *memory,
    Process *process,
    uint64_t virtual_address,
    VirtualMemoryAccessType access_type
)
{
    if (
        memory == NULL ||
        process == NULL ||
        !virtual_memory_access_type_is_valid(access_type)
    ) {
        return VM_ACCESS_INVALID_ARGUMENT;
    }

    PageTable *page_table =
        process_get_page_table(process);

    if (page_table == NULL) {
        return VM_ACCESS_INTERNAL_ERROR;
    }

    virtual_memory_register_access(
        memory,
        access_type
    );

    const PageTableEntry *entry =
        page_table_lookup(
            page_table,
            virtual_address
        );

    bool is_write =
        access_type == VM_ACCESS_WRITE;

    /*
     * Caso a página já esteja mapeada, o acesso
     * não provoca page fault.
     */
    if (entry != NULL) {
        int result = physical_memory_mark_access(
            memory->physical_memory,
            entry->frame_number,
            is_write
        );

        if (result != 0) {
            return VM_ACCESS_INTERNAL_ERROR;
        }

        return VM_ACCESS_OK;
    }

    /*
     * A página não está presente na tabela:
     * ocorreu um page fault.
     */
    memory->stats.page_faults++;

    uint32_t frame_number =
        INVALID_FRAME_NUMBER;

    uint64_t virtual_page =
        address_page_number(virtual_address);

    int allocation_result =
        physical_memory_allocate_frame(
            memory->physical_memory,
            process_get_pid(process),
            virtual_page,
            &frame_number
        );

    if (allocation_result == -2) {
        return VM_ACCESS_NO_FREE_FRAME;
    }

    if (allocation_result != 0) {
        return VM_ACCESS_INTERNAL_ERROR;
    }

    /*
     * Associa a página virtual ao quadro recém-alocado.
     */
    int mapping_result = page_table_map(
        page_table,
        virtual_address,
        frame_number
    );

    if (mapping_result != 0) {
        physical_memory_release_frame(
            memory->physical_memory,
            frame_number
        );

        return VM_ACCESS_INTERNAL_ERROR;
    }

    /*
     * Registra o acesso inicial ao quadro.
     */
    int access_result = physical_memory_mark_access(
        memory->physical_memory,
        frame_number,
        is_write
    );

    if (access_result != 0) {
        page_table_unmap(
            page_table,
            virtual_address
        );

        physical_memory_release_frame(
            memory->physical_memory,
            frame_number
        );

        return VM_ACCESS_INTERNAL_ERROR;
    }

    return VM_ACCESS_PAGE_FAULT;
}

const VirtualMemoryStats *virtual_memory_get_stats(
    const VirtualMemory *memory
)
{
    if (memory == NULL) {
        return NULL;
    }

    return &memory->stats;
}

const PhysicalMemory *virtual_memory_get_physical_memory(
    const VirtualMemory *memory
)
{
    if (memory == NULL) {
        return NULL;
    }

    return memory->physical_memory;
}