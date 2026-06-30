#include <stdbool.h>
#include <stdlib.h>

#include "address.h"
#include "page_table.h"
#include "physical_memory.h"
#include "process.h"
#include "replacement.h"
#include "virtual_memory.h"

struct VirtualMemory {
    PhysicalMemory *physical_memory;
    ReplacementPolicy *replacement_policy;

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

VirtualMemory *virtual_memory_create_with_policy(
    size_t frame_count,
    ReplacementPolicyType policy_type
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

    memory->replacement_policy =
        replacement_policy_create(
            policy_type,
            frame_count
        );

    if (memory->replacement_policy == NULL) {
        physical_memory_destroy(
            memory->physical_memory
        );

        free(memory);

        return NULL;
    }

    return memory;
}

VirtualMemory *virtual_memory_create(
    size_t frame_count
)
{
    return virtual_memory_create_with_policy(
        frame_count,
        REPLACEMENT_FIFO
    );
}

void virtual_memory_destroy(
    VirtualMemory *memory
)
{
    if (memory == NULL) {
        return;
    }

    replacement_policy_destroy(
        memory->replacement_policy
    );

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

static int virtual_memory_notify_access(
    VirtualMemory *memory,
    uint32_t frame_number,
    bool is_write
)
{
    if (
        physical_memory_mark_access(
            memory->physical_memory,
            frame_number,
            is_write
        ) != 0
    ) {
        return -1;
    }

    if (
        replacement_policy_on_access(
            memory->replacement_policy,
            frame_number
        ) != 0
    ) {
        return -1;
    }

    replacement_policy_tick(
        memory->replacement_policy
    );

    return 0;
}

static int virtual_memory_load_free_frame(
    VirtualMemory *memory,
    Process *process,
    uint64_t virtual_address,
    uint32_t frame_number,
    bool is_write
)
{
    PageTable *page_table =
        process_get_page_table(process);

    if (
        page_table_map(
            page_table,
            virtual_address,
            frame_number
        ) != 0
    ) {
        physical_memory_release_frame(
            memory->physical_memory,
            frame_number
        );

        return -1;
    }

    if (
        replacement_policy_on_load(
            memory->replacement_policy,
            frame_number
        ) != 0
    ) {
        page_table_unmap(
            page_table,
            virtual_address
        );

        physical_memory_release_frame(
            memory->physical_memory,
            frame_number
        );

        return -1;
    }

    if (
        physical_memory_mark_access(
            memory->physical_memory,
            frame_number,
            is_write
        ) != 0
    ) {
        replacement_policy_on_evict(
            memory->replacement_policy,
            frame_number
        );

        page_table_unmap(
            page_table,
            virtual_address
        );

        physical_memory_release_frame(
            memory->physical_memory,
            frame_number
        );

        return -1;
    }

    replacement_policy_tick(
        memory->replacement_policy
    );

    return 0;
}

static int virtual_memory_replace_page(
    VirtualMemory *memory,
    Process *new_process,
    uint64_t new_virtual_address,
    uint64_t new_virtual_page,
    bool is_write
)
{
    uint32_t victim_frame =
        replacement_policy_choose_victim(
            memory->replacement_policy
        );

    if (victim_frame == REPLACEMENT_INVALID_FRAME) {
        return -1;
    }

    const PhysicalFrame *victim =
        physical_memory_get_frame(
            memory->physical_memory,
            victim_frame
        );

    if (
        victim == NULL ||
        !victim->occupied ||
        victim->owner_process == NULL
    ) {
        return -1;
    }

    Process *old_process =
        victim->owner_process;

    uint64_t old_virtual_page =
        victim->virtual_page;

    bool victim_was_dirty =
        victim->dirty;

    PageTable *old_page_table =
        process_get_page_table(old_process);

    PageTable *new_page_table =
        process_get_page_table(new_process);

    if (
        old_page_table == NULL ||
        new_page_table == NULL
    ) {
        return -1;
    }

    uint64_t old_virtual_address =
        old_virtual_page << PAGE_OFFSET_BITS;

    /*
     * Primeiro cria o novo mapeamento.
     *
     * Caso a alocação interna da Trie falhe, o
     * mapeamento antigo ainda permanece intacto.
     */
    if (
        page_table_map(
            new_page_table,
            new_virtual_address,
            victim_frame
        ) != 0
    ) {
        return -1;
    }

    /*
     * Depois remove o mapeamento da página vítima.
     */
    if (
        page_table_unmap(
            old_page_table,
            old_virtual_address
        ) != 0
    ) {
        page_table_unmap(
            new_page_table,
            new_virtual_address
        );

        return -1;
    }

    if (
        replacement_policy_on_evict(
            memory->replacement_policy,
            victim_frame
        ) != 0
    ) {
        return -1;
    }

    if (
        physical_memory_replace_frame(
            memory->physical_memory,
            victim_frame,
            new_process,
            new_virtual_page
        ) != 0
    ) {
        return -1;
    }

    if (
        replacement_policy_on_load(
            memory->replacement_policy,
            victim_frame
        ) != 0
    ) {
        return -1;
    }

    if (
        physical_memory_mark_access(
            memory->physical_memory,
            victim_frame,
            is_write
        ) != 0
    ) {
        return -1;
    }

    memory->stats.replacements++;

    if (victim_was_dirty) {
        memory->stats.dirty_evictions++;
    }

    replacement_policy_tick(
        memory->replacement_policy
    );

    return 0;
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
     * Hit na tabela de páginas.
     */
    if (entry != NULL) {
        if (
            virtual_memory_notify_access(
                memory,
                entry->frame_number,
                is_write
            ) != 0
        ) {
            return VM_ACCESS_INTERNAL_ERROR;
        }

        return VM_ACCESS_OK;
    }

    /*
     * A página não está presente.
     */
    memory->stats.page_faults++;

    uint64_t virtual_page =
        address_page_number(virtual_address);

    uint32_t frame_number =
        INVALID_FRAME_NUMBER;

    int allocation_result =
        physical_memory_allocate_frame(
            memory->physical_memory,
            process,
            virtual_page,
            &frame_number
        );

    /*
     * Existe um quadro livre.
     */
    if (allocation_result == 0) {
        if (
            virtual_memory_load_free_frame(
                memory,
                process,
                virtual_address,
                frame_number,
                is_write
            ) != 0
        ) {
            return VM_ACCESS_INTERNAL_ERROR;
        }

        return VM_ACCESS_PAGE_FAULT;
    }

    /*
     * Memória cheia: escolher e substituir uma vítima.
     */
    if (allocation_result == -2) {
        if (
            virtual_memory_replace_page(
                memory,
                process,
                virtual_address,
                virtual_page,
                is_write
            ) != 0
        ) {
            return VM_ACCESS_INTERNAL_ERROR;
        }

        return VM_ACCESS_PAGE_FAULT;
    }

    return VM_ACCESS_INTERNAL_ERROR;
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

const char *virtual_memory_policy_name(
    const VirtualMemory *memory
)
{
    if (memory == NULL) {
        return "invalid";
    }

    return replacement_policy_name(
        memory->replacement_policy
    );
}