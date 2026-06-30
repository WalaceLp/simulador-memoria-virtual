#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "tlb.h"

static void test_create_and_destroy(void)
{
    Tlb *tlb = tlb_create(4);

    assert(tlb != NULL);
    assert(tlb_capacity(tlb) == 4);
    assert(tlb_size(tlb) == 0);

    tlb_destroy(tlb);
}

static void test_invalid_creation(void)
{
    assert(tlb_create(0) == NULL);
}

static void test_insert_and_lookup(void)
{
    Tlb *tlb = tlb_create(2);

    assert(tlb != NULL);

    assert(
        tlb_insert(
            tlb,
            1,
            10,
            3
        ) == 0
    );

    uint32_t frame_number = UINT32_MAX;

    assert(
        tlb_lookup(
            tlb,
            1,
            10,
            &frame_number
        )
    );

    assert(frame_number == 3);
    assert(tlb_size(tlb) == 1);

    tlb_destroy(tlb);
}

static void test_miss(void)
{
    Tlb *tlb = tlb_create(2);

    assert(tlb != NULL);

    uint32_t frame_number = UINT32_MAX;

    assert(
        !tlb_lookup(
            tlb,
            1,
            10,
            &frame_number
        )
    );

    assert(frame_number == UINT32_MAX);

    tlb_destroy(tlb);
}

static void test_same_page_different_processes(void)
{
    Tlb *tlb = tlb_create(4);

    assert(tlb != NULL);

    assert(tlb_insert(tlb, 1, 5, 10) == 0);
    assert(tlb_insert(tlb, 2, 5, 20) == 0);

    uint32_t first_frame = UINT32_MAX;
    uint32_t second_frame = UINT32_MAX;

    assert(
        tlb_lookup(
            tlb,
            1,
            5,
            &first_frame
        )
    );

    assert(
        tlb_lookup(
            tlb,
            2,
            5,
            &second_frame
        )
    );

    assert(first_frame == 10);
    assert(second_frame == 20);

    tlb_destroy(tlb);
}

static void test_update_existing_entry(void)
{
    Tlb *tlb = tlb_create(2);

    assert(tlb != NULL);

    assert(tlb_insert(tlb, 1, 4, 8) == 0);
    assert(tlb_insert(tlb, 1, 4, 12) == 0);

    assert(tlb_size(tlb) == 1);

    uint32_t frame_number = UINT32_MAX;

    assert(
        tlb_lookup(
            tlb,
            1,
            4,
            &frame_number
        )
    );

    assert(frame_number == 12);

    tlb_destroy(tlb);
}

static void test_lru_replacement(void)
{
    Tlb *tlb = tlb_create(2);

    assert(tlb != NULL);

    assert(tlb_insert(tlb, 1, 1, 10) == 0);
    assert(tlb_insert(tlb, 1, 2, 20) == 0);

    uint32_t frame_number = UINT32_MAX;

    /*
     * A página 1 volta a ser a mais recente.
     */
    assert(
        tlb_lookup(
            tlb,
            1,
            1,
            &frame_number
        )
    );

    /*
     * A página 2 é a menos recente e deve sair.
     */
    assert(tlb_insert(tlb, 1, 3, 30) == 0);

    assert(
        !tlb_lookup(
            tlb,
            1,
            2,
            &frame_number
        )
    );

    assert(
        tlb_lookup(
            tlb,
            1,
            1,
            &frame_number
        )
    );

    assert(
        tlb_lookup(
            tlb,
            1,
            3,
            &frame_number
        )
    );

    tlb_destroy(tlb);
}

static void test_invalidate_entry(void)
{
    Tlb *tlb = tlb_create(2);

    assert(tlb != NULL);

    assert(tlb_insert(tlb, 1, 1, 7) == 0);

    assert(tlb_invalidate(tlb, 1, 1));
    assert(!tlb_invalidate(tlb, 1, 1));

    uint32_t frame_number = UINT32_MAX;

    assert(
        !tlb_lookup(
            tlb,
            1,
            1,
            &frame_number
        )
    );

    assert(tlb_size(tlb) == 0);

    tlb_destroy(tlb);
}

static void test_invalidate_process(void)
{
    Tlb *tlb = tlb_create(4);

    assert(tlb != NULL);

    assert(tlb_insert(tlb, 1, 1, 1) == 0);
    assert(tlb_insert(tlb, 1, 2, 2) == 0);
    assert(tlb_insert(tlb, 2, 1, 3) == 0);

    assert(tlb_invalidate_process(tlb, 1) == 2);
    assert(tlb_size(tlb) == 1);

    uint32_t frame_number = UINT32_MAX;

    assert(
        !tlb_lookup(
            tlb,
            1,
            1,
            &frame_number
        )
    );

    assert(
        tlb_lookup(
            tlb,
            2,
            1,
            &frame_number
        )
    );

    assert(frame_number == 3);

    tlb_destroy(tlb);
}

static void test_clear(void)
{
    Tlb *tlb = tlb_create(3);

    assert(tlb != NULL);

    assert(tlb_insert(tlb, 1, 1, 1) == 0);
    assert(tlb_insert(tlb, 2, 2, 2) == 0);

    tlb_clear(tlb);

    assert(tlb_size(tlb) == 0);
    assert(tlb_capacity(tlb) == 3);

    tlb_destroy(tlb);
}

int main(void)
{
    test_create_and_destroy();
    test_invalid_creation();
    test_insert_and_lookup();
    test_miss();
    test_same_page_different_processes();
    test_update_existing_entry();
    test_lru_replacement();
    test_invalidate_entry();
    test_invalidate_process();
    test_clear();

    printf("Todos os testes de tlb passaram.\n");

    return 0;
}