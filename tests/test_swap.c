#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "address.h"
#include "swap.h"

#define TEST_SWAP_PATH "bin/test_swap_file.bin"

static void fill_page(
    uint8_t *page,
    uint8_t value
)
{
    memset(page, value, PAGE_SIZE);
}

static void test_create_and_destroy(void)
{
    Swap *swap = swap_create(
        TEST_SWAP_PATH,
        4,
        true
    );

    assert(swap != NULL);
    assert(swap_slot_count(swap) == 4);
    assert(swap_used_slot_count(swap) == 0);

    swap_destroy(swap);
}

static void test_write_and_read_page(void)
{
    Swap *swap = swap_create(
        TEST_SWAP_PATH,
        2,
        true
    );

    assert(swap != NULL);

    uint8_t source[PAGE_SIZE];
    uint8_t destination[PAGE_SIZE];

    fill_page(source, 0xAB);
    fill_page(destination, 0x00);

    assert(
        swap_write_page(
            swap,
            1,
            10,
            source
        ) == 0
    );

    assert(swap_contains_page(swap, 1, 10));
    assert(swap_used_slot_count(swap) == 1);

    assert(
        swap_read_page(
            swap,
            1,
            10,
            destination
        ) == 0
    );

    assert(
        memcmp(
            source,
            destination,
            PAGE_SIZE
        ) == 0
    );

    assert(swap_write_count(swap) == 1);
    assert(swap_read_count(swap) == 1);

    swap_destroy(swap);
}

static void test_overwrite_existing_page(void)
{
    Swap *swap = swap_create(
        TEST_SWAP_PATH,
        1,
        true
    );

    assert(swap != NULL);

    uint8_t first[PAGE_SIZE];
    uint8_t second[PAGE_SIZE];
    uint8_t result[PAGE_SIZE];

    fill_page(first, 0x11);
    fill_page(second, 0x22);
    fill_page(result, 0x00);

    assert(swap_write_page(swap, 1, 1, first) == 0);
    assert(swap_write_page(swap, 1, 1, second) == 0);

    assert(swap_used_slot_count(swap) == 1);

    assert(swap_read_page(swap, 1, 1, result) == 0);

    assert(memcmp(second, result, PAGE_SIZE) == 0);

    swap_destroy(swap);
}

static void test_swap_full(void)
{
    Swap *swap = swap_create(
        TEST_SWAP_PATH,
        1,
        true
    );

    assert(swap != NULL);

    uint8_t page[PAGE_SIZE];

    fill_page(page, 0x33);

    assert(swap_write_page(swap, 1, 1, page) == 0);
    assert(swap_write_page(swap, 1, 2, page) == -2);

    swap_destroy(swap);
}

static void test_remove_page(void)
{
    Swap *swap = swap_create(
        TEST_SWAP_PATH,
        2,
        true
    );

    assert(swap != NULL);

    uint8_t page[PAGE_SIZE];

    fill_page(page, 0x44);

    assert(swap_write_page(swap, 2, 5, page) == 0);
    assert(swap_remove_page(swap, 2, 5) == 0);

    assert(!swap_contains_page(swap, 2, 5));
    assert(swap_used_slot_count(swap) == 0);

    swap_destroy(swap);
}

int main(void)
{
    test_create_and_destroy();
    test_write_and_read_page();
    test_overwrite_existing_page();
    test_swap_full();
    test_remove_page();

    printf("Todos os testes de swap passaram.\n");

    return 0;
}