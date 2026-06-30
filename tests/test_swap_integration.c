#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "process.h"
#include "replacement.h"
#include "swap.h"
#include "virtual_memory.h"

#define INTEGRATION_SWAP_PATH \
    "bin/test_swap_integration.bin"

static void test_dirty_page_returns_from_swap(void)
{
    VirtualMemory *memory =
        virtual_memory_create_complete(
            1,
            REPLACEMENT_FIFO,
            2,
            INTEGRATION_SWAP_PATH,
            4,
            true
        );

    Process *process = process_create(1);

    assert(memory != NULL);
    assert(process != NULL);

    /*
     * Carrega a primeira página e escreve um valor nela.
     */
    assert(
        virtual_memory_write_byte(
            memory,
            process,
            0x1123ULL,
            0xAB
        ) == 0
    );

    /*
     * Com apenas um quadro, este acesso remove a página
     * anterior. Como ela está suja, deve ir para o swap.
     */
    assert(
        virtual_memory_access(
            memory,
            process,
            0x2000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    const Swap *swap =
        virtual_memory_get_swap(memory);

    assert(swap != NULL);
    assert(swap_used_slot_count(swap) == 1);

    /*
     * O acesso à primeira página causa swap-in.
     */
    uint8_t value = 0;

    assert(
        virtual_memory_read_byte(
            memory,
            process,
            0x1123ULL,
            &value
        ) == 0
    );

    assert(value == 0xAB);

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->swap_writes == 1);
    assert(stats->swap_reads == 1);
    assert(stats->dirty_evictions == 1);
    assert(stats->replacements == 2);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

static void test_clean_page_is_not_written_to_swap(void)
{
    VirtualMemory *memory =
        virtual_memory_create_complete(
            1,
            REPLACEMENT_FIFO,
            2,
            INTEGRATION_SWAP_PATH,
            4,
            true
        );

    Process *process = process_create(2);

    assert(memory != NULL);
    assert(process != NULL);

    assert(
        virtual_memory_access(
            memory,
            process,
            0x1000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    assert(
        virtual_memory_access(
            memory,
            process,
            0x2000ULL,
            VM_ACCESS_READ
        ) == VM_ACCESS_PAGE_FAULT
    );

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    assert(stats != NULL);
    assert(stats->swap_writes == 0);
    assert(stats->dirty_evictions == 0);

    process_destroy(process);
    virtual_memory_destroy(memory);
}

int main(void)
{
    test_dirty_page_returns_from_swap();
    test_clean_page_is_not_written_to_swap();

    printf(
        "Todos os testes de integração com swap passaram.\n"
    );

    return 0;
}