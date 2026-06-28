#include <assert.h>
#include <stdio.h>
#include "address.h"

static void test_page_number(void)
{
    assert(address_page_number(0x0000) == 0);
    assert(address_page_number(0x0FFF) == 0);
    assert(address_page_number(0x1000) == 1);
    assert(address_page_number(0x2000) == 2);
}

static void test_offset(void)
{
    assert(address_offset(0x0000) == 0x000);
    assert(address_offset(0x0001) == 0x001);
    assert(address_offset(0x0FFF) == 0xFFF);
    assert(address_offset(0x1000) == 0x000);
    assert(address_offset(0x1234) == 0x234);
}

static void test_level_indexes(void)
{
    uint64_t address = 0x0000000123456789ULL;

    for (int level = 0; level < PAGE_TABLE_LEVELS; level++) {
        uint16_t index = address_level_index(address, level);
        assert(index < PAGE_TABLE_ENTRIES);
    }
}

int main(void)
{
    test_page_number();
    test_offset();
    test_level_indexes();

    printf("Todos os testes de address passaram.\n");
    return 0;
}