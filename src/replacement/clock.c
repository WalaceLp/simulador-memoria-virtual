#include <stdbool.h>
#include <stdlib.h>

#include "replacement_internal.h"

typedef struct {
    bool *loaded;
    bool *referenced;

    size_t hand;
} ClockState;

static void clock_destroy(
    ReplacementPolicy *policy
)
{
    ClockState *state = policy->state;

    free(state->loaded);
    free(state->referenced);
    free(state);
    free(policy);
}

static int clock_on_load(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    ClockState *state = policy->state;

    state->loaded[frame_number] = true;
    state->referenced[frame_number] = true;

    return 0;
}

static int clock_on_access(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    ClockState *state = policy->state;

    if (!state->loaded[frame_number]) {
        return -1;
    }

    state->referenced[frame_number] = true;

    return 0;
}

static int clock_on_evict(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    ClockState *state = policy->state;

    if (!state->loaded[frame_number]) {
        return -1;
    }

    state->loaded[frame_number] = false;
    state->referenced[frame_number] = false;

    return 0;
}

static uint32_t clock_choose_victim(
    ReplacementPolicy *policy
)
{
    ClockState *state = policy->state;

    size_t loaded_count = 0;

    for (
        size_t index = 0;
        index < policy->frame_count;
        index++
    ) {
        if (state->loaded[index]) {
            loaded_count++;
        }
    }

    if (loaded_count == 0) {
        return REPLACEMENT_INVALID_FRAME;
    }

    while (true) {
        size_t current = state->hand;

        state->hand =
            (state->hand + 1) % policy->frame_count;

        if (!state->loaded[current]) {
            continue;
        }

        if (state->referenced[current]) {
            state->referenced[current] = false;
            continue;
        }

        return (uint32_t)current;
    }
}

static void clock_tick(
    ReplacementPolicy *policy
)
{
    (void)policy;
}

static const ReplacementPolicyOperations CLOCK_OPERATIONS = {
    .destroy = clock_destroy,
    .on_load = clock_on_load,
    .on_access = clock_on_access,
    .on_evict = clock_on_evict,
    .choose_victim = clock_choose_victim,
    .tick = clock_tick
};

ReplacementPolicy *replacement_clock_create(
    size_t frame_count
)
{
    ReplacementPolicy *policy =
        replacement_policy_allocate(
            REPLACEMENT_CLOCK,
            frame_count,
            sizeof(ClockState),
            &CLOCK_OPERATIONS
        );

    if (policy == NULL) {
        return NULL;
    }

    ClockState *state = policy->state;

    state->loaded = calloc(
        frame_count,
        sizeof(bool)
    );

    state->referenced = calloc(
        frame_count,
        sizeof(bool)
    );

    if (
        state->loaded == NULL ||
        state->referenced == NULL
    ) {
        clock_destroy(policy);
        return NULL;
    }

    return policy;
}