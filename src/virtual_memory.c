#include <stdbool.h>
#include <stdlib.h>

#include "address.h"
#include "page_table.h"
#include "physical_memory.h"
#include "process.h"
#include "replacement.h"
#include "tlb.h"
#include "virtual_memory.h"

#define DEFAULT_TLB_ENTRIES 16

struct VirtualMemory {
    PhysicalMemory *physical_memory;
    ReplacementPolicy *replacement_policy;
    Tlb *tlb;

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

VirtualMemory *virtual_memory_create_with_policy_and_tlb(
    size_t frame_count,
    ReplacementPolicyType policy_type,
    size_t tlb_entries
)
{
    if (frame_count == 0 || tlb_entries == 0) {
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

    memory->tlb = tlb_create(tlb_entries);

    if (memory->tlb == NULL) {
        replacement_policy_destroy(
            memory->replacement_policy
        );

        physical_memory_destroy(
            memory->physical_memory
        );

        free(memory);

        return NULL;
    }

    return memory;
}

VirtualMemory *virtual_memory_create_with_policy(
    size_t frame_count,
    ReplacementPolicyType policy_type
)
{
    return virtual_memory_create_with_policy_and_tlb(
        frame_count,
        policy_type,
        DEFAULT_TLB_ENTRIES
    );
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

    tlb_destroy(memory->tlb);

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
    uint64_t virtual_page,
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

    if (
        tlb_insert(
            memory->tlb,
            process_get_pid(process),
            virtual_page,
            frame_number
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

    int old_pid = victim->owner_pid;

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
     * A entrada antiga da TLB deve ser removida antes
     * que o quadro passe a representar outra página.
     */
    tlb_invalidate(
        memory->tlb,
        old_pid,
        old_virtual_page
    );

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

    if (
        tlb_insert(
            memory->tlb,
            process_get_pid(new_process),
            new_virtual_page,
            victim_frame
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

    int pid = process_get_pid(process);

    uint64_t virtual_page =
        address_page_number(virtual_address);

    bool is_write =
        access_type == VM_ACCESS_WRITE;

    uint32_t frame_number =
        INVALID_FRAME_NUMBER;

    /*
     * Primeiro consulta a TLB.
     */
    if (
        tlb_lookup(
            memory->tlb,
            pid,
            virtual_page,
            &frame_number
        )
    ) {
        memory->stats.tlb_hits++;

        if (
            virtual_memory_notify_access(
                memory,
                frame_number,
                is_write
            ) != 0
        ) {
            return VM_ACCESS_INTERNAL_ERROR;
        }

        return VM_ACCESS_OK;
    }

    memory->stats.tlb_misses++;
    memory->stats.page_walks++;

    size_t levels_visited = 0;

    const PageTableEntry *entry =
        page_table_lookup_with_levels(
            page_table,
            virtual_address,
            &levels_visited
        );

    memory->stats.page_walk_levels +=
        levels_visited;

    /*
     * TLB miss, mas a página estava na tabela.
     */
    if (entry != NULL) {
        frame_number = entry->frame_number;

        if (
            tlb_insert(
                memory->tlb,
                pid,
                virtual_page,
                frame_number
            ) != 0
        ) {
            return VM_ACCESS_INTERNAL_ERROR;
        }

        if (
            virtual_memory_notify_access(
                memory,
                frame_number,
                is_write
            ) != 0
        ) {
            return VM_ACCESS_INTERNAL_ERROR;
        }

        return VM_ACCESS_OK;
    }

    /*
     * A página não está presente na tabela.
     */
    memory->stats.page_faults++;

    int allocation_result =
        physical_memory_allocate_frame(
            memory->physical_memory,
            process,
            virtual_page,
            &frame_number
        );

    if (allocation_result == 0) {
        if (
            virtual_memory_load_free_frame(
                memory,
                process,
                virtual_address,
                virtual_page,
                frame_number,
                is_write
            ) != 0
        ) {
            return VM_ACCESS_INTERNAL_ERROR;
        }

        return VM_ACCESS_PAGE_FAULT;
    }

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

double virtual_memory_tlb_hit_ratio(
    const VirtualMemory *memory
)
{
    if (memory == NULL) {
        return 0.0;
    }

    uint64_t total =
        memory->stats.tlb_hits +
        memory->stats.tlb_misses;

    if (total == 0) {
        return 0.0;
    }

    return (double)memory->stats.tlb_hits /
           (double)total;
}

double virtual_memory_average_page_walk_levels(
    const VirtualMemory *memory
)
{
    if (
        memory == NULL ||
        memory->stats.page_walks == 0
    ) {
        return 0.0;
    }

    return
        (double)memory->stats.page_walk_levels /
        (double)memory->stats.page_walks;
}