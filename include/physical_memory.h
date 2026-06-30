#ifndef PHYSICAL_MEMORY_H
#define PHYSICAL_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define INVALID_FRAME_NUMBER UINT32_MAX

typedef struct Process Process;

typedef struct {
    bool occupied;
    bool dirty;
    bool referenced;

    int owner_pid;
    Process *owner_process;

    uint64_t virtual_page;
} PhysicalFrame;

typedef struct PhysicalMemory PhysicalMemory;

PhysicalMemory *physical_memory_create(
    size_t frame_count
);

void physical_memory_destroy(
    PhysicalMemory *memory
);

size_t physical_memory_frame_count(
    const PhysicalMemory *memory
);

size_t physical_memory_free_frame_count(
    const PhysicalMemory *memory
);

int physical_memory_allocate_frame(
    PhysicalMemory *memory,
    Process *owner_process,
    uint64_t virtual_page,
    uint32_t *frame_number
);

int physical_memory_replace_frame(
    PhysicalMemory *memory,
    uint32_t frame_number,
    Process *new_owner_process,
    uint64_t new_virtual_page
);

int physical_memory_release_frame(
    PhysicalMemory *memory,
    uint32_t frame_number
);

int physical_memory_mark_access(
    PhysicalMemory *memory,
    uint32_t frame_number,
    bool is_write
);

const PhysicalFrame *physical_memory_get_frame(
    const PhysicalMemory *memory,
    uint32_t frame_number
);

int physical_memory_read_page(
    const PhysicalMemory *memory,
    uint32_t frame_number,
    uint8_t *page_data,
    size_t page_size
);

int physical_memory_write_page(
    PhysicalMemory *memory,
    uint32_t frame_number,
    const uint8_t *page_data,
    size_t page_size
);

int physical_memory_read_byte(
    const PhysicalMemory *memory,
    uint32_t frame_number,
    uint16_t offset,
    uint8_t *value
);

int physical_memory_write_byte(
    PhysicalMemory *memory,
    uint32_t frame_number,
    uint16_t offset,
    uint8_t value
);

#endif