#ifndef REPLACEMENT_H
#define REPLACEMENT_H

#include <stddef.h>
#include <stdint.h>

#define REPLACEMENT_INVALID_FRAME UINT32_MAX

typedef enum {
    REPLACEMENT_FIFO = 0,
    REPLACEMENT_LRU,
    REPLACEMENT_CLOCK,
    REPLACEMENT_AGING
} ReplacementPolicyType;

typedef struct ReplacementPolicy ReplacementPolicy;

ReplacementPolicy *replacement_policy_create(
    ReplacementPolicyType type,
    size_t frame_count
);

void replacement_policy_destroy(
    ReplacementPolicy *policy
);

int replacement_policy_on_load(
    ReplacementPolicy *policy,
    uint32_t frame_number
);

int replacement_policy_on_access(
    ReplacementPolicy *policy,
    uint32_t frame_number
);

int replacement_policy_on_evict(
    ReplacementPolicy *policy,
    uint32_t frame_number
);

uint32_t replacement_policy_choose_victim(
    ReplacementPolicy *policy
);

void replacement_policy_tick(
    ReplacementPolicy *policy
);

const char *replacement_policy_name(
    const ReplacementPolicy *policy
);

#endif