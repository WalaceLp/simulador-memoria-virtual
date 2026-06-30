#include <stdlib.h>

#include "physical_memory.h"
#include "process.h"

struct PhysicalMemory {
    PhysicalFrame *frames;
    size_t frame_count;
    size_t free_frame_count;
};

static void physical_frame_reset(
    PhysicalFrame *frame
)
{
    if (frame == NULL) {
        return;
    }

    frame->occupied = false;
    frame->dirty = false;
    frame->referenced = false;
    frame->owner_pid = -1;
    frame->owner_process = NULL;
    frame->virtual_page = 0;
}

static void physical_frame_assign(
    PhysicalFrame *frame,
    Process *owner_process,
    uint64_t virtual_page
)
{
    frame->occupied = true;
    frame->dirty = false;
    frame->referenced = false;
    frame->owner_pid = process_get_pid(owner_process);
    frame->owner_process = owner_process;
    frame->virtual_page = virtual_page;
}

PhysicalMemory *physical_memory_create(
    size_t frame_count
)
{
    if (frame_count == 0) {
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

    memory->frame_count = frame_count;
    memory->free_frame_count = frame_count;

    for (size_t index = 0; index < frame_count; index++) {
        physical_frame_reset(&memory->frames[index]);
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
        PhysicalFrame *frame = &memory->frames[index];

        if (frame->occupied) {
            continue;
        }

        physical_frame_assign(
            frame,
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

    PhysicalFrame *frame =
        &memory->frames[frame_number];

    if (!frame->occupied) {
        return -1;
    }

    physical_frame_assign(
        frame,
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

    PhysicalFrame *frame =
        &memory->frames[frame_number];

    if (!frame->occupied) {
        return -1;
    }

    physical_frame_reset(frame);
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
        frame_number >= memory->frame_count
    ) {
        return -1;
    }

    PhysicalFrame *frame =
        &memory->frames[frame_number];

    if (!frame->occupied) {
        return -1;
    }

    frame->referenced = true;

    if (is_write) {
        frame->dirty = true;
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