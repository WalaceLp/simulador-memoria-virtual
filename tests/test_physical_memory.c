#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "physical_memory.h"
#include "process.h"

static void test_create_and_destroy(void)
{
    PhysicalMemory *memory =
        physical_memory_create(4);

    assert(memory != NULL);
    assert(physical_memory_frame_count(memory) == 4);
    assert(physical_memory_free_frame_count(memory) == 4);

    physical_memory_destroy(memory);
}

static void test_invalid_frame_count(void)
{
    assert(physical_memory_create(0) == NULL);
}

static void test_allocate_frame(void)
{
    PhysicalMemory *memory =
        physical_memory_create(2);

    Process *process =
        process_create(10);

    assert(memory != NULL);
    assert(process != NULL);

    uint32_t frame_number =
        INVALID_FRAME_NUMBER;

    assert(
        physical_memory_allocate_frame(
            memory,
            process,
            5,
            &frame_number
        ) == 0
    );

    assert(frame_number == 0);
    assert(physical_memory_free_frame_count(memory) == 1);

    const PhysicalFrame *frame =
        physical_memory_get_frame(
            memory,
            frame_number
        );

    assert(frame != NULL);
    assert(frame->occupied);
    assert(frame->owner_pid == 10);
    assert(frame->owner_process == process);
    assert(frame->virtual_page == 5);
    assert(!frame->dirty);
    assert(!frame->referenced);

    process_destroy(process);
    physical_memory_destroy(memory);
}

static void test_memory_full(void)
{
    PhysicalMemory *memory =
        physical_memory_create(1);

    Process *first_process =
        process_create(1);

    Process *second_process =
        process_create(2);

    assert(memory != NULL);
    assert(first_process != NULL);
    assert(second_process != NULL);

    uint32_t first = INVALID_FRAME_NUMBER;
    uint32_t second = INVALID_FRAME_NUMBER;

    assert(
        physical_memory_allocate_frame(
            memory,
            first_process,
            1,
            &first
        ) == 0
    );

    assert(
        physical_memory_allocate_frame(
            memory,
            second_process,
            2,
            &second
        ) == -2
    );

    assert(second == INVALID_FRAME_NUMBER);

    process_destroy(first_process);
    process_destroy(second_process);
    physical_memory_destroy(memory);
}

static void test_replace_frame(void)
{
    PhysicalMemory *memory =
        physical_memory_create(1);

    Process *first_process =
        process_create(1);

    Process *second_process =
        process_create(2);

    assert(memory != NULL);
    assert(first_process != NULL);
    assert(second_process != NULL);

    uint32_t frame_number =
        INVALID_FRAME_NUMBER;

    assert(
        physical_memory_allocate_frame(
            memory,
            first_process,
            10,
            &frame_number
        ) == 0
    );

    assert(
        physical_memory_mark_access(
            memory,
            frame_number,
            true
        ) == 0
    );

    assert(
        physical_memory_replace_frame(
            memory,
            frame_number,
            second_process,
            20
        ) == 0
    );

    const PhysicalFrame *frame =
        physical_memory_get_frame(
            memory,
            frame_number
        );

    assert(frame != NULL);
    assert(frame->occupied);
    assert(frame->owner_pid == 2);
    assert(frame->owner_process == second_process);
    assert(frame->virtual_page == 20);
    assert(!frame->dirty);
    assert(!frame->referenced);

    assert(physical_memory_free_frame_count(memory) == 0);

    process_destroy(first_process);
    process_destroy(second_process);
    physical_memory_destroy(memory);
}

static void test_mark_access(void)
{
    PhysicalMemory *memory =
        physical_memory_create(1);

    Process *process =
        process_create(1);

    assert(memory != NULL);
    assert(process != NULL);

    uint32_t frame_number =
        INVALID_FRAME_NUMBER;

    assert(
        physical_memory_allocate_frame(
            memory,
            process,
            10,
            &frame_number
        ) == 0
    );

    assert(
        physical_memory_mark_access(
            memory,
            frame_number,
            false
        ) == 0
    );

    const PhysicalFrame *frame =
        physical_memory_get_frame(
            memory,
            frame_number
        );

    assert(frame != NULL);
    assert(frame->referenced);
    assert(!frame->dirty);

    assert(
        physical_memory_mark_access(
            memory,
            frame_number,
            true
        ) == 0
    );

    assert(frame->dirty);

    process_destroy(process);
    physical_memory_destroy(memory);
}

static void test_release_frame(void)
{
    PhysicalMemory *memory =
        physical_memory_create(1);

    Process *process =
        process_create(3);

    assert(memory != NULL);
    assert(process != NULL);

    uint32_t frame_number =
        INVALID_FRAME_NUMBER;

    assert(
        physical_memory_allocate_frame(
            memory,
            process,
            7,
            &frame_number
        ) == 0
    );

    assert(
        physical_memory_release_frame(
            memory,
            frame_number
        ) == 0
    );

    assert(physical_memory_free_frame_count(memory) == 1);

    const PhysicalFrame *frame =
        physical_memory_get_frame(
            memory,
            frame_number
        );

    assert(frame != NULL);
    assert(!frame->occupied);
    assert(frame->owner_pid == -1);
    assert(frame->owner_process == NULL);
    assert(!frame->dirty);
    assert(!frame->referenced);

    process_destroy(process);
    physical_memory_destroy(memory);
}

int main(void)
{
    test_create_and_destroy();
    test_invalid_frame_count();
    test_allocate_frame();
    test_memory_full();
    test_replace_frame();
    test_mark_access();
    test_release_frame();

    printf(
        "Todos os testes de physical_memory passaram.\n"
    );

    return 0;
}