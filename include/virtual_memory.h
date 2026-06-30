#ifndef VIRTUAL_MEMORY_H
#define VIRTUAL_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#include "physical_memory.h"
#include "process.h"
#include "replacement.h"

typedef enum {
    VM_ACCESS_READ = 0,
    VM_ACCESS_WRITE = 1
} VirtualMemoryAccessType;

typedef enum {
    VM_ACCESS_OK = 0,
    VM_ACCESS_PAGE_FAULT = 1,

    VM_ACCESS_INVALID_ARGUMENT = -1,
    VM_ACCESS_NO_FREE_FRAME = -2,
    VM_ACCESS_INTERNAL_ERROR = -3
} VirtualMemoryAccessResult;

typedef struct {
    uint64_t total_accesses;
    uint64_t read_accesses;
    uint64_t write_accesses;

    uint64_t page_faults;
    uint64_t replacements;
    uint64_t dirty_evictions;

    uint64_t tlb_hits;
    uint64_t tlb_misses;

    uint64_t page_walks;
    uint64_t page_walk_levels;
} VirtualMemoryStats;

typedef struct VirtualMemory VirtualMemory;

VirtualMemory *virtual_memory_create(
    size_t frame_count
);

VirtualMemory *virtual_memory_create_with_policy(
    size_t frame_count,
    ReplacementPolicyType policy_type
);

VirtualMemory *virtual_memory_create_with_policy_and_tlb(
    size_t frame_count,
    ReplacementPolicyType policy_type,
    size_t tlb_entries
);

void virtual_memory_destroy(
    VirtualMemory *memory
);

VirtualMemoryAccessResult virtual_memory_access(
    VirtualMemory *memory,
    Process *process,
    uint64_t virtual_address,
    VirtualMemoryAccessType access_type
);

const VirtualMemoryStats *virtual_memory_get_stats(
    const VirtualMemory *memory
);

const PhysicalMemory *virtual_memory_get_physical_memory(
    const VirtualMemory *memory
);

const char *virtual_memory_policy_name(
    const VirtualMemory *memory
);

double virtual_memory_tlb_hit_ratio(
    const VirtualMemory *memory
);

double virtual_memory_average_page_walk_levels(
    const VirtualMemory *memory
);

#endif