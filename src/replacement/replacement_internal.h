#ifndef REPLACEMENT_INTERNAL_H
#define REPLACEMENT_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "replacement.h"

typedef struct {
    void (*destroy)(ReplacementPolicy *policy);

    int (*on_load)(
        ReplacementPolicy *policy,
        uint32_t frame_number
    );

    int (*on_access)(
        ReplacementPolicy *policy,
        uint32_t frame_number
    );

    int (*on_evict)(
        ReplacementPolicy *policy,
        uint32_t frame_number
    );

    uint32_t (*choose_victim)(
        ReplacementPolicy *policy
    );

    void (*tick)(
        ReplacementPolicy *policy
    );
} ReplacementPolicyOperations;

struct ReplacementPolicy {
    ReplacementPolicyType type;
    size_t frame_count;

    void *state;

    const ReplacementPolicyOperations *operations;
};

ReplacementPolicy *replacement_fifo_create(
    size_t frame_count
);

ReplacementPolicy *replacement_lru_create(
    size_t frame_count
);

ReplacementPolicy *replacement_clock_create(
    size_t frame_count
);

ReplacementPolicy *replacement_aging_create(
    size_t frame_count
);

ReplacementPolicy *replacement_policy_allocate(
    ReplacementPolicyType type,
    size_t frame_count,
    size_t state_size,
    const ReplacementPolicyOperations *operations
);

int replacement_frame_is_valid(
    const ReplacementPolicy *policy,
    uint32_t frame_number
);

#endif