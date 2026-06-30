#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "page_table.h"
#include "physical_memory.h"
#include "process.h"
#include "replacement.h"
#include "virtual_memory.h"

#define COW_SWAP_PATH \
    "bin/cow_integration.swap"

static void test_fork_shares_page_until_write(void)
{
    VirtualMemory *memory =
        virtual_memory_create_complete(
            4,
            REPLACEMENT_LRU,
            4,
            COW_SWAP_PATH,
            16,
            true
        );

    Process *parent =
        process_create(1);

    assert(memory != NULL);
    assert(parent != NULL);

    assert(
        virtual_memory_write_byte(
            memory,
            parent,
            0x1123ULL,
            10
        ) == 0
    );

    const PageTableEntry *parent_before =
        page_table_lookup(
            process_get_page_table(parent),
            0x1123ULL
        );

    assert(parent_before != NULL);

    uint32_t shared_frame =
        parent_before->frame_number;

    Process *child = NULL;

    assert(
        virtual_memory_fork_process(
            memory,
            parent,
            2,
            &child
        ) == 0
    );

    assert(child != NULL);

    const PageTableEntry *child_before =
        page_table_lookup(
            process_get_page_table(child),
            0x1123ULL
        );

    assert(child_before != NULL);

    assert(
        child_before->frame_number ==
        shared_frame
    );

    uint8_t value = 0;

    assert(
        virtual_memory_read_byte(
            memory,
            child,
            0x1123ULL,
            &value
        ) == 0
    );

    assert(value == 10);

    /*
     * Primeira escrita do filho provoca COW físico.
     */
    assert(
        virtual_memory_write_byte(
            memory,
            child,
            0x1123ULL,
            20
        ) == 0
    );

    const PageTableEntry *parent_after =
        page_table_lookup(
            process_get_page_table(parent),
            0x1123ULL
        );

    const PageTableEntry *child_after =
        page_table_lookup(
            process_get_page_table(child),
            0x1123ULL
        );

    assert(parent_after != NULL);
    assert(child_after != NULL);

    assert(
        parent_after->frame_number !=
        child_after->frame_number
    );

    value = 0;

    assert(
        virtual_memory_read_byte(
            memory,
            parent,
            0x1123ULL,
            &value
        ) == 0
    );

    assert(value == 10);

    value = 0;

    assert(
        virtual_memory_read_byte(
            memory,
            child,
            0x1123ULL,
            &value
        ) == 0
    );

    assert(value == 20);

    assert(virtual_memory_validate(memory));

    assert(
        virtual_memory_release_process(
            memory,
            child
        ) == 0
    );

    process_destroy(child);

    value = 0;

    assert(
        virtual_memory_read_byte(
            memory,
            parent,
            0x1123ULL,
            &value
        ) == 0
    );

    assert(value == 10);

    assert(
        virtual_memory_release_process(
            memory,
            parent
        ) == 0
    );

    process_destroy(parent);
    virtual_memory_destroy(memory);
}

static void test_parent_write_does_not_change_child(void)
{
    VirtualMemory *memory =
        virtual_memory_create_complete(
            4,
            REPLACEMENT_FIFO,
            4,
            COW_SWAP_PATH,
            16,
            true
        );

    Process *parent =
        process_create(10);

    assert(memory != NULL);
    assert(parent != NULL);

    assert(
        virtual_memory_write_byte(
            memory,
            parent,
            0x2050ULL,
            30
        ) == 0
    );

    Process *child = NULL;

    assert(
        virtual_memory_fork_process(
            memory,
            parent,
            11,
            &child
        ) == 0
    );

    assert(child != NULL);

    assert(
        virtual_memory_write_byte(
            memory,
            parent,
            0x2050ULL,
            40
        ) == 0
    );

    uint8_t parent_value = 0;
    uint8_t child_value = 0;

    assert(
        virtual_memory_read_byte(
            memory,
            parent,
            0x2050ULL,
            &parent_value
        ) == 0
    );

    assert(
        virtual_memory_read_byte(
            memory,
            child,
            0x2050ULL,
            &child_value
        ) == 0
    );

    assert(parent_value == 40);
    assert(child_value == 30);

    assert(
        virtual_memory_release_process(
            memory,
            parent
        ) == 0
    );

    process_destroy(parent);

    /*
     * O filho deve permanecer válido após a saída do pai.
     */
    child_value = 0;

    assert(
        virtual_memory_read_byte(
            memory,
            child,
            0x2050ULL,
            &child_value
        ) == 0
    );

    assert(child_value == 30);

    assert(
        virtual_memory_release_process(
            memory,
            child
        ) == 0
    );

    process_destroy(child);
    virtual_memory_destroy(memory);
}

static void test_cow_write_with_full_physical_memory(void)
{
    VirtualMemory *memory =
        virtual_memory_create_complete(
            1,
            REPLACEMENT_FIFO,
            2,
            COW_SWAP_PATH,
            8,
            true
        );

    Process *parent =
        process_create(20);

    assert(memory != NULL);
    assert(parent != NULL);

    assert(
        virtual_memory_write_byte(
            memory,
            parent,
            0x3123ULL,
            10
        ) == 0
    );

    Process *child = NULL;

    assert(
        virtual_memory_fork_process(
            memory,
            parent,
            21,
            &child
        ) == 0
    );

    assert(child != NULL);

    /*
     * Com apenas um quadro físico, a escrita COW do filho
     * precisa preservar a página antiga do pai no swap.
     */
    assert(
        virtual_memory_write_byte(
            memory,
            child,
            0x3123ULL,
            20
        ) == 0
    );

    uint8_t value = 0;

    assert(
        virtual_memory_read_byte(
            memory,
            child,
            0x3123ULL,
            &value
        ) == 0
    );

    assert(value == 20);

    value = 0;

    assert(
        virtual_memory_read_byte(
            memory,
            parent,
            0x3123ULL,
            &value
        ) == 0
    );

    assert(value == 10);

    value = 0;

    assert(
        virtual_memory_read_byte(
            memory,
            child,
            0x3123ULL,
            &value
        ) == 0
    );

    assert(value == 20);

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->swap_writes >= 2);
    assert(stats->swap_reads >= 2);

    assert(virtual_memory_validate(memory));

    assert(
        virtual_memory_release_process(
            memory,
            child
        ) == 0
    );

    process_destroy(child);

    assert(
        virtual_memory_release_process(
            memory,
            parent
        ) == 0
    );

    process_destroy(parent);
    virtual_memory_destroy(memory);
}

int main(void)
{
    test_fork_shares_page_until_write();
    test_parent_write_does_not_change_child();
    test_cow_write_with_full_physical_memory();

    printf(
        "Todos os testes integrados de COW passaram.\n"
    );

    return 0;
}
