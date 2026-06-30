#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "replacement_internal.h"

typedef struct {
    bool *loaded;
    bool *referenced;
    uint8_t *ages;
} AgingState;

static void aging_destroy(
    ReplacementPolicy *policy
)
{
    AgingState *state = policy->state;

    free(state->loaded);
    free(state->referenced);
    free(state->ages);
    free(state);
    free(policy);
}

static int aging_on_load(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    AgingState *state = policy->state;

    state->loaded[frame_number] = true;
    state->referenced[frame_number] = true;
    state->ages[frame_number] = 0;

    return 0;
}

static int aging_on_access(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    AgingState *state = policy->state;

    if (!state->loaded[frame_number]) {
        return -1;
    }

    state->referenced[frame_number] = true;

    return 0;
}

static int aging_on_evict(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    AgingState *state = policy->state;

    if (!state->loaded[frame_number]) {
        return -1;
    }

    state->loaded[frame_number] = false;
    state->referenced[frame_number] = false;
    state->ages[frame_number] = 0;

    return 0;
}

static uint32_t aging_choose_victim(
    ReplacementPolicy *policy
)
{
    AgingState *state = policy->state;

    uint32_t victim = REPLACEMENT_INVALID_FRAME;
    uint8_t smallest_age = UINT8_MAX;

    for (
        size_t index = 0;
        index < policy->frame_count;
        index++
    ) {
        if (!state->loaded[index]) {
            continue;
        }

        if (
            victim == REPLACEMENT_INVALID_FRAME ||
            state->ages[index] < smallest_age
        ) {
            smallest_age = state->ages[index];
            victim = (uint32_t)index;
        }
    }

    return victim;
}

static void aging_tick(
    ReplacementPolicy *policy
)
{
    AgingState *state = policy->state;

    for (
        size_t index = 0;
        index < policy->frame_count;
        index++
    ) {
        if (!state->loaded[index]) {
            continue;
        }

        state->ages[index] >>= 1;

        if (state->referenced[index]) {
            state->ages[index] |= 0x80U;
        }

        state->referenced[index] = false;
    }
}

static const ReplacementPolicyOperations AGING_OPERATIONS = {
    .destroy = aging_destroy,
    .on_load = aging_on_load,
    .on_access = aging_on_access,
    .on_evict = aging_on_evict,
    .choose_victim = aging_choose_victim,
    .tick = aging_tick
};

ReplacementPolicy *replacement_aging_create(
    size_t frame_count
)
{
    ReplacementPolicy *policy =
        replacement_policy_allocate(
            REPLACEMENT_AGING,
            frame_count,
            sizeof(AgingState),
            &AGING_OPERATIONS
        );

    if (policy == NULL) {
        return NULL;
    }

    AgingState *state = policy->state;

    state->loaded = calloc(
        frame_count,
        sizeof(bool)
    );

    state->referenced = calloc(
        frame_count,
        sizeof(bool)
    );

    state->ages = calloc(
        frame_count,
        sizeof(uint8_t)
    );

    if (
        state->loaded == NULL ||
        state->referenced == NULL ||
        state->ages == NULL
    ) {
        aging_destroy(policy);
        return NULL;
    }

    return policy;
}