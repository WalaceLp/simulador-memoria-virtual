#ifndef SWAP_H
#define SWAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Swap Swap;

Swap *swap_create(
    const char *path,
    size_t slot_count,
    bool remove_file_on_destroy
);

void swap_destroy(Swap *swap);

int swap_write_page(
    Swap *swap,
    int pid,
    uint64_t virtual_page,
    const uint8_t *page_data
);

int swap_read_page(
    Swap *swap,
    int pid,
    uint64_t virtual_page,
    uint8_t *page_data
);

bool swap_contains_page(
    const Swap *swap,
    int pid,
    uint64_t virtual_page
);

int swap_remove_page(
    Swap *swap,
    int pid,
    uint64_t virtual_page
);

size_t swap_slot_count(const Swap *swap);

size_t swap_used_slot_count(const Swap *swap);

uint64_t swap_read_count(const Swap *swap);

uint64_t swap_write_count(const Swap *swap);

size_t swap_remove_process(
    Swap *swap,
    int pid
);

#endif