#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "address.h"
#include "process.h"
#include "replacement.h"
#include "virtual_memory.h"

#define STRESS_PAGE_COUNT 64
#define STRESS_ACCESS_COUNT 10000
#define STRESS_FRAME_COUNT 8
#define STRESS_TLB_ENTRIES 16
#define STRESS_SWAP_SLOTS 128

#define STRESS_SWAP_FIFO \
    "bin/stress_fifo.swap"

#define STRESS_SWAP_LRU \
    "bin/stress_lru.swap"

#define STRESS_SWAP_CLOCK \
    "bin/stress_clock.swap"

#define STRESS_SWAP_AGING \
    "bin/stress_aging.swap"

typedef struct {
    uint64_t state;
} StressRandom;

static uint64_t stress_random_next(
    StressRandom *random
)
{
    random->state =
        random->state *
        6364136223846793005ULL +
        1442695040888963407ULL;

    return random->state;
}

static const char *stress_swap_path(
    ReplacementPolicyType policy
)
{
    switch (policy) {
        case REPLACEMENT_FIFO:
            return STRESS_SWAP_FIFO;

        case REPLACEMENT_LRU:
            return STRESS_SWAP_LRU;

        case REPLACEMENT_CLOCK:
            return STRESS_SWAP_CLOCK;

        case REPLACEMENT_AGING:
            return STRESS_SWAP_AGING;

        default:
            return NULL;
    }
}

static void validate_statistics(
    const VirtualMemoryStats *stats
)
{
    assert(stats != NULL);

    assert(
        stats->total_accesses ==
        stats->read_accesses +
        stats->write_accesses
    );

    assert(
        stats->total_accesses ==
        stats->tlb_hits +
        stats->tlb_misses
    );

    assert(
        stats->page_walks ==
        stats->tlb_misses
    );

    assert(
        stats->page_faults <=
        stats->total_accesses
    );

    assert(
        stats->replacements <=
        stats->page_faults
    );

    assert(
        stats->dirty_evictions <=
        stats->replacements
    );

    assert(
        stats->swap_writes <=
        stats->dirty_evictions
    );

    assert(
        stats->swap_reads <=
        stats->page_faults
    );
}

static void run_policy_stress(
    ReplacementPolicyType policy,
    uint64_t seed
)
{
    const char *swap_path =
        stress_swap_path(policy);

    assert(swap_path != NULL);

    VirtualMemory *memory =
        virtual_memory_create_complete(
            STRESS_FRAME_COUNT,
            policy,
            STRESS_TLB_ENTRIES,
            swap_path,
            STRESS_SWAP_SLOTS,
            true
        );

    Process *process =
        process_create(
            100 + (int)policy
        );

    assert(memory != NULL);
    assert(process != NULL);
    assert(virtual_memory_validate(memory));

    uint8_t expected_values[
        STRESS_PAGE_COUNT
    ];

    memset(
        expected_values,
        0,
        sizeof(expected_values)
    );

    StressRandom random = {
        .state = seed
    };

    for (
        size_t access_index = 0;
        access_index < STRESS_ACCESS_COUNT;
        access_index++
    ) {
        uint64_t random_value =
            stress_random_next(&random);

        size_t page_index =
            (size_t)(
                random_value %
                STRESS_PAGE_COUNT
            );

        uint64_t virtual_address =
            0x1000ULL +
            (uint64_t)page_index *
            PAGE_SIZE;

        /*
         * Utiliza sempre o mesmo deslocamento dentro
         * da página para facilitar a verificação.
         */
        virtual_address += 123ULL;

        bool is_write =
            (random_value % 4ULL) == 0ULL;

        if (is_write) {
            uint8_t value =
                (uint8_t)(
                    stress_random_next(
                        &random
                    ) & 0xFFULL
                );

            assert(
                virtual_memory_write_byte(
                    memory,
                    process,
                    virtual_address,
                    value
                ) == 0
            );

            expected_values[page_index] =
                value;
        } else {
            uint8_t value = 0;

            assert(
                virtual_memory_read_byte(
                    memory,
                    process,
                    virtual_address,
                    &value
                ) == 0
            );

            assert(
                value ==
                expected_values[page_index]
            );
        }

        if (access_index % 100 == 0) {
            assert(
                virtual_memory_validate(memory)
            );
        }
    }

    assert(virtual_memory_validate(memory));

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    validate_statistics(stats);

    assert(
        stats->total_accesses ==
        STRESS_ACCESS_COUNT
    );

    assert(stats->page_faults > 0);
    assert(stats->replacements > 0);
    assert(stats->tlb_misses > 0);
    assert(stats->page_walks > 0);
    assert(stats->swap_writes > 0);
    assert(stats->swap_reads > 0);

    virtual_memory_destroy(memory);
    process_destroy(process);
}

static void test_all_policies_under_stress(void)
{
    run_policy_stress(
        REPLACEMENT_FIFO,
        1001ULL
    );

    run_policy_stress(
        REPLACEMENT_LRU,
        1002ULL
    );

    run_policy_stress(
        REPLACEMENT_CLOCK,
        1003ULL
    );

    run_policy_stress(
        REPLACEMENT_AGING,
        1004ULL
    );
}

static void test_repeated_sequential_accesses(void)
{
    VirtualMemory *memory =
        virtual_memory_create_complete(
            16,
            REPLACEMENT_LRU,
            8,
            "bin/stress_sequential.swap",
            128,
            true
        );

    Process *process =
        process_create(200);

    assert(memory != NULL);
    assert(process != NULL);

    for (
        size_t repetition = 0;
        repetition < 100;
        repetition++
    ) {
        for (
            size_t page = 0;
            page < 32;
            page++
        ) {
            uint64_t address =
                0x1000ULL +
                (uint64_t)page *
                PAGE_SIZE;

            VirtualMemoryAccessResult result =
                virtual_memory_access(
                    memory,
                    process,
                    address,
                    VM_ACCESS_READ
                );

            assert(result >= 0);
        }

        assert(virtual_memory_validate(memory));
    }

    const VirtualMemoryStats *stats =
        virtual_memory_get_stats(memory);

    validate_statistics(stats);

    assert(stats->total_accesses == 3200);
    assert(stats->page_faults > 0);

    virtual_memory_destroy(memory);
    process_destroy(process);
}

int main(void)
{
    test_all_policies_under_stress();
    test_repeated_sequential_accesses();

    printf(
        "Todos os testes de estresse passaram.\n"
    );

    return 0;
}