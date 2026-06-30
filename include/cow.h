#ifndef COW_H
#define COW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "process.h"

typedef struct CowManager CowManager;

CowManager *cow_manager_create(
    size_t frame_count
);

void cow_manager_destroy(
    CowManager *manager
);

int cow_manager_add_mapping(
    CowManager *manager,
    uint32_t frame_number,
    Process *process,
    uint64_t virtual_page
);

int cow_manager_remove_mapping(
    CowManager *manager,
    uint32_t frame_number,
    Process *process,
    uint64_t virtual_page
);

bool cow_manager_has_mapping(
    const CowManager *manager,
    uint32_t frame_number,
    const Process *process,
    uint64_t virtual_page
);

size_t cow_manager_mapping_count(
    const CowManager *manager,
    uint32_t frame_number
);

int cow_manager_share_process_pages(
    CowManager *manager,
    const Process *parent,
    Process *child
);

#endif