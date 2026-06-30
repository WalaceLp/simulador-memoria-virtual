#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "physical_memory.h"

static void test_create_and_destroy(void)
{
    PhysicalMemory *memory =
        physical_memory_create(4);

    assert(memory != NULL);

    assert(
        physical_memory_frame_count(memory) == 4
    );

    assert(
        physical_memory_free_frame_count(memory) == 4
    );

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

    assert(memory != NULL);

    uint32_t frame_number =
        INVALID_FRAME_NUMBER;

    assert(
        physical_memory_allocate_frame(
            memory,
            10,
            5,
            &frame_number
        ) == 0
    );

    assert(frame_number == 0);

    assert(
        physical_memory_free_frame_count(memory) == 1
    );

    const PhysicalFrame *frame =
        physical_memory_get_frame(
            memory,
            frame_number
        );

    assert(frame != NULL);
    assert(frame->occupied);
    assert(frame->owner_pid == 10);
    assert(frame->virtual_page == 5);
    assert(!frame->dirty);
    assert(!frame->referenced);

    physical_memory_destroy(memory);
}

static void test_memory_full(void)
{
    PhysicalMemory *memory =
        physical_memory_create(1);

    assert(memory != NULL);

    uint32_t first =
        INVALID_FRAME_NUMBER;

    uint32_t second =
        INVALID_FRAME_NUMBER;

    assert(
        physical_memory_allocate_frame(
            memory,
            1,
            1,
            &first
        ) == 0
    );

    assert(
        physical_memory_allocate_frame(
            memory,
            2,
            2,
            &second
        ) == -2
    );

    assert(second == INVALID_FRAME_NUMBER);

    physical_memory_destroy(memory);
}

static void test_mark_read_access(void)
{
    PhysicalMemory *memory =
        physical_memory_create(1);

    assert(memory != NULL);

    uint32_t frame_number =
        INVALID_FRAME_NUMBER;

    assert(
        physical_memory_allocate_frame(
            memory,
            1,
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

    physical_memory_destroy(memory);
}

static void test_mark_write_access(void)
{
    PhysicalMemory *memory =
        physical_memory_create(1);

    assert(memory != NULL);

    uint32_t frame_number =
        INVALID_FRAME_NUMBER;

    assert(
        physical_memory_allocate_frame(
            memory,
            1,
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

    const PhysicalFrame *frame =
        physical_memory_get_frame(
            memory,
            frame_number
        );

    assert(frame != NULL);
    assert(frame->referenced);
    assert(frame->dirty);

    physical_memory_destroy(memory);
}

static void test_release_frame(void)
{
    PhysicalMemory *memory =
        physical_memory_create(1);

    assert(memory != NULL);

    uint32_t frame_number =
        INVALID_FRAME_NUMBER;

    assert(
        physical_memory_allocate_frame(
            memory,
            3,
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

    assert(
        physical_memory_free_frame_count(memory) == 1
    );

    const PhysicalFrame *frame =
        physical_memory_get_frame(
            memory,
            frame_number
        );

    assert(frame != NULL);
    assert(!frame->occupied);
    assert(frame->owner_pid == -1);
    assert(!frame->dirty);
    assert(!frame->referenced);

    physical_memory_destroy(memory);
}

int main(void)
{
    test_create_and_destroy();
    test_invalid_frame_count();
    test_allocate_frame();
    test_memory_full();
    test_mark_read_access();
    test_mark_write_access();
    test_release_frame();

    printf(
        "Todos os testes de physical_memory passaram.\n"
    );

    return 0;
}