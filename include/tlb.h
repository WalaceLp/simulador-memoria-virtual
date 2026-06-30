#ifndef TLB_H
#define TLB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Tlb Tlb;

Tlb *tlb_create(size_t capacity);

void tlb_destroy(Tlb *tlb);

bool tlb_lookup(
    Tlb *tlb,
    int pid,
    uint64_t virtual_page,
    uint32_t *frame_number
);

int tlb_insert(
    Tlb *tlb,
    int pid,
    uint64_t virtual_page,
    uint32_t frame_number
);

bool tlb_invalidate(
    Tlb *tlb,
    int pid,
    uint64_t virtual_page
);

size_t tlb_invalidate_process(
    Tlb *tlb,
    int pid
);

void tlb_clear(Tlb *tlb);

size_t tlb_capacity(const Tlb *tlb);

size_t tlb_size(const Tlb *tlb);

#endif