#include <stdlib.h>
#include <string.h>

#include "address.h"
#include "physical_memory.h"
#include "process.h"

struct PhysicalMemory {
    PhysicalFrame *frames;
    uint8_t *data;

    size_t frame_count;
    size_t free_frame_count;
};

static uint8_t *physical_memory_frame_data(
    PhysicalMemory *memory,
    uint32_t frame_number
)
{
    return memory->data +
           ((size_t)frame_number * PAGE_SIZE);
}

static const uint8_t *physical_memory_frame_data_const(
    const PhysicalMemory *memory,
    uint32_t frame_number
)
{
    return memory->data +
           ((size_t)frame_number * PAGE_SIZE);
}

static void physical_frame_reset(
    PhysicalMemory *memory,
    uint32_t frame_number
)
{
    PhysicalFrame *frame =
        &memory->frames[frame_number];

    frame->occupied = false;
    frame->dirty = false;
    frame->referenced = false;
    frame->owner_pid = -1;
    frame->owner_process = NULL;
    frame->virtual_page = 0;

    memset(
        physical_memory_frame_data(
            memory,
            frame_number
        ),
        0,
        PAGE_SIZE
    );
}

static void physical_frame_assign(
    PhysicalMemory *memory,
    uint32_t frame_number,
    Process *owner_process,
    uint64_t virtual_page
)
{
    PhysicalFrame *frame =
        &memory->frames[frame_number];

    frame->occupied = true;
    frame->dirty = false;
    frame->referenced = false;
    frame->owner_pid = process_get_pid(owner_process);
    frame->owner_process = owner_process;
    frame->virtual_page = virtual_page;

    memset(
        physical_memory_frame_data(
            memory,
            frame_number
        ),
        0,
        PAGE_SIZE
    );
}

PhysicalMemory *physical_memory_create(
    size_t frame_count
)
{
    if (
        frame_count == 0 ||
        frame_count > SIZE_MAX / PAGE_SIZE
    ) {
        return NULL;
    }

    PhysicalMemory *memory = calloc(
        1,
        sizeof(PhysicalMemory)
    );

    if (memory == NULL) {
        return NULL;
    }

    memory->frames = calloc(
        frame_count,
        sizeof(PhysicalFrame)
    );

    if (memory->frames == NULL) {
        free(memory);
        return NULL;
    }

    memory->data = calloc(
        frame_count,
        PAGE_SIZE
    );

    if (memory->data == NULL) {
        free(memory->frames);
        free(memory);
        return NULL;
    }

    memory->frame_count = frame_count;
    memory->free_frame_count = frame_count;

    for (
        size_t index = 0;
        index < frame_count;
        index++
    ) {
        physical_frame_reset(
            memory,
            (uint32_t)index
        );
    }

    return memory;
}

void physical_memory_destroy(
    PhysicalMemory *memory
)
{
    if (memory == NULL) {
        return;
    }

    free(memory->data);
    free(memory->frames);
    free(memory);
}

size_t physical_memory_frame_count(
    const PhysicalMemory *memory
)
{
    if (memory == NULL) {
        return 0;
    }

    return memory->frame_count;
}

size_t physical_memory_free_frame_count(
    const PhysicalMemory *memory
)
{
    if (memory == NULL) {
        return 0;
    }

    return memory->free_frame_count;
}

int physical_memory_allocate_frame(
    PhysicalMemory *memory,
    Process *owner_process,
    uint64_t virtual_page,
    uint32_t *frame_number
)
{
    if (
        memory == NULL ||
        owner_process == NULL ||
        frame_number == NULL ||
        process_get_pid(owner_process) < 0
    ) {
        return -1;
    }

    if (memory->free_frame_count == 0) {
        return -2;
    }

    for (
        size_t index = 0;
        index < memory->frame_count;
        index++
    ) {
        if (memory->frames[index].occupied) {
            continue;
        }

        physical_frame_assign(
            memory,
            (uint32_t)index,
            owner_process,
            virtual_page
        );

        memory->free_frame_count--;

        *frame_number = (uint32_t)index;

        return 0;
    }

    return -2;
}

int physical_memory_replace_frame(
    PhysicalMemory *memory,
    uint32_t frame_number,
    Process *new_owner_process,
    uint64_t new_virtual_page
)
{
    if (
        memory == NULL ||
        new_owner_process == NULL ||
        frame_number >= memory->frame_count ||
        process_get_pid(new_owner_process) < 0
    ) {
        return -1;
    }

    if (!memory->frames[frame_number].occupied) {
        return -1;
    }

    physical_frame_assign(
        memory,
        frame_number,
        new_owner_process,
        new_virtual_page
    );

    return 0;
}

int physical_memory_release_frame(
    PhysicalMemory *memory,
    uint32_t frame_number
)
{
    if (
        memory == NULL ||
        frame_number >= memory->frame_count
    ) {
        return -1;
    }

    if (!memory->frames[frame_number].occupied) {
        return -1;
    }

    physical_frame_reset(memory, frame_number);
    memory->free_frame_count++;

    return 0;
}

int physical_memory_mark_access(
    PhysicalMemory *memory,
    uint32_t frame_number,
    bool is_write
)
{
    if (
        memory == NULL ||
        frame_number >= memory->frame_count ||
        !memory->frames[frame_number].occupied
    ) {
        return -1;
    }

    memory->frames[frame_number].referenced = true;

    if (is_write) {
        memory->frames[frame_number].dirty = true;
    }

    return 0;
}

const PhysicalFrame *physical_memory_get_frame(
    const PhysicalMemory *memory,
    uint32_t frame_number
)
{
    if (
        memory == NULL ||
        frame_number >= memory->frame_count
    ) {
        return NULL;
    }

    return &memory->frames[frame_number];
}

int physical_memory_read_page(
    const PhysicalMemory *memory,
    uint32_t frame_number,
    uint8_t *page_data,
    size_t page_size
)
{
    if (
        memory == NULL ||
        page_data == NULL ||
        page_size != PAGE_SIZE ||
        frame_number >= memory->frame_count ||
        !memory->frames[frame_number].occupied
    ) {
        return -1;
    }

    memcpy(
        page_data,
        physical_memory_frame_data_const(
            memory,
            frame_number
        ),
        PAGE_SIZE
    );

    return 0;
}

int physical_memory_write_page(
    PhysicalMemory *memory,
    uint32_t frame_number,
    const uint8_t *page_data,
    size_t page_size
)
{
    if (
        memory == NULL ||
        page_data == NULL ||
        page_size != PAGE_SIZE ||
        frame_number >= memory->frame_count ||
        !memory->frames[frame_number].occupied
    ) {
        return -1;
    }

    memcpy(
        physical_memory_frame_data(
            memory,
            frame_number
        ),
        page_data,
        PAGE_SIZE
    );

    return 0;
}

int physical_memory_read_byte(
    const PhysicalMemory *memory,
    uint32_t frame_number,
    uint16_t offset,
    uint8_t *value
)
{
    if (
        memory == NULL ||
        value == NULL ||
        frame_number >= memory->frame_count ||
        offset >= PAGE_SIZE ||
        !memory->frames[frame_number].occupied
    ) {
        return -1;
    }

    const uint8_t *data =
        physical_memory_frame_data_const(
            memory,
            frame_number
        );

    *value = data[offset];

    return 0;
}

int physical_memory_write_byte(
    PhysicalMemory *memory,
    uint32_t frame_number,
    uint16_t offset,
    uint8_t value
)
{
    if (
        memory == NULL ||
        frame_number >= memory->frame_count ||
        offset >= PAGE_SIZE ||
        !memory->frames[frame_number].occupied
    ) {
        return -1;
    }

    uint8_t *data =
        physical_memory_frame_data(
            memory,
            frame_number
        );

    data[offset] = value;

    return 0;
}