#include <limits.h>
#include <stdlib.h>

#include "tlb.h"

typedef struct {
    bool valid;

    int pid;
    uint64_t virtual_page;
    uint32_t frame_number;

    uint64_t last_access;
} TlbEntry;

struct Tlb {
    TlbEntry *entries;

    size_t capacity;
    size_t size;

    uint64_t logical_clock;
};

Tlb *tlb_create(size_t capacity)
{
    if (capacity == 0) {
        return NULL;
    }

    Tlb *tlb = calloc(1, sizeof(Tlb));

    if (tlb == NULL) {
        return NULL;
    }

    tlb->entries = calloc(
        capacity,
        sizeof(TlbEntry)
    );

    if (tlb->entries == NULL) {
        free(tlb);
        return NULL;
    }

    tlb->capacity = capacity;

    return tlb;
}

void tlb_destroy(Tlb *tlb)
{
    if (tlb == NULL) {
        return;
    }

    free(tlb->entries);
    free(tlb);
}

static bool tlb_arguments_are_valid(
    const Tlb *tlb,
    int pid
)
{
    return (
        tlb != NULL &&
        tlb->entries != NULL &&
        pid >= 0
    );
}

static void tlb_register_access(
    Tlb *tlb,
    TlbEntry *entry
)
{
    tlb->logical_clock++;
    entry->last_access = tlb->logical_clock;
}

bool tlb_lookup(
    Tlb *tlb,
    int pid,
    uint64_t virtual_page,
    uint32_t *frame_number
)
{
    if (
        !tlb_arguments_are_valid(tlb, pid) ||
        frame_number == NULL
    ) {
        return false;
    }

    for (
        size_t index = 0;
        index < tlb->capacity;
        index++
    ) {
        TlbEntry *entry = &tlb->entries[index];

        if (
            entry->valid &&
            entry->pid == pid &&
            entry->virtual_page == virtual_page
        ) {
            *frame_number = entry->frame_number;

            tlb_register_access(tlb, entry);

            return true;
        }
    }

    return false;
}

static TlbEntry *tlb_find_existing_entry(
    Tlb *tlb,
    int pid,
    uint64_t virtual_page
)
{
    for (
        size_t index = 0;
        index < tlb->capacity;
        index++
    ) {
        TlbEntry *entry = &tlb->entries[index];

        if (
            entry->valid &&
            entry->pid == pid &&
            entry->virtual_page == virtual_page
        ) {
            return entry;
        }
    }

    return NULL;
}

static TlbEntry *tlb_find_free_entry(Tlb *tlb)
{
    for (
        size_t index = 0;
        index < tlb->capacity;
        index++
    ) {
        if (!tlb->entries[index].valid) {
            return &tlb->entries[index];
        }
    }

    return NULL;
}

static TlbEntry *tlb_find_lru_entry(Tlb *tlb)
{
    TlbEntry *victim = NULL;
    uint64_t oldest_access = UINT64_MAX;

    for (
        size_t index = 0;
        index < tlb->capacity;
        index++
    ) {
        TlbEntry *entry = &tlb->entries[index];

        if (
            entry->valid &&
            entry->last_access < oldest_access
        ) {
            oldest_access = entry->last_access;
            victim = entry;
        }
    }

    return victim;
}

int tlb_insert(
    Tlb *tlb,
    int pid,
    uint64_t virtual_page,
    uint32_t frame_number
)
{
    if (!tlb_arguments_are_valid(tlb, pid)) {
        return -1;
    }

    TlbEntry *entry = tlb_find_existing_entry(
        tlb,
        pid,
        virtual_page
    );

    if (entry != NULL) {
        entry->frame_number = frame_number;
        tlb_register_access(tlb, entry);

        return 0;
    }

    entry = tlb_find_free_entry(tlb);

    if (entry == NULL) {
        entry = tlb_find_lru_entry(tlb);
    } else {
        tlb->size++;
    }

    if (entry == NULL) {
        return -1;
    }

    entry->valid = true;
    entry->pid = pid;
    entry->virtual_page = virtual_page;
    entry->frame_number = frame_number;

    tlb_register_access(tlb, entry);

    return 0;
}

bool tlb_invalidate(
    Tlb *tlb,
    int pid,
    uint64_t virtual_page
)
{
    if (!tlb_arguments_are_valid(tlb, pid)) {
        return false;
    }

    TlbEntry *entry = tlb_find_existing_entry(
        tlb,
        pid,
        virtual_page
    );

    if (entry == NULL) {
        return false;
    }

    entry->valid = false;
    entry->pid = -1;
    entry->virtual_page = 0;
    entry->frame_number = 0;
    entry->last_access = 0;

    tlb->size--;

    return true;
}

size_t tlb_invalidate_process(
    Tlb *tlb,
    int pid
)
{
    if (!tlb_arguments_are_valid(tlb, pid)) {
        return 0;
    }

    size_t invalidated = 0;

    for (
        size_t index = 0;
        index < tlb->capacity;
        index++
    ) {
        TlbEntry *entry = &tlb->entries[index];

        if (!entry->valid || entry->pid != pid) {
            continue;
        }

        entry->valid = false;
        entry->pid = -1;
        entry->virtual_page = 0;
        entry->frame_number = 0;
        entry->last_access = 0;

        invalidated++;
    }

    tlb->size -= invalidated;

    return invalidated;
}

void tlb_clear(Tlb *tlb)
{
    if (tlb == NULL || tlb->entries == NULL) {
        return;
    }

    for (
        size_t index = 0;
        index < tlb->capacity;
        index++
    ) {
        tlb->entries[index].valid = false;
        tlb->entries[index].pid = -1;
        tlb->entries[index].virtual_page = 0;
        tlb->entries[index].frame_number = 0;
        tlb->entries[index].last_access = 0;
    }

    tlb->size = 0;
    tlb->logical_clock = 0;
}

size_t tlb_capacity(const Tlb *tlb)
{
    if (tlb == NULL) {
        return 0;
    }

    return tlb->capacity;
}

size_t tlb_size(const Tlb *tlb)
{
    if (tlb == NULL) {
        return 0;
    }

    return tlb->size;
}