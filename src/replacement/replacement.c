#include <stdlib.h>

#include "replacement_internal.h"

ReplacementPolicy *replacement_policy_allocate(
    ReplacementPolicyType type,
    size_t frame_count,
    size_t state_size,
    const ReplacementPolicyOperations *operations
)
{
    if (
        frame_count == 0 ||
        state_size == 0 ||
        operations == NULL
    ) {
        return NULL;
    }

    ReplacementPolicy *policy = calloc(
        1,
        sizeof(ReplacementPolicy)
    );

    if (policy == NULL) {
        return NULL;
    }

    policy->state = calloc(1, state_size);

    if (policy->state == NULL) {
        free(policy);
        return NULL;
    }

    policy->type = type;
    policy->frame_count = frame_count;
    policy->operations = operations;

    return policy;
}

int replacement_frame_is_valid(
    const ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    if (policy == NULL) {
        return 0;
    }

    return frame_number < policy->frame_count;
}

ReplacementPolicy *replacement_policy_create(
    ReplacementPolicyType type,
    size_t frame_count
)
{
    switch (type) {
        case REPLACEMENT_FIFO:
            return replacement_fifo_create(frame_count);

        case REPLACEMENT_LRU:
            return replacement_lru_create(frame_count);

        case REPLACEMENT_CLOCK:
            return replacement_clock_create(frame_count);

        case REPLACEMENT_AGING:
            return replacement_aging_create(frame_count);

        default:
            return NULL;
    }
}

void replacement_policy_destroy(
    ReplacementPolicy *policy
)
{
    if (policy == NULL) {
        return;
    }

    if (
        policy->operations != NULL &&
        policy->operations->destroy != NULL
    ) {
        policy->operations->destroy(policy);
        return;
    }

    free(policy->state);
    free(policy);
}

int replacement_policy_on_load(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    if (
        !replacement_frame_is_valid(
            policy,
            frame_number
        ) ||
        policy->operations == NULL ||
        policy->operations->on_load == NULL
    ) {
        return -1;
    }

    return policy->operations->on_load(
        policy,
        frame_number
    );
}

int replacement_policy_on_access(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    if (
        !replacement_frame_is_valid(
            policy,
            frame_number
        ) ||
        policy->operations == NULL ||
        policy->operations->on_access == NULL
    ) {
        return -1;
    }

    return policy->operations->on_access(
        policy,
        frame_number
    );
}

int replacement_policy_on_evict(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    if (
        !replacement_frame_is_valid(
            policy,
            frame_number
        ) ||
        policy->operations == NULL ||
        policy->operations->on_evict == NULL
    ) {
        return -1;
    }

    return policy->operations->on_evict(
        policy,
        frame_number
    );
}

uint32_t replacement_policy_choose_victim(
    ReplacementPolicy *policy
)
{
    if (
        policy == NULL ||
        policy->operations == NULL ||
        policy->operations->choose_victim == NULL
    ) {
        return REPLACEMENT_INVALID_FRAME;
    }

    return policy->operations->choose_victim(policy);
}

void replacement_policy_tick(
    ReplacementPolicy *policy
)
{
    if (
        policy == NULL ||
        policy->operations == NULL ||
        policy->operations->tick == NULL
    ) {
        return;
    }

    policy->operations->tick(policy);
}

const char *replacement_policy_name(
    const ReplacementPolicy *policy
)
{
    if (policy == NULL) {
        return "invalid";
    }

    switch (policy->type) {
        case REPLACEMENT_FIFO:
            return "FIFO";

        case REPLACEMENT_LRU:
            return "LRU";

        case REPLACEMENT_CLOCK:
            return "Clock";

        case REPLACEMENT_AGING:
            return "Aging";

        default:
            return "invalid";
    }
}