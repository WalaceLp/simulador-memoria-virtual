#include <stdbool.h>
#include <stdlib.h>

#include "address.h"
#include "page_table.h"
#include "physical_memory.h"
#include "process.h"
#include "replacement.h"
#include "swap.h"
#include "tlb.h"
#include "virtual_memory.h"

#define DEFAULT_TLB_ENTRIES 16

struct VirtualMemory {
    PhysicalMemory *physical_memory;
    ReplacementPolicy *replacement_policy;
    Tlb *tlb;
    Swap *swap;

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

static VirtualMemory *virtual_memory_create_internal(
    size_t frame_count,
    ReplacementPolicyType policy_type,
    size_t tlb_entries,
    const char *swap_path,
    size_t swap_slots,
    bool remove_swap_on_destroy
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

    if (swap_path != NULL && swap_slots > 0) {
        memory->swap = swap_create(
            swap_path,
            swap_slots,
            remove_swap_on_destroy
        );

        if (memory->swap == NULL) {
            tlb_destroy(memory->tlb);

            replacement_policy_destroy(
                memory->replacement_policy
            );

            physical_memory_destroy(
                memory->physical_memory
            );

            free(memory);

            return NULL;
        }
    }

    return memory;
}

VirtualMemory *virtual_memory_create_complete(
    size_t frame_count,
    ReplacementPolicyType policy_type,
    size_t tlb_entries,
    const char *swap_path,
    size_t swap_slots,
    bool remove_swap_on_destroy
)
{
    if (swap_path == NULL || swap_slots == 0) {
        return NULL;
    }

    return virtual_memory_create_internal(
        frame_count,
        policy_type,
        tlb_entries,
        swap_path,
        swap_slots,
        remove_swap_on_destroy
    );
}

VirtualMemory *virtual_memory_create_with_policy_and_tlb(
    size_t frame_count,
    ReplacementPolicyType policy_type,
    size_t tlb_entries
)
{
    return virtual_memory_create_internal(
        frame_count,
        policy_type,
        tlb_entries,
        NULL,
        0,
        false
    );
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

    swap_destroy(memory->swap);
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

static int virtual_memory_restore_page(
    VirtualMemory *memory,
    Process *process,
    uint64_t virtual_page,
    uint32_t frame_number
)
{
    if (
        memory->swap == NULL ||
        !swap_contains_page(
            memory->swap,
            process_get_pid(process),
            virtual_page
        )
    ) {
        return 0;
    }

    uint8_t page_data[PAGE_SIZE];

    if (
        swap_read_page(
            memory->swap,
            process_get_pid(process),
            virtual_page,
            page_data
        ) != 0
    ) {
        return -1;
    }

    if (
        physical_memory_write_page(
            memory->physical_memory,
            frame_number,
            page_data,
            PAGE_SIZE
        ) != 0
    ) {
        return -1;
    }

    if (
        swap_remove_page(
            memory->swap,
            process_get_pid(process),
            virtual_page
        ) != 0
    ) {
        return -1;
    }

    memory->stats.swap_reads++;

    return 0;
}

static int virtual_memory_save_victim(
    VirtualMemory *memory,
    const PhysicalFrame *victim,
    uint32_t victim_frame
)
{
    if (
        memory == NULL ||
        victim == NULL ||
        !victim->dirty
    ) {
        return 0;
    }

    if (memory->swap == NULL) {
        return 0;
    }

    uint8_t page_data[PAGE_SIZE];

    if (
        physical_memory_read_page(
            memory->physical_memory,
            victim_frame,
            page_data,
            PAGE_SIZE
        ) != 0
    ) {
        return -1;
    }

    if (
        swap_write_page(
            memory->swap,
            victim->owner_pid,
            victim->virtual_page,
            page_data
        ) != 0
    ) {
        return -1;
    }

    memory->stats.swap_writes++;

    return 0;
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
        virtual_memory_restore_page(
            memory,
            process,
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
        tlb_insert(
            memory->tlb,
            process_get_pid(process),
            virtual_page,
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

    if (
        virtual_memory_save_victim(
            memory,
            victim,
            victim_frame
        ) != 0
    ) {
        return -1;
    }

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
        virtual_memory_restore_page(
            memory,
            new_process,
            new_virtual_page,
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
            return memory->swap != NULL
                ? VM_ACCESS_SWAP_ERROR
                : VM_ACCESS_INTERNAL_ERROR;
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
            return memory->swap != NULL
                ? VM_ACCESS_SWAP_ERROR
                : VM_ACCESS_INTERNAL_ERROR;
        }

        return VM_ACCESS_PAGE_FAULT;
    }

    return VM_ACCESS_INTERNAL_ERROR;
}

int virtual_memory_read_byte(
    VirtualMemory *memory,
    Process *process,
    uint64_t virtual_address,
    uint8_t *value
)
{
    if (value == NULL) {
        return -1;
    }

    VirtualMemoryAccessResult result =
        virtual_memory_access(
            memory,
            process,
            virtual_address,
            VM_ACCESS_READ
        );

    if (result < 0) {
        return -1;
    }

    const PageTableEntry *entry =
        page_table_lookup(
            process_get_page_table(process),
            virtual_address
        );

    if (entry == NULL) {
        return -1;
    }

    return physical_memory_read_byte(
        memory->physical_memory,
        entry->frame_number,
        address_offset(virtual_address),
        value
    );
}

int virtual_memory_write_byte(
    VirtualMemory *memory,
    Process *process,
    uint64_t virtual_address,
    uint8_t value
)
{
    VirtualMemoryAccessResult result =
        virtual_memory_access(
            memory,
            process,
            virtual_address,
            VM_ACCESS_WRITE
        );

    if (result < 0) {
        return -1;
    }

    const PageTableEntry *entry =
        page_table_lookup(
            process_get_page_table(process),
            virtual_address
        );

    if (entry == NULL) {
        return -1;
    }

    return physical_memory_write_byte(
        memory->physical_memory,
        entry->frame_number,
        address_offset(virtual_address),
        value
    );
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

const Swap *virtual_memory_get_swap(
    const VirtualMemory *memory
)
{
    if (memory == NULL) {
        return NULL;
    }

    return memory->swap;
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

bool virtual_memory_validate(
    const VirtualMemory *memory
)
{
    if (
        memory == NULL ||
        memory->physical_memory == NULL ||
        memory->replacement_policy == NULL ||
        memory->tlb == NULL
    ) {
        return false;
    }

    size_t frame_count =
        physical_memory_frame_count(
            memory->physical_memory
        );

    size_t counted_free_frames = 0;

    for (
        size_t index = 0;
        index < frame_count;
        index++
    ) {
        const PhysicalFrame *frame =
            physical_memory_get_frame(
                memory->physical_memory,
                (uint32_t)index
            );

        if (frame == NULL) {
            return false;
        }

        if (!frame->occupied) {
            counted_free_frames++;

            if (
                frame->owner_pid != -1 ||
                frame->owner_process != NULL ||
                frame->dirty ||
                frame->referenced
            ) {
                return false;
            }

            continue;
        }

        if (
            frame->owner_process == NULL ||
            frame->owner_pid < 0
        ) {
            return false;
        }

        if (
            process_get_pid(frame->owner_process) !=
            frame->owner_pid
        ) {
            return false;
        }

        PageTable *page_table =
            process_get_page_table(
                frame->owner_process
            );

        if (page_table == NULL) {
            return false;
        }

        if (
            frame->virtual_page >
            (UINT64_MAX >> PAGE_OFFSET_BITS)
        ) {
            return false;
        }

        uint64_t virtual_address =
            frame->virtual_page <<
            PAGE_OFFSET_BITS;

        const PageTableEntry *entry =
            page_table_lookup(
                page_table,
                virtual_address
            );

        if (
            entry == NULL ||
            !entry->present ||
            entry->frame_number !=
                (uint32_t)index
        ) {
            return false;
        }
    }

    return counted_free_frames ==
           physical_memory_free_frame_count(
               memory->physical_memory
           );
}

int virtual_memory_release_process(
    VirtualMemory *memory,
    Process *process
)
{
    if (
        memory == NULL ||
        process == NULL ||
        memory->physical_memory == NULL ||
        memory->replacement_policy == NULL ||
        memory->tlb == NULL
    ) {
        return -1;
    }

    int pid = process_get_pid(process);

    if (pid < 0) {
        return -1;
    }

    PageTable *page_table =
        process_get_page_table(process);

    if (page_table == NULL) {
        return -1;
    }

    size_t frame_count =
        physical_memory_frame_count(
            memory->physical_memory
        );

    for (
        size_t index = 0;
        index < frame_count;
        index++
    ) {
        const PhysicalFrame *frame =
            physical_memory_get_frame(
                memory->physical_memory,
                (uint32_t)index
            );

        if (
            frame == NULL ||
            !frame->occupied ||
            frame->owner_process != process
        ) {
            continue;
        }

        uint64_t virtual_address =
            frame->virtual_page <<
            PAGE_OFFSET_BITS;

        tlb_invalidate(
            memory->tlb,
            pid,
            frame->virtual_page
        );

        const PageTableEntry *entry =
            page_table_lookup(
                page_table,
                virtual_address
            );

        if (entry != NULL) {
            if (
                page_table_unmap(
                    page_table,
                    virtual_address
                ) != 0
            ) {
                return -1;
            }
        }

        if (
            replacement_policy_on_evict(
                memory->replacement_policy,
                (uint32_t)index
            ) != 0
        ) {
            return -1;
        }

        if (
            physical_memory_release_frame(
                memory->physical_memory,
                (uint32_t)index
            ) != 0
        ) {
            return -1;
        }
    }

    tlb_invalidate_process(
        memory->tlb,
        pid
    );

    if (memory->swap != NULL) {
        swap_remove_process(
            memory->swap,
            pid
        );
    }

    return 0;
}