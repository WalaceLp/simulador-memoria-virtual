#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "replacement_internal.h"

typedef struct {
    bool *loaded;
    uint64_t *last_access;

    uint64_t logical_clock;
} LruState;

static void lru_destroy(
    ReplacementPolicy *policy
)
{
    LruState *state = policy->state;

    free(state->loaded);
    free(state->last_access);
    free(state);
    free(policy);
}

static void lru_update_access(
    LruState *state,
    uint32_t frame_number
)
{
    state->logical_clock++;
    state->last_access[frame_number] =
        state->logical_clock;
}

static int lru_on_load(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    LruState *state = policy->state;

    state->loaded[frame_number] = true;
    lru_update_access(state, frame_number);

    return 0;
}

static int lru_on_access(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    LruState *state = policy->state;

    if (!state->loaded[frame_number]) {
        return -1;
    }

    lru_update_access(state, frame_number);

    return 0;
}

static int lru_on_evict(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    LruState *state = policy->state;

    if (!state->loaded[frame_number]) {
        return -1;
    }

    state->loaded[frame_number] = false;
    state->last_access[frame_number] = 0;

    return 0;
}

static uint32_t lru_choose_victim(
    ReplacementPolicy *policy
)
{
    LruState *state = policy->state;

    uint32_t victim = REPLACEMENT_INVALID_FRAME;
    uint64_t oldest_access = UINT64_MAX;

    for (
        size_t index = 0;
        index < policy->frame_count;
        index++
    ) {
        if (!state->loaded[index]) {
            continue;
        }

        if (state->last_access[index] < oldest_access) {
            oldest_access = state->last_access[index];
            victim = (uint32_t)index;
        }
    }

    return victim;
}

static void lru_tick(
    ReplacementPolicy *policy
)
{
    (void)policy;
}

static const ReplacementPolicyOperations LRU_OPERATIONS = {
    .destroy = lru_destroy,
    .on_load = lru_on_load,
    .on_access = lru_on_access,
    .on_evict = lru_on_evict,
    .choose_victim = lru_choose_victim,
    .tick = lru_tick
};

ReplacementPolicy *replacement_lru_create(
    size_t frame_count
)
{
    ReplacementPolicy *policy =
        replacement_policy_allocate(
            REPLACEMENT_LRU,
            frame_count,
            sizeof(LruState),
            &LRU_OPERATIONS
        );

    if (policy == NULL) {
        return NULL;
    }

    LruState *state = policy->state;

    state->loaded = calloc(
        frame_count,
        sizeof(bool)
    );

    state->last_access = calloc(
        frame_count,
        sizeof(uint64_t)
    );

    if (
        state->loaded == NULL ||
        state->last_access == NULL
    ) {
        lru_destroy(policy);
        return NULL;
    }

    return policy;
}