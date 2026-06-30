#include <stdlib.h>

#include "physical_memory.h"

struct PhysicalMemory {
    PhysicalFrame *frames;
    size_t frame_count;
    size_t free_frame_count;
};

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
        memory->frames[index].owner_pid = -1;
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
    int owner_pid,
    uint64_t virtual_page,
    uint32_t *frame_number
)
{
    if (
        memory == NULL ||
        frame_number == NULL ||
        owner_pid < 0
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

        frame->occupied = true;
        frame->dirty = false;
        frame->referenced = false;
        frame->owner_pid = owner_pid;
        frame->virtual_page = virtual_page;

        memory->free_frame_count--;

        *frame_number = (uint32_t)index;

        return 0;
    }

    return -2;
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

    frame->occupied = false;
    frame->dirty = false;
    frame->referenced = false;
    frame->owner_pid = -1;
    frame->virtual_page = 0;

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