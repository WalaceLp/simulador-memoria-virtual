#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "address.h"
#include "swap.h"

typedef struct {
    bool used;
    int pid;
    uint64_t virtual_page;
} SwapSlot;

struct Swap {
    FILE *file;
    char *path;

    SwapSlot *slots;
    size_t slot_count;
    size_t used_slot_count;

    bool remove_file_on_destroy;

    uint64_t reads;
    uint64_t writes;
};

static char *swap_duplicate_string(const char *source)
{
    if (source == NULL) {
        return NULL;
    }

    size_t length = strlen(source);

    char *copy = malloc(length + 1);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, source, length + 1);

    return copy;
}

static long swap_slot_offset(size_t slot_index)
{
    size_t offset = slot_index * PAGE_SIZE;

    if (offset > (size_t)LONG_MAX) {
        return -1;
    }

    return (long)offset;
}

static int swap_initialize_file(
    FILE *file,
    size_t slot_count
)
{
    if (
        file == NULL ||
        slot_count == 0 ||
        slot_count > SIZE_MAX / PAGE_SIZE
    ) {
        return -1;
    }

    size_t total_size = slot_count * PAGE_SIZE;

    if (total_size > (size_t)LONG_MAX) {
        return -1;
    }

    if (fseek(file, (long)(total_size - 1), SEEK_SET) != 0) {
        return -1;
    }

    if (fputc(0, file) == EOF) {
        return -1;
    }

    if (fflush(file) != 0) {
        return -1;
    }

    return 0;
}

Swap *swap_create(
    const char *path,
    size_t slot_count,
    bool remove_file_on_destroy
)
{
    if (path == NULL || slot_count == 0) {
        return NULL;
    }

    Swap *swap = calloc(1, sizeof(Swap));

    if (swap == NULL) {
        return NULL;
    }

    swap->path = swap_duplicate_string(path);

    if (swap->path == NULL) {
        free(swap);
        return NULL;
    }

    swap->slots = calloc(
        slot_count,
        sizeof(SwapSlot)
    );

    if (swap->slots == NULL) {
        free(swap->path);
        free(swap);
        return NULL;
    }

    swap->file = fopen(path, "w+b");

    if (swap->file == NULL) {
        free(swap->slots);
        free(swap->path);
        free(swap);
        return NULL;
    }

    if (swap_initialize_file(swap->file, slot_count) != 0) {
        fclose(swap->file);
        remove(path);

        free(swap->slots);
        free(swap->path);
        free(swap);

        return NULL;
    }

    swap->slot_count = slot_count;
    swap->remove_file_on_destroy =
        remove_file_on_destroy;

    return swap;
}

void swap_destroy(Swap *swap)
{
    if (swap == NULL) {
        return;
    }

    if (swap->file != NULL) {
        fclose(swap->file);
    }

    if (
        swap->remove_file_on_destroy &&
        swap->path != NULL
    ) {
        remove(swap->path);
    }

    free(swap->slots);
    free(swap->path);
    free(swap);
}

static size_t swap_find_page_slot(
    const Swap *swap,
    int pid,
    uint64_t virtual_page
)
{
    if (swap == NULL || pid < 0) {
        return SIZE_MAX;
    }

    for (
        size_t index = 0;
        index < swap->slot_count;
        index++
    ) {
        const SwapSlot *slot = &swap->slots[index];

        if (
            slot->used &&
            slot->pid == pid &&
            slot->virtual_page == virtual_page
        ) {
            return index;
        }
    }

    return SIZE_MAX;
}

static size_t swap_find_free_slot(
    const Swap *swap
)
{
    if (swap == NULL) {
        return SIZE_MAX;
    }

    for (
        size_t index = 0;
        index < swap->slot_count;
        index++
    ) {
        if (!swap->slots[index].used) {
            return index;
        }
    }

    return SIZE_MAX;
}

int swap_write_page(
    Swap *swap,
    int pid,
    uint64_t virtual_page,
    const uint8_t *page_data
)
{
    if (
        swap == NULL ||
        swap->file == NULL ||
        pid < 0 ||
        page_data == NULL
    ) {
        return -1;
    }

    size_t slot_index = swap_find_page_slot(
        swap,
        pid,
        virtual_page
    );

    bool is_new_slot = false;

    if (slot_index == SIZE_MAX) {
        slot_index = swap_find_free_slot(swap);
        is_new_slot = true;
    }

    if (slot_index == SIZE_MAX) {
        return -2;
    }

    long offset = swap_slot_offset(slot_index);

    if (offset < 0) {
        return -1;
    }

    if (fseek(swap->file, offset, SEEK_SET) != 0) {
        return -1;
    }

    size_t written = fwrite(
        page_data,
        1,
        PAGE_SIZE,
        swap->file
    );

    if (written != PAGE_SIZE) {
        return -1;
    }

    if (fflush(swap->file) != 0) {
        return -1;
    }

    if (is_new_slot) {
        swap->slots[slot_index].used = true;
        swap->slots[slot_index].pid = pid;
        swap->slots[slot_index].virtual_page =
            virtual_page;

        swap->used_slot_count++;
    }

    swap->writes++;

    return 0;
}

int swap_read_page(
    Swap *swap,
    int pid,
    uint64_t virtual_page,
    uint8_t *page_data
)
{
    if (
        swap == NULL ||
        swap->file == NULL ||
        pid < 0 ||
        page_data == NULL
    ) {
        return -1;
    }

    size_t slot_index = swap_find_page_slot(
        swap,
        pid,
        virtual_page
    );

    if (slot_index == SIZE_MAX) {
        return -2;
    }

    long offset = swap_slot_offset(slot_index);

    if (offset < 0) {
        return -1;
    }

    if (fseek(swap->file, offset, SEEK_SET) != 0) {
        return -1;
    }

    size_t read = fread(
        page_data,
        1,
        PAGE_SIZE,
        swap->file
    );

    if (read != PAGE_SIZE) {
        return -1;
    }

    swap->reads++;

    return 0;
}

bool swap_contains_page(
    const Swap *swap,
    int pid,
    uint64_t virtual_page
)
{
    return swap_find_page_slot(
        swap,
        pid,
        virtual_page
    ) != SIZE_MAX;
}

int swap_remove_page(
    Swap *swap,
    int pid,
    uint64_t virtual_page
)
{
    if (swap == NULL || pid < 0) {
        return -1;
    }

    size_t slot_index = swap_find_page_slot(
        swap,
        pid,
        virtual_page
    );

    if (slot_index == SIZE_MAX) {
        return -1;
    }

    swap->slots[slot_index].used = false;
    swap->slots[slot_index].pid = -1;
    swap->slots[slot_index].virtual_page = 0;

    swap->used_slot_count--;

    return 0;
}

size_t swap_slot_count(const Swap *swap)
{
    if (swap == NULL) {
        return 0;
    }

    return swap->slot_count;
}

size_t swap_used_slot_count(const Swap *swap)
{
    if (swap == NULL) {
        return 0;
    }

    return swap->used_slot_count;
}

uint64_t swap_read_count(const Swap *swap)
{
    if (swap == NULL) {
        return 0;
    }

    return swap->reads;
}

uint64_t swap_write_count(const Swap *swap)
{
    if (swap == NULL) {
        return 0;
    }

    return swap->writes;
}

size_t swap_remove_process(
    Swap *swap,
    int pid
)
{
    if (swap == NULL || pid < 0) {
        return 0;
    }

    size_t removed = 0;

    for (
        size_t index = 0;
        index < swap->slot_count;
        index++
    ) {
        SwapSlot *slot = &swap->slots[index];

        if (!slot->used || slot->pid != pid) {
            continue;
        }

        slot->used = false;
        slot->pid = -1;
        slot->virtual_page = 0;

        removed++;
    }

    swap->used_slot_count -= removed;

    return removed;
}