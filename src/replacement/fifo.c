#include <stdbool.h>
#include <stdlib.h>

#include "replacement_internal.h"

typedef struct {
    uint32_t *queue;
    bool *loaded;

    size_t size;
} FifoState;

static void fifo_destroy(
    ReplacementPolicy *policy
)
{
    FifoState *state = policy->state;

    free(state->queue);
    free(state->loaded);
    free(state);
    free(policy);
}

static int fifo_on_load(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    FifoState *state = policy->state;

    if (state->loaded[frame_number]) {
        return 0;
    }

    if (state->size >= policy->frame_count) {
        return -1;
    }

    state->queue[state->size] = frame_number;
    state->size++;

    state->loaded[frame_number] = true;

    return 0;
}

static int fifo_on_access(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    FifoState *state = policy->state;

    return state->loaded[frame_number] ? 0 : -1;
}

static int fifo_on_evict(
    ReplacementPolicy *policy,
    uint32_t frame_number
)
{
    FifoState *state = policy->state;

    if (!state->loaded[frame_number]) {
        return -1;
    }

    size_t position = state->size;

    for (size_t index = 0; index < state->size; index++) {
        if (state->queue[index] == frame_number) {
            position = index;
            break;
        }
    }

    if (position == state->size) {
        return -1;
    }

    for (
        size_t index = position;
        index + 1 < state->size;
        index++
    ) {
        state->queue[index] = state->queue[index + 1];
    }

    state->size--;
    state->loaded[frame_number] = false;

    return 0;
}

static uint32_t fifo_choose_victim(
    ReplacementPolicy *policy
)
{
    FifoState *state = policy->state;

    if (state->size == 0) {
        return REPLACEMENT_INVALID_FRAME;
    }

    return state->queue[0];
}

static void fifo_tick(
    ReplacementPolicy *policy
)
{
    (void)policy;
}

static const ReplacementPolicyOperations FIFO_OPERATIONS = {
    .destroy = fifo_destroy,
    .on_load = fifo_on_load,
    .on_access = fifo_on_access,
    .on_evict = fifo_on_evict,
    .choose_victim = fifo_choose_victim,
    .tick = fifo_tick
};

ReplacementPolicy *replacement_fifo_create(
    size_t frame_count
)
{
    ReplacementPolicy *policy =
        replacement_policy_allocate(
            REPLACEMENT_FIFO,
            frame_count,
            sizeof(FifoState),
            &FIFO_OPERATIONS
        );

    if (policy == NULL) {
        return NULL;
    }

    FifoState *state = policy->state;

    state->queue = calloc(
        frame_count,
        sizeof(uint32_t)
    );

    state->loaded = calloc(
        frame_count,
        sizeof(bool)
    );

    if (
        state->queue == NULL ||
        state->loaded == NULL
    ) {
        fifo_destroy(policy);
        return NULL;
    }

    return policy;
}