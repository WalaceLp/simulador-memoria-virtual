#include <stdlib.h>

#include "cow.h"
#include "page_table.h"

typedef struct CowMapping {
    Process *process;
    uint64_t virtual_page;

    struct CowMapping *next;
} CowMapping;

struct CowManager {
    CowMapping **frames;
    size_t frame_count;
};

CowManager *cow_manager_create(
    size_t frame_count
)
{
    if (frame_count == 0) {
        return NULL;
    }

    CowManager *manager = calloc(
        1,
        sizeof(CowManager)
    );

    if (manager == NULL) {
        return NULL;
    }

    manager->frames = calloc(
        frame_count,
        sizeof(CowMapping *)
    );

    if (manager->frames == NULL) {
        free(manager);
        return NULL;
    }

    manager->frame_count = frame_count;

    return manager;
}

void cow_manager_destroy(
    CowManager *manager
)
{
    if (manager == NULL) {
        return;
    }

    for (
        size_t frame = 0;
        frame < manager->frame_count;
        frame++
    ) {
        CowMapping *mapping =
            manager->frames[frame];

        while (mapping != NULL) {
            CowMapping *next =
                mapping->next;

            free(mapping);
            mapping = next;
        }
    }

    free(manager->frames);
    free(manager);
}

bool cow_manager_has_mapping(
    const CowManager *manager,
    uint32_t frame_number,
    const Process *process,
    uint64_t virtual_page
)
{
    if (
        manager == NULL ||
        process == NULL ||
        frame_number >= manager->frame_count
    ) {
        return false;
    }

    const CowMapping *mapping =
        manager->frames[frame_number];

    while (mapping != NULL) {
        if (
            mapping->process == process &&
            mapping->virtual_page == virtual_page
        ) {
            return true;
        }

        mapping = mapping->next;
    }

    return false;
}

int cow_manager_add_mapping(
    CowManager *manager,
    uint32_t frame_number,
    Process *process,
    uint64_t virtual_page
)
{
    if (
        manager == NULL ||
        process == NULL ||
        frame_number >= manager->frame_count
    ) {
        return -1;
    }

    if (
        cow_manager_has_mapping(
            manager,
            frame_number,
            process,
            virtual_page
        )
    ) {
        return 0;
    }

    CowMapping *mapping = calloc(
        1,
        sizeof(CowMapping)
    );

    if (mapping == NULL) {
        return -1;
    }

    mapping->process = process;
    mapping->virtual_page = virtual_page;
    mapping->next =
        manager->frames[frame_number];

    manager->frames[frame_number] = mapping;

    return 0;
}

int cow_manager_remove_mapping(
    CowManager *manager,
    uint32_t frame_number,
    Process *process,
    uint64_t virtual_page
)
{
    if (
        manager == NULL ||
        process == NULL ||
        frame_number >= manager->frame_count
    ) {
        return -1;
    }

    CowMapping **current =
        &manager->frames[frame_number];

    while (*current != NULL) {
        CowMapping *mapping = *current;

        if (
            mapping->process == process &&
            mapping->virtual_page == virtual_page
        ) {
            *current = mapping->next;
            free(mapping);

            return 0;
        }

        current = &mapping->next;
    }

    return -1;
}

size_t cow_manager_mapping_count(
    const CowManager *manager,
    uint32_t frame_number
)
{
    if (
        manager == NULL ||
        frame_number >= manager->frame_count
    ) {
        return 0;
    }

    size_t count = 0;

    const CowMapping *mapping =
        manager->frames[frame_number];

    while (mapping != NULL) {
        count++;
        mapping = mapping->next;
    }

    return count;
}

size_t cow_manager_collect_frame_mappings(
    const CowManager *manager,
    uint32_t frame_number,
    CowMappingInfo *mappings,
    size_t capacity
)
{
    if (
        manager == NULL ||
        frame_number >= manager->frame_count
    ) {
        return 0;
    }

    size_t count = 0;

    const CowMapping *mapping =
        manager->frames[frame_number];

    while (mapping != NULL) {
        if (mappings != NULL && count < capacity) {
            mappings[count].process = mapping->process;
            mappings[count].virtual_page =
                mapping->virtual_page;
        }

        count++;
        mapping = mapping->next;
    }

    return count;
}

int cow_manager_share_process_pages(
    CowManager *manager,
    const Process *parent,
    Process *child
)
{
    if (
        manager == NULL ||
        parent == NULL ||
        child == NULL
    ) {
        return -1;
    }

    const PageTable *parent_table =
        process_get_page_table(
            (Process *)parent
        );

    PageTable *child_table =
        process_get_page_table(child);

    if (
        parent_table == NULL ||
        child_table == NULL
    ) {
        return -1;
    }

    /*
     * O compartilhamento estrutural da Trie já foi
     * realizado por process_fork().
     *
     * Os registros físicos são adicionados pela memória
     * virtual, que conhece os quadros ocupados.
     */
    return 0;
}
